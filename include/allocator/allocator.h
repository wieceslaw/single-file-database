//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_ALLOCATOR_H
#define LLP_LAB1_ALLOCATOR_H

#include <stdint-gcc.h>

#define BLOCK_SIZE 65536
#define MAGIC 0xABCD

typedef uint64_t offset_t;

typedef enum {
    ALLOCATOR_SUCCESS = 0,
    ALLOCATOR_UNABLE_MAP = 1,
    ALLOCATOR_UNABLE_UNMAP = 2,
    ALLOCATOR_UNABLE_EXTEND = 3,
} allocator_result;

typedef enum {
    FILE_ST_OK = 0,
    FILE_ST_ALREADY_EXISTS = 1,
    FILE_ST_NOT_EXIST = 2,
    FILE_ST_WRONG_FORMAT = 3,
    FILE_ST_UNABLE_OPEN = 4,
    FILE_ST_UNABLE_RELEASE = 5,
    FILE_ST_ERROR = 6,
} file_status;

typedef enum {
    FILE_OPEN_CREATE,
    FILE_OPEN_EXIST,
    FILE_OPEN_CLEAR
} file_open_type;

typedef struct {
    char *path;
    file_open_type open_type;
} file_settings;

typedef struct {
    int32_t magic;
    uint32_t free_blocks_count;
    offset_t free_blocks_next;
    // entry point
} file_h;

typedef struct allocator allocator;

typedef struct block block;

offset_t block_offset(block *);

void *block_ptr(block *);

typedef struct {
    block *block;
    uint16_t offset;
} block_addr;

void *block_addr_get(block_addr *block_addr);

file_status allocator_init(file_settings *, allocator **);

file_status allocator_free(allocator *);

allocator_result allocator_return_block(allocator *, offset_t offset);

block *allocator_get_block(allocator *);

allocator_result allocator_reserve_blocks(allocator *, uint32_t n);

block *allocator_map_block(allocator *, offset_t offset);

allocator_result allocator_unmap_block(allocator *, block *block);

#endif //LLP_LAB1_ALLOCATOR_H
