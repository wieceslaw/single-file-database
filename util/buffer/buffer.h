//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_BUFFER_H
#define LLP_LAB1_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct buffer {
    uint64_t size;
    char *data;
    uint64_t rcur;
    uint64_t wcur;
} *buffer_t;

typedef union u32 {
    int32_t i32;
    uint32_t ui32;
    float f32;
} b32_t;

typedef union u64 {
    int64_t i64;
    uint64_t ui64;
    double f64;
} b64_t;

typedef union u8 {
    int8_t i8;
    uint8_t ui8;
} b8_t;

buffer_t buffer_init(uint64_t size);

void buffer_free(buffer_t *buffer);

buffer_t buffer_copy(buffer_t buffer);

void buffer_reset(buffer_t buffer);

bool buffer_is_empty(buffer_t buffer);

char *buffer_read_string(buffer_t buffer);

b64_t buffer_read_b64(buffer_t buffer);

b32_t buffer_read_b32(buffer_t buffer);

b8_t buffer_read_b8(buffer_t buffer);

void buffer_write_string(buffer_t buffer, const char *string);

void buffer_write_b64(buffer_t buffer, b64_t num);

void buffer_write_b32(buffer_t buffer, b32_t num);

void buffer_write_b8(buffer_t buffer, b8_t num);

#endif //LLP_LAB1_BUFFER_H
