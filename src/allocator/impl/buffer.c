//
// Created by vyach on 27.09.2023.
//

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include "allocator/buffer.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

void buffer_init(buffer *bf, int32_t size) {
    bf->size = size;
    if (size == 0) {
        bf->data = NULL;
    } else {
        bf->data = malloc(size);
    }
}

void buffer_free(buffer *bf) {
    free(bf->data);
    bf->data = NULL;
    bf->size = 0;
}

void buffer_copy(const buffer *from, buffer *to) {
    memcpy(to->data, from->data, MIN(from->size, to->size));
}

void buffer_from_string(buffer *bf, const char *string) {
    int32_t len = strlen(string) + 1;
    bf->size = len;
    bf->data = malloc(bf->size);
    strcpy(bf->data, string);
}
