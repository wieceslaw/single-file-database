//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_ALLOCATOR_H
#define LLP_LAB1_ALLOCATOR_H

#include <stdint-gcc.h>

#define BLOCK_SIZE 4096

typedef uint64_t offset_t;

typedef enum {
    ALLOCATOR_SUCCESS = 0,
    ALLOCATOR_UNABLE_MAP = 1,
    ALLOCATOR_UNABLE_UNMAP = 2,
    ALLOCATOR_UNABLE_EXTEND = 3,
} allocator_result;

typedef struct allocator allocator;

typedef struct block block;

offset_t block_offset(block *);

void *block_ptr(block *);

typedef struct {
    block* block;
    uint16_t offset;
} block_addr;

void* block_addr_get(block_addr* block_addr) {
    return block_ptr(block_addr->block) + block_addr->offset;
}

allocator_result allocator_collect_block(allocator *allocator, offset_t offset);

block *allocator_get_block(allocator *allocator);

allocator_result allocator_reserve_blocks(allocator *allocator, uint32_t n);

block *allocator_map_block(allocator *allocator, offset_t offset);

allocator_result allocator_unmap_block(allocator *allocator, block *block);

#endif //LLP_LAB1_ALLOCATOR_H
