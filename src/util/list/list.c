//
// Created by vyach on 04.10.2023.
//

#include <assert.h>
#include "list.h"
#include "util/exceptions/exceptions.h"

struct list_node {
    void *value;
    struct list_node *next, *prev;
};

struct list {
    list_node_t head, tail;
    size_t size;
};

list_t list_init(void) { // throws: [MALLOC_EXCEPTION]
    list_t list = rmalloc(sizeof(struct list));
    *list = (struct list) {.size = 0, .head = NULL, .tail = NULL};
    return list;
}

void list_free(list_t list) {
    free(list);
}

size_t list_size(list_t list) {
    assert(list != NULL);
    return list->size;
}

bool list_is_empty(list_t list) {
    assert(list != NULL);
    return 0 == list_size(list);
}

static list_node_t list_node_init(list_value value) { // throws: [MALLOC_EXCEPTION]
    list_node_t node = rmalloc(sizeof(struct list_node));
    *node = (struct list_node) {.value = value, .next = NULL, .prev = NULL};
    return node;
}

void list_append_head(list_t list, list_value value) { // throws: [MALLOC_EXCEPTION]
    list_node_t node = list_node_init(value);
    if (NULL == list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->head->prev = node;
        node->next = list->head;
    }
    list->size++;
}

void list_append_tail(list_t list, list_value value) {
    list_node_t node = list_node_init(value);
    if (NULL == list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
    }
    list->size++;
}

void list_remove_head(list_t list) {
    if (list_is_empty(list)) {
        return;
    }
    list_node_t head = list->head;
    if (1 == list->size) {
        free(head);
        list->head = NULL;
        list->tail = NULL;
    } else {
        head->next->prev = NULL;
        free(head);
    }
    list->size--;
}

void list_remove_tail(list_t list) {
    if (list_is_empty(list)) {
        return;
    }
    list_node_t tail = list->tail;
    if (1 == list->size) {
        free(tail);
        list->head = NULL;
        list->tail = NULL;
    } else {
        tail->prev->next = NULL;
        free(list->head);
    }
    list->size--;
}

list_value list_get_head(list_t list) {
    if (list_is_empty(list)) {
        return NULL;
    }
    return list->head->value;
}

list_value list_get_tail(list_t list) {
    if (list_is_empty(list)) {
        return NULL;
    }
    return list->tail->value;
}

list_it list_head_iterator(list_t list) {
    list_it it = rmalloc(sizeof(list_node_t));
    *it = list->head;
    return it;
}

list_it list_tail_iterator(list_t list) {
    list_it it = rmalloc(sizeof(list_node_t));
    *it = list->tail;
    return it;
}

list_value list_it_get(list_it it) {
    if (list_it_is_empty(it)) {
        return NULL;
    }
    return (*it)->value;
}

bool list_it_is_empty(list_it it) {
    return NULL == *it;
}

void list_it_free(list_it it) {
    if (NULL == it) {
        return;
    }
    *it = NULL;
    free(it);
}

void list_it_next(list_it it) {
    if (list_it_is_empty(it)) {
        return;
    }
    *it = (*it)->next;
}

void list_it_prev(list_it it) {
    if (list_it_is_empty(it)) {
        return;
    }
    *it = (*it)->prev;
}

// TODO: Implement

void list_it_delete(list_it it);

void list_clear(list_t list);
