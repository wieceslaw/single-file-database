//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "Cursor.h"

// THROWS: ?
static bool join_condition_check(join_condition *condition, Cursor cur) {
    assert(condition->right.type == COLUMN_DESC_INDEX && condition->left.type == COLUMN_DESC_INDEX);
    Column right_column = CursorGetColumn(cur->join.right, condition->right.index.table_idx,
                                          condition->right.index.column_idx);
    Column left_column = CursorGetColumn(cur->join.left, condition->left.index.table_idx,
                                         condition->left.index.column_idx);
    return columns_equals(right_column, left_column);
}

// THROWS: ?
static void CursorFree_JOIN(Cursor cur) {
    CursorFree(&(cur->join.right));
    CursorFree(&(cur->join.left));
    free(cur);
}

// THROWS: ?
static bool CursorIsEmpty_JOIN(Cursor cur) {
    return CursorIsEmpty(cur->join.left) || CursorIsEmpty(cur->join.right);
}

// THROWS:
static void CursorNext_JOIN(Cursor cur) {
    if (CursorIsEmpty(cur)) {
        return;
    }
    CursorNext(cur->join.right);
    if (CursorIsEmpty(cur->join.right)) {
        CursorNext(cur->join.left);
        CursorRestart(cur->join.right);
    }
    while (!CursorIsEmpty(cur->join.left) && !CursorIsEmpty(cur->join.right)) {
        if (join_condition_check(&(cur->join.condition), cur)) {
            break;
        }
        CursorNext(cur->join.right);
        if (CursorIsEmpty(cur->join.right)) {
            CursorNext(cur->join.left);
            CursorRestart(cur->join.right);
        }
    }
}

// THROWS: ?
static void CursorRestart_JOIN(Cursor cur) {
    CursorRestart(cur->join.left);
    CursorRestart(cur->join.right);
    if (!join_condition_check(&(cur->join.condition), cur)) {
        CursorNext(cur);
    }
}

// THROWS: ?
static void CursorFlush_JOIN(Cursor cur) {
    CursorFlush(cur->join.left);
    CursorFlush(cur->join.right);
}

// THROWS: ?
static Column CursorGet_JOIN(Cursor cur, size_t table_idx, size_t column_idx) {
    Column left_column = CursorGetColumn(cur->join.left, table_idx, column_idx);
    Column right_column = CursorGetColumn(cur->join.right, table_idx, column_idx);
    if (left_column.type != 0) {
        assert(right_column.type == 0);
        return left_column;
    } else {
        assert(right_column.type != 0);
        assert(left_column.type == 0);
        return right_column;
    }
}

// THROWS: ?
static void CursorDelete_JOIN(Cursor cur, size_t table_idx) {
    CursorDeleteRow(cur->join.left, table_idx);
    CursorDeleteRow(cur->join.right, table_idx);
}

// THROWS: ?
static void CursorUpdate_JOIN(Cursor cur, size_t table_idx, updater_builder_t updater) {
    CursorUpdateRow(cur->join.left, table_idx, updater);
    CursorUpdateRow(cur->join.right, table_idx, updater);
}

Cursor CursorNew_JOIN(Cursor left, Cursor right, join_condition condition) {
    assert(left != NULL && right != NULL && condition.right.type == COLUMN_DESC_INDEX && condition.left.type == COLUMN_DESC_INDEX);
    Cursor cur = malloc(sizeof(struct Cursor));
    if (cur == NULL) {
        return NULL;
    }
    cur->type = CURSOR_JOIN;
    cur->join.condition = condition;
    cur->join.right = right;
    cur->join.left = left;
    cur->free = CursorFree_JOIN;
    cur->is_empty = CursorIsEmpty_JOIN;
    cur->next = CursorNext_JOIN;
    cur->restart = CursorRestart_JOIN;
    cur->flush = CursorFlush_JOIN;
    cur->get = CursorGet_JOIN;
    cur->delete = CursorDelete_JOIN;
    cur->update = CursorUpdate_JOIN;
    if (!CursorIsEmpty(cur) && !join_condition_check(&(cur->join.condition), cur)) {
        CursorNext(cur);
    }
    return cur;
}
