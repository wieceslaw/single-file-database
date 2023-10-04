//
// Created by vyach on 04.10.2023.
//

#include "list.h"

typedef struct list_node {
    void* value;
    struct list_node *next, *prev;
} *list_node_t;

struct list {
    list_node_t head, prev;
    size_t size;
};

// TODO: Implement

list_t list_init(void);

void list_free(list_t list);

void list_clear(list_t list);

size_t list_size(list_t list);

bool list_append_head(list_t list, list_value value);

bool list_append_tail(list_t list, list_value value);

void list_remove_head(list_t list);

void list_remove_tail(list_t list);

list_value list_get_head(list_t list);

list_value list_get_tail(list_t list);

list_it list_head_iterator(list_t list);

list_it list_tail_iterator(list_t list);

void list_it_free(list_it it);

list_value list_it_get(list_it it);

bool list_it_is_empty(list_it it);

void list_it_next(list_it it);

void list_it_prev(list_it it);
