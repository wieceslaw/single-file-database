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

void pool_free(pool_t *);

pool_result pool_append(pool_t *, buffer_t *); // append row and mark as "to_be_added"

pool_result pool_flush(pool_it *); // real deletion of "to_be_deleted" and unmarking "to_be_added", compressing heaps

pool_it *pool_iterator(pool_t *); // "to_be_deleted" won't be ignored by iterator, but "to_be_added" would

pool_result pool_iterator_free(pool_it *);

bool pool_iterator_is_empty(pool_it *);

pool_result pool_iterator_next(pool_it *);

buffer_t * pool_iterator_get(pool_it *);

buffer_t * pool_iterator_restart(pool_it *);

pool_result pool_iterator_delete(pool_it *); // mark current row as "to_be_deleted"

// delete query
// 1) iterate over rows, call delete()
// 2) flush pool

// update query
// 1) iterate over rows, call delete()
// 2) call add() with new row
// 3) flush pool

#endif //LLP_LAB1_POOL_H
