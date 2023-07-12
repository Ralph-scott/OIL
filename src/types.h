#ifndef TYPES_H_
#define TYPES_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct AST AST;

typedef enum DataTypeType {
    TYPE_NULL,

    TYPE_VOID,

    TYPE_FUNCTION,

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
        struct {
            size_t len;
            size_t cap;
            struct DataType **arguments;
            struct DataType *return_type;
        } function;
    };
} DataType;

DataType *data_type_type(DataTypeType type);
DataType *data_type_reference(DataType *dereference);
DataType *data_type_function(size_t arguments_len, size_t arguments_cap, DataType **arguments, DataType *return_type);
DataType *data_type_new(const AST *ast);
DataType *data_type_copy(const DataType *type);
bool data_type_equals(const DataType *lhs, const DataType *rhs);
void data_type_free(DataType *type);

#endif // TYPES_H_
