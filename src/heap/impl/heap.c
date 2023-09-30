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
            offset_t end;
        }
) heap_h;

typedef enum {
    MASK_TO_BE_DELETED = 0x1,
    MASK_TO_BE_ADDED = 0x2,
    MASK_IS_DELETED = 0x4,
} RECORD_FLAG_MASK;

typedef PACK(
        struct {
            uint8_t flags;
            union {
                uint64_t size;
                offset_t next;
            };
        }
) record_h;

struct heap_t {
    allocator_t *allocator;
    list_t *list;
    heap_h *header;
};

struct heap_it {
    heap_t *heap;
    buffer_t *record;
    offset_t record_begin;
    offset_t record_end;
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
    if (list_extend(heap->list, extra_pages) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    if (is_empty) {
        heap->header->end = heap->header->list_header.head + sizeof(list_node_h);
    }
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

static offset_t heap_read_forward(heap_t *heap, offset_t begin_offset, buffer_t *buffer) {
    assert(NULL != heap && NULL != buffer);
    list_it *lit = list_get_iterator(heap->list, record_page_offset(begin_offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = buffer->size;
    uint64_t full_read = 0;
    offset_t begin = begin_offset - record_page_offset(begin_offset - 1);
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

static offset_t heap_read_backward(heap_t *heap, offset_t end_offset, buffer_t *buffer) {
    list_it *lit = list_get_iterator(heap->list, record_page_offset(end_offset - 1));
    if (NULL == lit) {
        return 0;
    }
    uint64_t size = buffer->size;
    uint64_t full_read = 0;
    offset_t end = end_offset - record_page_offset(end_offset - 1);
    offset_t begin = 0;
    while (full_read != size) {
        page_t *page = list_iterator_get(lit);
        if (NULL == page) {
            list_iterator_free(lit);
            return 0;
        }
        uint16_t page_read = MIN(size - full_read, end - sizeof(list_node_h));
        begin = page_offset(page) + end - page_read;
        void *src = page_ptr(page) + end - page_read;
        void *dst = buffer->data + buffer->size - full_read - page_read;
        memcpy(dst, src, page_read);
        full_read += page_read;
        end = PAGE_SIZE;
        if (list_iterator_prev(lit) != LIST_OP_SUCCESS) {
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
    return begin;
}

offset_t heap_size(void) {
    return sizeof(heap_h);
}

bool heap_is_empty(heap_t *heap) {
    assert(heap != NULL);
    return list_is_empty(heap->list);
}

void heap_place(page_t *page, offset_t offset, offset_t record_size) {
    assert(page != NULL);
    list_place(page, offset);
    heap_h *header = (heap_h *) page_ptr(page) + offset;
    header->record_size = record_size + sizeof(record_h);
    header->end = 0;
}

heap_result heap_clear(heap_t *heap) {
    assert(NULL != heap);
    if (list_clear(heap->list) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    heap->header->end = 0;
    return HEAP_OP_SUCCESS;
}

heap_t *heap_init(page_t *page, offset_t offset, allocator_t *allocator) {
    assert(NULL != page && NULL != allocator);
    if (NULL == page || NULL == allocator) {
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
                .record_begin = 0,
                .record_end = 0,
                .record = NULL
        };
    } else {
        *it = (heap_it) {
                .heap = heap,
                .record_begin = sizeof(list_node_h) + heap->header->list_header.head,
                .record_end = 0,
                .record = NULL
        };
        buffer_t *record = buffer_init(heap->header->record_size);
        if (record == NULL) {
            heap_iterator_free(it);
            return NULL;
        }
        offset_t end = heap_read_forward(heap, heap_iterator_offset(it), record);
        if (0 == end) {
            buffer_free(record);
            heap_iterator_free(it);
            return NULL;
        }
        it->record_end = end;
        it->record = record;
    }
    return it;
}

heap_result heap_iterator_free(heap_it *it) {
    assert(NULL != it);
    if (it->record != NULL) {
        buffer_free(it->record);
    }
    it->record = NULL;
    it->record_begin = 0;
    it->record_end = 0;
    free(it);
    return HEAP_OP_SUCCESS;
}

static bool heap_iterator_is_empty_no_skip(heap_it *it) {
    assert(NULL != it);
    return NULL == it->record || heap_iterator_offset(it) == it->heap->header->end;
}

static heap_result heap_iterator_next_no_skip(heap_it *it) {
    // iterator validity is not responsibility of function
    assert(NULL != it);
    if (heap_iterator_is_empty_no_skip(it)) {
        return HEAP_OP_SUCCESS;
    }
    it->record_begin = it->record_end;
    if (it->record_begin == it->heap->header->end) {
        it->record_begin = 0;
        it->record_end = 0;
        buffer_free(it->record);
        it->record = NULL;
        return HEAP_OP_SUCCESS;
    }
    offset_t new_end = heap_read_forward(it->heap, it->record_begin, it->record);
    if (0 == new_end) {
        return HEAP_OP_ERROR;
    }
    it->record_end = new_end;
    return HEAP_OP_SUCCESS;
}

bool heap_iterator_is_empty(heap_it *it) {
    assert(NULL != it);
    if (NULL == it->record || heap_iterator_offset(it) == it->heap->header->end) {
        return true;
    }
    record_h *header = (record_h *) it->record->data;
    return record_is_to_be_added(header);
}

heap_result heap_iterator_next(heap_it *it) {
    // function should skip to_be_added
    // iterator validity is not responsibility of function
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
        header = (record_h *) it->record->data;
    } while (record_is_to_be_added(header));
    return HEAP_OP_SUCCESS;
}

buffer_t *heap_iterator_get(heap_it *it) {
    // function is not responsible for iterator validity
    assert(NULL != it);
    if (heap_iterator_is_empty(it)) {
        return NULL;
    }
    record_h *header = (record_h *) it->record->data;
    buffer_t *buffer = buffer_init(header->size);
    if (NULL == buffer) {
        return NULL;
    }
    memcpy(buffer->data, it->record->data + sizeof(record_h), buffer->size);
    return buffer;
}

heap_result heap_iterator_replace(heap_it *it, buffer_t *buffer) {
    assert(NULL != it && NULL != buffer);
    uint64_t heap_size = it->heap->header->record_size;
    buffer_t *record = buffer_init(heap_size);
    uint64_t data_size = MIN(heap_size - sizeof(record_h), buffer->size);
    record_h *record_header = (record_h *) record->data;
    *record_header = (record_h) {.flags = 0, .size = data_size};
    memcpy(record->data + sizeof(record_h), buffer->data, data_size);
    if (0 == heap_write(it->heap, heap_iterator_offset(it), record)) {
        return HEAP_OP_ERROR;
    }
    buffer_free(record);
    return HEAP_OP_SUCCESS;
}

static heap_result heap_record_set_flags(heap_it *it, uint8_t flags) {
    assert(NULL != it);
    buffer_t record;
    record.data = (char *) &flags;
    record.size = sizeof(uint8_t);
    if (0 == heap_write(it->heap, heap_iterator_offset(it), &record)) {
        return HEAP_OP_ERROR;
    }
    return HEAP_OP_SUCCESS;
}

heap_result heap_iterator_delete(heap_it *it) {
    assert(NULL != it);
    return heap_record_set_flags(it, MASK_TO_BE_DELETED);
}

static offset_t heap_get_last_record(heap_t *heap, buffer_t *buffer) {
    assert(NULL != heap);
    return heap_read_backward(heap, heap->header->end, buffer);
}

static heap_result heap_record_fill(heap_it *it) {
    assert(NULL != it);
    buffer_t *last_record_buffer = buffer_init(it->heap->header->record_size);
    if (NULL == last_record_buffer) {
        return HEAP_OP_ERROR;
    }
    offset_t cur_record_offset = heap_iterator_offset(it);
    record_h *last_record_header = NULL;
    do {
        offset_t last_record_offset = heap_get_last_record(it->heap, last_record_buffer);
        if (last_record_offset == 0) {
            buffer_free(last_record_buffer);
            return HEAP_OP_ERROR;
        }
        last_record_header = (record_h *) last_record_buffer->data;
        last_record_header->flags ^= MASK_TO_BE_ADDED;
        if (0 == heap_write(it->heap, cur_record_offset, last_record_buffer)) {
            buffer_free(last_record_buffer);
            return HEAP_OP_ERROR;
        }
        it->heap->header->end = last_record_offset;
        if (last_record_offset == cur_record_offset) {
            buffer_free(last_record_buffer);
            return HEAP_OP_SUCCESS;
        }
    } while (record_is_to_be_deleted(last_record_header));
    buffer_free(last_record_buffer);
    return HEAP_OP_SUCCESS;
}

static heap_result heap_unmask(heap_t *heap) {
    assert(NULL != heap);
    heap_it *it = heap_iterator(heap);
    if (NULL == it) {
        return HEAP_OP_ERROR;
    }
    while (!heap_iterator_is_empty_no_skip(it)) {
        record_h *header = (record_h *) it->record->data;
        if (record_is_to_be_deleted(header)) {
            if (heap_record_fill(it) != HEAP_OP_SUCCESS) {
                heap_iterator_free(it);
                return HEAP_OP_ERROR;
            }
        } else if (record_is_to_be_added(header)) {
            if (heap_record_set_flags(it, header->flags ^ MASK_TO_BE_ADDED) != HEAP_OP_SUCCESS) {
                heap_iterator_free(it);
                return HEAP_OP_ERROR;
            }
        }
        if (heap_iterator_next_no_skip(it) != HEAP_OP_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    return HEAP_OP_SUCCESS;
}

static heap_result heap_shrink_list(heap_t *heap) {
    assert(NULL != heap);
    offset_t last_page_offset = record_page_offset(heap->header->end - 1);
    list_it *lit = list_get_tail_iterator(heap->list);
    if (NULL == lit) {
        return HEAP_OP_ERROR;
    }
    if (last_page_offset + sizeof(list_node_h) == heap->header->end) {
        if (last_page_offset == heap->header->list_header.head) {
            heap->header->end = 0;
        } else {
            page_t *page = list_iterator_get(lit);
            if (NULL == page) {
                return HEAP_OP_ERROR;
            }
            list_node_h *node = (list_node_h *) page_ptr(page);
            heap->header->end = node->prev + PAGE_SIZE;
            if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
                return HEAP_OP_ERROR;
            }
        }
    }
    last_page_offset = record_page_offset(heap->header->end - 1);
    while (!list_iterator_is_empty(lit) && last_page_offset != list_iterator_offset(lit)) {
        page_t *page = list_iterator_get(lit);
        if (NULL == page) {
            return HEAP_OP_ERROR;
        }
        if (list_iterator_prev(lit) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            return HEAP_OP_ERROR;
        }
        if (list_delete_node(heap->list, page) != LIST_OP_SUCCESS) {
            allocator_unmap_page(heap->allocator, page);
            return HEAP_OP_ERROR;
        }
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    return HEAP_OP_SUCCESS;
}

heap_result heap_flush(heap_t *heap) {
    assert(NULL != heap);
    if (heap_unmask(heap) != HEAP_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    if (heap_shrink_list(heap) != HEAP_OP_SUCCESS) {
        return HEAP_OP_ERROR;
    }
    return HEAP_OP_SUCCESS;
}
