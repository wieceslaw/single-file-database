//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_CURSOR_H
#define LLP_LAB1_CURSOR_H

#include <stdbool.h>
#include "database/table/table.h"
#include "database/query/join.h"
#include "database/query/where.h"

typedef enum {
    CURSOR_FROM = 0,
    CURSOR_JOIN = 1,
    CURSOR_WHERE = 2,
} cursor_type;

typedef struct cursor *cursor_t;

struct cursor {
    cursor_type type;
    union {
        struct {
            table_t table;
            pool_it it;
            char* alias;
        } from;
        struct {
            join_info info;
            cursor_t left;
            cursor_t right;
            struct {
                bool found;
                row_set_t val1;
                row_set_t val2;
            } left_join;
        } join;
        struct {
            cursor_t base;
            where_condition condition;
        } where;
    };
};

cursor_t cursor_init_from(table_t table, char* alias);

cursor_t cursor_init_join(cursor_t left, cursor_t right, join_info info);

cursor_t cursor_init_where(cursor_t base, where_condition condition);

bool cursor_is_empty(cursor_t cur);

void cursor_next(cursor_t cur);

void cursor_restart(cursor_t cur);

row_set_t cursor_get(cursor_t cur);

void cursor_delete(cursor_t cur, char *alias);

// TODO: Implement
//void cursor_update(cursor_t *cur, char *alias, updater *updater);

#endif //LLP_LAB1_CURSOR_H
