//
// Created by vyach on 11.10.2023.
//

#include "util_string.h"
#include "string.h"
#include "exceptions/exceptions.h"

/// THROWS: [MALLOC_EXCEPTION]
char* string_copy(char* src) {
    size_t length = strlen(src) + 1;
    char* copy = rmalloc(length);
    memcpy(copy, src, length);
    return copy;
}
