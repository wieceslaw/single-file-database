//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "cursor.h"

static bool column_greater_than(column_t first, column_t second) {
    assert(first.type == second.type);
    assert(first.type == COLUMN_TYPE_FLOAT || first.type == COLUMN_TYPE_INT);
    switch (first.type) {
        case COLUMN_TYPE_INT:
            return first.value.val_int > second.value.val_int;
        case COLUMN_TYPE_FLOAT:
            return first.value.val_float > second.value.val_float;
    }
    return false;
}

static bool column_lesser_than(column_t first, column_t second) {
    assert(first.type == second.type);
    assert(first.type == COLUMN_TYPE_FLOAT || first.type == COLUMN_TYPE_INT);
    switch (first.type) {
        case COLUMN_TYPE_INT:
            return first.value.val_int < second.value.val_int;
        case COLUMN_TYPE_FLOAT:
            return first.value.val_float < second.value.val_float;
    }
    return false;
}

static bool compare_columns(comparing_type compare_type, column_t first, column_t second) {
    assert(first.type == second.type);
    switch (compare_type) {
        case COMPARE_EQ:
            return columns_equals(first, second);
        case COMPARE_NE:
            return !columns_equals(first, second);
        case COMPARE_GT:
            return column_greater_than(first, second);
        case COMPARE_LT:
            return column_lesser_than(first, second);
        case COMPARE_GE:
            return column_greater_than(first, second) || columns_equals(first, second);
        case COMPARE_LE:
            return column_lesser_than(first, second) || columns_equals(first, second);
    }
}

static column_t operand_extract_column(operand op, cursor_t cur) {
    switch (op.type) {
        case OPERAND_VALUE_LITERAL:
            return op.literal;
        case OPERAND_VALUE_COLUMN:
            return cursor_get(cur, op.column.index.table_idx, op.column.index.column_idx);
    }
}

static bool where_condition_check_compare(where_condition *condition, cursor_t cur) {
    assert(condition != NULL && condition->type == CONDITION_COMPARE);
    column_t first_column = operand_extract_column(condition->compare.first, cur);
    column_t second_column = operand_extract_column(condition->compare.second, cur);
    return compare_columns(condition->compare.type, first_column, second_column);
}

static bool where_condition_check(where_condition *condition, cursor_t cur) {
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
    }
}

static void cursor_free_where(cursor_t cur) {
    where_condition_free(cur->where.condition);
    cursor_free(&(cur->where.base));
    free(cur);
}

static bool cursor_is_empty_where(cursor_t cur) {
    return cursor_is_empty(cur->where.base);
}

static void cursor_next_where(cursor_t cur) {
    cursor_t base = cur->where.base;
    cursor_next(base);
    while (!cursor_is_empty(base) && !where_condition_check(cur->where.condition, cur)) {
        cursor_next(base);
    }
}

static void cursor_restart_where(cursor_t cur) {
    cursor_restart(cur->where.base);
    while (!where_condition_check(cur->where.condition, cur)) {
        cursor_next(cur->where.base);
    }
}

static void cursor_flush_where(cursor_t cur) {
    cursor_flush(cur->where.base);
}

static column_t cursor_get_where(cursor_t cur, size_t table_idx, size_t column_idx) {
    return cursor_get(cur->where.base, table_idx, column_idx);
}

static void cursor_delete_where(cursor_t cur, size_t table_idx) {
    cursor_delete(cur->where.base, table_idx);
}

static void cursor_update_where(cursor_t cur, size_t table_idx, updater_builder_t updater) {
    cursor_update(cur->where.base, table_idx, updater);
}

/// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_where(cursor_t base, where_condition *condition) {
    assert(base != NULL && condition != NULL);

    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->type = CURSOR_WHERE;
    cur->where.condition = condition;
    cur->where.base = base;
    cur->free = cursor_free_where;
    cur->is_empty = cursor_is_empty_where;
    cur->next = cursor_next_where;
    cur->restart = cursor_restart_where;
    cur->flush = cursor_flush_where;
    cur->get = cursor_get_where;
    cur->delete = cursor_delete_where;
    cur->update = cursor_update_where;

    while (!cursor_is_empty(cur) && !where_condition_check(cur->where.condition, cur)) {
        cursor_next(cur->where.base);
    }
    return cur;
}
