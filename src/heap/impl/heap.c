//
// Created by vyach on 25.09.2023.
//

#include <malloc.h>
#include <string.h>
#include <assert.h>
#include "heap/heap.h"
#include "list.h"

typedef PACK (struct {
    list_h list_header;
    offset_t record_size;
    offset_t free_record;
    offset_t end;
}) heap_h;

typedef PACK(struct {
    uint8_t is_free;
    uint64_t size;
}) record_h;

typedef PACK(struct {
    record_h header;
    offset_t next_free;
}) free_record_h;

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

static offset_t heap_write(heap_t *heap, offset_t offset, buffer_t *buffer) {
    assert(NULL != heap && NULL != buffer);
    offset_t page_off = record_page_offset(offset - 1);
    list_it *lit = list_get_iterator(heap->list, page_off);
    if (NULL == lit) {
        return 0;
    }
    if (offset % PAGE_SIZE == 0) {
        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
            list_iterator_free(lit);
            return 0;
        }
        page_off = list_iterator_offset(lit);
        if (page_off == 0) {
            return 0;
        }
        offset = page_off + sizeof(list_node_h);
    }
    offset_t start_offset = offset - page_off - sizeof(list_node_h);
    offset_t end_offset = 0;
    uint64_t written = 0;
    uint64_t size = MIN(buffer->size, heap->header->record_size);
    while (written != size) {
        page_t *page = list_iterator_get(lit);
        if (NULL == page) {
            list_iterator_free(lit);
            return 0;
        }
        void *dest = page_ptr(page) + sizeof(list_node_h) + start_offset;
        void *src = buffer->data + written;
        int current_written = MIN(size - written, PAGE_CAPACITY - start_offset);
        end_offset = page_offset(page) + sizeof(list_node_h) + start_offset + current_written;
        memcpy(dest, src, current_written);
        written += current_written;
        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
            list_iterator_free(lit);
            return 0;
        }
        start_offset = 0;
        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
            return HEAP_OP_ERROR;
        }
    }
    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
        return 0;
    }
    return end_offset;
}

static offset_t heap_read(heap_t *heap, offset_t offset, buffer_t *buffer) {
    // todo: rework like heap_write

    assert(NULL != heap && NULL != buffer);
//    uint64_t size = buffer->size;
//    uint64_t read = 0;
//    list_it *lit = list_get_iterator(heap->list, record_page_offset(offset));
//    if (NULL == lit) {
//        return 0;
//    }
//    while (read != size) {
//        page_t *page = list_iterator_get(lit);
//        if (NULL == page) {
//            list_iterator_free(lit);
//            return 0;
//        }
//        void *src = page_ptr(page) + offset;
//        void *dst = buffer->data + read;
//        uint16_t cur_read = MIN(size - read, PAGE_SIZE - offset);
//        memcpy(dst, src, cur_read);
//        read += cur_read;
//        offset = sizeof(list_node_h);
//        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
//            allocator_unmap_page(heap->allocator, page);
//            list_iterator_free(lit);
//            return 0;
//        }
//        if (allocator_unmap_page(heap->allocator, page) != ALLOCATOR_SUCCESS) {
//            list_iterator_free(lit);
//            return 0;
//        }
//    }
//    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
//        return 0;
//    }
//
//    record_h *record_header = (record_h *) record->data;
//    uint64_t record_size = record_header->size;
//    buffer_t *result = buffer_init(record_size);

}

static offset_t heap_record_header(heap_t *heap, offset_t record_offset, record_h *header) {
    assert(heap != NULL && header != NULL);
    buffer_t buffer;
    buffer.size = sizeof(record_h);
    buffer.data = (char *) header;
    return heap_read(heap, record_offset, &buffer);
}

