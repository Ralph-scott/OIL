#ifndef TYPES_H_
#define TYPES_H_

#include "ast.h"

typedef enum DataTypeName
{
    TYPE_NULL,

    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,

    DATA_TYPES
} DataTypeName;

static const char *INT_TYPE_ASM[DATA_TYPES] = {
    [TYPE_INT8]  = "BYTE",
    [TYPE_INT16] = "WORD",
    [TYPE_INT32] = "DWORD",
    [TYPE_INT64] = "QWORD",
};

typedef struct DataType
{
    DataTypeName type;
    union {};
} DataType;

DataType data_type_name(const AST *ast);

#endif // TYPES_H_
