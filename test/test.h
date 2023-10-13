//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_TEST_H
#define LLP_LAB1_TEST_H

#include "heap/heap.h"
#include "pool/pool.h"
#include "database/database.h"

void print_heap(heap_t *heap);

void test_heap(allocator_t *allocator);

void print_pool(pool_t *pool);

void test_pool(allocator_t *allocator);

void test_map(int n);

void test_insert(database_t db, int n);

void test_select(database_t db, char* table_name);

void test_delete(database_t db, char *table_name);

#endif //LLP_LAB1_TEST_H
