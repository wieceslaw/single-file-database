////
//// Created by vyach on 15.09.2023.
////
//
//
//#include <stddef.h>
//#include <malloc.h>
//#include "list.h"
//
//struct list_it{
//    list *list;
//    offset_t node;
//};
//
//list_it* list_get_iterator(list *list) {
//    if (NULL == list) {
//        return NULL;
//    }
//    list_it* it = malloc(sizeof(list_it));
//    it->list = list;
//    it->node = list_header(list)->head;
//    return it;
//}
//
//void list_iterator_free(list_it *it) {
//    free(it);
//}
//
//bool list_iterator_is_empty(list_it *it) {
//    if (NULL == it) {
//        return true;
//    }
//    return it->node == 0;
//}
//
//block* list_iterator_get_block(list_it *it) {
//    if (NULL == it) {
//        return NULL;
//    }
//    return allocator_map_block_ref(it->list->allocator, it->node);
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
//    if (allocator_unmap_block_ref(it->list->allocator, (block *) it->node) != ALLOCATOR_SUCCESS) {
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
//    if (allocator_return_block(it->list->allocator, node_offset) != ALLOCATOR_SUCCESS) {
//        return LIST_OP_ERROR;
//    }
//    return LIST_OP_SUCCESS;
//}
