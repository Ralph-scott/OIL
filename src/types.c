#include <string.h>
#include "types.h"
#include "utils.h"

DataType data_type_new(const char *name, size_t name_len)
{
    if (strncmp(name, "s8", name_len) == 0) {
        return TYPE_INT8;
    } else if (strncmp(name, "s16", name_len) == 0) {
        return TYPE_INT16;
    } else if (strncmp(name, "s32", name_len) == 0
            || strncmp(name, "int", name_len) == 0) {
        return TYPE_INT32;
    } else if (strncmp(name, "s64", name_len) == 0) {
        return TYPE_INT64;
    } else {
        ERROR("Type `%.*s` does not exist.", (int) name_len, name);
    }
}
