//
// Created by vyach on 16.09.2023.
//

#include "heap/heap.h"
#include "heap/list.h"

struct heap {
    list *list;
};

heap *heap_init(block_addr addr);

void heap_free(heap *heap);

heap_result heap_compress(heap_it *heap);

heap_result heap_append(heap *heap, buffer *data);

heap_result heap_iterator(heap *heap, heap_it *it);

heap_result heap_iterator_free(heap_it *it);

bool heap_iterator_is_empty(heap_it *it);

heap_result heap_iterator_next(heap_it *it);

heap_result heap_iterator_get(heap_it *it, buffer *data);

heap_result heap_iterator_delete(heap_it *it); // should move objects? should return back excess blocks

heap_result heap_iterator_replace(heap_it *it, buffer *data);
