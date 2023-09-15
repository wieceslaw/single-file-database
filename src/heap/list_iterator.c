//
// Created by vyach on 15.09.2023.
//


#include <stddef.h>
#include "heap/list.h"

list_result list_iterator_free(list_it *it) {
    if (allocator_unmap_block(it->list->allocator, (block *) it->node) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    return LIST_OP_SUCCESS;
}

bool list_iterator_is_empty(list_it *it) {
    return it->node == NULL;
}

block* list_iterator_get(list_it *it) {
    return (block *) it->node;
}

list_result list_iterator_next(list_it *it) {
    if (list_iterator_is_empty(it)) {
        return LIST_OP_SUCCESS;
    }
    list_node_h *next_node = (list_node_h *) allocator_map_block(it->list->allocator, it->node->next);
    if (next_node == NULL) {
        allocator_unmap_block(it->list->allocator, (block *) it->node);
        return LIST_OP_ERROR;
    }
    if (allocator_unmap_block(it->list->allocator, (block *) it->node) != ALLOCATOR_SUCCESS) {
        return LIST_OP_ERROR;
    }
    it->node = next_node;
    return LIST_OP_SUCCESS;
}

list_result list_iterator_delete(list_it *it);
