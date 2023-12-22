//
// Created by vyach on 25.09.2023.
//

#ifndef LLP_LAB1_POOL_H
#define LLP_LAB1_POOL_H

#include <stdbool.h>
#include "allocator/allocator.h"
#include "buffer/buffer.h"

typedef struct pool_t pool_t;

typedef struct pool_it *pool_it;

int pool_create(allocator_t *allocator, offset_t *off); // create on file memory

void pool_clear(allocator_t *, offset_t); // remove from file memory

pool_t *pool_init(allocator_t *, offset_t);

void pool_free(pool_t *);

int pool_append(pool_t *pool, buffer_t buffer);

int pool_flush(pool_t *pool); // all data modifications will only be applied after flush call

pool_it pool_iterator(pool_t *);

void pool_iterator_free(pool_it*);

bool pool_iterator_is_empty(pool_it);

void pool_iterator_next(pool_it);

buffer_t pool_iterator_get(pool_it);

void pool_iterator_delete(pool_it);

#endif //LLP_LAB1_POOL_H
