//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "cursor.h"
#include "util/exceptions/exceptions.h"
#include "database/query/where_condition.h"
#include "database/query/updater_builder.h"
#include "util/string.h"

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_from(table_t table) {
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->from.table = table;
    cur->from.it = pool_iterator(table->data_pool);
    cur->type = CURSOR_FROM;
    return cur;
}

static bool join_condition_check(join_condition *condition, cursor_t cur) {
    assert(condition->right.type == COLUMN_DESC_INDEX && condition->left.type == COLUMN_DESC_INDEX);
    column right_column = cursor_get_column(cur, condition->right);
    column left_column = cursor_get_column(cur, condition->left);
    return columns_equals(right_column, left_column);
}

static bool column_greater_than(column first, column second) {
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

static bool column_lesser_than(column first, column second) {
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

static bool compare_columns(comparing_type compare_type, column first, column second) {
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

static column operand_extract_column(operand op, cursor_t cur) {
    assert(cur != NULL);
    switch (op.type) {
        case OPERAND_VALUE_LITERAL:
            return op.literal;
        case OPERAND_VALUE_COLUMN:
            return cursor_get_column(cur, op.column);
    }
}

static bool where_condition_check_compare(where_condition *condition, cursor_t cur) {
    assert(condition != NULL && cur != NULL && condition->type == CONDITION_COMPARE);
    column first_column = operand_extract_column(condition->compare.first, cur);
    column second_column = operand_extract_column(condition->compare.second, cur);
    return compare_columns(condition->compare.type, first_column, second_column);
}

static bool where_condition_check(where_condition *condition, cursor_t cur) {
    assert(condition != NULL && cur != NULL);
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

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_join(cursor_t left, cursor_t right, join_condition condition) {
    assert(left != NULL && right != NULL &&
        condition.right.type == COLUMN_DESC_INDEX && condition.left.type == COLUMN_DESC_INDEX);
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->join.condition = condition;
    cur->join.right = right;
    cur->join.left = left;
    if (!join_condition_check(&(cur->join.condition), cur)) {
        cursor_next(cur);
    }
    cur->type = CURSOR_JOIN;
    return cur;
}

// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_where(cursor_t base, where_condition *condition) {
    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->where.condition = condition;
    cur->where.base = base;
    cur->type = CURSOR_WHERE;
    while (!cursor_is_empty(cur) && !where_condition_check(cur->where.condition, cur)) {
        cursor_next(cur->where.base);
    }
    return cur;
}

bool cursor_is_empty(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            return pool_iterator_is_empty(cur->from.it);
        }
        case CURSOR_JOIN: {
            return cursor_is_empty(cur->join.left) || cursor_is_empty(cur->join.right);
        }
        case CURSOR_WHERE: {
            return cursor_is_empty(cur->where.base);
        }
    }
}

void cursor_delete(cursor_t cur, size_t table_idx) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            if (cur->from.table_idx == table_idx) {
                pool_iterator_delete(cur->from.it);
            }
            return;
        }
        case CURSOR_JOIN: {
            cursor_delete(cur->join.left, table_idx);
            cursor_delete(cur->join.right, table_idx);
            return;
        }
        case CURSOR_WHERE: {
            cursor_delete(cur->where.base, table_idx);
            return;
        }
    }
}

static row_value cursor_from_get_row(cursor_t cur) {
    assert(cur != NULL && cur->type == CURSOR_FROM);
    buffer_t buffer = pool_iterator_get(cur->from.it);
    row_value value = row_deserialize(cur->from.table->scheme, buffer);
    buffer_free(&buffer);
    return value;
}

column cursor_get_column(cursor_t cur, column_description description) {
    assert(cur != NULL && description.type == COLUMN_DESC_INDEX);
    if (cursor_is_empty(cur)) {
        return (column) {0};
    }
    switch (cur->type) {
        case CURSOR_FROM: {
            if (cur->from.table_idx == description.index.table_idx) {
                // TODO: Add caching of rows
                row_value row = cursor_from_get_row(cur);
                column_type type = cur->from.table->scheme->columns[description.index.column_idx].type;
                column_value value;
                if (type == COLUMN_TYPE_STRING) {
                    value.val_string = string_copy(row->values[description.index.column_idx].val_string);
                } else {
                    value = row->values[description.index.column_idx];
                }
                row_value_free(cur->from.table->scheme, row);
                return (column) {
                        .type = type,
                        .value = value
                };
            }
            return (column) {0};
        }
        case CURSOR_JOIN: {
            column left_column = cursor_get_column(cur->join.left, description);
            column right_column = cursor_get_column(cur->join.right, description);
            // both can't be full and both can't be empty
            if (left_column.type != 0) {
                assert(right_column.type == 0);
                return left_column;
            } else {
                assert(right_column.type != 0);
                assert(left_column.type == 0);
                return right_column;
            }
        }
        case CURSOR_WHERE: {
            return cursor_get_column(cur->where.base, description);
        }
    }
}

