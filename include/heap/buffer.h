//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_BUFFER_H
#define LLP_LAB1_BUFFER_H

#include <stdint-gcc.h>

typedef struct {
    int32_t size;
    char *data;
} buffer;

void buffer_init(buffer *bf, int32_t size);

void buffer_free(buffer *bf);

void buffer_copy(const buffer *from, buffer *to);

void buffer_from_string(buffer *bf, const char *string);

#endif //LLP_LAB1_BUFFER_H
