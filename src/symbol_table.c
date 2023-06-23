#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "symbol_table.h"
#include "hashmap.h"
#include "types.h"
#include "utils.h"

static uint32_t variable_id_hash(const void *_key)
{
    const VariableID *key = _key;

    return key->scope_id ^ hash_string(key->name, key->name_len);
}

static bool variable_id_equals(const void *_lhs, const void *_rhs)
{
    const VariableID *lhs = _lhs;
    const VariableID *rhs = _rhs;

    return lhs->scope_id == rhs->scope_id
        && lhs->name_len == rhs->name_len
        && strncmp(lhs->name, rhs->name, lhs->name_len) == 0;
}

static void symbol_table_scan(SymbolTable *table, const AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            if (ast->node.type == TOKEN_IDENT) {
                symbol_table_variable(table, ast->node.text, ast->node.len);
            }
            break;
        }

        case AST_PREFIX: {
            symbol_table_scan(table, ast->prefix.node);
            break;
        }

        case AST_INFIX: {
            symbol_table_scan(table, ast->infix.lhs);
            symbol_table_scan(table, ast->infix.rhs);
            break;
        }

        case AST_BLOCK: {
            symbol_table_begin_scope(table);
            for (size_t i = 0; i < ast->block.len; ++i) {
                symbol_table_scan(table, ast->block.statements[i]);
            }
            symbol_table_end_scope(table);
            break;
        }

        case AST_IF_STATEMENT: {
            symbol_table_scan(table, ast->if_statement.condition);
            symbol_table_scan(table, ast->if_statement.if_branch);
            break;
        }

        case AST_DECLARATION: {
            VariableID variable_id = {
                .scope_id = table->scope_id,
                .name_len = ast->declaration.name.len,
                .name     = ast->declaration.name.text
            };

            Variable variable = {
                .id = table->variable_id++,
                .type = data_type_name(ast->declaration.type)
            };

            hashmap_insert(&table->symbols, &variable_id, &variable);

            if (ast->declaration.value != NULL) {
                symbol_table_scan(table, ast->declaration.value);
            }

            break;
        }

        default: {
            break;
        }
    }
}

SymbolTable symbol_table_new(const AST *ast)
{
    SymbolTable table = {
        .symbols = hashmap_new(
            variable_id_hash,
            variable_id_equals,
            sizeof(VariableID),
            sizeof(Variable)
        ),
        .variable_id  = 0,
        .scope_id     = 0,
        .scopes_len   = 1,
        .scopes_cap   = 16
    };

    table.scopes = malloc(sizeof(size_t) * table.scopes_cap);

    if (table.scopes == NULL) {
        ALLOCATION_ERROR();
    }

    table.scopes[0] = table.scope_id;

    symbol_table_scan(&table, ast);

    table.scope_id = 0;

    return table;
}

Variable symbol_table_variable(SymbolTable *table, const char *name, const size_t name_len)
{
    // check every variable scope starting from the current scope
    for (size_t i = 0; i < table->scopes_len; ++i) {
        const size_t index = table->scopes_len - i - 1;

        const VariableID id = {
            .scope_id = table->scopes[index],
            .name_len = name_len,
            .name     = (char *) name
        };

        Variable *variable = hashmap_get(&table->symbols, &id);

        if (variable != NULL) {
            return *variable;
        }
    }

    ERROR("Variable `%.*s` was not defined.", (int) name_len, name);
}

void symbol_table_begin_scope(SymbolTable *table)
{
    // push
    if (table->scopes_len >= table->scopes_cap) {
        while (table->scopes_len >= table->scopes_cap) {
            table->scopes_cap *= 2;
        }

        table->scopes = realloc(table->scopes, sizeof(size_t) * table->scopes_cap);

        if (table->scopes == NULL) {
            ALLOCATION_ERROR();
        }
    }
    
    table->scopes[table->scopes_len++] = ++table->scope_id;
}

void symbol_table_end_scope(SymbolTable *table)
{
    // pop
    --table->scopes_len;
}

void symbol_table_free(SymbolTable *table)
{
    hashmap_free(&table->symbols);

    free(table->scopes);
}
