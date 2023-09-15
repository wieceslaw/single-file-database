//
// Created by vyach on 13.09.2023.
//

#include <stddef.h>
#include "heap/list.h"

list_h *list_header(list *list) {
    return block_addr_get(&list->header);
}

static list_result p_list_append(list *list, block* b) {
    if (NULL == b) {
        return LIST_OP_ERROR;
    }
    list_h* header = list_header(list);
    if (header->head == 0) {
        header->head = block_offset(b);
    }
    if (header->tail != 0) {
        list_node_h* last_node = (list_node_h *) allocator_map_block(list->allocator, header->tail);
        if (last_node == NULL) {
            return LIST_OP_ERROR;
        }
        last_node->next = block_offset(b);
        list_node_h* node = (list_node_h *) block_ptr(b);
        node->prev = header->tail;
        if (allocator_unmap_block(list->allocator, (block *) last_node) != ALLOCATOR_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    header->tail = block_offset(b);
    return LIST_OP_SUCCESS;
}

list_result list_extend(list *list, uint32_t n) {
    allocator_reserve_blocks(list->allocator, n);
    for (uint32_t i = 0; i < n; i++) {
        block *block = allocator_get_block(list->allocator);
        if (NULL == block) {
            return LIST_OP_ERROR;
        }
        if (p_list_append(list, block) != LIST_OP_SUCCESS) {
            allocator_unmap_block(list->allocator, block);
            return LIST_OP_ERROR;
        }
        if (allocator_unmap_block(list->allocator, block) != ALLOCATOR_SUCCESS) {
            return LIST_OP_ERROR;
        }
    }
    return LIST_OP_SUCCESS;
}

list_result list_iterator(list *list, list_it *it) {
    if (it == NULL) {
        return LIST_OP_ERROR;
    }
    offset_t head_offset = list_header(it->list)->head;
    it->list = list;
    if (head_offset != 0) {
        list_node_h* node = (list_node_h *) allocator_map_block(it->list->allocator, list_header(it->list)->head);
        if (node == NULL) {
            return LIST_OP_ERROR;
        }
        it->node = node;
    }
    return LIST_OP_SUCCESS;
}

list_result list_clear(list *list) {
    ;
}
