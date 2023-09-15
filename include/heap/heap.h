//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_HEAP_H
#define LLP_LAB1_HEAP_H

#include <stdbool.h>
#include "buffer.h"

typedef struct {
} heap;

typedef struct {
} heap_it;

bool heap_iterator_is_empty(heap_it *it);

void heap_iterator_next(heap_it *it);

buffer heap_iterator_get(heap_it *it);

void heap_iterator_delete(heap_it *it);

void heap_iterator_free(heap_it *it);

heap_it heap_get_iterator(heap *heap);

void heap_append(heap *heap, buffer *data);

#endif //LLP_LAB1_HEAP_H
