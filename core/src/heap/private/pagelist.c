//
// Created by vyach on 13.09.2023.
//

#include <malloc.h>
#include <assert.h>
#include "pagelist.h"

struct page_list_it {
    page_list_t *page_list;
    page_t *page;
};

struct page_list_t {
    allocator_t *allocator;
    list_h *header;
};

offset_t page_list_iterator_offset(page_list_it *it) {
    assert(NULL != it);
    if (page_list_iterator_is_empty(it)) {
        return 0;
    }
    return page_offset(it->page);
}

bool page_list_is_empty(page_list_t *list) {
    assert(NULL != list);
    return 0 == list->header->head;
}

void page_list_place(page_t *page, offset_t offset) {
    assert(NULL != page);
    list_h *header = (list_h *) (page_ptr(page) + offset);
    *header = (list_h) {0};
}

page_list_result page_list_clear(page_list_t *list) {
    assert(NULL != list);
    page_list_it *it = page_list_get_head_iterator(list);
    if (NULL == it) {
        return LIST_OP_ERROR;
    }
    while (!page_list_iterator_is_empty(it)) {
        if (page_list_iterator_delete_goto_next(it) != LIST_OP_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    *list->header = (list_h) {0};
    return page_list_iterator_free(it);
}

page_list_t *page_list_init(list_h *header, allocator_t *allocator) {
    assert(NULL != header && NULL != allocator);
    if (NULL == header || NULL == allocator) {
        return NULL;
    }
    page_list_t *list_ptr = malloc(sizeof(page_list_t));
    if (NULL == list_ptr) {
        return NULL;
    }
    *list_ptr = (page_list_t) {.header = header, .allocator = allocator};
    return list_ptr;
}

void page_list_free(page_list_t *list) {
    free(list);
}

static page_list_result page_list_append(page_list_t *list, page_t *page) {
    assert(NULL != list && NULL != page);
    if (0 == list->header->head) {
        list->header->head = page_offset(page);
    }
    page_list_node_h *node = (page_list_node_h *) page_ptr(page);
    *node = (page_list_node_h) {0};
    if (0 != list->header->tail) {
        page_t *tail_page = allocator_map_page(list->allocator, list->header->tail);
        if (NULL == tail_page) {
            return LIST_OP_ERROR;
        }
        page_list_node_h *tail_node = (page_list_node_h *) page_ptr(tail_page);
        tail_node->next = page_offset(page);;
        node->prev = list->header->tail;
        if (allocator_unmap_page(list->allocator, tail_page) != ALLOCATOR_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    list->header->tail = page_offset(page);
    return LIST_OP_SUCCESS;
}

page_list_result page_list_extend(page_list_t *list, uint32_t n) {
    assert(NULL != list);
    if (allocator_reserve_pages(list->allocator, n) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    for (uint32_t i = 0; i < n; i++) {
        page_t *free_page = allocator_get_page(list->allocator);
        if (NULL == free_page) {
            return LIST_OP_ERROR;
        }
        if (page_list_append(list, free_page) != LIST_OP_SUCCESS) {
            allocator_unmap_page(list->allocator, free_page);
            return LIST_OP_ERROR;
        }
        if (allocator_unmap_page(list->allocator, free_page) != ALLOCATOR_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    return LIST_OP_SUCCESS;
}

page_list_it *page_list_get_head_iterator(page_list_t *list) {
    assert(NULL != list);
    return page_list_get_iterator(list, list->header->head);
}

page_list_it *page_list_get_tail_iterator(page_list_t *list) {
    assert(NULL != list);
    return page_list_get_iterator(list, list->header->tail);
}

page_list_it *page_list_get_iterator(page_list_t *list, offset_t page_offset) {
    assert(NULL != list);
    page_list_it *it = malloc(sizeof(page_list_it));
    if (NULL == it) {
        return NULL;
    }
    it->page_list = list;
    if (0 == page_offset) {
        it->page = NULL;
        return it;
    }
    it->page = allocator_map_page(list->allocator, page_offset);
    if (NULL == it->page) {
        free(it);
        return NULL;
    }
    return it;
}

page_list_it *page_list_iterator_copy(page_list_it *it) {
    assert(NULL != it);
    return page_list_get_iterator(it->page_list, page_offset(it->page));
}

page_list_result page_list_iterator_free(page_list_it *it) {
    assert(NULL != it);
    if (NULL != it->page) {
        if (allocator_unmap_page(it->page_list->allocator, it->page) != ALLOCATOR_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    free(it);
    return LIST_OP_SUCCESS;
}

bool page_list_iterator_is_empty(page_list_it *it) {
    assert(NULL != it);
    return NULL == it->page;
}

page_list_result page_list_iterator_next(page_list_it *it) {
    assert(NULL != it);
    if (page_list_iterator_is_empty(it)) {
        return LIST_OP_ERROR;
    }
    page_list_node_h *node = (page_list_node_h *) page_ptr(it->page);
    if (NULL == node) {
        return LIST_OP_ERROR;
    }
    offset_t next = node->next;
    if (allocator_unmap_page(it->page_list->allocator, it->page) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    if (0 == next) {
        it->page = NULL;
        return LIST_OP_SUCCESS;
    }
    it->page = allocator_map_page(it->page_list->allocator, next);
    if (NULL == it->page) {
        return LIST_OP_ERROR;
    }
    return LIST_OP_SUCCESS;
}

page_list_result page_list_iterator_prev(page_list_it *it) {
    assert(NULL != it);
    if (page_list_iterator_is_empty(it)) {
        return LIST_OP_ERROR;
    }
    page_list_node_h *node = (page_list_node_h *) page_ptr(it->page);
    if (NULL == node) {
        return LIST_OP_ERROR;
    }
    offset_t prev = node->prev;
    if (allocator_unmap_page(it->page_list->allocator, it->page) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    if (0 == prev) {
        it->page = NULL;
        return LIST_OP_SUCCESS;
    }
    it->page = allocator_map_page(it->page_list->allocator, prev);
    if (NULL == it->page) {
        return LIST_OP_ERROR;
    }
    return LIST_OP_SUCCESS;
}

page_t *page_list_iterator_get(page_list_it *it) {
    assert(NULL != it);
    if (page_list_iterator_is_empty(it)) {
        return NULL;
    }
    return page_copy(it->page_list->allocator, it->page);
}

page_list_result page_list_delete_node(page_list_t *list, page_t *node_page) {
    assert(NULL != node_page && NULL != list);
    allocator_t *allocator = list->allocator;
    page_list_node_h *node = (page_list_node_h *) page_ptr(node_page);
    if (node->prev != 0 && node->next != 0) {
        page_t *next_node_page = allocator_map_page(allocator, node->next);
        if (NULL == next_node_page) {
            return LIST_OP_ERROR;
        }
        page_list_node_h *next_node = (page_list_node_h *) page_ptr(next_node_page);
        page_t *prev_node_page = allocator_map_page(allocator, node->prev);
        if (NULL == prev_node_page) {
            return LIST_OP_ERROR;
        }
        page_list_node_h *prev_node = (page_list_node_h *) page_ptr(prev_node_page);
        prev_node->next = node->next;
        next_node->prev = node->prev;
        if (allocator_unmap_page(allocator, next_node_page) != ALLOCATOR_SUCCESS) {
            allocator_unmap_page(allocator, prev_node_page);
            return LIST_OP_ERROR;
        }
        if (allocator_unmap_page(allocator, prev_node_page) != ALLOCATOR_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    if (node->prev == 0) {
        list->header->head = node->next;
        if (node->next != 0) {
            page_t *next_node_page = allocator_map_page(allocator, node->next);
            if (NULL == next_node_page) {
                return LIST_OP_ERROR;
            }
            page_list_node_h *next_node = (page_list_node_h *) page_ptr(next_node_page);
            next_node->prev = 0;
            if (allocator_unmap_page(allocator, next_node_page) != ALLOCATOR_SUCCESS) {
                return LIST_OP_ERROR;
            }
        }
    }
    if (node->next == 0) {
        list->header->tail = node->prev;
        if (node->prev != 0) {
            page_t *prev_node_page = allocator_map_page(allocator, node->prev);
            if (NULL == prev_node_page) {
                return LIST_OP_ERROR;
            }
            page_list_node_h *prev_node = (page_list_node_h *) page_ptr(prev_node_page);
            prev_node->next = 0;
            if (allocator_unmap_page(allocator, prev_node_page) != ALLOCATOR_SUCCESS) {
                return LIST_OP_ERROR;
            }
        }
    }
    if (allocator_return_page(allocator, page_offset(node_page)) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    return LIST_OP_SUCCESS;
}

page_list_result page_list_iterator_delete_goto_next(page_list_it *it) {
    assert(NULL != it);
    if (page_list_iterator_is_empty(it)) {
        return LIST_OP_ERROR;
    }
    allocator_t *allocator = it->page_list->allocator;
    page_t *node_page = page_list_iterator_get(it);
    if (NULL == node_page) {
        return LIST_OP_ERROR;
    }
    if (page_list_iterator_next(it) != LIST_OP_SUCCESS) {
        allocator_unmap_page(allocator, node_page);
        return LIST_OP_ERROR;
    }
    if (page_list_delete_node(it->page_list, node_page) != LIST_OP_SUCCESS) {
        allocator_unmap_page(allocator, node_page);
        return LIST_OP_ERROR;
    }
    if (allocator_unmap_page(allocator, node_page) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    return LIST_OP_SUCCESS;
}
