//
// Created by vyach on 20.09.2023.
//

#include <stdbool.h>
#include <stddef.h>
#include <malloc.h>
#include <assert.h>
#include "allocator/allocator.h"
#include "blocklist.h"

block_list_node *block_list_append(block_list *list, block *block) {
    assert(NULL != list && NULL != block);
    block_list_node *node = malloc(sizeof(block_list_node));
    if (NULL == node) {
        return NULL;
    }
    *node = (block_list_node) {0};
    block->node = node;
    node->block = block;
    node->file_offset = block->file_offset;
    if (NULL == list->head) {
        list->head = node;
    } else {
        block_list_node *last_node = list->tail;
        last_node->next = node;
        node->prev = last_node;
    }
    list->tail = node;
    MAP_PUT(list->map, &(block->file_offset), node);
    return node;
}

bool block_list_delete(block_list *list, block_list_node *node) {
    assert(NULL != list && NULL != node);
    MAP_REMOVE(list->map, &(node->file_offset));
    block_list_node *prev = node->prev;
    block_list_node *next = node->next;
    if (NULL != prev && NULL != next) {
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
    free(node);
    return true;
}

void block_list_iterator(block_list *list, block_list_it *it) {
    assert(NULL != list && NULL != it);
    it->list = list;
    it->node = list->head;
}

bool block_list_iterator_is_empty(block_list_it *it) {
    assert(NULL != it);
    return NULL == it->node;
}

void block_list_iterator_next(block_list_it *it) {
    assert(NULL != it);
    if (block_list_iterator_is_empty(it)) {
        return;
    }
    it->node = it->node->next;
}

block *block_list_iterator_get(block_list_it *it) {
    assert(NULL != it);
    if (block_list_iterator_is_empty(it)) {
        return NULL;
    }
    return it->node->block;
}

block_list_node *block_list_find(block_list *list, offset_t offset) {
    assert(NULL != list);
    block_list_node* node = MAP_GET(list->map, &offset);
    return node;
}
