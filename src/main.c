#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "compile.h"
#include "hashmap.h"
#include "utils.h"

const char *extension = ".asm";

char *read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ERROR("Could not open file %s.", path);
    }

    fseek(f, 0, SEEK_END);
    const size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *text = malloc(sizeof(*text) * (size + 1));
    
    if (text == NULL) {
        ALLOCATION_ERROR();
    }

    fread(text, size, 1, f);
    text[size] = '\0';

    fclose(f);

    return text;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        ERROR("Not enough arguments.");
    }

    const char *file_name = argv[1];
    
    const char *path = argv[2];

    char *text = read_file(file_name);

    Lexer lexer = lexer_new(text);

    AST *ast = parse(&lexer);

    //ast_print(ast);
    //printf("\n");

    FILE *file = fopen(path, "w");

    compile(ast, file);

    fclose(file);

    ast_free(ast);

    free(text);

    return 0;
}
