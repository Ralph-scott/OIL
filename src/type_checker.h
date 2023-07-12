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
    DataType *data_type;
} Variable;

typedef struct SymbolTable
{
    HashMap symbols;

    size_t scope_id;

    size_t scopes_len;
    size_t scopes_cap;
    size_t *scopes;
} SymbolTable;

uint32_t variable_id_hash(const void *_key);
bool variable_id_equals(const void *_lhs, const void *_rhs);

SymbolTable symbol_table_new(void);

void symbol_table_scan(SymbolTable *table, AST *ast);

Variable symbol_table_variable(SymbolTable *table, size_t name_len, const char *name);
VariableID symbol_table_variable_id(SymbolTable *table, size_t name_len, const char *name);

void symbol_table_begin_scope(SymbolTable *table);
void symbol_table_add_variable(SymbolTable *table, size_t name_len, const char *name, Variable variable);
void symbol_table_end_scope(SymbolTable *table);

void symbol_table_free(SymbolTable *table);

#endif // VARIABLE_H_
