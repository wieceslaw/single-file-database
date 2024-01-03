//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "Cursor.h"

// THROWS: ?
static bool column_greater_than(Column first, Column second) {
    assert(first.type == second.type);
    assert(first.type == COLUMN_TYPE_FLOAT32 || first.type == COLUMN_TYPE_INT32);
    switch (first.type) {
        case COLUMN_TYPE_INT32:
            return first.value.i32 > second.value.i32;
        case COLUMN_TYPE_FLOAT32:
            return first.value.f32 > second.value.f32;
        default:
            assert(0);
    }
}

// THROWS: ?
static bool column_lesser_than(Column first, Column second) {
    assert(first.type == second.type);
    assert(first.type == COLUMN_TYPE_FLOAT32 || first.type == COLUMN_TYPE_INT32);
    switch (first.type) {
        case COLUMN_TYPE_INT32:
            return first.value.i32 < second.value.i32;
        case COLUMN_TYPE_FLOAT32:
            return first.value.f32 < second.value.f32;
        default:
            assert(0);
    }
}

// THROWS: ?
static bool compare_columns(comparing_type compare_type, Column first, Column second) {
    assert(first.type == second.type);
    switch (compare_type) {
        case COMPARE_EQ:
            return ColumnEquals(first, second);
        case COMPARE_NE:
            return !ColumnEquals(first, second);
        case COMPARE_GT:
            return column_greater_than(first, second);
        case COMPARE_LT:
            return column_lesser_than(first, second);
        case COMPARE_GE:
            return column_greater_than(first, second) || ColumnEquals(first, second);
        case COMPARE_LE:
            return column_lesser_than(first, second) || ColumnEquals(first, second);
        default:
            assert(0);
    }
}

// THROWS: ?
static Column operand_extract_column(operand op, Cursor cur) {
    switch (op.type) {
        case OPERAND_VALUE_LITERAL:
            return op.literal;
        case OPERAND_VALUE_COLUMN:
            return CursorGetColumn(cur, op.column.index.table_idx, op.column.index.column_idx);
        default:
            assert(0);
    }
}

// THROWS: ?
static bool where_condition_check_compare(where_condition *condition, Cursor cur) {
    assert(condition != NULL && condition->type == CONDITION_COMPARE);
    Column first_column = operand_extract_column(condition->compare.first, cur);
    Column second_column = operand_extract_column(condition->compare.second, cur);
    return compare_columns(condition->compare.type, first_column, second_column);
}

// THROWS: ?
static bool where_condition_check(where_condition *condition, Cursor cur) {
    assert(condition != NULL);
    switch (condition->type) {
        case CONDITION_AND:
            return where_condition_check(condition->and.first, cur) &&
                   where_condition_check(condition->and.second, cur);
        case CONDITION_OR:
            return where_condition_check(condition->or.first, cur) ||
                   where_condition_check(condition->or.second, cur);
        case CONDITION_NOT:
            return !where_condition_check(condition->not.first, cur);
        case CONDITION_COMPARE:
            return where_condition_check_compare(condition, cur);
        default:
            assert(0);
    }
}

// THROWS: ?
static void CursorFree_WHERE(Cursor cur) {
    where_condition_free(cur->where.condition);
    CursorFree(&(cur->where.base));
    free(cur);
}

// THROWS: ?
static bool CursorIsEmpty_WHERE(Cursor cur) {
    return CursorIsEmpty(cur->where.base);
}

// THROWS: ?
static void CursorNext_WHERE(Cursor cur) {
    Cursor base = cur->where.base;
    CursorNext(base);
    while (!CursorIsEmpty(base) && !where_condition_check(cur->where.condition, cur)) {
        CursorNext(base);
    }
}

// THROWS: ?
static void CursorRestart_WHERE(Cursor cur) {
    CursorRestart(cur->where.base);
    while (!where_condition_check(cur->where.condition, cur)) {
        CursorNext(cur->where.base);
    }
}

// THROWS: ?
static void CursorFlush_WHERE(Cursor cur) {
    CursorFlush(cur->where.base);
}

// THROWS: ?
static Column CursorGet_WHERE(Cursor cur, size_t table_idx, size_t column_idx) {
    return CursorGetColumn(cur->where.base, table_idx, column_idx);
}

// THROWS: ?
static void CursorDelete_WHERE(Cursor cur, size_t table_idx) {
    CursorDeleteRow(cur->where.base, table_idx);
}

// THROWS: ?
static void CursorUpdate_WHERE(Cursor cur, size_t table_idx, updater_builder_t updater) {
    CursorUpdateRow(cur->where.base, table_idx, updater);
}

Cursor CursorNew_WHERE(Cursor base, where_condition *condition) {
    assert(base != NULL && condition != NULL);
    Cursor cur = malloc(sizeof(struct Cursor));
    cur->type = CURSOR_WHERE;
    cur->where.condition = condition;
    cur->where.base = base;
    cur->free = CursorFree_WHERE;
    cur->is_empty = CursorIsEmpty_WHERE;
    cur->next = CursorNext_WHERE;
    cur->restart = CursorRestart_WHERE;
    cur->flush = CursorFlush_WHERE;
    cur->get = CursorGet_WHERE;
    cur->delete = CursorDelete_WHERE;
    cur->update = CursorUpdate_WHERE;
    while (!CursorIsEmpty(cur) && !where_condition_check(cur->where.condition, cur)) {
        CursorNext(cur->where.base);
    }
    return cur;
}
