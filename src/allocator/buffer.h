//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_BUFFER_H
#define LLP_LAB1_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t size;
    char *data;
    uint64_t cur;
} buffer_t;

typedef union u32 {
    int32_t i32;
    uint32_t ui32;
    float f32;
} u32_t;

buffer_t *buffer_init(uint64_t size);

void buffer_free(buffer_t *buffer);

buffer_t *buffer_copy(const buffer_t *buffer);

void buffer_reset(buffer_t *buffer);

bool buffer_is_empty(buffer_t *buffer);

char* buffer_read_string(buffer_t *buffer);

u32_t buffer_read_u32(buffer_t *buffer);

uint8_t buffer_read_u8(buffer_t *buffer);

#endif //LLP_LAB1_BUFFER_H
