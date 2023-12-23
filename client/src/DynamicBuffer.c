//
// Created by wieceslaw on 24.12.23.
//

#include <malloc.h>
#include <string.h>
#include "DynamicBuffer.h"

int DynamicBufferPut(struct DynamicBuffer *buffer, char c) {
    if (buffer->data == NULL) {
        buffer->data = malloc(4);
        buffer->size = 4;
    }
    if (buffer->size == buffer->ptr) {
        size_t newSize = buffer->size * 2;
        char *newData = malloc(newSize);
        memcpy(newData, buffer->data, buffer->size);
        free(buffer->data);
        buffer->data = newData;
        buffer->size = newSize;
    }
    buffer->data[buffer->ptr++] = c;
    return 0;
}

void DynamicBufferFree(struct DynamicBuffer *buffer) {
    free(buffer->data);
    *buffer = (struct DynamicBuffer) {0};
}

void DynamicBufferReset(struct DynamicBuffer *buffer) {
    buffer->ptr = 0;
}
