//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_CURSOR_H
#define LLP_LAB1_CURSOR_H

#include <stdbool.h>
#include "database/table/table.h"
#include "database/query/join_builder.h"
#include "database/query/where_condition.h"
#include "database/query/updater_builder.h"

typedef enum {
    CURSOR_FROM = 0,
    CURSOR_JOIN = 1,
    CURSOR_WHERE = 2,
} cursor_type;

typedef struct cursor {
    cursor_type type;
    union {
        struct {
            table_t table;
            pool_it it;
            size_t table_idx;
        } from;
        struct {
            // left_table_idx
            // left_column_idx
            // right_table_idx
            // right_column_idx
            join_condition condition;
            struct cursor *left;
            struct cursor *right;
        } join;
        struct {
            where_condition *condition;
            struct cursor *base;
        } where;
    };
} *cursor_t;

// TODO: Remake using index conditions

cursor_t cursor_init_from(table_t table);

cursor_t cursor_init_join(cursor_t left, cursor_t right, join_condition condition);

cursor_t cursor_init_where(cursor_t base, where_condition *condition);

void cursor_free(cursor_t cur);

bool cursor_is_empty(cursor_t cur);

void cursor_next(cursor_t cur);

void cursor_restart(cursor_t cur);

column cursor_get_column(cursor_t cur, column_description description);

void cursor_delete(cursor_t cur, size_t table_idx);

void cursor_update(cursor_t cur, size_t table_idx, updater_builder_t updater);

void cursor_flush(cursor_t cur);

#endif //LLP_LAB1_CURSOR_H
