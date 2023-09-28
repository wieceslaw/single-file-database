#include <stdio.h>
#include <string.h>
#include "src/allocator/allocator.h"
#include "heap/heap.h"


static void test_heap(allocator_t *allocator) {
    page_t *page = allocator_get_page(allocator);
    if (page == NULL) {
        printf("unable to get block");
        return;
    }

    if (heap_place(page, 0, 32) != HEAP_OP_SUCCESS) {
        printf("unable to create heap");
        return;
    }

    heap_t *heap = heap_init(page, 0, allocator);
    if (heap == NULL) {
        printf("unable to create heap");
        return;
    }

    for (int i = 0; i < 1000; i++) {
        buffer_t *buffer = buffer_init(10);
        memcpy(buffer->data, "123456789", 10);
        if (heap_append(heap, buffer) != HEAP_OP_SUCCESS) {
            printf("unable to append data to heap");
            return;
        }
        buffer_free(buffer);
    }

    int count = 0;
    heap_it *it = heap_iterator(heap);
    while (!heap_iterator_is_empty(it)) {
        count++;
        buffer_t *buffer = heap_iterator_get(it);
        if (buffer == NULL) {
            printf("unable to get heap it data");
            return;
        }
        printf("%s \n", buffer->data);
        buffer_free(buffer);
        if (heap_iterator_next(it) != HEAP_OP_SUCCESS) {
            printf("unable to next heap it");
            return;
        }
    }
    printf("count: %d", count);

    if (heap_iterator_free(it) != HEAP_OP_SUCCESS) {
        printf("unable to free heap iterator");
        return;
    }

    heap_free(heap);
    if (allocator_unmap_page(allocator, page) != ALLOCATOR_SUCCESS) {
        printf("unable unmap page");
        return;
    }
}

int main(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_CLEAR};
    allocator_t *allocator;
    int res = allocator_init(&settings, &allocator);
    if (res != FILE_ST_OK) {
        printf("File error %d", res);
        return -1;
    }

    test_heap(allocator);

    if (allocator_free(allocator) != FILE_ST_OK) {
        printf("unable free allocator");
        return -1;
    }
    return 0;
}
