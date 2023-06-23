#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>

#define loop for (;;)

#define ARRAY_LEN(_arr) (sizeof((_arr)) / sizeof((_arr)[0]))

#define RED   "\x1B[31m"
#define RESET "\x1B[0m"

#define ERROR(_msg, ...)                   \
    do {                                   \
        fprintf(                           \
            stderr,                        \
            RED "ERROR: " RESET _msg "\n", \
            ##__VA_ARGS__                  \
        );                                 \
        exit(1);                           \
    } while (0)

#define UNREACHABLE() ERROR("Unreachable.")
#define TODO() ERROR("Unimplemented.")
#define ALLOCATION_ERROR() ERROR("Failed to allocate memory.")

#endif // UTILS_H_
