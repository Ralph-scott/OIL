#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "type_checker.h"
#include "hashmap.h"
#include "types.h"
#include "utils.h"

uint32_t variable_id_hash(const void *_key)
{
    const VariableID *key = _key;

    return key->scope_id ^ hash_string(key->name, key->name_len);
}

bool variable_id_equals(const void *_lhs, const void *_rhs)
{
    const VariableID *lhs = _lhs;
    const VariableID *rhs = _rhs;

    return lhs->scope_id == rhs->scope_id
        && lhs->name_len == rhs->name_len
        && strncmp(lhs->name, rhs->name, lhs->name_len) == 0;
}

static bool ast_is_lvalue(AST *ast)
{
    return (ast->type == AST_PREFIX && ast->prefix.oper.type == TOKEN_DEREFERENCE)
        || (ast->type == AST_NODE && ast->node.type == TOKEN_IDENT);
}

static void infer_type(AST *ast, DataType *type, bool type_owned)
{
    if (ast->data_type->type != TYPE_NULL) {
        if (type->type != TYPE_NULL && !data_type_equals(type, ast->data_type)) {
            ERROR("Data types are not the same.");
        }

        if (type_owned) {
            data_type_free(type);
        }
        return;
    }
    data_type_free(ast->data_type);

    switch (ast->type) {
        case AST_NODE: {
            break;
        }

        case AST_PREFIX: {
            switch (ast->prefix.oper.type) {
                case TOKEN_REFERENCE: {
                    infer_type(ast->prefix.node, type->dereference, false);
                    break;
                }

                case TOKEN_DEREFERENCE: {
                    infer_type(ast->prefix.node, data_type_reference(data_type_copy(type)), true);
                    break;
                }

                default: {
                    infer_type(ast->prefix.node, type, false);
                    break;
                }
            }
            break;
        }

        case AST_INFIX: {
            infer_type(ast->infix.lhs, type, false);
            infer_type(ast->infix.rhs, type, false);
            break;
        }

        case AST_BLOCK: {
            if (ast->block.len > 0) {
                infer_type(ast->block.statements[ast->block.len - 1], type, false);
            }
            break;
        }

        case AST_IF_STATEMENT: {
            infer_type(ast->if_statement.if_branch, type, false);
            if (ast->if_statement.else_branch != NULL) {
                infer_type(ast->if_statement.else_branch, type, false);
            }
            break;
        }

        default: {
            break;
        }
    }

    if (type->type != TYPE_NULL) {
        ast->data_type = type_owned ? type : data_type_copy(type);
        return;
    }

    if (type_owned) {
        data_type_free(type);
    }

    ast->data_type = data_type_type(TYPE_INT32);
}

