////
//// Created by vyach on 12.09.2023.
////
//
//#ifndef LLP_LAB1_HEAP_H
//#define LLP_LAB1_HEAP_H
//
//#include <stdbool.h>
//#include <stddef.h>
//#include "../allocator/buffer.h"
//#include "../allocator/allocator.h"
//
//// data structure for storing fixed-size records
//typedef struct heap heap;
//
//typedef struct heap_it heap_it;
//
//typedef enum {
//    HEAP_OP_SUCCESS = 0,
//    HEAP_OP_ERROR = 1
//} heap_result;
//
////size_t heap_size();
//
//heap *heap_init(block_addr addr);
//
//void heap_free(heap *heap);
//
//heap_result heap_compress(heap_it *heap);
//
//heap_result heap_append(heap *heap, buffer *data);
//
//heap_result heap_iterator(heap *heap, heap_it *it);
//
//heap_result heap_iterator_free(heap_it *it);
//
//bool heap_iterator_is_empty(heap_it *it);
//
//heap_result heap_iterator_next(heap_it *it);
//
//heap_result heap_iterator_get(heap_it *it, buffer *data);
//
//heap_result heap_iterator_delete(heap_it *it); // should move objects? should return back excess blocks
//
//heap_result heap_iterator_replace(heap_it *it, buffer *data);
//
//#endif //LLP_LAB1_HEAP_H
