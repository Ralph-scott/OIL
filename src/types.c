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

DataType *data_type_function(size_t arguments_len, size_t arguments_cap, DataType **arguments, DataType *return_type)
{
    DataType *data_type = data_type_type(TYPE_FUNCTION);

    data_type->function.len = arguments_len;
    data_type->function.cap = arguments_cap;
    data_type->function.arguments = arguments;
    data_type->function.return_type = return_type;

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

        case AST_FUNCTION_CALL: {
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

        case TYPE_FUNCTION: {
            DataType **arguments = malloc(sizeof(*arguments) * type->function.cap);

            if (arguments == NULL) {
                ALLOCATION_ERROR();
            }

            for (size_t i = 0; i < type->function.len; ++i) {
                arguments[i] = data_type_copy(type->function.arguments[i]);
            }

            return data_type_function(type->function.len, type->function.cap, arguments, data_type_copy(type->function.return_type));
        }

        default: {
            return data_type_type(type->type);
        }
    }
}

bool data_type_equals(const DataType *lhs, const DataType *rhs)
{
    if (lhs->type != rhs->type) {
        return false;
    }

    switch (lhs->type) {
        case TYPE_REFERENCE: {
            return data_type_equals(lhs->dereference, rhs->dereference);
        }

        case TYPE_FUNCTION: {
            if (!data_type_equals(lhs->function.return_type, rhs->function.return_type)
                || lhs->function.len != rhs->function.len) {
                return false;
            }

            for (size_t i = 0; i < lhs->function.len; ++i) {
                if (!data_type_equals(lhs->function.arguments[i], rhs->function.arguments[i])) {
                    return false;
                }
            }

            return true;
        }

        default: {
            return true;
        }
    }
}

void data_type_free(DataType *type)
{
    switch (type->type) {
        case TYPE_REFERENCE: {
            data_type_free(type->dereference);
            break;
        }

        case TYPE_FUNCTION: {
            data_type_free(type->function.return_type);
            for (size_t i = 0; i < type->function.len; ++i) {
                data_type_free(type->function.arguments[i]);
            }
            free(type->function.arguments);
            break;
        }

        default: {
            break;
        }
    }

    free(type);
}
