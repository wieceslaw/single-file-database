//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_PAGELIST_H
#define LLP_LAB1_PAGELIST_H

#include <stdint.h>
#include <stdbool.h>
#include "allocator/allocator.h"

typedef enum {
    LIST_OP_SUCCESS = 0,
    LIST_OP_ERROR = 1
} page_list_result;

typedef PACK(
        struct {
            offset_t head;
            offset_t tail;
        }
) list_h;

typedef PACK(
        struct {
            offset_t next;
            offset_t prev;
        }
) page_list_node_h;

typedef struct page_list_t page_list_t;

typedef struct page_list_it page_list_it;

bool page_list_is_empty(page_list_t *list);

void page_list_place(page_t *page, offset_t offset);

page_list_result page_list_clear(page_list_t *list);

page_list_t *page_list_init(list_h *header, allocator_t *allocator);

void page_list_free(page_list_t *list);

page_list_result page_list_extend(page_list_t *list, uint32_t n);

page_list_result page_list_delete_node(page_list_t *list, page_t *node_page);

page_list_it *page_list_get_iterator(page_list_t *list, offset_t page_offset);

page_list_it *page_list_get_head_iterator(page_list_t *list);

page_list_it *page_list_get_tail_iterator(page_list_t *list);

page_list_it *page_list_iterator_copy(page_list_it *it);

page_list_result page_list_iterator_free(page_list_it *it);

bool page_list_iterator_is_empty(page_list_it *it);

page_list_result page_list_iterator_next(page_list_it *it);

page_list_result page_list_iterator_prev(page_list_it *it);

offset_t page_list_iterator_offset(page_list_it *it);

page_t *page_list_iterator_get(page_list_it *it);

page_list_result page_list_iterator_delete_goto_next(page_list_it *it);

#endif //LLP_LAB1_PAGELIST_H
