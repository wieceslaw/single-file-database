//
// Created by vyach on 27.09.2023.
//

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "allocator/buffer.h"

buffer_t *buffer_init(uint64_t size) {
    buffer_t *buffer = malloc(sizeof(buffer_t));
    if (NULL == buffer) {
        return NULL;
    }
    buffer->size = size;
    buffer->cur = 0;
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
    buffer->cur = 0;
    free(buffer);
}

buffer_t *buffer_copy(const buffer_t *buffer) {
    assert(buffer != NULL);
    buffer_t *result = buffer_init(buffer->size);
    if (NULL == buffer) {
        return NULL;
    }
    memcpy(result->data, buffer->data, buffer->size);
    return result;
}

void buffer_reset(buffer_t *buffer) {
    assert(buffer != NULL);
    buffer->cur = 0;
}

bool buffer_is_empty(buffer_t *buffer) {
    assert(buffer != NULL);
    return buffer->cur >= buffer->size;
}

char *buffer_read_string(buffer_t *buffer) {
    size_t length = strlen(buffer->data + buffer->cur);
    char *string = malloc(length + 1);
    if (NULL == string) {
        return NULL;
    }
    memcpy(string, buffer->data + buffer->cur, length);
    buffer->cur += length;
    return string;
}

u32_t buffer_read_u32(buffer_t *buffer) {
    u32_t number = *((u32_t *) (buffer->data + buffer->cur));
    buffer->cur += sizeof(u32_t);
    return number;
}

uint8_t buffer_read_u8(buffer_t *buffer) {
    uint8_t number = *((uint8_t *) (buffer->data + buffer->cur));
    buffer->cur += sizeof(uint8_t);
    return number;
}
