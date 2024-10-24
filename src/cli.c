/*
 * This file is part of the tinyhcc project.
 * https://github.com/wofflevm/tinyhcc
 * See LICENSE for license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "lexer.h"

typedef struct CliArgs {
    const char *outFile;
    const char **inFiles;
    size_t nInFiles;
    bool showHelp;
} CliArgs;

void showHelp(const char *argv0) {
    printf("tinyhcc - Tiny HolyC compiler.\n");
    printf("Usage: %s <file(s).HC>\n", argv0);
    printf(" -o, --output <path>: The path to the file/folder to place the final binary in\n");
    printf(" -h, --help: Show this menu\n");
}

CliArgs parseArgs(int argc, const char **argv) {
    CliArgs args;
    args.nInFiles = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            args.showHelp = true;
            return args;
        } else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
            if (argc == i){
                fprintf(stderr, "Expected argument to '%s'.\n", argv[i]);
                exit(1);
            }
            args.outFile = argv[++i];
        } else {
            size_t len = strlen(argv[i]);
            if (len < 3) goto err;
            if (argv[i][len - 3] == '.' && tolower(argv[i][len - 2]) == 'h' && tolower(argv[i][len - 1]) == 'c') {
                if (args.nInFiles == 0) {
                    args.inFiles = malloc(sizeof(char*));
                } else {
                    args.inFiles = realloc(args.inFiles, sizeof(char*) * args.nInFiles + 1);
                }
                args.inFiles[args.nInFiles++] = argv[i];
                continue;
            }
            err:
                fprintf(stderr, "Unrecognized argument '%s'.\n", argv[i]);
                exit(1);
        }
    }
    return args;
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        showHelp(argv[0]);
        return 0;
    }
    CliArgs args = parseArgs(argc, argv);
    for (size_t i = 0; i < args.nInFiles; i++) {
        FILE *f = fopen(args.inFiles[i], "rb");
        if (f == NULL) {
            fprintf(stderr, "Fatal: couldn't open input file '%s'.", args.inFiles[i]);
            fprintf(stderr, "Aborting.\n");
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);

        char *buffer = malloc(len + 1);
        if (buffer == NULL) {
            fprintf(stderr, "Fatal: Input file '%s' too big.", args.inFiles[i]);
            fprintf(stderr, "Aborting.\n");
            fclose(f);
            return 1;
        }

        size_t read = fread(buffer, 1, len, f);
        if (read != len) {
            fprintf(stderr, "Fatal: Failed to read input file '%s'.", args.inFiles[i]);
            fprintf(stderr, "Aborting.\n");
            free(buffer);
            fclose(f);
            return 1;
        }
        buffer[len] = 0;

        Token* tokens = tokenize(buffer, args.inFiles[i]);
    #ifdef DEBUG
        for (size_t i = 0; tokens[i].type != TT_EOF; i++) {
            printf("%zu type='%s' value='%s' file='%s' line=%zu column=%zu index=%zu\n", i, tokenTypeAsString(tokens[i]), tokens[i].value, tokens[i].file, tokens[i].line, tokens[i].col, tokens[i].index);
        }
    #endif

        free(buffer);
        fclose(f);
    }
    return 0;
}
