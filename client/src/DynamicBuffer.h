//
// Created by wieceslaw on 24.12.23.
//

#ifndef SINGLE_FILE_DATABASE_DYNAMICBUFFER_H
#define SINGLE_FILE_DATABASE_DYNAMICBUFFER_H

#include <stddef.h>

struct DynamicBuffer {
    char *data;
    size_t ptr;
    size_t size;
};

int DynamicBufferPut(struct DynamicBuffer *buffer, char c);

void DynamicBufferFree(struct DynamicBuffer *buffer);

void DynamicBufferReset(struct DynamicBuffer *buffer);

#endif //SINGLE_FILE_DATABASE_DYNAMICBUFFER_H
