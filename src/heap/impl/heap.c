//
// Created by vyach on 25.09.2023.
//

#include "heap/heap.h"
#include "list.h"

typedef struct __attribute__((__packed__)) {
    list_h list_header;
    offset_t record_size;
    offset_t free_record;
} heap_h;

offset_t heap_size() {
    return sizeof(heap_h);
}

heap_result heap_place(page_t *page, offset_t offset, offset_t record_size);

heap_result heap_clear(page_t *page, offset_t page_offset, allocator_t *allocator);

heap *heap_init(allocator_t *allocator, offset_t size);

void heap_free(heap *heap);

heap_result heap_compress(heap_it *heap);

heap_result heap_append(heap *heap, buffer *data);

heap_result heap_iterator(heap *heap, heap_it *it);

heap_result heap_iterator_free(heap_it *it);

bool heap_iterator_is_empty(heap_it *it);

heap_result heap_iterator_next(heap_it *it);

heap_result heap_iterator_get(heap_it *it, buffer *data);

heap_result heap_iterator_delete(heap_it *it);

heap_result heap_iterator_replace(heap_it *it, buffer *data);
