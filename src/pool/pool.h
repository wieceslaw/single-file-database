//
// Created by vyach on 25.09.2023.
//

#ifndef LLP_LAB1_POOL_H
#define LLP_LAB1_POOL_H

#include "allocator/allocator.h"

typedef enum {
    POOL_OP_SUCCESS = 0,
    POOL_OP_ERROR = 1
} pool_result;

typedef struct pool_t pool_t;

offset_t pool_create(allocator_t *);

pool_result pool_clear(allocator_t *, offset_t);

pool_t* pool_init(allocator_t *, offset_t);

#endif //LLP_LAB1_POOL_H
