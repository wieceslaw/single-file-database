//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "util_string.h"
#include "string.h"

char* string_copy(char* src) {
    size_t length = strlen(src) + 1;
    char* copy = malloc(length);
    assert(copy != NULL);
    memcpy(copy, src, length);
    return copy;
}
