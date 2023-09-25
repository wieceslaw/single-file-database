//
// Created by vyach on 12.09.2023.
//

#ifndef LLP_LAB1_HEAP_H
#define LLP_LAB1_HEAP_H

#include <stdbool.h>
#include <stddef.h>
#include "../allocator/buffer.h"
#include "../allocator/allocator.h"

typedef struct heap heap;

typedef struct heap_it heap_it;

typedef enum {
    HEAP_OP_SUCCESS = 0,
    HEAP_OP_ERROR = 1
} heap_result;

heap_result heap_place(page_t *page, offset_t offset, offset_t record_size);

heap_result heap_clear(page_t *page, offset_t page_offset, allocator_t *allocator);

heap *heap_init(allocator_t *allocator, offset_t size);

void heap_free(heap *heap);

offset_t heap_size();

heap_result heap_compress(heap_it *heap); // fills records from "freelist" moving last records, after removes empty nodes

heap_result heap_append(heap *heap, buffer *data);

heap_result heap_iterator(heap *heap, heap_it *it);

heap_result heap_iterator_free(heap_it *it);

bool heap_iterator_is_empty(heap_it *it);

heap_result heap_iterator_next(heap_it *it);

heap_result heap_iterator_get(heap_it *it, buffer *data);

heap_result heap_iterator_delete(heap_it *it); // marks record as deleted (which will be ignored), add to "freelist"

heap_result heap_iterator_replace(heap_it *it, buffer *data);

#endif //LLP_LAB1_HEAP_H
