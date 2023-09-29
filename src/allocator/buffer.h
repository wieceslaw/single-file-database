//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_BUFFER_H
#define LLP_LAB1_BUFFER_H

#include <stdint.h>

typedef struct {
    uint64_t size;
    char *data;
} buffer_t;

buffer_t *buffer_init(uint64_t size);

void buffer_free(buffer_t *buffer);

void buffer_copy(const buffer_t *from, buffer_t *to);

#endif //LLP_LAB1_BUFFER_H
