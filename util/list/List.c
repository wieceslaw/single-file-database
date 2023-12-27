//
// Created by vyach on 04.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "List.h"

typedef struct list_node {
    void *value;
    struct list_node *next, *prev;
} *list_node_t;

struct List {
    list_node_t head, tail;
    size_t size;
};

struct ListIterator {
    List list;
    list_node_t node;
};

static list_node_t list_node_init(ListValue value) {
    list_node_t node = malloc(sizeof(struct list_node));
    if (node == NULL) {
        assert(0);
    }
    *node = (struct list_node){.value = value, .next = NULL, .prev = NULL};
    return node;
}

static void list_node_free(list_node_t *node_ptr) {
    assert(node_ptr != NULL);
    if (NULL == *node_ptr) {
        return;
    }
    free(*node_ptr);
    *node_ptr = NULL;
}

List ListNew(void) {
    List list = malloc(sizeof(struct List));
    if (list == NULL) {
        assert(0);
    }
    *list = (struct List){.size = 0, .head = NULL, .tail = NULL};
    return list;
}

size_t ListSize(List list) {
    assert(list != NULL);
    return list->size;
}

bool ListIsEmpty(List list) {
    assert(list != NULL);
    return 0 == ListSize(list);
}

void ListAppendHead(List list, ListValue value) {
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

void ListAppendTail(List list, ListValue value) {
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

void ListRemoveHead(List list) {
    assert(list != NULL);
    if (ListIsEmpty(list)) {
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

void ListRemoveTail(List list) {
    assert(list != NULL);
    if (ListIsEmpty(list)) {
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

ListValue ListGetHead(List list) {
    assert(list != NULL);
    if (ListIsEmpty(list)) {
        return NULL;
    }
    return list->head->value;
}

ListValue ListGetTail(List list) {
    assert(list != NULL);
    if (ListIsEmpty(list)) {
        return NULL;
    }
    return list->tail->value;
}

ListIterator ListHeadIterator(List list) {
    assert(list != NULL);
    ListIterator it = malloc(sizeof(struct ListIterator));
    if (it == NULL) {
        assert(0);
    }
    it->node = list->head;
    it->list = list;
    return it;
}

ListIterator ListTailIterator(List list) {
    assert(list != NULL);
    ListIterator it = malloc(sizeof(struct ListIterator));
    if (it == NULL) {
        assert(0);
    }
    it->node = list->tail;
    it->list = list;
    return it;
}

bool ListIteratorIsEmpty(ListIterator it) {
    assert(it != NULL);
    return NULL == it->node;
}

ListValue ListIteratorGet(ListIterator it) {
    assert(it != NULL);
    if (ListIteratorIsEmpty(it)) {
        return NULL;
    }
    return it->node->value;
}

void ListIteratorFree(ListIterator *it) {
    assert(it != NULL);
    if (NULL == *it) {
        return;
    }
    (*it)->list = NULL;
    (*it)->node = NULL;
    free(*it);
    *it = NULL;
}

void ListIteratorNext(ListIterator it) {
    assert(it != NULL);
    if (ListIteratorIsEmpty(it)) {
        return;
    }
    it->node = it->node->next;
}

void ListIteratorPrev(ListIterator it) {
    assert(it != NULL);
    if (ListIteratorIsEmpty(it)) {
        return;
    }
    it->node = it->node->prev;
}

void ListIteratorDeleteNode(ListIterator it) {
    assert(it != NULL && it->list != NULL);
    if (ListIteratorIsEmpty(it)) {
        return;
    }
    List list = it->list;
    list_node_t cur = it->node;
    list_node_t next = cur->next;
    list_node_t prev = cur->prev;
    ListIteratorNext(it);
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

void ListApply(List list, Applier applier) {
    assert(list != NULL && applier != NULL);
    ListIterator it = ListHeadIterator(list);
    while (!ListIteratorIsEmpty(it)) {
        applier(ListIteratorGet(it));
        ListIteratorNext(it);
    }
    ListIteratorFree(&it);
}

static void ListClear(List list) {
    assert(list != NULL);
    ListIterator it = ListHeadIterator(list);
    while (!ListIteratorIsEmpty(it)) {
        ListIteratorDeleteNode(it);
    }
    ListIteratorFree(&it);
}

void ListFree(List list) {
    if (list == NULL) {
        return;
    }
    ListClear(list);
    free(list);
}
