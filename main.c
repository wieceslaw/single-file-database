#include <stdio.h>
#include <string.h>
#include "src/allocator/allocator.h"
#include "heap/heap.h"

int main(void) {
    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_CLEAR};
    allocator_t *allocator;
    int res = allocator_init(&settings, &allocator);
    if (res != FILE_ST_OK) {
        printf("File error %d", res);
        return -1;
    }

    if (allocator_reserve_pages(allocator, 10) != ALLOCATOR_SUCCESS) {
        printf("unable extend");
        return -1;
    }

    page_t* page = allocator_get_page(allocator);
    if (page == NULL) {
        printf("unable to get block");
        return -1;
    }

    int size = 5000;

    if (heap_place(page, 0, size) != HEAP_OP_SUCCESS) {
        printf("unable to create heap");
        return -1;
    }

    heap_t *heap = heap_init(page, 0, allocator);
    if (heap == NULL) {
        printf("unable to create heap");
        return -1;
    }

    for (int i = 0; i < 1000; i++) {
        buffer bf;
        buffer_init(&bf, size);
        memcpy(bf.data, "123456789", 10);
        if (heap_append(heap, &bf) != HEAP_OP_SUCCESS) {
            printf("unable to append data to heap");
            return -1;
        }
        buffer_free(&bf);
    }

    int count = 0;
    heap_it *it = heap_iterator(heap);
    while (!heap_iterator_is_empty(it)) {
        count++;
        buffer bf;
        buffer_init(&bf, size);
        if (heap_iterator_get(it, &bf) != HEAP_OP_SUCCESS) {
            printf("unable to get heap it data");
            return -1;
        }
        printf("%s \n", bf.data);
        buffer_free(&bf);
        if (heap_iterator_next(it) != HEAP_OP_SUCCESS) {
            printf("unable to next heap it");
            return -1;
        }
    }
    printf("count: %d", count);

    if (heap_iterator_free(it) != HEAP_OP_SUCCESS) {
        printf("unable to free heap iterator");
        return -1;
    }

    heap_free(heap);
    if (allocator_unmap_page(allocator, page) != ALLOCATOR_SUCCESS) {
        printf("unable unmap page");
        return -1;
    }

    if (allocator_free(allocator) != FILE_ST_OK) {
        printf("unable free allocator");
        return -1;
    }
    return 0;
}
