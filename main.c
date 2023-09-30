#include <stdio.h>
#include <string.h>
#include "src/allocator/allocator.h"
#include "heap/heap.h"

static void print_heap(heap_t *heap) {
    int count = 0;
    heap_it *it = heap_iterator(heap);
    while (!heap_iterator_is_empty(it)) {
        count++;
        buffer_t *record = heap_iterator_get(it);
        if (NULL == record) {
            printf("unable to get heap_idx it buffer");
            return;
        }
//        printf("%s \n", record->data);
        buffer_free(record);
        if (heap_iterator_next(it) != HEAP_OP_SUCCESS) {
            printf("unable to next heap_idx it");
            return;
        }
    }
    printf("count: %d \n", count);
    if (heap_iterator_free(it) != HEAP_OP_SUCCESS) {
        printf("unable to free heap_idx iterator");
        return;
    }
}

static void test_heap(allocator_t *allocator) {
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
        buffer_t *buffer = buffer_init(10);
        memcpy(buffer->data, "123456789", 10);
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

//    heap_it *it = heap_iterator(heap);
//    while (!heap_iterator_is_empty(it)) {
//        heap_iterator_delete(it);
//        heap_iterator_next(it);
//    }

//    printf("before delete flush \n");
//    print_heap(heap);
//
//    if (heap_flush(heap) != HEAP_OP_SUCCESS) {
//        printf("unable to flush after delete \n");
//    }
//
//    printf("after delete flush \n");
//    print_heap(heap);

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
