#ifndef TYPES_H_
#define TYPES_H_

typedef enum DataType
{
    TYPE_NULL,

    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,

    DATA_TYPES,
} DataType;

DataType data_type_new(const char *name, size_t name_len);

#endif // TYPES_H_