static bool heap_iterator_is_on_free_record(heap_it *it) {
    assert(it != NULL);
    record_h header;
    offset_t it_offset = heap_iterator_offset(it);
    if (0 == it_offset) {
        return false;
    }
    if (heap_record_header(it->heap, heap_iterator_offset(it), &header) == 0) {
        return false;
    }
    return header.is_free;
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
    *((record_h *) record->data) = (record_h) {.is_free = false, .size = data_size};
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
        while (!heap_iterator_is_empty(it) && heap_iterator_is_on_free_record(it)) {
            if (heap_iterator_next(it) == HEAP_OP_ERROR) {
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
    if (0 == it->record_offset || heap_iterator_offset(it) == it->heap->header->end) {
        return true;
    }
    return false;
}

heap_result heap_iterator_next(heap_it *it) {
    // todo: rework, consider free records
    // reqs: function should skip freed records, though iterator validity is not responsibility of function
    // do {
    //     next(it);
    // } while (is_free_record(it));
    //
    // next:
    // 1) calculate:
    // - number of bytes to skip at the first page
    // - number of pages to skip
    // - number of bytes to skip on the last page
    // 2) check if last byte is heap end
    assert(NULL != it);

//    if (heap_iterator_is_empty(it)) {
//        return HEAP_OP_SUCCESS;
//    }
//    uint64_t size = it->heap->header->record_size;
//    offset_t page_off = list_iterator_offset(it->lit);
//    offset_t end = page_off + it->record_offset + size;
//    while (record_page_offset(end) != page_off && end != it->heap->header->end) {
//        size -= PAGE_SIZE - it->record_offset;
//        it->record_offset = sizeof(list_node_h);
//        if (list_iterator_next(it->lit) != LIST_OP_SUCCESS) {
//            return HEAP_OP_ERROR;
//        }
//        page_off = list_iterator_offset(it->lit);
//        end = page_off + it->record_offset + size;
//    }
//    if (end == it->heap->header->end) {
//        it->record_offset = 0;
//        return HEAP_OP_SUCCESS;
//    }
//    it->record_offset = end - list_iterator_offset(it->lit);
    return HEAP_OP_SUCCESS;
}

buffer_t *heap_iterator_get(heap_it *it) {
    // todo: rework
    // reqs: function is not responsible for iterator validity
    // 1) read header -> return end of header (may be end of the page)
    // 2) allocate buffer with size of header
    // 3) read record body to buffer
    // 4) return buffer

    assert(NULL != it);
//    if (heap_iterator_is_empty(it)) {
//        return NULL;
//    }
//    uint64_t size = it->heap->header->record_size;
//    buffer_t *record = buffer_init(size);
//    if (NULL == record) {
//        return NULL;
//    }
//    uint64_t read = 0;
//    uint16_t offset = it->record_offset;
//    list_it *lit = list_iterator_copy(it->lit);
//    if (NULL == lit) {
//        return NULL;
//    }
//    while (read != size) {
//        page_t *page = list_iterator_get(lit);
//        if (NULL == page) {
//            list_iterator_free(lit);
//            buffer_free(record);
//            return NULL;
//        }
//        void *src = page_ptr(page) + offset;
//        void *dst = record->data + read;
//        uint16_t cur_read = MIN(size - read, PAGE_SIZE - offset);
//        memcpy(dst, src, cur_read);
//        read += cur_read;
//        offset = sizeof(list_node_h);
//        if (list_iterator_next(lit) != LIST_OP_SUCCESS) {
//            allocator_unmap_page(it->heap->allocator, page);
//            list_iterator_free(lit);
//            buffer_free(record);
//            return NULL;
//        }
//        if (allocator_unmap_page(it->heap->allocator, page) != ALLOCATOR_SUCCESS) {
//            list_iterator_free(lit);
//            buffer_free(record);
//            return NULL;
//        }
//    }
//    if (list_iterator_free(lit) != LIST_OP_SUCCESS) {
//        buffer_free(record);
//        return NULL;
//    }
//    record_h *record_header = (record_h *) record->data;
//    uint64_t record_size = record_header->size;
//    buffer_t *result = buffer_init(record_size);
//    if (NULL == result) {
//        buffer_free(record);
//        return NULL;
//    }
//    memcpy(result->data, record->data + sizeof(record_h), record_size);
//    buffer_free(record);
//    return result;
    return NULL;
}

heap_result heap_iterator_delete(heap_it *it);

heap_result heap_iterator_replace(heap_it *it, buffer_t *data);

heap_result heap_compress(heap_it *heap);
