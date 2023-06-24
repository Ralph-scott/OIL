#include <stdio.h>
#include <string.h>
#include "types.h"
#include "utils.h"

DataType data_type_name(const AST *ast)
{
    switch (ast->type) {
        case AST_NODE: {
            if (strncmp(ast->node.text, "u64", ast->node.len) == 0) {
                return (DataType) {
                    .type = TYPE_INT64
                };
            } else if (strncmp(ast->node.text, "int", ast->node.len) == 0
             || strncmp(ast->node.text, "u32", ast->node.len) == 0) {
                return (DataType) {
                    .type = TYPE_INT32
                };
            } else if (strncmp(ast->node.text, "u16", ast->node.len) == 0) {
                return (DataType) {
                    .type = TYPE_INT16
                };
            } else if (strncmp(ast->node.text, "u8", ast->node.len) == 0) {
                return (DataType) {
                    .type = TYPE_INT8
                };
            }
            break;
        }

        default: {
            break;
        }
    }

    // We need ast formatting
    ERROR("Type does not exist.\n");
}
