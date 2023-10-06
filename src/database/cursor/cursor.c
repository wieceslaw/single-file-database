//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include <stddef.h>
#include "cursor.h"

//bool cursor_is_empty(cursor_t *cur) {
//    assert(cur != NULL);
//    switch (cur->type) {
//        case CURSOR_TYPE_FROM: {
//            return pool_iterator_is_empty(cur->from.it);
//        }
//        case CURSOR_TYPE_JOIN: {
//            switch (cur->join.type) {
//                case JOIN_TYPE_INNER:
//                    return cursor_is_empty(cur->join.left) || cursor_is_empty(cur->join.right);
//                case JOIN_TYPE_LEFT:
//                    return cursor_is_empty(cur->join.left);
//                case JOIN_TYPE_RIGHT:
//                    return cursor_is_empty(cur->join.right);
//            }
//        }
//        case CURSOR_TYPE_WHERE: {
//            return cursor_is_empty(cur);
//        }
//    }
//}
//
//cursor_result cursor_restart(cursor_t *cur) {
//    assert(cur != NULL);
//    switch (cur->type) {
//        case CURSOR_TYPE_FROM: {
//            pool_iterator_restart(cur->from.it);
//            return CURSOR_OP_OK;
//        }
//        case CURSOR_TYPE_JOIN: {
//            // TODO: implement
//            return CURSOR_OP_ERR;
//        }
//        case CURSOR_TYPE_WHERE: {
//            // TODO: implement
//            return CURSOR_OP_ERR;
//        }
//    }
//}
//
//cursor_result cursor_next(cursor_t *cur) {
//    assert(cur != NULL);
//    switch (cur->type) {
//        case CURSOR_TYPE_FROM: {
//            pool_iterator_next(cur->from.it);
//            return CURSOR_OP_OK;
//        }
//        case CURSOR_TYPE_JOIN: {
//            // TODO: implement
//            return CURSOR_OP_ERR;
//        }
//        case CURSOR_TYPE_WHERE: {
//            // TODO: implement
//            return CURSOR_OP_ERR;
//        }
//    }
//}
//
//row_set_t *cursor_get(cursor_t *cur) {
//    assert(cur != NULL);
//    switch (cur->type) {
//        case CURSOR_TYPE_FROM: {
//            pool_iterator_next(cur->from.it);
//            return ;
//        }
//        case CURSOR_TYPE_JOIN: {
//            // TODO: implement
//            return NULL;
//        }
//        case CURSOR_TYPE_WHERE: {
//            // TODO: implement
//            return NULL;
//        }
//    }
//}
//
//cursor_result cursor_delete(cursor_t *, char *alias);
//
//cursor_result cursor_update(cursor_t *, char *alias, updater_t *updater);
