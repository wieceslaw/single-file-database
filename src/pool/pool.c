//
// Created by vyach on 25.09.2023.
//

#include <assert.h>
#include <malloc.h>
#include "pool.h"
#include "heap/heap.h"

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

offset_t pool_create(allocator_t *allocator) {
    assert(NULL != allocator);
    page_t *page = allocator_get_page(allocator);
    if (NULL == page) {
        return 0;
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
        return 0;
    }
    return pool_offset;
}

pool_t *pool_init(allocator_t *allocator, offset_t pool_offset) {
    assert(NULL != allocator);
    pool_t *pool = malloc(sizeof(pool_t));
    if (NULL == pool) {
        return NULL;
    }
    pool->page = allocator_map_page(allocator, pool_offset);
    if (NULL == pool->page) {
        free(pool);
        return NULL;
    }
    pool->allocator = allocator;
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_t *heap = heap_init(pool->page, heap_size() * i, pool->allocator);
        if (NULL == heap) {
            free(pool);
            return NULL;
        }
        pool->heaps[i] = heap;
    }
    return pool;
}

pool_result pool_free(pool_t *pool) {
    assert(pool_is_valid(pool));
    if (allocator_unmap_page(pool->allocator, pool->page) != ALLOCATOR_SUCCESS) {
        pool->page = NULL;
        pool->allocator = NULL;
        free(pool);
        return POOL_OP_ERROR;
    }
    pool->page = NULL;
    pool->allocator = NULL;
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_free(pool->heaps[i]);
        pool->heaps[i] = NULL;
    }
    free(pool);
    return POOL_OP_SUCCESS;
}

pool_result pool_clear(allocator_t *allocator, offset_t pool_offset) {
    assert(NULL != allocator);
    pool_t *pool = pool_init(allocator, pool_offset);
    if (NULL == pool) {
        return POOL_OP_ERROR;
    }
    for (int i = 0; i < POOL_SIZE; i++) {
        heap_t *heap = pool->heaps[i];
        if (heap_clear(heap) != HEAP_OP_SUCCESS) {
            pool_free(pool);
            return POOL_OP_ERROR;
        }
    }
    if (pool_free(pool) != POOL_OP_SUCCESS) {
        return POOL_OP_ERROR;
    }
    if (allocator_return_page(allocator, pool_offset) != ALLOCATOR_SUCCESS) {
        return POOL_OP_ERROR;
    }
    return POOL_OP_SUCCESS;
}

pool_result pool_append(pool_t *pool, buffer_t *buffer) {
    assert(pool_is_valid(pool));
    int idx = suitable_heap_idx(buffer->size);
    if (heap_append(pool->heaps[idx], buffer) != HEAP_OP_SUCCESS) {
        return POOL_OP_ERROR;
    }
    return POOL_OP_SUCCESS;
}

pool_result pool_flush(pool_t *pool) {
    assert(pool_is_valid(pool));
    for (int i = 0; i < POOL_SIZE; i++) {
        if (heap_flush(pool->heaps[i]) != HEAP_OP_SUCCESS) {
            return POOL_OP_ERROR;
        }
    }
    return POOL_OP_SUCCESS;
}

pool_it *pool_iterator(pool_t *pool) {
    assert(pool_is_valid(pool));
    pool_it* it = malloc(sizeof(pool_it));
    if (NULL == it) {
        return NULL;
    }
    it->pool = pool;
    it->heap_idx = 0;
    it->heap_it = heap_iterator(pool->heaps[0]);
    if (NULL == it->heap_it) {
        free(it);
        return NULL;
    }
    if (heap_iterator_is_empty(it->heap_it)) {
        if (pool_iterator_next(it) != POOL_OP_SUCCESS) {
            free(it);
            return NULL;
        }
    }
    return it;
}

pool_result pool_iterator_free(pool_it *it) {
    assert(NULL != it);
    if (NULL != it->heap_it) {
        heap_iterator_free(it->heap_it);
        it->heap_it = NULL;
    }
    it->heap_idx = -1;
    free(it);
    return POOL_OP_SUCCESS;
}

bool pool_iterator_is_empty(pool_it *it) {
    assert(NULL != it);
    return -1 == it->heap_idx;
}

buffer_t * pool_iterator_get(pool_it *it) {
    assert(NULL != it);
    if (pool_iterator_is_empty(it)) {
        return NULL;
    }
    return heap_iterator_get(it->heap_it);
}

pool_result pool_iterator_restart(pool_it *it) {
    assert(NULL != it);
    it->heap_idx = 0;
    if (NULL != it->heap_it) {
        heap_iterator_free(it->heap_it);
        it->heap_it = NULL;
    }
    it->heap_it = heap_iterator(it->pool->heaps[0]);
    if (NULL == it->heap_it) {
        return POOL_OP_ERROR;
    }
    return POOL_OP_SUCCESS;
}

pool_result pool_iterator_delete(pool_it *it) {
    assert(NULL != it);
    if (pool_iterator_is_empty(it)) {
        return POOL_OP_ERROR;
    }
    if (heap_iterator_delete(it->heap_it) != HEAP_OP_SUCCESS) {
        return POOL_OP_ERROR;
    }
    return POOL_OP_SUCCESS;
}

pool_result pool_iterator_next(pool_it *it) {
    assert(NULL != it);
    if (pool_iterator_is_empty(it)) {
        return POOL_OP_ERROR;
    }
    if (heap_iterator_next(it->heap_it) != HEAP_OP_SUCCESS) {
        return POOL_OP_ERROR;
    }
    if (!heap_iterator_is_empty(it->heap_it)) {
        return POOL_OP_SUCCESS;
    }
    while (heap_iterator_is_empty(it->heap_it)) {
        it->heap_idx++;
        if (it->heap_idx == POOL_SIZE) {
            it->heap_idx = -1;
            heap_iterator_free(it->heap_it);
            it->heap_it = NULL;
            return POOL_OP_SUCCESS;
        }
        heap_iterator_free(it->heap_it);
        it->heap_it = heap_iterator(it->pool->heaps[it->heap_idx]);
        if (NULL == it->heap_it) {
            return POOL_OP_ERROR;
        }
    }
    return POOL_OP_SUCCESS;
}
