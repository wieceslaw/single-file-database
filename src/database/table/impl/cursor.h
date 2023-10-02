//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_CURSOR_H
#define LLP_LAB1_CURSOR_H

#include <stdbool.h>
#include "database/table/table.h"
#include "joinset.h"

typedef enum {
    JOIN_TYPE_INNER = 0,
    JOIN_TYPE_LEFT = 1,
    JOIN_TYPE_RIGHT = 2,
} join_type;

typedef enum {
    CURSOR_TYPE_FROM = 0,
    CURSOR_TYPE_JOIN = 1,
    CURSOR_TYPE_WHERE = 2,
} cursor_type;

typedef enum {
    CURSOR_OP_OK = 0,
    CURSOR_OP_ERR = 1,
} cursor_result;

typedef struct cursor cursor_t;

typedef struct predicate predicate_t;

typedef struct join_condition join_condition_t;

typedef struct updater updater_t;

bool join_condition_check(row_set_t* left, row_set_t* right);

struct cursor {
    cursor_type type;
    union {
        struct {
            table_t *table;
            pool_it *it;
        } from;
        struct {
            cursor_t *left;
            cursor_t *right;
            join_type type;
            join_condition_t* condition;
        } join;
        struct {
            cursor_t *base;
            predicate_t *condition;
        } filter;
    };
};

bool cursor_is_empty(cursor_t *cur);

cursor_result cursor_next(cursor_t *cur);

cursor_result cursor_restart(cursor_t *cur);

row_set_t *cursor_get(cursor_t *cur);

cursor_result cursor_delete(cursor_t *cur, char *alias);

cursor_result cursor_update(cursor_t *cur, char *alias, updater_t *updater);

#endif //LLP_LAB1_CURSOR_H
