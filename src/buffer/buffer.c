//
// Created by vyach on 27.09.2023.
//

#include <stddef.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "buffer/buffer.h"

buffer_t *buffer_init(uint64_t size) {
    buffer_t *buffer = malloc(sizeof(buffer_t));
    if (NULL == buffer) {
        return NULL;
    }
    buffer->size = size;
    buffer->rcur = 0;
    buffer->wcur = 0;
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
    buffer->rcur = 0;
    buffer->wcur = 0;
    free(buffer);
}

buffer_t *buffer_copy(const buffer_t *const buffer) {
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
    buffer->rcur = 0;
    buffer->wcur = 0;
}

bool buffer_is_empty(const buffer_t *const buffer) {
    assert(buffer != NULL);
    return buffer->rcur >= buffer->size;
}

char *buffer_read_string(buffer_t *const buffer) {
    assert(buffer != NULL);
    size_t length = strlen(buffer->data + buffer->rcur) + 1;
    uint64_t moved = buffer->rcur + length;
    if (moved > buffer->size) {
        return NULL;
    }
    char *string = malloc(length);
    if (NULL == string) {
        return NULL;
    }
    memcpy(string, buffer->data + buffer->rcur, length);
    buffer->rcur = moved;
    return string;
}

b64_t buffer_read_b64(buffer_t *const buffer) {
    assert(buffer != NULL);
    uint64_t moved = buffer->rcur + sizeof(b64_t);
    if (moved > buffer->size) {
        return (b64_t) {0};
    }
    b64_t num = *((b64_t *) (buffer->data + buffer->rcur));
    buffer->rcur = moved;
    return num;
}

b32_t buffer_read_b32(buffer_t *const buffer) {
    assert(buffer != NULL);
    uint64_t moved = buffer->rcur + sizeof(b32_t);
    if (moved > buffer->size) {
        return (b32_t) {0};
    }
    b32_t num = *((b32_t *) (buffer->data + buffer->rcur));
    buffer->rcur = moved;
    return num;
}

b8_t buffer_read_b8(buffer_t *const buffer) {
    assert(buffer != NULL);
    uint64_t moved = buffer->rcur + sizeof(b8_t);
    if (moved > buffer->size) {
        return (b8_t) {0};
    }
    b8_t num = *((b8_t *) (buffer->data + buffer->rcur));
    buffer->rcur = moved;
    return num;
}

void buffer_write_string(buffer_t *const buffer, const char *const string) {
    assert(buffer != NULL && string != NULL);
    size_t length = strlen(string) + 1;
    uint64_t moved = buffer->wcur + length;
    if (moved > buffer->size) {
        return;
    }
    memcpy(buffer->data + buffer->wcur, string, length);
    buffer->wcur = moved;
}

void buffer_write_b64(buffer_t *const buffer, b64_t num) {
    assert(buffer != NULL);
    uint64_t moved = buffer->wcur + sizeof(num);
    if (moved > buffer->size) {
        return;
    }
    b64_t *ptr = (b64_t *) (buffer->data + buffer->wcur);
    *ptr = num;
    buffer->wcur = moved;
}

void buffer_write_b32(buffer_t *const buffer, b32_t num) {
    assert(buffer != NULL);
    uint64_t moved = buffer->wcur + sizeof(num);
    if (moved > buffer->size) {
        return;
    }
    b32_t *ptr = (b32_t *) (buffer->data + buffer->wcur);
    *ptr = num;
    buffer->wcur = moved;
}

void buffer_write_b8(buffer_t *const buffer, b8_t num) {
    assert(buffer != NULL);
    uint64_t moved = buffer->wcur + sizeof(num);
    if (moved > buffer->size) {
        return;
    }
    b8_t *ptr = (b8_t *) (buffer->data + buffer->wcur);
    *ptr = num;
    buffer->wcur = moved;
}
