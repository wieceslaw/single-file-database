//
// Created by vyach on 25.09.2023.
//

#include "pool.h"
#include "heap/heap.h"

#define POOL_SIZE  57
#define POOL_START 64

struct pool_t {
    page_t *page;
};

//typedef struct {
//    pool_t *pool;
//    int heap;
//} pool_heaps_it;
//
//bool pool_heaps_it_is_empty();
//
//void pool_heaps_it_next();
//
//void pool_heaps_it_get();
//
//offset_t pool_create(allocator_t *allocator) {
//    page_t *page = allocator_get_page(allocator);
//    if (NULL == page) {
//        return 0;
//    }
//    uint64_t acc = POOL_START;
//    offset_t offset = 0;
//    for (int i = 0; i < POOL_SIZE; i++) {
//        heap_place(page, offset, acc);
//        acc *= 2;
//        offset += heap_size();
//    }
//    offset_t result = page_offset(page);
//    if (allocator_unmap_page(allocator, page) != ALLOCATOR_SUCCESS) {
//        return 0;
//    }
//    return result;
//}
//
////pool_result pool_clear(allocator_t *allocator, offset_t offset) {
////    page_t *page = allocator_map_page(allocator, offset);
////    if (NULL == page) {
////        return POOL_OP_ERROR;
////    }
////}
