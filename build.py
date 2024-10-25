"""
This file is part of the tinyhcc project.
https://github.com/wofflevm/tinyhcc
See LICENSE for license.
"""

import os
import sys
import shutil
import subprocess

from typing import Callable, TypeVar, ParamSpec, Type

class BuilderError(Exception):"""An exception thrown by the builder."""
class UnexpectedError(Exception):"""An exception thrown if a function raises an unexpected exception."""

T = TypeVar("T"); P = ParamSpec("P");
def raises(*exceptions: Type[Exception]) -> Callable[[Callable[P, T]], Callable[P, T]]:
    def inner(f: Callable[P, T]) -> Callable[P, T]:
        def wrapper(*args: P.args, **kwargs: P.kwargs) -> T:
            try:
                return f(*args, **kwargs)
            except exceptions as e:
                raise e
            except Exception as be:
                raise UnexpectedError(
                    f"Unexpected error type raised by function {f.__name__}. Allowed exception types: {', '.join(e.__name__ for e in exceptions)}. Got: {type(be).__name__}"
                )
        return wrapper
    return inner

class Builder:
    def __init__(self,
        cc: str                    = "clang",   # The C compiler to use when building. MUST BE POSIX COMPLIANT
        ld: str                    = "clang",   # The linker to use when linking. MUST BE POSIX COMPLIANT
        src: str                   = "src",     # The source directory
        include: list[str]  | None = None,      # Directories to include
        bin: str                   = "bin",     # The destination folder for final binaries
        obj: str                   = "obj",     # The folder for intermediate binaries
        cflags: list[str]   | None = None,      # Extra C flags to pass to the C compiler. POSIX STYLE
        ldflags: list[str]  | None = None,      # Extra linker flags to pass to the linker. POSIX STYLE
        debug: bool                = False,     # Whether to compile a debug build.
        rebuild: bool              = False      # Whether to rebuild up-to-date files.
    ) -> None:
        self.cc: str = cc
        self.ld: str = ld
        self.src: str = src
        self.include: list[str] = include if include else ["include"]
        self.bin: str = bin
        self.obj: str = obj
        self.cflags: list[str] = cflags if cflags else [
            "-std=c99",
            "-Wall",
            "-Werror",
            "-pedantic"
        ]
        self.ldflags: list[str] = ldflags if ldflags else []

        self.debug: bool = debug
        self.errors: int = 0
        self.cdefinitions: dict[str, str] = {}
        self.rebuild = rebuild

        if self.debug:
            self.cdefine("DEBUG", "1")

    #
    # Utility functions
    #

    @raises(BuilderError)
    def err(self, message: str) -> None:
        raise BuilderError(message)

    def log(self, message: str) -> None:
        sys.stdout.write(f"[*] {message}\n")
        sys.stdout.flush()

    def warn(self, message: str) -> None:
        sys.stdout.write(f"[!] {message}\n")
        sys.stdout.flush()

    def cmd(self, command: list[str]) -> None:
        sys.stdout.write(f"[>] {' '.join(command)}\n")
        sys.stdout.flush()
        res: subprocess.CompletedProcess[bytes] = subprocess.run(command)
        if res.returncode != 0:
            self.warn(f"Command failed with returncode {res.returncode}.")
            self.errors += 1

    @raises(BuilderError)
    def last_update_date(self, path: str) -> float:
        if not os.path.exists(path):
            self.err(f"Attempt to get the last modified date of file '{path}' which does not exist")
        return os.path.getmtime(path)

    def cdefine(self, key: str, value: str) -> None:
        if key in self.cdefinitions:
            self.warn(f"Redefining key '{key}' from '{self.cdefinitions[key]}' to '{value}'")
        self.cdefinitions[key] = value

    @raises(BuilderError)
    def cundef(self, key: str) -> None:
        if key not in self.cdefinitions:
            self.err(f"Attempt to undefine key '{key}' which does not exist")
        del self.cdefinitions[key]

    def filename_from_path(self, path: str) -> str:
        return path.split("\\")[-1]

    def base_filename(self, path: str) -> str:
        filename: str = self.filename_from_path(path)
        dots: list[str] = filename.split(".")
        if len(dots) < 1:
            return filename
        return filename[:-(len(dots[-1]) + 1)]

    #
    # Build utilities
    #

    @raises(BuilderError)
    def build(self, path: str) -> None:
        path = os.path.join(self.src, path)
        if not os.path.exists(path):
            self.err(f"Attempt to build file '{path}' which does not exist")
        if not os.path.exists(self.obj):
            os.makedirs(self.obj)
        objfile: str = os.path.join(self.obj, self.base_filename(path) + ".o")
        if os.path.exists(objfile):
            if self.last_update_date(path) <= self.last_update_date(objfile) and not self.rebuild:
                self.log(f"File {path} already up to date. Skipping")
                return
        self.cmd([
            self.cc, "-c", "-o", objfile, *self.cflags, path, *[f"-I{x}" for x in self.include], *[f"-D{key}={self.cdefinitions[key]}" for key in self.cdefinitions.keys()]
        ])

    @raises(BuilderError)
    def link(self, objects: list[str], binpath: str) -> None:
        if self.errors:
            self.err(f"Errors encountered while building, aborting")
        if not os.path.exists(self.bin):
            os.makedirs(self.bin)
        objects = [os.path.join(self.obj, path) for path in objects]
        binpath = os.path.join(self.bin, binpath)
        skiplink: bool = not self.rebuild
        for path in objects:
            if not os.path.exists(path):
                self.err(f"Attempt to link file '{path}' which does not exist")
            if os.path.exists(binpath):
                if self.last_update_date(path) >= self.last_update_date(binpath):
                    skiplink = False
            else:
                skiplink = False
        if skiplink:
            self.log(f"File {binpath} already up to date. Skipping")
            return
        self.cmd([
            self.ld, "-o", binpath, *self.ldflags, *objects
        ])

def main() -> None:
    flp: Callable[[str], str] = lambda path: os.path.join(*path.split("/"))
    builder: Builder = Builder(debug="debug" in sys.argv, rebuild="rebuild" in sys.argv)
    builder.cdefine("_CRT_SECURE_NO_WARNINGS", "1")
    builder.build(flp("cli.c"))
    builder.build(flp("lexer.c"))
    builder.build(flp("parser.c"))
    builder.link([flp("cli.o"), flp("lexer.o"), flp("parser.o")], "thcc.exe")

if __name__ == "__main__":
    main()
