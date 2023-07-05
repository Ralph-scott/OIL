#include <string.h>
#include "types.h"
#include "utils.h"
#include "ast.h"

DataType *data_type_type(DataTypeType type)
{
    DataType *data_type = malloc(sizeof(DataType));

    if (data_type == NULL) {
        ALLOCATION_ERROR();
    }

    data_type->type = type;

    return data_type;
}

DataType *data_type_reference(DataType *dereference)
{
    DataType *data_type = data_type_type(TYPE_REFERENCE);

    data_type->dereference = dereference;
    
    return data_type;
}

DataType *data_type_new(const AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            if (ast->node.type == TOKEN_IDENT) {
                if (strncmp(ast->node.text, "s8", ast->node.len) == 0) {
                    return data_type_type(TYPE_INT8);
                } else if (strncmp(ast->node.text, "s16", ast->node.len) == 0) {
                    return data_type_type(TYPE_INT16);
                } else if (strncmp(ast->node.text, "s32", ast->node.len) == 0
                       || strncmp(ast->node.text, "int", ast->node.len) == 0) {
                    return data_type_type(TYPE_INT32);
                } else if (strncmp(ast->node.text, "s64", ast->node.len) == 0) {
                    return data_type_type(TYPE_INT64);
                } else if (strncmp(ast->node.text, "void", ast->node.len) == 0) {
                    return data_type_type(TYPE_VOID);
                } else if (strncmp(ast->node.text, "string", ast->node.len) == 0) {
                    return data_type_reference(data_type_type(TYPE_INT8));
                }
            }
            break;
        }

        case AST_PREFIX: {
            switch (ast->prefix.oper.type) {
                case TOKEN_REFERENCE: {
                    return data_type_reference(data_type_new(ast->prefix.node));
                }

                default: {
                    break;
                }
            }
            break;
        }

        default: {
            break;
        }
    }
    ERROR("Bad type.");
}

DataType *data_type_copy(const DataType *type)
{
    switch (type->type) {
        case TYPE_REFERENCE: {
            return data_type_reference(data_type_copy(type->dereference));
        }

        default: {
            return data_type_type(type->type);
        }
    }
}

void data_type_free(DataType *type)
{
    if (type->type == TYPE_REFERENCE) {
        data_type_free(type->dereference);
    }
    free(type);
}
