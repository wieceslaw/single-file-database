//
// Created by vyach on 27.09.2023.
//

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "allocator/buffer.h"
#include "allocator/allocator.h"

buffer_t *buffer_init(uint64_t size) {
    buffer_t *buffer = malloc(sizeof(buffer_t));
    if (NULL == buffer) {
        return NULL;
    }
    buffer->size = size;
    if (size == 0) {
        buffer->data = NULL;
    } else {
        buffer->data = malloc(size);
        if (NULL == buffer->data) {
            free(buffer);
            return NULL;
        }
    }
    return buffer;
}

void buffer_free(buffer_t *buffer) {
    assert(buffer != NULL);
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    free(buffer);
}

void buffer_copy(const buffer_t *from, buffer_t *to) {
    assert(from != NULL && to != NULL);
    memcpy(to->data, from->data, MIN(from->size, to->size));
}
