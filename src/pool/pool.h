//
// Created by vyach on 25.09.2023.
//

#ifndef LLP_LAB1_POOL_H
#define LLP_LAB1_POOL_H

#include <stdbool.h>
#include "allocator/allocator.h"
#include "allocator/buffer.h"

typedef enum {
    POOL_OP_SUCCESS = 0,
    POOL_OP_ERROR = 1
} pool_result;

typedef struct pool_t pool_t;

typedef struct pool_it pool_it;

offset_t pool_create(allocator_t *); // create on file memory

pool_result pool_clear(allocator_t *, offset_t); // remove from file memory

pool_t *pool_init(allocator_t *, offset_t);

pool_result pool_free(pool_t *);

pool_result pool_append(pool_t *, buffer_t *);

pool_result pool_flush(pool_t *); // all data modifications will only be applied after flush call

pool_it *pool_iterator(pool_t *);

pool_result pool_iterator_free(pool_it *);

bool pool_iterator_is_empty(pool_it *);

pool_result pool_iterator_next(pool_it *);

buffer_t * pool_iterator_get(pool_it *);

pool_result pool_iterator_restart(pool_it *);

pool_result pool_iterator_delete(pool_it *);

#endif //LLP_LAB1_POOL_H
