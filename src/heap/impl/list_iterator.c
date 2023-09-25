////
//// Created by vyach on 15.09.2023.
////
//
//
//#include <stddef.h>
//#include <malloc.h>
//#include "list_t.h"

//block* list_iterator_get_block(list_it *it) {
//    if (NULL == it) {
//        return NULL;
//    }
//    return allocator_map_page(it->list_t->allocator_t, it->node);
//}
//
//offset_t list_iterator_get_offset(list_it *it) {
//    if (NULL == it) {
//        return 0;
//    }
//    return it->node;
//}
//
//list_result list_iterator_next(list_it *it) {
//    if (NULL == it) {
//        return LIST_OP_ERROR;
//    }
//    if (list_iterator_is_empty(it)) {
//        return LIST_OP_SUCCESS;
//    }
//    list_node_h *node = (list_node_h *) list_iterator_get_block(it);
//    if (NULL == node) {
//        return LIST_OP_ERROR;
//    }
//    it->node = node->next;
//    if (allocator_unmap_page(it->list_t->allocator_t, (block *) it->node) != ALLOCATOR_SUCCESS) {
//        return LIST_OP_ERROR;
//    }
//    return LIST_OP_SUCCESS;
//}
//
//list_result list_iterator_delete(list_it *it) {
//    if (NULL == it) {
//        return LIST_OP_ERROR;
//    }
//    if (list_iterator_is_empty(it)) {
//        return LIST_OP_SUCCESS;
//    }
//    offset_t node_offset = list_iterator_get_offset(it);
//    if (list_iterator_next(it) != LIST_OP_SUCCESS) {
//        return LIST_OP_ERROR;
//    }
//    if (allocator_return_page(it->list_t->allocator_t, node_offset) != ALLOCATOR_SUCCESS) {
//        return LIST_OP_ERROR;
//    }
//    return LIST_OP_SUCCESS;
//}
