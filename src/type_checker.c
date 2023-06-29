#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "type_checker.h"
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

static void infer_type(AST *ast, DataType type)
{
    if (ast->data_type != TYPE_NULL) {
        if (type != TYPE_NULL && ast->data_type != type) {
            ERROR("Data types are not the same.");
        }
        return;
    }

    switch (ast->type) {
        case AST_NODE: {
            break;
        }

        case AST_PREFIX: {
            infer_type(ast->prefix.node, type);
            break;
        }

        case AST_INFIX: {
            infer_type(ast->infix.lhs, type);
            infer_type(ast->infix.rhs, type);
            break;
        }

        case AST_BLOCK: {
            if (ast->block.len > 0) {
                infer_type(ast->block.statements[ast->block.len - 1], type);
            }
            break;
        }

        case AST_IF_STATEMENT: {
            infer_type(ast->if_statement.if_branch, type);
            if (ast->if_statement.else_branch != NULL) {
                infer_type(ast->if_statement.else_branch, type);
            }
            break;
        }

        default: {
            break;
        }
    }
    // TODO: Destroy this piece of code
    {
        // piece of code to destroy
        if (type == TYPE_NULL) {
            ast->data_type = TYPE_INT32;
        } else {
            ast->data_type = type;
        }
    }
}

static void symbol_table_scan(SymbolTable *table, AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            switch (ast->node.type) {
                case TOKEN_IDENT: {
                    Variable variable = symbol_table_variable(table, ast->node.text, ast->node.len);
                    ast->data_type = variable.data_type;
                    break;
                }

                case TOKEN_NUMBER: {
                    break;
                }

                default: {
                    break;
                }
            }
            break;
        }

        case AST_PREFIX: {
            symbol_table_scan(table, ast->prefix.node);
            infer_type(ast->prefix.node, TYPE_NULL);

            switch (ast->infix.oper.type) {
                case TOKEN_OPER_SUB: {
                    ast->data_type = ast->prefix.node->data_type;
                    break;
                }

                case TOKEN_NOT: {
                    // TODO: add booleans
                    // ast->data_type = TYPE_BOOL
                    ast->data_type = ast->prefix.node->data_type;
                    break;
                }

                default: {
                    break;
                }
            }

            ast->data_type = ast->prefix.node->data_type;
            break;
        }

        case AST_INFIX: {
            symbol_table_scan(table, ast->infix.lhs);
            symbol_table_scan(table, ast->infix.rhs);
            infer_type(ast->infix.lhs, ast->infix.rhs->data_type);
            infer_type(ast->infix.rhs, ast->infix.lhs->data_type);

            ast->data_type = ast->infix.lhs->data_type;
            break;
        }

        case AST_BLOCK: {
            DataType type = TYPE_NULL;
            symbol_table_begin_scope(table);
            for (size_t i = 0; i < ast->block.len; ++i) {
                symbol_table_scan(table, ast->block.statements[i]);
                infer_type(ast->block.statements[i], TYPE_NULL);
                type = ast->block.statements[i]->data_type;
            }
            symbol_table_end_scope(table);
            ast->data_type = type;
            break;
        }

        case AST_IF_STATEMENT: {
            symbol_table_scan(table, ast->if_statement.condition);

            DataType if_statement_type =
                ast->if_statement.else_branch == NULL
                ? TYPE_NULL
                : ast->if_statement.else_branch->data_type;

            symbol_table_scan(table, ast->if_statement.if_branch);

            infer_type(ast->if_statement.condition, TYPE_NULL);

            if (ast->if_statement.else_branch != NULL) {
                symbol_table_scan(table, ast->if_statement.else_branch);
                infer_type(ast->if_statement.else_branch, ast->if_statement.if_branch->data_type);
            }

            infer_type(ast->if_statement.if_branch, if_statement_type);

            ast->data_type = ast->if_statement.if_branch->data_type;
            break;
        }

        case AST_WHILE_LOOP: {
            symbol_table_scan(table, ast->while_loop.condition);
            symbol_table_scan(table, ast->while_loop.body);
            infer_type(ast->while_loop.condition, TYPE_NULL);
            infer_type(ast->while_loop.body, TYPE_NULL);
            break;
        }

        case AST_DECLARATION: {
            VariableID variable_id = {
                .scope_id = table->scope_id,
                .name_len = ast->declaration.name.len,
                .name     = ast->declaration.name.text
            };

            if (ast->declaration.type->type != TOKEN_IDENT) {
                ERROR("Types can just be identifiers for now.");
            }

            Variable variable = {
                .id = table->variable_id++,
                .data_type = data_type_new(ast->declaration.type->node.text, ast->declaration.type->node.len)
            };

            hashmap_insert(&table->symbols, &variable_id, &variable);

            if (ast->declaration.value != NULL) {
                symbol_table_scan(table, ast->declaration.value);
                infer_type(ast->declaration.value, variable.data_type);
            }

            break;
        }

        default: {
            break;
        }
    }
}

SymbolTable symbol_table_new(AST *ast)
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
