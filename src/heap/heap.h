//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_HEAP_H
#define LLP_LAB1_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include "../allocator/buffer.h"
#include "../allocator/allocator.h"

typedef struct heap_t heap_t;

typedef struct heap_it heap_it;

typedef enum {
    HEAP_OP_SUCCESS = 0,
    HEAP_OP_ERROR = 1
} heap_result;

void heap_place(page_t *page, offset_t offset, offset_t record_size);

heap_result heap_clear(heap_t *heap);

bool heap_is_empty(heap_t *heap);

heap_t *heap_init(page_t *page, offset_t page_offset, allocator_t *allocator);

void heap_free(heap_t *heap);

offset_t heap_size(void);

heap_result heap_flush(heap_t *heap);

heap_result heap_append(heap_t *heap, buffer_t *buffer);

heap_it *heap_iterator(heap_t *heap);

heap_result heap_iterator_free(heap_it *it);

bool heap_iterator_is_empty(heap_it *it);

heap_result heap_iterator_next(heap_it *it);

buffer_t *heap_iterator_get(heap_it *it);

heap_result heap_iterator_delete(heap_it *it);

#endif //LLP_LAB1_HEAP_H
