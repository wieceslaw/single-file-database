//
// Created by vyach on 25.09.2023.
//

#include <malloc.h>
#include <string.h>
#include "heap/heap.h"
#include "list.h"

#define PAGE_CAPACITY (PAGE_SIZE - sizeof(list_node_h))

#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct __attribute__((__packed__)) {
    list_h list_header;
    offset_t record_size;
    offset_t free_record;
    offset_t end;
} heap_h;

typedef struct __attribute__((__packed__)) {
    uint8_t is_free;
} record_h;

typedef struct __attribute__((__packed__)) {
    record_h header;
    offset_t next_free;
} free_record_h;

struct heap_t {
    allocator_t *allocator;
    list_t *list;
    heap_h *header;
};

struct heap_it {
    heap_t *heap;
    list_it *lit;
    offset_t record_offset;
};

static offset_t record_page_offset(offset_t record_offset) {
    return (record_offset / PAGE_SIZE) * PAGE_SIZE;
}

static offset_t heap_free_space(heap_t *heap) {
    if (heap_is_empty(heap)) {
        return 0;
    }
    return heap->header->list_header.tail + PAGE_SIZE - heap->header->end;
}

static heap_result heap_reserve(heap_t *heap, offset_t size) {
    bool is_empty = heap_is_empty(heap);
    offset_t capacity = heap_free_space(heap);
    if (capacity >= size) {
        return HEAP_OP_SUCCESS;
    }
    offset_t diff = size - capacity;
    offset_t extra_pages = (diff / PAGE_CAPACITY) + (diff % PAGE_CAPACITY == 0 ? 0 : 1);
    if (list_extend(heap->list, extra_pages) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    if (is_empty) {
        heap->header->end = heap->header->list_header.head + sizeof(list_node_h);
    }
    return HEAP_OP_SUCCESS;
}

static offset_t heap_write(heap_t *heap, offset_t offset, buffer *data) {
    offset_t page_off;
    page_off = record_page_offset(offset - 1);
    list_it *it = list_get_iterator(heap->list, page_off);
    if (NULL == it) {
        return 0;
    }
    if (offset % PAGE_SIZE == 0) {
        if (list_iterator_next(it) != LIST_OP_SUCCESS) {
            list_iterator_free(it);
            return 0;
        }
        page_off = list_iterator_offset(it);
        if (page_off == 0) {
            return 0;
        }
        offset = page_off + sizeof(list_node_h);
    }
    offset_t start_offset = offset - page_off - sizeof(list_node_h);
    offset_t written = 0;
    offset_t end_offset = 0;
    while (written != data->size) {
        page_t *page = list_iterator_get(it);
        if (NULL == page) {
            list_iterator_free(it);
            return 0;
        }
        void *dest = page_ptr(page) + sizeof(list_node_h) + start_offset;
        void *src = data->data + written;
        int current_written = MIN(data->size - written, PAGE_CAPACITY - start_offset);
        end_offset = page_offset(page) + sizeof(list_node_h) + start_offset + current_written;
        memcpy(dest, src, current_written);
        written += current_written;
        if (list_iterator_next(it) != LIST_OP_SUCCESS) {
            list_iterator_free(it);
            return 0;
        }
        start_offset = 0;
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    if (list_iterator_free(it) != LIST_OP_SUCCESS) {
        return 0;
    }
    return end_offset;
}

offset_t heap_size() {
    return sizeof(heap_h);
}

bool heap_is_empty(heap_t *heap) {
    return list_is_empty(heap->list);
}

heap_result heap_place(page_t *page, offset_t offset, offset_t record_size) {
    if (list_place(page, offset) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    heap_h *header = page_ptr(page) + offset;
    header->record_size = record_size;
    header->free_record = 0;
    header->end = 0;
    return HEAP_OP_SUCCESS;
}

heap_result heap_clear(heap_t *heap) {
    if (list_clear(heap->list) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    heap->header->free_record = 0;
    heap->header->end = 0;
    return HEAP_OP_SUCCESS;
}

heap_t *heap_init(page_t *page, offset_t offset, allocator_t *allocator) {
    if (NULL == page || NULL == allocator) {
        return NULL;
    }
    if (LIST_OP_SUCCESS != list_place(page, offset)) {
        return NULL;
    }
    heap_t *heap = malloc(sizeof(heap_t));
    if (NULL == heap) {
        return NULL;
    }
    heap->allocator = allocator;
    heap->header = page_ptr(page) + offset;
    heap->list = list_init(&(heap->header->list_header), allocator);
    if (NULL == heap->list) {
        free(heap);
        return NULL;
    }
    return heap;
}

void heap_free(heap_t *heap) {
    list_free(heap->list);
    free(heap);
}

heap_result heap_append(heap_t *heap, buffer *data) {
    offset_t size = heap->header->record_size;
    if (size != data->size) {
        return HEAP_OP_ERROR;
    }
    if (HEAP_OP_SUCCESS != heap_reserve(heap, data->size)) {
        return HEAP_OP_ERROR;
    }
    offset_t end_offset = heap_write(heap, heap->header->end, data);
    if (0 == end_offset) {
        return HEAP_OP_ERROR;
    }
    heap->header->end = end_offset;
    return HEAP_OP_SUCCESS;
}

heap_it *heap_iterator(heap_t *heap) {
    heap_it *it = malloc(sizeof(heap_it));
    if (NULL == it) {
        return NULL;
    }
    if (heap_is_empty(heap)) {
        *it = (heap_it) {
                .heap = heap,
                .record_offset = 0,
                .lit = list_get_head_iterator(heap->list)
        };
    } else {
        *it = (heap_it) {
                .heap = heap,
                .record_offset = sizeof(list_node_h),
                .lit = list_get_head_iterator(heap->list)
        };
    }
    return it;
}

heap_result heap_iterator_free(heap_it *it) {
    if (list_iterator_free(it->lit) != LIST_OP_SUCCESS) {
        free(it);
        return HEAP_OP_ERROR;
    }
    free(it);
    return HEAP_OP_SUCCESS;
}

bool heap_iterator_is_empty(heap_it *it) {
    return 0 == it->record_offset;
}

heap_result heap_iterator_next(heap_it *it) {
    if (heap_iterator_is_empty(it)) {
        return HEAP_OP_SUCCESS;
    }
    uint64_t size = it->heap->header->record_size;
    offset_t page_off = list_iterator_offset(it->lit);
    offset_t end = page_off + it->record_offset + size;
    while (record_page_offset(end) != page_off && end != it->heap->header->end) {
        size -= PAGE_SIZE - it->record_offset;
        it->record_offset = sizeof(list_node_h);
        if (list_iterator_next(it->lit) != LIST_OP_SUCCESS) {
            return HEAP_OP_ERROR;
        }
        page_off = list_iterator_offset(it->lit);
        end = page_off + it->record_offset + size;
    }
    if (end == it->heap->header->end) {
        it->record_offset = 0;
        return HEAP_OP_SUCCESS;
    }
    it->record_offset = end - list_iterator_offset(it->lit);
    return HEAP_OP_SUCCESS;
}

heap_result heap_iterator_get(heap_it *it, buffer *data) {
    if (heap_iterator_is_empty(it)) {
        return HEAP_OP_SUCCESS;
    }
    uint64_t size = it->heap->header->record_size;
    if (data->size != size) {
        return HEAP_OP_ERROR;
    }
    uint64_t read = 0;
    uint16_t offset = it->record_offset;
    list_it *lit = list_iterator_copy(it->lit);
    if (lit == NULL) {
        return HEAP_OP_ERROR;
    }
    while (read != size) {
        page_t *page = list_iterator_get(lit);
        if (NULL == page) {
            list_iterator_free(lit);
            return HEAP_OP_ERROR;
        }
        void* src = page_ptr(page) + offset;
        void* dst = data->data + read;
        uint16_t cur_read = MIN(size - read, PAGE_SIZE - offset);
        memcpy(dst, src, cur_read);
        read += cur_read;
        offset = sizeof(list_node_h);
        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(it->heap->allocator, page);
            list_iterator_free(lit);
            return HEAP_OP_ERROR;
        }
        if (allocator_unmap_page(it->heap->allocator, page) != ALLOCATOR_SUCCESS) {
            list_iterator_free(lit);
            return HEAP_OP_ERROR;
        }
    }
    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    return HEAP_OP_SUCCESS;
}

heap_result heap_iterator_delete(heap_it *it);

heap_result heap_iterator_replace(heap_it *it, buffer *data);

heap_result heap_compress(heap_it *heap);
