#ifndef COMPILE_H_
#define COMPILE_H_

#include <stdio.h>
#include "type_checker.h"
#include "asm_context.h"
#include "parser.h"

typedef struct Compiler
{
    SymbolTable table;
    AsmContext asm_context;
} Compiler;

void compile(AST *ast, FILE *file);

#endif // COMPILE_H_
