//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_CURSOR_H
#define LLP_LAB1_CURSOR_H

#include <stdbool.h>
#include "database/table/table.h"
#include "query/updater_builder.h"
#include "query/where_condition.h"
#include "query/JoinBuilder.h"

typedef enum {
    CURSOR_FROM = 0,
    CURSOR_JOIN = 1,
    CURSOR_WHERE = 2,
} cursor_type;

typedef struct cursor {
    cursor_type type;
    void (*free)(struct cursor* cur);
    bool (*is_empty)(struct cursor* cur);
    void (*next)(struct cursor* cur);
    void (*restart)(struct cursor* cur);
    void (*flush)(struct cursor* cur);
    void (*delete)(struct cursor* cur, size_t table_idx);
    void (*update)(struct cursor* cur, size_t table_idx, updater_builder_t updater);
    column_t (*get)(struct cursor* cur, size_t table_idx, size_t column_idx);
    union {
        struct {
            table_t table;
            pool_it it;
            size_t table_idx;
            row_t cached_row;
        } from;
        struct {
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

/// THROWS: [EXCEPTION]
cursor_t cursor_init_from(table_t table, size_t table_idx);

/// THROWS: [EXCEPTION]
cursor_t cursor_init_join(cursor_t left, cursor_t right, join_condition condition);

/// THROWS: [EXCEPTION]
cursor_t cursor_init_where(cursor_t base, where_condition *condition);

/// THROWS: [EXCEPTION]
void cursor_free(cursor_t *cur);

/// THROWS: [EXCEPTION]
bool cursor_is_empty(cursor_t cur);

/// THROWS: [EXCEPTION]
void cursor_next(cursor_t cur);

/// THROWS: [EXCEPTION]
void cursor_restart(cursor_t cur);

/// THROWS: [EXCEPTION]
void cursor_flush(cursor_t cur);

/// THROWS: [EXCEPTION]
column_t cursor_get(cursor_t cur, size_t table_idx, size_t column_idx);

/// THROWS: [EXCEPTION]
void cursor_delete(cursor_t cur, size_t table_idx);

/// THROWS: [EXCEPTION]
void cursor_update(cursor_t cur, size_t table_idx, updater_builder_t updater);

#endif //LLP_LAB1_CURSOR_H
