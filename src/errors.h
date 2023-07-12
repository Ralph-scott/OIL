#ifndef ERRORS_H_
#define ERRORS_H_

#include "lexer.h"

#define UNEXPECTED_TOKEN(_tok)              \
    do {                                    \
        if ((_tok).type == TOKEN_EOF) {     \
            ERROR("Unexpected EOF.");       \
        } else {                            \
            ERROR(                          \
                "Unexpected token `%.*s`.", \
                (int) token.len,            \
                token.text                  \
            );                              \
        }                                   \
    } while (0)

#endif // ERRORS_H_
