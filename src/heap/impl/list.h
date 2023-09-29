//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_LIST_H
#define LLP_LAB1_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include "allocator/allocator.h"

typedef enum {
    LIST_OP_SUCCESS = 0,
    LIST_OP_ERROR = 1
} list_result;

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
) list_node_h;

typedef struct list_t list_t;

typedef struct list_it list_it;

bool list_is_empty(list_t *list);

list_result list_place(page_t *page, offset_t offset);

list_result list_clear(list_t *);

list_t *list_init(list_h *header, allocator_t *allocator);

void list_free(list_t *);

list_result list_extend(list_t *, uint32_t n);

list_result list_delete_node(list_t *list, page_t *page);

list_it *list_get_iterator(list_t *, offset_t page_offset);

list_it *list_get_head_iterator(list_t *list);

list_it *list_get_tail_iterator(list_t *list);

list_it *list_iterator_copy(list_it *);

list_result list_iterator_free(list_it *);

bool list_iterator_is_empty(list_it *);

list_result list_iterator_next(list_it *);

list_result list_iterator_prev(list_it *it);

offset_t list_iterator_offset(list_it *);

page_t *list_iterator_get(list_it *);

list_result list_iterator_delete_goto_next(list_it *it);

#endif //LLP_LAB1_LIST_H
