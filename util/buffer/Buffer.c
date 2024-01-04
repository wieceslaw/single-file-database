//
// Created by vyach on 27.09.2023.
//

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "buffer/Buffer.h"

Buffer BufferNew(uint64_t size) {
    Buffer buffer = NULL;
    buffer = malloc(sizeof(struct Buffer));
    if (buffer == NULL) {
        assert(0);
    }
    buffer->size = size;
    buffer->rcur = 0;
    buffer->wcur = 0;
    if (size == 0) {
        buffer->data = NULL;
    } else {
        buffer->data = malloc(size);
        if (buffer->data == NULL) {
            free(buffer);
            assert(0);
        }
    }
    return buffer;
}

void BufferFree(Buffer *pBuffer) {
    assert(pBuffer != NULL);
    Buffer buffer = *pBuffer;
    if (NULL == buffer) {
        return;
    }
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->rcur = 0;
    buffer->wcur = 0;
    free(buffer);
    *pBuffer = NULL;
}

Buffer BufferCopy(Buffer buffer) {
    assert(buffer != NULL);
    Buffer result = BufferNew(buffer->size);
    memcpy(result->data, buffer->data, buffer->size);
    return result;
}

void BufferReset(Buffer buffer) {
    assert(buffer != NULL);
    buffer->rcur = 0;
    buffer->wcur = 0;
}

bool BufferIsEmpty(Buffer buffer) {
    assert(buffer != NULL);
    return buffer->rcur >= buffer->size;
}

char *BufferReadString(Buffer buffer) {
    assert(buffer != NULL);
    size_t length = strlen(buffer->data + buffer->rcur) + 1;
    uint64_t moved = buffer->rcur + length;
    if (moved > buffer->size) {
        return NULL;
    }
    char *string = malloc(length);
    if (string == NULL) {
        assert(0);
    }
    memcpy(string, buffer->data + buffer->rcur, length);
    buffer->rcur = moved;
    return string;
}

b64_t BufferReadB64(Buffer buffer) {
    assert(buffer != NULL);
    uint64_t moved = buffer->rcur + sizeof(b64_t);
    if (moved > buffer->size) {
        return (b64_t){0};
    }
    b64_t num = *((b64_t *) (buffer->data + buffer->rcur));
    buffer->rcur = moved;
    return num;
}

b32_t BufferReadB32(Buffer buffer) {
    assert(buffer != NULL);
    uint64_t moved = buffer->rcur + sizeof(b32_t);
    if (moved > buffer->size) {
        return (b32_t){0};
    }
    b32_t num;
    void* ptr = buffer->data + buffer->rcur;
    memcpy(&num, ptr, sizeof(b32_t));
    buffer->rcur = moved;
    return num;
}

b8_t BufferReadB8(Buffer buffer) {
    assert(buffer != NULL);
    uint64_t moved = buffer->rcur + sizeof(b8_t);
    if (moved > buffer->size) {
        return (b8_t){0};
    }
    b8_t num = *((b8_t *) (buffer->data + buffer->rcur));
    buffer->rcur = moved;
    return num;
}

void BufferWriteString(Buffer buffer, const char *string) {
    assert(buffer != NULL && string != NULL);
    size_t length = strlen(string) + 1;
    uint64_t moved = buffer->wcur + length;
    if (moved > buffer->size) {
        return;
    }
    memcpy(buffer->data + buffer->wcur, string, length);
    buffer->wcur = moved;
}

void BufferWriteB64(Buffer buffer, b64_t num) {
    assert(buffer != NULL);
    uint64_t moved = buffer->wcur + sizeof(num);
    if (moved > buffer->size) {
        return;
    }
    b64_t *ptr = (b64_t *) (buffer->data + buffer->wcur);
    *ptr = num;
    buffer->wcur = moved;
}

void BufferWriteB32(Buffer buffer, b32_t num) {
    assert(buffer != NULL);
    uint64_t moved = buffer->wcur + sizeof(num);
    if (moved > buffer->size) {
        return;
    }
    void *ptr = (b32_t *) (buffer->data + buffer->wcur);
    memcpy(ptr, &num, sizeof(b32_t));
    buffer->wcur = moved;
}

void BufferWriteB8(Buffer buffer, b8_t num) {
    assert(buffer != NULL);
    uint64_t moved = buffer->wcur + sizeof(num);
    if (moved > buffer->size) {
        return;
    }
    b8_t *ptr = (b8_t *) (buffer->data + buffer->wcur);
    *ptr = num;
    buffer->wcur = moved;
}
