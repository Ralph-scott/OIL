#ifndef TYPES_H_
#define TYPES_H_

#include "ast.h"

typedef enum DataTypeName
{
    TYPE_INT32,
    TYPE_INT64,
} DataTypeName;

typedef struct DataType
{
    DataTypeName type;
    union {};
} DataType;

DataType data_type_name(const AST *ast);

#endif // TYPES_H_