void symbol_table_scan(SymbolTable *table, AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            switch (ast->node.type) {
                case TOKEN_IDENT: {
                    Variable variable = symbol_table_variable(table, ast->node.len, ast->node.text);
                    infer_type(ast, variable.data_type, false);
                    break;
                }

                case TOKEN_STRING: {
                    data_type_free(ast->data_type);
                    ast->data_type = data_type_reference(data_type_type(TYPE_INT8));
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
            infer_type(ast->prefix.node, data_type_type(TYPE_NULL), true);

            data_type_free(ast->data_type);
            switch (ast->node.type) {
                case TOKEN_REFERENCE: {
                    if (!ast_is_lvalue(ast->prefix.node)) {
                        ERROR("You can only reference an lvalue.");
                    }

                    ast->data_type = data_type_reference(data_type_copy(ast->prefix.node->data_type));
                    break;
                }

                case TOKEN_DEREFERENCE: {
                    if (ast->prefix.node->data_type->type != TYPE_REFERENCE) {
                        ERROR("You can only dereference a reference.");
                    }

                    ast->data_type = data_type_copy(ast->prefix.node->data_type->dereference);
                    break;
                }

                default: {
                    ast->data_type = data_type_copy(ast->prefix.node->data_type);
                    break;
                }
            }
            break;
        }

        case AST_INFIX: {
            symbol_table_scan(table, ast->infix.lhs);
            symbol_table_scan(table, ast->infix.rhs);
            infer_type(ast->infix.lhs, ast->infix.rhs->data_type, false);
            infer_type(ast->infix.rhs, ast->infix.lhs->data_type, false);

            data_type_free(ast->data_type);
            ast->data_type = data_type_copy(ast->infix.lhs->data_type);
            break;
        }

        case AST_BLOCK: {
            symbol_table_begin_scope(table);
            for (size_t i = 0; i < ast->block.len; ++i) {
                symbol_table_scan(table, ast->block.statements[i]);
                infer_type(ast->block.statements[i], data_type_type(TYPE_NULL), true);
            }
            symbol_table_end_scope(table);

            data_type_free(ast->data_type);
            if (ast->block.len > 0) {
                ast->data_type = data_type_copy(ast->block.statements[ast->block.len - 1]->data_type);
            } else {
                ast->data_type = data_type_type(TYPE_VOID);
            }
            break;
        }

        case AST_IF_STATEMENT: {
            symbol_table_scan(table, ast->if_statement.condition);

            symbol_table_scan(table, ast->if_statement.if_branch);

            infer_type(ast->if_statement.condition, data_type_type(TYPE_NULL), true);

            if (ast->if_statement.else_branch != NULL) {
                symbol_table_scan(table, ast->if_statement.else_branch);
                infer_type(ast->if_statement.else_branch, ast->if_statement.if_branch->data_type, false);
            }

            data_type_free(ast->data_type);
            ast->data_type = data_type_copy(ast->if_statement.if_branch->data_type);
            break;
        }

        case AST_WHILE_LOOP: {
            symbol_table_scan(table, ast->while_loop.condition);
            symbol_table_scan(table, ast->while_loop.body);
            infer_type(ast->while_loop.condition, data_type_type(TYPE_NULL), true);
            infer_type(ast->while_loop.body, data_type_type(TYPE_NULL), true);
            break;
        }

        case AST_FUNCTION_CALL: {
            symbol_table_scan(table, ast->function_call.lhs);
            infer_type(ast->function_call.lhs, data_type_type(TYPE_NULL), true);

            if (ast->function_call.lhs->data_type->type != TYPE_FUNCTION) {
                ERROR("You can only call a function.");
            }

            if (ast->function_call.lhs->data_type->function.len != ast->function_call.len) {
                ERROR("Wrong number of function arguments provided.");
            }

            for (size_t i = 0; i < ast->function_call.len; ++i) {
                symbol_table_scan(table, ast->function_call.arguments[i]);
                infer_type(ast->function_call.arguments[i], ast->function_call.lhs->data_type->function.arguments[i], false);
            }

            data_type_free(ast->data_type);
            ast->data_type = data_type_copy(ast->function_call.lhs->data_type->function.return_type);
            break;
        }

        case AST_DECLARATION: {
            Variable variable = {
                .data_type = data_type_new(ast->declaration.type)
            };

            symbol_table_add_variable(table, ast->declaration.name.len, ast->declaration.name.text, variable);

            if (ast->declaration.value != NULL) {
                symbol_table_scan(table, ast->declaration.value);
                infer_type(ast->declaration.value, variable.data_type, false);
            }

            data_type_free(ast->data_type);
            ast->data_type = data_type_type(TYPE_VOID);
            break;
        }
    }
}

SymbolTable symbol_table_new(void)
{
    SymbolTable table = {
        .symbols = hashmap_new(
            variable_id_hash,
            variable_id_equals,
            sizeof(VariableID),
            sizeof(Variable)
        ),
        .scopes_len   = 1,
        .scopes_cap   = 16
    };

    table.scopes = malloc(sizeof(*table.scopes) * table.scopes_cap);

    if (table.scopes == NULL) {
        ALLOCATION_ERROR();
    }

    table.scopes[0] = table.scope_id;

    return table;
}

Variable symbol_table_variable(SymbolTable *table, size_t name_len, const char *name)
{
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

VariableID symbol_table_variable_id(SymbolTable *table, size_t name_len, const char *name)
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
            return id;
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

        table->scopes = realloc(table->scopes, sizeof(*table->scopes) * table->scopes_cap);

        if (table->scopes == NULL) {
            ALLOCATION_ERROR();
        }
    }
    
    table->scopes[table->scopes_len++] = ++table->scope_id;
}

void symbol_table_add_variable(SymbolTable *table, size_t name_len, const char *name, Variable variable)
{
    VariableID variable_id = {
        .scope_id = table->scope_id,
        .name_len = name_len,
        .name     = (char *) name
    };

    hashmap_insert(&table->symbols, &variable_id, &variable);
}

void symbol_table_end_scope(SymbolTable *table)
{
    if (table->scopes_len == 0) {
        UNREACHABLE();
    }

    --table->scopes_len;
}

void symbol_table_free(SymbolTable *table)
{
    for (size_t i = 0; i < HASHMAP_SIZE; ++i) {
        HashMapEntry *bucket = table->symbols.buckets[i];
        while (bucket != NULL) {
            data_type_free(((Variable *) bucket->value)->data_type);
            bucket = bucket->next;
        }
    }

    hashmap_free(&table->symbols);

    free(table->scopes);
}
