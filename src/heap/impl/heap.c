//
// Created by vyach on 25.09.2023.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "heap/heap.h"
#include "list.h"

typedef PACK (
        struct {
            list_h list_header;
            offset_t record_size;
            offset_t free_record;
            offset_t end;
        }
) heap_h;

typedef enum {
    MASK_TO_BE_DELETED = 0x1,
    MASK_TO_BE_ADDED = 0x2,
} RECORD_FLAG_MASK;

typedef PACK(
        struct {
            uint8_t flags;
            uint64_t size;
        }
) record_h;

typedef PACK(
        struct {
            record_h header;
            offset_t next_free;
        }
) free_record_h;

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

static uint8_t has_mask(uint8_t value, uint8_t mask) {
    return value & mask;
}

static uint8_t set_mask(uint8_t value, uint8_t mask) {
    return value | mask;
}

static uint8_t remove_mask(uint8_t value, uint8_t mask) {
    return value ^ mask;
}

static bool record_is_to_be_added(record_h *header) {
    return has_mask(header->flags, MASK_TO_BE_ADDED);
}

static bool record_is_to_be_deleted(record_h *header) {
    return has_mask(header->flags, MASK_TO_BE_DELETED);
}

static offset_t record_page_offset(offset_t record_offset) {
    return (record_offset / PAGE_SIZE) * PAGE_SIZE;
}

static offset_t heap_free_space(heap_t *heap) {
    assert(NULL != heap);
    if (heap_is_empty(heap)) {
        return 0;
    }
    return heap->header->list_header.tail + PAGE_SIZE - heap->header->end;
}

static offset_t heap_iterator_offset(heap_it *it) {
    assert(NULL != it);
    return it->record_offset + list_iterator_offset(it->lit);
}