static void cursor_restart_from(cursor_t cur) {
    pool_iterator_free(&(cur->from.it));
    cur->from.it = pool_iterator(cur->from.table->data_pool);
}

static void cursor_restart_join(cursor_t cur) {
    cursor_restart(cur->join.left);
    cursor_restart(cur->join.right);
    if (!join_condition_check(&(cur->join.condition), cur)) {
        cursor_next(cur);
    }
}

static void cursor_restart_where(cursor_t cur) {
    cursor_restart(cur->where.base);
    while (!where_condition_check(cur->where.condition, cur)) {
        cursor_next(cur->where.base);
    }
}

void cursor_restart(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            cursor_restart_from(cur);
            return;
        }
        case CURSOR_JOIN: {
            cursor_restart_join(cur);
            return;
        }
        case CURSOR_WHERE: {
            cursor_restart_where(cur);
            return;
        }
    }
}

static void cursor_next_from(cursor_t cur) {
    pool_iterator_next(cur->from.it);
}

static void cursor_next_join(cursor_t cur) {
    if (cursor_is_empty(cur)) {
        return;
    }
    cursor_next(cur->join.right);
    if (cursor_is_empty(cur->join.right)) {
        cursor_next(cur->join.left);
        cursor_restart(cur->join.right);
    }
    while (!cursor_is_empty(cur->join.left) && !cursor_is_empty(cur->join.right)) {
        if (join_condition_check(&(cur->join.condition), cur)) {
            break;
        }
        cursor_next(cur->join.right);
        if (cursor_is_empty(cur->join.right)) {
            cursor_next(cur->join.left);
            cursor_restart(cur->join.right);
        }
    }
}

static void cursor_next_where(cursor_t cur) {
    cursor_t base = cur->where.base;
    cursor_next(base);
    while (!cursor_is_empty(base) && !where_condition_check(cur->where.condition, cur)) {
        cursor_next(base);
    }
}

void cursor_next(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            cursor_next_from(cur);
            return;
        }
        case CURSOR_JOIN: {
            cursor_next_join(cur);
            return;
        }
        case CURSOR_WHERE: {
            cursor_next_where(cur);
            return;
        }
    }
}

void cursor_update(cursor_t cur, size_t table_idx, updater_builder_t updater) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            if (cur->from.table_idx == table_idx) {
                row_value value = cursor_from_get_row(cur);
                updater_builder_update(updater, cur->from.table->scheme, value);
                buffer_t serialized = row_serialize(cur->from.table->scheme, value);
                row_value_free(cur->from.table->scheme, value);
                pool_append(cur->from.table->data_pool, serialized);
                pool_iterator_delete(cur->from.it);
                buffer_free(&serialized);
            }
            return;
        }
        case CURSOR_JOIN: {
            cursor_update(cur->join.left, table_idx, updater);
            cursor_update(cur->join.right, table_idx, updater);
            return;
        }
        case CURSOR_WHERE: {
            cursor_update(cur->where.base, table_idx, updater);
            return;
        }
    }
}

void cursor_flush(cursor_t cur) {
    assert(cur != NULL);
    switch (cur->type) {
        case CURSOR_FROM: {
            pool_flush(cur->from.table->data_pool);
            return;
        }
        case CURSOR_JOIN: {
            cursor_flush(cur->join.left);
            cursor_flush(cur->join.right);
            return;
        }
        case CURSOR_WHERE: {
            cursor_flush(cur->where.base);
            return;
        }
    }
}

void cursor_free(cursor_t cur) {
    if (NULL == cur) {
        return;
    }
    switch (cur->type) {
        case CURSOR_FROM:
            table_free(&(cur->from.table));
            pool_iterator_free(&(cur->from.it));
        case CURSOR_JOIN:
            cursor_free(cur->join.right);
            cursor_free(cur->join.left);
            break;
        case CURSOR_WHERE:
            where_condition_free(cur->where.condition);
            cursor_free(cur->where.base);
            break;
    }
    free(cur);
}