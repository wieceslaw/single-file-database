//
// Created by vyach on 11.10.2023.
//

#include "string.h"
#include "util/exceptions/exceptions.h"

/// THROWS: [MALLOC_EXCEPTION]
char* string_copy(char* src) {
    size_t length = strlen(src) + 1;
    char* copy = rmalloc(length);
    memcpy(copy, src, length);
    return copy;
}
