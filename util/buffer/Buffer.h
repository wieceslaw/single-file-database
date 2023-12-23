//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_BUFFER_H
#define LLP_LAB1_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct Buffer {
    uint64_t size;
    char *data;
    uint64_t rcur;
    uint64_t wcur;
} *Buffer;

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

Buffer BufferNew(uint64_t size);

void BufferFree(Buffer *buffer);

Buffer BufferCopy(Buffer buffer);

void BufferReset(Buffer buffer);

bool BufferIsEmpty(Buffer buffer);

char *BufferReadString(Buffer buffer);

b64_t BufferReadB64(Buffer buffer);

b32_t BufferReadB32(Buffer buffer);

b8_t BufferReadB8(Buffer buffer);

void BufferWriteString(Buffer buffer, const char *string);

void BufferWriteB64(Buffer buffer, b64_t num);

void BufferWriteB32(Buffer buffer, b32_t num);

void BufferWriteB8(Buffer buffer, b8_t num);

#endif //LLP_LAB1_BUFFER_H
