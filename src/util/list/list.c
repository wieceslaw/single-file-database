//
// Created by vyach on 04.10.2023.
//

#include <assert.h>
#include "list.h"
#include "util/exceptions/exceptions.h"

typedef struct list_node {
    void *value;
    struct list_node *next, *prev;
} *list_node_t;

struct list {
    list_node_t head, tail;
    size_t size;
};

struct list_iterator {
    list_t list;
    list_node_t node;
};

/// throws: [MALLOC_EXCEPTION]
static list_node_t list_node_init(list_value value) {
    list_node_t node = rmalloc(sizeof(struct list_node));
    *node = (struct list_node) {.value = value, .next = NULL, .prev = NULL};
    return node;
}

static void list_node_free(list_node_t *node_ptr) {
    assert(node_ptr != NULL);
    if (NULL == *node_ptr) {
        return;
    }
    (*node_ptr)->prev = NULL;
    (*node_ptr)->next = NULL;
    (*node_ptr)->value = NULL;
    free(*node_ptr);
    *node_ptr = NULL;
}

// throws: [MALLOC_EXCEPTION]
list_t list_init(void) {
    list_t list = rmalloc(sizeof(struct list));
    *list = (struct list) {.size = 0, .head = NULL, .tail = NULL};
    return list;
}

size_t list_size(list_t list) {
    assert(list != NULL);
    return list->size;
}

bool list_is_empty(list_t list) {
    assert(list != NULL);
    return 0 == list_size(list);
}

// throws: [MALLOC_EXCEPTION]
void list_append_head(list_t list, list_value value) {
    assert(list != NULL);
    list_node_t node = list_node_init(value);
    if (NULL == list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->head->prev = node;
        node->next = list->head;
        list->head = node;
    }
    list->size++;
}

void list_append_tail(list_t list, list_value value) {
    assert(list != NULL);
    list_node_t node = list_node_init(value);
    if (NULL == list->head) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->size++;
}

void list_remove_head(list_t list) {
    assert(list != NULL);
    if (list_is_empty(list)) {
        return;
    }
    list_node_t head = list->head;
    if (1 == list->size) {
        list_node_free(&head);
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->head = head->next;
        head->next->prev = NULL;
        list_node_free(&head);
    }
    list->size--;
}

void list_remove_tail(list_t list) {
    assert(list != NULL);
    if (list_is_empty(list)) {
        return;
    }
    list_node_t tail = list->tail;
    if (1 == list->size) {
        list_node_free(&tail);
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->tail = tail->prev;
        tail->prev->next = NULL;
        list_node_free(&tail);
    }
    list->size--;
}

list_value list_get_head(list_t list) {
    assert(list != NULL);
    if (list_is_empty(list)) {
        return NULL;
    }
    return list->head->value;
}

list_value list_get_tail(list_t list) {
    assert(list != NULL);
    if (list_is_empty(list)) {
        return NULL;
    }
    return list->tail->value;
}

list_it list_head_iterator(list_t list) {
    assert(list != NULL);
    list_it it = rmalloc(sizeof(struct list_iterator));
    it->node = list->head;
    it->list = list;
    return it;
}

list_it list_tail_iterator(list_t list) {
    assert(list != NULL);
    list_it it = rmalloc(sizeof(struct list_iterator));
    it->node = list->tail;
    it->list = list;
    return it;
}

bool list_it_is_empty(list_it it) {
    assert(it != NULL);
    return NULL == it->node;
}

list_value list_it_get(list_it it) {
    assert(it != NULL);
    if (list_it_is_empty(it)) {
        return NULL;
    }
    return it->node->value;
}

void list_it_free(list_it *it) {
    assert(it != NULL);
    if (NULL == *it) {
        return;
    }
    (*it)->list = NULL;
    (*it)->node = NULL;
    free(*it);
    *it = NULL;
}

void list_it_next(list_it it) {
    assert(it != NULL);
    if (list_it_is_empty(it)) {
        return;
    }
    it->node = it->node->next;
}

void list_it_prev(list_it it) {
    assert(it != NULL);
    if (list_it_is_empty(it)) {
        return;
    }
    it->node = it->node->prev;
}

void list_it_delete(list_it it) {
    assert(it != NULL && it->list != NULL);
    if (list_it_is_empty(it)) {
        return;
    }
    list_t list = it->list;
    list_node_t cur = it->node;
    list_node_t next = cur->next;
    list_node_t prev = cur->prev;
    list_it_next(it);
    if (prev != NULL && next != NULL) {
        prev->next = next;
        next->prev = prev;
    }
    if (NULL == prev) {
        list->head = next;
        if (NULL != next) {
            next->prev = NULL;
        }
    }
    if (NULL == next) {
        list->tail = prev;
        if (NULL != prev) {
            prev->next = NULL;
        }
    }
    list_node_free(&cur);
    list->size--;
}

void list_clear(list_t list) {
    assert(list != NULL);
    list_it it = list_head_iterator(list);
    while (!list_it_is_empty(it)) {
        list_it_delete(it);
    }
    list_it_free(&it);
}

void list_free(list_t *list_ptr) {
    assert(NULL != list_ptr);
    if (NULL == *list_ptr) {
        return;
    }
    list_clear(*list_ptr);
    free(*list_ptr);
    *list_ptr = NULL;
}
