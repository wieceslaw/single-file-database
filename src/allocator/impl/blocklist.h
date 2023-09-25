//
// Created by vyach on 20.09.2023.
//

#ifndef LLP_LAB1_BLOCKLIST_H
#define LLP_LAB1_BLOCKLIST_H

#include <stdbool.h>

typedef struct block_list_node block_list_node;

typedef struct block_ref block_ref;

typedef struct block block;

struct block {
    void *ptr;
    offset_t file_offset;
    block_list_node *node;
};

struct block_ref {
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

#endif //LLP_LAB1_BLOCKLIST_H