static heap_result heap_reserve(heap_t *heap, offset_t size) {
    assert(NULL != heap);
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

static heap_result heap_iterator_move(heap_it *it, offset_t to) {
    if (0 == to) {
        it->record_offset = 0;
        return HEAP_OP_SUCCESS;
    }
    if (list_iterator_free(it->lit) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    offset_t page = record_page_offset(to - 1);
    list_it *lit = list_get_iterator(it->heap->list, page);
    if (NULL == it->lit) {
        return HEAP_OP_ERROR;
    }
    it->lit = lit;
    it->record_offset = to - page;
    return HEAP_OP_SUCCESS;
}

static offset_t heap_write(heap_t *heap, offset_t offset, buffer_t *buffer) {
    assert(NULL != heap && NULL != buffer);
    list_it *lit = list_get_iterator(heap->list, record_page_offset(offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = MIN(buffer->size, heap->header->record_size);
    uint64_t full_written = 0;
    offset_t begin = offset - record_page_offset(offset - 1);
    offset_t end = 0;
    while (full_written != size) {
        page_t *page = list_iterator_get(lit);
        if (NULL == page) {
            list_iterator_free(lit);
            return 0;
        }
        void *dst = page_ptr(page) + begin;
        void *src = buffer->data + full_written;
        uint16_t page_written = MIN(size - full_written, PAGE_SIZE - begin);
        end = page_offset(page) + begin + page_written;
        memcpy(dst, src, page_written);
        full_written += page_written;
        begin = sizeof(list_node_h);
        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            list_iterator_free(lit);
            return 0;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            return 0;
        }
    }
    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return 0;
    }
    return end;
}

static offset_t heap_read(heap_t *heap, offset_t offset, buffer_t *buffer) {
    assert(NULL != heap && NULL != buffer);
    list_it *lit = list_get_iterator(heap->list, record_page_offset(offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = buffer->size;
    uint64_t full_read = 0;
    offset_t begin = offset - record_page_offset(offset - 1);
    offset_t end = 0;
    while (full_read != size) {
        page_t *page = list_iterator_get(lit);
        if (NULL == page) {
            list_iterator_free(lit);
            return 0;
        }
        void *src = page_ptr(page) + begin;
        void *dst = buffer->data + full_read;
        uint16_t page_read = MIN(size - full_read, PAGE_SIZE - begin);
        end = page_read + begin + page_offset(page);
        memcpy(dst, src, page_read);
        full_read += page_read;
        begin = sizeof(list_node_h);
        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            list_iterator_free(lit);
            return 0;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            list_iterator_free(lit);
            return 0;
        }
    }
    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return 0;
    }
    return end;
}

static offset_t heap_record_header(heap_t *heap, offset_t record_offset, record_h *header) {
    assert(heap != NULL && header != NULL);
    buffer_t buffer;
    buffer.size = sizeof(record_h);
    buffer.data = (char *) header;
    return heap_read(heap, record_offset, &buffer);
}

offset_t heap_size(void) {
    return sizeof(heap_h);
}

bool heap_is_empty(heap_t *heap) {
    assert(heap != NULL);
    return list_is_empty(heap->list);
}

heap_result heap_place(page_t *page, offset_t offset, offset_t record_size) {
    assert(page != NULL);
    if (list_place(page, offset) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    heap_h *header = (heap_h *) page_ptr(page) + offset;
    header->record_size = record_size;
    header->free_record = 0;
    header->end = 0;
    return HEAP_OP_SUCCESS;
}

heap_result heap_clear(heap_t *heap) {
    assert(NULL != heap);
    if (list_clear(heap->list) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    heap->header->free_record = 0;
    heap->header->end = 0;
    return HEAP_OP_SUCCESS;
}

heap_t *heap_init(page_t *page, offset_t offset, allocator_t *allocator) {
    assert(NULL != page && NULL != allocator);
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
    heap->header = (heap_h *) page_ptr(page) + offset;
    heap->list = list_init(&(heap->header->list_header), allocator);
    if (NULL == heap->list) {
        free(heap);
        return NULL;
    }
    return heap;
}

void heap_free(heap_t *heap) {
    assert(NULL != heap);
    list_free(heap->list);
    heap->list = NULL;
    free(heap);
}

heap_result heap_append(heap_t *heap, buffer_t *buffer) {
    assert(NULL != heap && NULL != buffer);

    uint64_t heap_size = heap->header->record_size;
    buffer_t *record = buffer_init(heap_size);
    uint64_t data_size = MIN(heap_size - sizeof(record_h), buffer->size);
    record_h *record_header = (record_h *) record->data;
    *record_header = (record_h) {.flags = 0, .size = data_size};
    record_header->flags = set_mask(record_header->flags, MASK_TO_BE_ADDED);
    memcpy(record->data + sizeof(record_h), buffer->data, data_size);

    if (HEAP_OP_SUCCESS != heap_reserve(heap, record->size)) {
        return HEAP_OP_ERROR;
    }
    offset_t end_offset = heap_write(heap, heap->header->end, record);
    if (0 == end_offset) {
        return HEAP_OP_ERROR;
    }
    heap->header->end = end_offset;
    buffer_free(record);
    return HEAP_OP_SUCCESS;
}

heap_it *heap_iterator(heap_t *heap) {
    assert(NULL != heap);
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
        record_h header;
        if (0 == heap_record_header(heap, heap_iterator_offset(it), &header)) {
            heap_iterator_free(it);
            return NULL;
        }
        if (record_is_to_be_added(&header)) {
            if (heap_iterator_next(it) != HEAP_OP_SUCCESS) {
                heap_iterator_free(it);
                return NULL;
            }
        }
    }
    return it;
}

heap_result heap_iterator_free(heap_it *it) {
    assert(NULL != it);
    if (list_iterator_free(it->lit) != LIST_OP_SUCCESS) {
        free(it);
        return HEAP_OP_ERROR;
    }
    free(it);
    return HEAP_OP_SUCCESS;
}

bool heap_iterator_is_empty(heap_it *it) {
    assert(NULL != it);
    return 0 == it->record_offset;
}

heap_result heap_iterator_next(heap_it *it) {
    // function should skip to_be_added
    // iterator validity is not responsibility of function
    assert(NULL != it);
    if (heap_iterator_is_empty(it)) {
        return HEAP_OP_ERROR;
    }
    record_h *header;
    buffer_t *buffer = buffer_init(it->heap->header->record_size);
    if (NULL == buffer) {
        return HEAP_OP_ERROR;
    }
    do {
        offset_t next = heap_read(it->heap, heap_iterator_offset(it), buffer);
        if (next == it->heap->header->end) {
            heap_iterator_move(it, 0);
            return HEAP_OP_SUCCESS;
        }
        if (0 == next) {
            buffer_free(buffer);
            return HEAP_OP_ERROR;
        }
        heap_iterator_move(it, next);
        header = (record_h *) buffer->data;
    } while (has_mask(header->flags, MASK_TO_BE_ADDED));
    buffer_free(buffer);
    return HEAP_OP_SUCCESS;
}

buffer_t *heap_iterator_get(heap_it *it) {
    // function is not responsible for iterator validity
    assert(NULL != it);
    if (heap_iterator_is_empty(it)) {
        return NULL;
    }
    record_h header;
    offset_t record_body_offset = heap_record_header(it->heap, heap_iterator_offset(it), &header);
    if (0 == record_body_offset) {
        return NULL;
    }
    uint64_t size = header.size;
    buffer_t *record = buffer_init(size);
    if (NULL == record) {
        return NULL;
    }
    if (0 == heap_read(it->heap, record_body_offset, record)) {
        buffer_free(record);
        return NULL;
    }
    return record;
}

heap_result heap_iterator_delete(heap_it *it);

heap_result heap_iterator_replace(heap_it *it, buffer_t *data);

heap_result heap_compress(heap_it *heap);
