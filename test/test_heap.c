//
// Created by vyach on 01.10.2023.
//

#include <stdio.h>
#include "test.h"

void print_heap(heap_t *heap) {
    int count = 0;
    heap_it *it = heap_iterator(heap);
    while (!heap_iterator_is_empty(it)) {
        count++;
        buffer_t *record = heap_iterator_get(it);
        if (NULL == record) {
            printf("unable to get heap_idx it buffer");
            return;
        }
        printf("%s \n", record->data);
        buffer_free(record);
        if (heap_iterator_next(it) != HEAP_OP_SUCCESS) {
            printf("unable to next heap_idx it");
            return;
        }
    }
    printf("count: %d \n", count);
    heap_iterator_free(it);
}

void test_heap(allocator_t *allocator) {
    page_t *page = allocator_get_page(allocator);
    if (page == NULL) {
        printf("unable to get block");
        return;
    }

    heap_place(page, 0, 32);

    heap_t *heap = heap_init(page, 0, allocator);
    if (heap == NULL) {
        printf("unable to create heap_idx");
        return;
    }

    printf("before append \n");
    print_heap(heap);

    for (int i = 0; i < 300; i++) {
        buffer_t *buffer = buffer_init(32);
        sprintf(buffer->data, "%d", i);
        if (heap_append(heap, buffer) != HEAP_OP_SUCCESS) {
            printf("unable to append buffer to heap_idx");
            return;
        }
        buffer_free(buffer);
    }

    printf("before append flush \n");
    print_heap(heap);

    if (heap_flush(heap) != HEAP_OP_SUCCESS) {
        printf("unable to flush after append \n");
    }

    printf("after append flush \n");
    print_heap(heap);

    heap_it *it = heap_iterator(heap);
    while (!heap_iterator_is_empty(it)) {
        buffer_t *record = heap_iterator_get(it);
        if (*record->data == '2') {
            if (heap_iterator_delete(it) != HEAP_OP_SUCCESS) {
                printf("unable to delete record");
            }
        }
        buffer_free(record);
        heap_iterator_next(it);
    }

    printf("before delete flush \n");
    print_heap(heap);

    if (heap_flush(heap) != HEAP_OP_SUCCESS) {
        printf("unable to flush after delete \n");
    }

    printf("after delete flush \n");
    print_heap(heap);

    heap_free(heap);
    if (allocator_unmap_page(allocator, page) != ALLOCATOR_SUCCESS) {
        printf("unable unmap page");
        return;
    }
}
