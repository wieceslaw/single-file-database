//
// Created by vyach on 25.09.2023.
//

#include <assert.h>
#include "pool.h"
#include "heap/heap.h"
#include "exceptions/exceptions.h"

#define POOL_SIZE  57
#define POOL_START 64

struct pool_t {
    allocator_t *allocator;
    page_t *page;
    heap_t *heaps[POOL_SIZE];
};

struct pool_it {
    pool_t *pool;
    int heap_idx;
    heap_it* heap_it;
};

static int pool_is_valid(pool_t *pool) {
    return NULL != pool && NULL != pool->allocator && NULL != pool->page;
}

static int suitable_heap_idx(uint64_t size) {
    uint64_t start = POOL_START;
    int count = 0;
    while (start < size) {
        start <<= 1;
        count++;
    }
    return count;
}

int pool_create(allocator_t *allocator, offset_t *off) {
    assert(NULL != allocator);
    page_t *page = allocator_get_page(allocator);
    if (NULL == page) {
        return -1;
    }
    uint64_t acc = POOL_START;
    offset_t offset = 0;
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_place(page, offset, acc);
        acc *= 2;
        offset += heap_size();
    }
    offset_t pool_offset = page_offset(page);
    if (allocator_unmap_page(allocator, page) != ALLOCATOR_SUCCESS) {
        return -1;
    }
    *off = pool_offset;
    return 0;
}

pool_t *pool_init(allocator_t *allocator, offset_t pool_offset) {
    assert(NULL != allocator);
    pool_t *pool = malloc(sizeof(pool_t));
    if (pool == NULL) {
        return NULL;
    }
    pool->page = allocator_map_page(allocator, pool_offset);
    if (pool->page == NULL) {
        free(pool);
        return NULL;
    }
    pool->allocator = allocator;
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_t *heap = heap_init(pool->page, heap_size() * i, pool->allocator);
        if (heap == NULL) {
            // TODO: Free all heaps before
            free(pool);
            return NULL;
        }
        pool->heaps[i] = heap;
    }
    return pool;
}

int pool_free(pool_t *pool) {
    assert(pool_is_valid(pool));
    if (allocator_unmap_page(pool->allocator, pool->page) != ALLOCATOR_SUCCESS) {
        pool->page = NULL;
        pool->allocator = NULL;
        free(pool);
        return -1;
    }
    pool->page = NULL;
    pool->allocator = NULL;
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_free(pool->heaps[i]);
        pool->heaps[i] = NULL;
    }
    free(pool);
    return 0;
}

/// THROWS: [POOL_EXCEPTION]
void pool_clear(allocator_t *allocator, offset_t pool_offset) {
    assert(NULL != allocator);
    pool_t *pool = pool_init(allocator, pool_offset);
    if (pool == NULL) {
        RAISE(POOL_EXCEPTION);
    }
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_t *heap = pool->heaps[i];
        if (heap_clear(heap) != HEAP_OP_SUCCESS) {
            if (pool_free(pool) != 0) {
                RAISE(POOL_EXCEPTION);
            }
            RAISE(POOL_EXCEPTION);
        }
    }
    if (pool_free(pool) != 0) {
        RAISE(POOL_EXCEPTION);
    }
    if (allocator_return_page(allocator, pool_offset) != ALLOCATOR_SUCCESS) {
        RAISE(POOL_EXCEPTION);
    }
}

int pool_append(pool_t *pool, Buffer buffer) {
    assert(pool_is_valid(pool));
    int idx = suitable_heap_idx(buffer->size);
    if (heap_append(pool->heaps[idx], buffer) != HEAP_OP_SUCCESS) {
        return -1;
    }
    return 0;
}

int pool_flush(pool_t *pool) {
    assert(pool_is_valid(pool));
    for (int i = 0; i < POOL_SIZE; i++) {
        if (heap_flush(pool->heaps[i]) != HEAP_OP_SUCCESS) {
            return -1;
        }
    }
    return 0;
}

pool_it pool_iterator(pool_t *pool) {
    assert(pool_is_valid(pool));
    pool_it iterator = malloc(sizeof(struct pool_it));
    if (iterator == NULL) {
        return NULL;
    }
    iterator->pool = pool;
    iterator->heap_idx = 0;
    iterator->heap_it = heap_iterator(pool->heaps[0]);
    if (iterator->heap_it == NULL) {
        free(iterator);
        debug("Unable to get heap iterator");
        return NULL;
    }
    while (heap_iterator_is_empty(iterator->heap_it)) {
        heap_iterator_free(iterator->heap_it);
        iterator->heap_idx++;
        if (iterator->heap_idx == POOL_SIZE) {
            iterator->heap_idx = -1;
            iterator->heap_it = NULL;
            break;
        }
        iterator->heap_it = heap_iterator(pool->heaps[iterator->heap_idx]);
        if (iterator->heap_it == NULL) {
            free(iterator);
            debug("Unable to get heap iterator");
            return NULL;
        }
    }
    return iterator;
}

void pool_iterator_free(pool_it *it_ptr) {
    assert(NULL != it_ptr);
    pool_it it = *it_ptr;
    if (NULL == it) {
        return;
    }
    heap_iterator_free(it->heap_it);
    it->heap_it = NULL;
    it->heap_idx = 0;
    free(it);
    *it_ptr = NULL;
}

bool pool_iterator_is_empty(pool_it it) {
    assert(NULL != it);
    return -1 == it->heap_idx;
}

Buffer pool_iterator_get(pool_it it) {
    assert(NULL != it);
    if (pool_iterator_is_empty(it)) {
        return NULL;
    }
    return heap_iterator_get(it->heap_it);
}

int pool_iterator_delete(pool_it it) {
    assert(NULL != it);
    if (pool_iterator_is_empty(it)) {
        return -1;
    }
    if (heap_iterator_delete(it->heap_it) != HEAP_OP_SUCCESS) {
        return -1;
    }
    return 0;
}

int pool_iterator_next(pool_it it) {
    assert(NULL != it);
    if (pool_iterator_is_empty(it)) {
        return -1;
    }
    if (heap_iterator_next(it->heap_it) != HEAP_OP_SUCCESS) {
        debug("Unable to move heap iterator");
        return -1;
    }
    while (heap_iterator_is_empty(it->heap_it)) {
        it->heap_idx++;
        if (it->heap_idx == POOL_SIZE) {
            it->heap_idx = -1;
            heap_iterator_free(it->heap_it);
            it->heap_it = NULL;
            return 0;
        }
        heap_iterator_free(it->heap_it);
        it->heap_it = heap_iterator(it->pool->heaps[it->heap_idx]);
        if (it->heap_it == NULL) {
            debug("Unable to get heap iterator");
            return -1;
        }
    }
    return 0;
}
