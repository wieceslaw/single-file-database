//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_LIST_H
#define LLP_LAB1_LIST_H

#include <stdint-gcc.h>
#include <stdbool.h>
#include "../allocator/allocator.h"

typedef enum {
    LIST_OP_SUCCESS = 0,
    LIST_OP_ERROR = 1
} list_result;

typedef struct {
    offset_t head;
    offset_t tail;
} list_h;

typedef struct {
    allocator *allocator;
    block_addr header;
} list;

typedef struct {
    offset_t next;
    offset_t prev;
} list_node_h;

typedef struct {
    list *list;
    list_node_h *node;
} list_it;

list_h *list_header(list *);

list_result list_extend(list *, uint32_t n);

list_result list_clear(list *);

list_result list_iterator(list *, list_it *);

list_result list_iterator_free(list_it *);

bool list_iterator_is_empty(list_it *);

block* list_iterator_get(list_it *);

list_result list_iterator_next(list_it *);

list_result list_iterator_delete(list_it *);

#endif //LLP_LAB1_LIST_H
