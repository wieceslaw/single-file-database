////
//// Created by vyach on 13.09.2023.
////
//
//#include <stddef.h>
//#include "list.h"
//
//list_h *list_header(list *list) {
//    return block_addr_get(&list->header);
//}
//
//static list_result list_append(list *list, block *b) {
//    if (NULL == list || NULL == b) {
//        return LIST_OP_ERROR;
//    }
//    list_h *header = list_header(list);
//    if (0 == header->head) {
//        header->head = block_offset(b);
//    }
//    if (0 != header->tail) {
//        list_node_h *last_node = (list_node_h *) allocator_map_page(list->allocator, header->tail);
//        if (NULL == last_node) {
//            return LIST_OP_ERROR;
//        }
//        last_node->next = block_offset(b);
//        list_node_h *node = (list_node_h *) block_ptr(b);
//        node->prev = header->tail;
//        if (allocator_unmap_page(list->allocator, (block *) last_node) != ALLOCATOR_SUCCESS) {
//            return LIST_OP_ERROR;
//        }
//    }
//    header->tail = block_offset(b);
//    return LIST_OP_SUCCESS;
//}
//
//list* list_init(block_addr addr);
//
//void list_free();
//
//list_result list_extend(list *list, uint32_t n) {
//    if (NULL == list) {
//        return LIST_OP_ERROR;
//    }
//    allocator_reserve_pages(list->allocator, n);
//    for (uint32_t i = 0; i < n; i++) {
//        block *block = allocator_get_block(list->allocator);
//        if (NULL == block) {
//            return LIST_OP_ERROR;
//        }
//        if (list_append(list, block) != LIST_OP_SUCCESS) {
//            allocator_unmap_page(list->allocator, block);
//            return LIST_OP_ERROR;
//        }
//        if (allocator_unmap_page(list->allocator, block) != ALLOCATOR_SUCCESS) {
//            return LIST_OP_ERROR;
//        }
//    }
//    return LIST_OP_SUCCESS;
//}
//
//list_result list_clear(list *list) {
//    list_it *it = list_get_iterator(list);
//    list_h *header = list_header(list);
//    while (!list_iterator_is_empty(it)) {
//        offset_t node_offset = list_iterator_get_offset(it);
//        if (list_iterator_next(it) != LIST_OP_SUCCESS) {
//            return LIST_OP_ERROR;
//        }
//        header->head = list_iterator_get_offset(it);
//        if (allocator_return_page(list->allocator, node_offset) != LIST_OP_SUCCESS) {
//            return LIST_OP_ERROR;
//        }
//    }
//    *header = (list_h) {0};
//    return LIST_OP_SUCCESS;
//}
