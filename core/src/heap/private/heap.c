//
// Created by vyach on 25.09.2023.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "heap/heap.h"
#include "pagelist.h"

typedef PACK (
        struct {
            list_h list_header;
            offset_t record_size;
            offset_t end;
            offset_t append_begin;
            uint8_t has_deleted;
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

struct heap_t {
    allocator_t *allocator;
    page_list_t *list;
    heap_h *header;
};

struct heap_it {
    heap_t *heap;
    Buffer record_buffer;
    offset_t record_begin;
    offset_t record_end;
};

static uint8_t has_mask(uint8_t value, uint8_t mask) {
    return value & mask;
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
    return it->record_begin;
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
    if (page_list_extend(heap->list, extra_pages) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    if (is_empty) {
        heap->header->end = heap->header->list_header.head + sizeof(page_list_node_h);
    }
    return HEAP_OP_SUCCESS;
}

static offset_t heap_write(heap_t *heap, offset_t offset, Buffer buffer) {
    assert(NULL != heap && NULL != buffer);
    page_list_it *lit = page_list_get_iterator(heap->list, record_page_offset(offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = MIN(buffer->size, heap->header->record_size);
    uint64_t full_written = 0;
    offset_t begin = offset - record_page_offset(offset - 1);
    offset_t end = 0;
    while (full_written != size) {
        page_t *page = page_list_iterator_get(lit);
        if (NULL == page) {
            page_list_iterator_free(lit);
            return 0;
        }
        void *dst = page_ptr(page) + begin;
        void *src = buffer->data + full_written;
        uint16_t page_written = MIN(size - full_written, PAGE_SIZE - begin);
        end = page_offset(page) + begin + page_written;
        memcpy(dst, src, page_written);
        full_written += page_written;
        begin = sizeof(page_list_node_h);
        if (page_list_iterator_next(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            page_list_iterator_free(lit);
            return 0;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            return 0;
        }
    }
    if (page_list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return 0;
    }
    return end;
}

static offset_t heap_read_forward(heap_t *heap, offset_t begin_offset, Buffer buffer) {
    assert(NULL != heap && NULL != buffer);
    page_list_it *lit = page_list_get_iterator(heap->list, record_page_offset(begin_offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = buffer->size;
    uint64_t full_read = 0;
    offset_t begin = begin_offset - record_page_offset(begin_offset - 1);
    offset_t end = 0;
    while (full_read != size) {
        page_t *page = page_list_iterator_get(lit);
        if (NULL == page) {
            page_list_iterator_free(lit);
            return 0;
        }
        void *src = page_ptr(page) + begin;
        void *dst = buffer->data + full_read;
        uint16_t page_read = MIN(size - full_read, PAGE_SIZE - begin);
        end = page_read + begin + page_offset(page);
        memcpy(dst, src, page_read);
        full_read += page_read;
        begin = sizeof(page_list_node_h);
        if (page_list_iterator_next(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            page_list_iterator_free(lit);
            return 0;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            page_list_iterator_free(lit);
            return 0;
        }
    }
    if (page_list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return 0;
    }
    return end;
}

static offset_t heap_read_backward(heap_t *heap, offset_t end_offset, Buffer buffer) {
    page_list_it *lit = page_list_get_iterator(heap->list, record_page_offset(end_offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = buffer->size;
    uint64_t full_read = 0;
    offset_t end = end_offset - record_page_offset(end_offset - 1);
    offset_t begin = 0;
    while (full_read != size) {
        page_t *page = page_list_iterator_get(lit);
        if (NULL == page) {
            page_list_iterator_free(lit);
            return 0;
        }
        uint16_t page_read = MIN(size - full_read, end - sizeof(page_list_node_h));
        begin = page_offset(page) + end - page_read;
        void *src = page_ptr(page) + end - page_read;
        void *dst = buffer->data + buffer->size - full_read - page_read;
        memcpy(dst, src, page_read);
        full_read += page_read;
        end = PAGE_SIZE;
        if (page_list_iterator_prev(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            page_list_iterator_free(lit);
            return 0;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            page_list_iterator_free(lit);
            return 0;
        }
    }
    if (page_list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return 0;
    }
    return begin;
}

offset_t heap_size(void) {
    return sizeof(heap_h);
}

bool heap_is_empty(heap_t *heap) {
    assert(heap != NULL);
    return page_list_is_empty(heap->list);
}

void heap_place(page_t *page, offset_t offset, offset_t record_size) {
    assert(page != NULL);
    page_list_place(page, offset);
    heap_h *header = (heap_h *) (page_ptr(page) + offset);
    header->record_size = record_size + sizeof(record_h);
    header->end = 0;
    header->append_begin = 0;
    header->has_deleted = 0;
}

heap_result heap_clear(heap_t *heap) {
    assert(NULL != heap);
    if (page_list_clear(heap->list) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    heap->header->end = 0;
    heap->header->append_begin = 0;
    heap->header->has_deleted = 0;
    return HEAP_OP_SUCCESS;
}

heap_t *heap_init(page_t *page, offset_t offset, allocator_t *allocator) {
    assert(NULL != page && NULL != allocator);
    heap_t *heap = malloc(sizeof(heap_t));
    if (NULL == heap) {
        return NULL;
    }
    heap->allocator = allocator;
    heap->header = (heap_h *) (page_ptr(page) + offset);
    heap->list = page_list_init(&(heap->header->list_header), allocator);
    if (NULL == heap->list) {
        free(heap);
        return NULL;
    }
    return heap;
}

void heap_free(heap_t *heap) {
    assert(NULL != heap);
    page_list_free(heap->list);
    heap->list = NULL;
    free(heap);
}

heap_result heap_append(heap_t *heap, Buffer data_buffer) {
    assert(NULL != heap && NULL != data_buffer);
    uint64_t heap_size = heap->header->record_size;
    Buffer record_buffer = BufferNew(heap_size);
    uint64_t data_size = MIN(heap_size - sizeof(record_h), data_buffer->size);
    record_h *record_header = (record_h *) record_buffer->data;
    *record_header = (record_h) {0};
    *record_header = (record_h) {.size = data_size};
    record_header->flags = record_header->flags | MASK_TO_BE_ADDED;
    memcpy(record_buffer->data + sizeof(record_h), data_buffer->data, data_size);
    if (HEAP_OP_SUCCESS != heap_reserve(heap, record_buffer->size)) {
        return HEAP_OP_ERROR;
    }
    offset_t end_offset = heap_write(heap, heap->header->end, record_buffer);
    if (0 == end_offset) {
        return HEAP_OP_ERROR;
    }
    if (0 == heap->header->append_begin) {
        heap->header->append_begin = heap->header->end;
    }
    heap->header->end = end_offset;
    BufferFree(&record_buffer);
    return HEAP_OP_SUCCESS;
}

static heap_it *heap_iterator_at(heap_t *heap, offset_t offset) {
    assert(NULL != heap);
    heap_it *it = malloc(sizeof(heap_it));
    if (NULL == it) {
        return NULL;
    }
    if (heap_is_empty(heap)) {
        *it = (heap_it) {
                .heap = heap,
                .record_begin = 0,
                .record_end = 0,
                .record_buffer = NULL
        };
    } else {
        *it = (heap_it) {
                .heap = heap,
                .record_begin = offset,
                .record_end = 0,
                .record_buffer = NULL
        };
        Buffer record = BufferNew(heap->header->record_size);
        if (record == NULL) {
            heap_iterator_free(it);
            return NULL;
        }
        offset_t end = heap_read_forward(heap, heap_iterator_offset(it), record);
        if (0 == end) {
            BufferFree(&record);
            heap_iterator_free(it);
            return NULL;
        }
        it->record_end = end;
        it->record_buffer = record;
    }
    return it;
}

heap_it *heap_iterator(heap_t *heap) {
    assert(NULL != heap);
    return heap_iterator_at(heap, sizeof(page_list_node_h) + heap->header->list_header.head);
}

void heap_iterator_free(heap_it *it) {
    if (NULL == it) {
        return;
    }
    if (it->record_buffer != NULL) {
        BufferFree(&(it->record_buffer));
    }
    it->record_buffer = NULL;
    it->record_begin = 0;
    it->record_end = 0;
    free(it);
}

static bool heap_iterator_is_empty_no_skip(heap_it *it) {
    assert(NULL != it);
    return NULL == it->record_buffer || heap_iterator_offset(it) == it->heap->header->end;
}

// iterator validity is not responsibility of function
static heap_result heap_iterator_next_no_skip(heap_it *it) {
    assert(NULL != it);
    if (heap_iterator_is_empty_no_skip(it)) {
        return HEAP_OP_SUCCESS;
    }
    it->record_begin = it->record_end;
    if (it->record_begin == it->heap->header->end) {
        it->record_begin = 0;
        it->record_end = 0;
        BufferFree(&(it->record_buffer));
        it->record_buffer = NULL;
        return HEAP_OP_SUCCESS;
    }
    offset_t new_end = heap_read_forward(it->heap, it->record_begin, it->record_buffer);
    if (0 == new_end) {
        return HEAP_OP_ERROR;
    }
    it->record_end = new_end;
    return HEAP_OP_SUCCESS;
}

bool heap_iterator_is_empty(heap_it *it) {
    assert(NULL != it);
    if (NULL == it->record_buffer || heap_iterator_offset(it) == it->heap->header->end) {
        return true;
    }
    record_h *header = (record_h *) it->record_buffer->data;
    return record_is_to_be_added(header);
}

// function should skip to_be_added
// iterator validity is not responsibility of function
heap_result heap_iterator_next(heap_it *it) {
    assert(NULL != it);
    if (heap_iterator_is_empty_no_skip(it)) {
        return HEAP_OP_ERROR;
    }
    record_h *header;
    do {
        if (heap_iterator_next_no_skip(it) != HEAP_OP_SUCCESS) {
            return HEAP_OP_ERROR;
        }
        if (heap_iterator_is_empty(it)) {
            return HEAP_OP_SUCCESS;
        }
        header = (record_h *) it->record_buffer->data;
    } while (record_is_to_be_added(header));
    return HEAP_OP_SUCCESS;
}

// function is not responsible for iterator validity
Buffer heap_iterator_get(heap_it *it) {
    assert(NULL != it);
    if (heap_iterator_is_empty(it)) {
        return NULL;
    }
    record_h *header = (record_h *) it->record_buffer->data;
    Buffer buffer = BufferNew(header->size);
    memcpy(buffer->data, it->record_buffer->data + sizeof(record_h), buffer->size);
    return buffer;
}

static heap_result heap_record_set_flags(heap_it *it, uint8_t record_flags) {
    assert(NULL != it);
    struct Buffer record_buffer;
    record_buffer.data = (char *) &record_flags;
    record_buffer.size = sizeof(uint8_t);
    if (0 == heap_write(it->heap, heap_iterator_offset(it), &record_buffer)) {
        return HEAP_OP_ERROR;
    }
    return HEAP_OP_SUCCESS;
}

heap_result heap_iterator_delete(heap_it *it) {
    assert(NULL != it);
    it->heap->header->has_deleted = 1;
    return heap_record_set_flags(it, MASK_TO_BE_DELETED);
}

static offset_t heap_get_last_record(heap_t *heap, Buffer buffer) {
    assert(NULL != heap);
    return heap_read_backward(heap, heap->header->end, buffer);
}

static heap_result heap_record_fill(heap_it *it) {
    assert(NULL != it);
    Buffer last_record_buffer = BufferNew(it->heap->header->record_size);
    if (NULL == last_record_buffer) {
        return HEAP_OP_ERROR;
    }
    offset_t cur_record_offset = heap_iterator_offset(it);
    record_h *last_record_header = NULL;
    do {
        offset_t last_record_offset = heap_get_last_record(it->heap, last_record_buffer);
        if (last_record_offset == 0) {
            BufferFree(&last_record_buffer);
            return HEAP_OP_ERROR;
        }
        last_record_header = (record_h *) last_record_buffer->data;
        assert(!record_is_to_be_added(last_record_header));
        it->heap->header->end = last_record_offset;
        if (last_record_offset == cur_record_offset) {
            BufferFree(&last_record_buffer);
            return HEAP_OP_SUCCESS;
        }
        if (record_is_to_be_deleted(last_record_header)) {
            continue;
        }
        if (0 == heap_write(it->heap, cur_record_offset, last_record_buffer)) {
            BufferFree(&last_record_buffer);
            return HEAP_OP_ERROR;
        }
    } while (record_is_to_be_deleted(last_record_header));
    BufferFree(&last_record_buffer);
    return HEAP_OP_SUCCESS;
}

static heap_result heap_flush_delete(heap_t *heap) {
    assert(NULL != heap);
    heap_it *it = heap_iterator(heap);
    if (NULL == it) {
        return HEAP_OP_ERROR;
    }
    while (!heap_iterator_is_empty_no_skip(it)) {
        record_h *header = (record_h *) it->record_buffer->data;
        if (record_is_to_be_deleted(header)) {
            if (heap_record_fill(it) != HEAP_OP_SUCCESS) {
                heap_iterator_free(it);
                return HEAP_OP_ERROR;
            }
        }
        if (heap_iterator_next_no_skip(it) != HEAP_OP_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    heap_iterator_free(it);
    heap->header->has_deleted = 0;
    return HEAP_OP_SUCCESS;
}

static heap_result heap_flush_append(heap_t *heap) {
    assert(NULL != heap);
    if (0 == heap->header->append_begin) {
        return HEAP_OP_SUCCESS;
    }
    heap_it *it = heap_iterator_at(heap, heap->header->append_begin);
    if (NULL == it) {
        return HEAP_OP_ERROR;
    }
    while (!heap_iterator_is_empty_no_skip(it)) {
        record_h *header = (record_h *) it->record_buffer->data;
        if (record_is_to_be_added(header)) {
            if (heap_record_set_flags(it, header->flags ^ MASK_TO_BE_ADDED) != HEAP_OP_SUCCESS) {
                heap_iterator_free(it);
                return HEAP_OP_ERROR;
            }
        }
        if (heap_iterator_next_no_skip(it) != HEAP_OP_SUCCESS) {
            heap_iterator_free(it);
            return HEAP_OP_ERROR;
        }
    }
    heap_iterator_free(it);
    heap->header->append_begin = 0;
    return HEAP_OP_SUCCESS;
}

static heap_result heap_shrink_list(heap_t *heap) {
    assert(NULL != heap);
    offset_t last_page_offset = record_page_offset(heap->header->end - 1);
    page_list_it *lit = page_list_get_tail_iterator(heap->list);
    if (NULL == lit) {
        return HEAP_OP_ERROR;
    }
    if (last_page_offset + sizeof(page_list_node_h) == heap->header->end) {
        if (last_page_offset == heap->header->list_header.head) {
            heap->header->end = 0;
        } else {
            page_t *page = page_list_iterator_get(lit);
            if (NULL == page) {
                return HEAP_OP_ERROR;
            }
            page_list_node_h *node = (page_list_node_h *) page_ptr(page);
            heap->header->end = node->prev + PAGE_SIZE;
            if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
                return HEAP_OP_ERROR;
            }
        }
    }
    last_page_offset = record_page_offset(heap->header->end - 1);
    while (!page_list_iterator_is_empty(lit) && last_page_offset != page_list_iterator_offset(lit)) {
        page_t *page = page_list_iterator_get(lit);
        if (NULL == page) {
            return HEAP_OP_ERROR;
        }
        if (page_list_iterator_prev(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            return HEAP_OP_ERROR;
        }
        if (page_list_delete_node(heap->list, page) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            return HEAP_OP_ERROR;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    if (page_list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    return HEAP_OP_SUCCESS;
}

heap_result heap_flush(heap_t *heap) {
    assert(NULL != heap);
    if (heap_flush_append(heap) != HEAP_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    if (heap->header->has_deleted) {
        if (heap_flush_delete(heap) != HEAP_OP_SUCCESS) {
            return HEAP_OP_ERROR;
        }
        if (heap_shrink_list(heap) != HEAP_OP_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    return HEAP_OP_SUCCESS;
}
