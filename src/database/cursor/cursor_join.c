//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "cursor.h"

static bool join_condition_check(join_condition *condition, cursor_t cur) {
    assert(condition->right.type == COLUMN_DESC_INDEX && condition->left.type == COLUMN_DESC_INDEX);
    column_t right_column = cursor_get(cur->join.right, condition->right.index.table_idx, condition->right.index.column_idx);
    column_t left_column = cursor_get(cur->join.left, condition->left.index.table_idx, condition->left.index.column_idx);
    return columns_equals(right_column, left_column);
}

static void cursor_free_join(cursor_t cur) {
    cursor_free(&(cur->join.right));
    cursor_free(&(cur->join.left));
    free(cur);
}

static bool cursor_is_empty_join(cursor_t cur) {
    return cursor_is_empty(cur->join.left) || cursor_is_empty(cur->join.right);
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

static void cursor_restart_join(cursor_t cur) {
    cursor_restart(cur->join.left);
    cursor_restart(cur->join.right);
    if (!join_condition_check(&(cur->join.condition), cur)) {
        cursor_next(cur);
    }
}

static void cursor_flush_join(cursor_t cur) {
    cursor_flush(cur->join.left);
    cursor_flush(cur->join.right);
}

static column_t cursor_get_join(cursor_t cur, size_t table_idx, size_t column_idx) {
    column_t left_column = cursor_get(cur->join.left, table_idx, column_idx);
    column_t right_column = cursor_get(cur->join.right, table_idx, column_idx);
    if (left_column.type != 0) {
        assert(right_column.type == 0);
        return left_column;
    } else {
        assert(right_column.type != 0);
        assert(left_column.type == 0);
        return right_column;
    }
}

static void cursor_delete_join(cursor_t cur, size_t table_idx) {
    cursor_delete(cur->join.left, table_idx);
    cursor_delete(cur->join.right, table_idx);
}

static void cursor_update_join(cursor_t cur, size_t table_idx, updater_builder_t updater) {
    cursor_update(cur->join.left, table_idx, updater);
    cursor_update(cur->join.right, table_idx, updater);
}

/// THROWS: [MALLOC_EXCEPTION]
cursor_t cursor_init_join(cursor_t left, cursor_t right, join_condition condition) {
    assert(left != NULL && right != NULL &&
           condition.right.type == COLUMN_DESC_INDEX &&
           condition.left.type == COLUMN_DESC_INDEX);

    cursor_t cur = rmalloc(sizeof(struct cursor));
    cur->type = CURSOR_JOIN;
    cur->join.condition = condition;
    cur->join.right = right;
    cur->join.left = left;
    cur->free = cursor_free_join;
    cur->is_empty = cursor_is_empty_join;
    cur->next = cursor_next_join;
    cur->restart = cursor_restart_join;
    cur->flush = cursor_flush_join;
    cur->get = cursor_get_join;
    cur->delete = cursor_delete_join;
    cur->update = cursor_update_join;
    if (!cursor_is_empty(cur) && !join_condition_check(&(cur->join.condition), cur)) {
        cursor_next(cur);
    }
    return cur;
}
