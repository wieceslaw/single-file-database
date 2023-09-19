//
// Created by vyach on 16.09.2023.
//

#ifndef LLP_LAB1_POOL_H
#define LLP_LAB1_POOL_H

#include <stdbool.h>
#include "heap/buffer.h"

typedef enum {
    POOL_OP_SUCCESS = 0,
    POOL_OP_ERROR = 1
} pool_result;

typedef struct pool pool;

typedef struct pool_it pool_it;

pool_result pool_iterator(pool *pool, pool_it *it);

pool_result pool_append(pool *pool, buffer *data);

pool_result pool_flush(pool_it *it); // real deletion and memory optimization

bool pool_iterator_is_empty(pool_it *it);

pool_result pool_iterator_free(pool_it *it);

pool_result pool_iterator_next(pool_it *it);

pool_result pool_iterator_get(pool_it *it, buffer *data);

pool_result pool_iterator_get_block_addr();

pool_result pool_iterator_delete(pool_it *it); // mark as deleted

#endif //LLP_LAB1_POOL_H
