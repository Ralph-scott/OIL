#ifndef VARIABLE_H_
#define VARIABLE_H_

#include <stddef.h>
#include "parser.h"
#include "hashmap.h"
#include "types.h"

typedef struct VariableID
{
    size_t scope_id;

    size_t name_len;
    char *name;
} VariableID;

typedef struct Variable
{
    size_t id;
    DataType data_type;
} Variable;

typedef struct SymbolTable
{
    HashMap symbols;

    size_t variable_id;

    size_t scope_id;

    size_t scopes_len;
    size_t scopes_cap;
    size_t *scopes;
} SymbolTable;

SymbolTable symbol_table_new(AST *ast);

Variable symbol_table_variable(SymbolTable *table, const char *name, const size_t name_len);

void symbol_table_begin_scope(SymbolTable *table);
void symbol_table_end_scope(SymbolTable *table);

void symbol_table_free(SymbolTable *table);

#endif // VARIABLE_H_
