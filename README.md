# tinyhcc
tinyhcc (thcc) is a minimalistic HolyC compiler made specifically for Windows.
## Usage
After building, you can use the resulting `thcc.exe` file like so:
```sh
thcc.exe <file(s).HC>
```
## Building
To build thcc, you will need python and clang installed. Then just run the `build.py` helper script:
```sh
py build.py
```
The resulting binaries will be put under `bin/`, and object files will be put under `obj/`.
