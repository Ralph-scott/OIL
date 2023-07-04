#ifndef TYPES_H_
#define TYPES_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct AST AST;

typedef enum DataTypeType {
    TYPE_NULL,

    TYPE_VOID,

    TYPE_REFERENCE,

    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,

    DATA_TYPES,
} DataTypeType;

typedef struct DataType
{
    DataTypeType type;
    union {
        struct DataType *dereference;
    };
} DataType;

DataType *data_type_type(DataTypeType type);
DataType *data_type_reference(DataType *dereference);
DataType *data_type_new(const AST *ast);
DataType *data_type_copy(const DataType *type);
void data_type_free(DataType *type);

#endif // TYPES_H_
