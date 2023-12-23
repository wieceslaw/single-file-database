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
} CursorType;

typedef struct Cursor {
    CursorType type;
    void (*free)(struct Cursor* cur);
    bool (*is_empty)(struct Cursor* cur);
    void (*next)(struct Cursor* cur);
    void (*restart)(struct Cursor* cur);
    void (*flush)(struct Cursor* cur);
    void (*delete)(struct Cursor* cur, size_t table_idx);
    void (*update)(struct Cursor* cur, size_t table_idx, updater_builder_t updater);
    Column (*get)(struct Cursor* cur, size_t table_idx, size_t column_idx);
    union {
        struct {
            table_t table;
            pool_it it;
            size_t table_idx;
            Row cached_row;
        } from;
        struct {
            join_condition condition;
            struct Cursor *left;
            struct Cursor *right;
        } join;
        struct {
            where_condition *condition;
            struct Cursor *base;
        } where;
    };
} *Cursor;

Cursor CursorNew_FROM(table_t table, size_t table_idx);

Cursor CursorNew_JOIN(Cursor left, Cursor right, join_condition condition);

Cursor CursorNew_WHERE(Cursor base, where_condition *condition);

/// THROWS: [EXCEPTION]
void CursorFree(Cursor *cur);

/// THROWS: [EXCEPTION]
bool CursorIsEmpty(Cursor cur);

/// THROWS: [EXCEPTION]
void CursorNext(Cursor cur);

/// THROWS: [EXCEPTION]
void CursorRestart(Cursor cur);

/// THROWS: [EXCEPTION]
void CursorFlush(Cursor cur);

/// THROWS: [EXCEPTION]
Column CursorGetColumn(Cursor cur, size_t table_idx, size_t column_idx);

/// THROWS: [EXCEPTION]
void CursorDeleteRow(Cursor cur, size_t table_idx);

/// THROWS: [EXCEPTION]
void CursorUpdateRow(Cursor cur, size_t table_idx, updater_builder_t updater);

#endif //LLP_LAB1_CURSOR_H
