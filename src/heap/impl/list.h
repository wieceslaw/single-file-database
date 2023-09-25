//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_LIST_H
#define LLP_LAB1_LIST_H

#include <stdint-gcc.h>
#include <stdbool.h>
#include "allocator/allocator.h"

typedef enum {
    LIST_OP_SUCCESS = 0,
    LIST_OP_ERROR = 1
} list_result;

typedef struct __attribute__((__packed__)) {
    offset_t head;
    offset_t tail;
} list_h;

typedef struct __attribute__((__packed__)) {
    offset_t next;
    offset_t prev;
} list_node_h;

typedef struct list_t list_t;

typedef struct list_it list_it;

offset_t list_size();

list_result list_place(page_t *page, offset_t offset);

list_result list_clear(list_t *);

list_t *list_init(page_t *page, offset_t offset, allocator_t *allocator);

void list_free(list_t *);

list_result list_extend(list_t *, uint32_t n);

list_it *list_get_iterator(list_t *);

list_result list_iterator_free(list_it *);

bool list_iterator_is_empty(list_it *);

list_result list_iterator_next(list_it *);

page_t *list_iterator_get(list_it *);

list_result list_iterator_delete(list_it *);

#endif //LLP_LAB1_LIST_H
