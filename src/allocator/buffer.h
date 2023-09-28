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

void buffer_init(buffer_t *bf, uint64_t size);

void buffer_free(buffer_t *bf);

void buffer_copy(const buffer_t *from, buffer_t *to);

void buffer_from_string(buffer_t *bf, const char *string);

#endif //LLP_LAB1_BUFFER_H
