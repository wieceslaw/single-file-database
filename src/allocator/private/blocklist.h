//
// Created by vyach on 20.09.2023.
//

#ifndef LLP_LAB1_BLOCKLIST_H
#define LLP_LAB1_BLOCKLIST_H

#define BLOCK_LIST_CAPACITY 500

#include <stdbool.h>
#include "util/map/map_impl.h"

typedef struct block_list_node block_list_node;

typedef struct page_t page_t;

typedef struct block block;

struct block {
    char *ptr;
    offset_t file_offset;
    block_list_node *node;
};

struct page_t {
    block *block;
    offset_t offset;
};

struct block_list_node {
    block *block;
    block_list_node *prev, *next;
    offset_t file_offset;
    int used;
};

typedef struct {
    block_list_node *head;
    block_list_node *tail;
    size_t size;
    uint64_void_map_t map;
} block_list;

typedef struct {
    block_list *list;
    block_list_node *node;
} block_list_it;

block_list_node *block_list_append(block_list *list, block *block);

bool block_list_delete(block_list *list, block_list_node *node);

void block_list_iterator(block_list *list, block_list_it *it);

bool block_list_iterator_is_empty(block_list_it *it);

void block_list_iterator_next(block_list_it *it);

block *block_list_iterator_get(block_list_it *it);

block_list_node *block_list_find(block_list *list, offset_t offset);

#endif //LLP_LAB1_BLOCKLIST_H
