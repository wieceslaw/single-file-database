////
//// Created by vyach on 25.09.2023.
////
//
//#ifndef LLP_LAB1_POOL_H
//#define LLP_LAB1_POOL_H
//
//#include "allocator/allocator.h"
//
//typedef enum {
//    POOL_OP_SUCCESS = 0,
//    POOL_OP_ERROR = 1
//} pool_result;
//
//typedef struct pool_t pool_t;
//
//offset_t pool_create(allocator_t *);
//
//pool_result pool_clear(allocator_t *, offset_t);
//
//pool_t* pool_init(allocator_t *, offset_t);
//
//// delete
//// 1) (while query) mark row as "to_be_deleted"
//// 2) (after query) iterate over pool, delete "to_be_deleted"
//// 3) (after query) compress heaps
//
//// update
//// 1) (while query) mark row as "to_be_deleted"
//// 2) (while query) append updated row and mark as "updated"
//// 3) (after query) iterate over pool, delete "to_be_deleted", unmark "updated"
//// 4) (after query) compress heaps
//
//#endif //LLP_LAB1_POOL_H
