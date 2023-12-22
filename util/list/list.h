//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_LIST_H
#define LLP_LAB1_LIST_H

#include <stdbool.h>
#include <stddef.h>

#define FOR_LIST(L, I, CODE)  {\
    list_it I = list_head_iterator(L); \
while (!list_it_is_empty(I)) { \
 CODE \
list_it_next(I); \
} \
list_it_free(&(I)); }\

typedef struct list *list_t;

typedef struct list_iterator *list_it;

typedef void *list_value;

typedef void(*clearer_t)(void *);

list_t list_init(void);

void list_free(list_t *list_ptr);

void list_clear(list_t list, clearer_t clearer);

size_t list_size(list_t list);

bool list_is_empty(list_t list);

void list_append_head(list_t list, list_value value);

void list_append_tail(list_t list, list_value value);

void list_remove_head(list_t list);

void list_remove_tail(list_t list);

list_value list_get_head(list_t list);

list_value list_get_tail(list_t list);

list_it list_head_iterator(list_t list);

list_it list_tail_iterator(list_t list);

void list_it_free(list_it *it);

list_value list_it_get(list_it it);

bool list_it_is_empty(list_it it);

void list_it_next(list_it it);

void list_it_prev(list_it it);

void list_it_delete(list_it it);

#endif //LLP_LAB1_LIST_H
