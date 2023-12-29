//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "Cursor.h"
#include "util_string.h"
#include "exceptions/exceptions.h"

static Row cursor_get_row_from(Cursor cur) {
    if (NULL != cur->from.cached_row.columns) {
        return cur->from.cached_row;
    }
    Buffer buffer = pool_iterator_get(cur->from.it);
    Row row = row_deserialize(cur->from.table->scheme, buffer);
    BufferFree(&buffer);
    cur->from.cached_row = row;
    return row;
}

static void cursor_free_cached_row(Cursor cur) {
    if (NULL == cur->from.cached_row.columns) {
        return;
    }
    RowFree(cur->from.cached_row);
    cur->from.cached_row.size = 0;
    cur->from.cached_row.columns = NULL;
}

static void CursorFree_FROM(Cursor cur) {
    cursor_free_cached_row(cur);
    table_free(&(cur->from.table));
    pool_iterator_free(&(cur->from.it));
    free(cur);
}

static bool CursorIsEmpty_FROM(Cursor cur) {
    return pool_iterator_is_empty(cur->from.it);
}

// THROWS:
static void CursorNext_FROM(Cursor cur) {
    if (CursorIsEmpty(cur)) {
        return;
    }
    cursor_free_cached_row(cur);
    if (pool_iterator_next(cur->from.it) != 0) {
        RAISE(POOL_EXCEPTION);
    }
}

static void CursorRestart_FROM(Cursor cur) {
    pool_iterator_free(&(cur->from.it));
    cur->from.it = pool_iterator(cur->from.table->data_pool);
    cursor_free_cached_row(cur);
}

// THROWS:
static void CursorFlush_FROM(Cursor cur) {
    cursor_free_cached_row(cur);
    if (pool_flush(cur->from.table->data_pool) != 0) {
        RAISE(POOL_EXCEPTION);
    }
}

static Column CursorGet_FROM(Cursor cur, size_t table_idx, size_t column_idx) {
    if (cur->from.table_idx == table_idx) {
        Row row = cursor_get_row_from(cur);
        Column column = row.columns[column_idx];
        ColumnType type = column.type;
        ColumnValue value;
        if (type == COLUMN_TYPE_STRING) {
            value.str = string_copy(column.value.str);
        } else {
            value = column.value;
        }
        return (Column) {
                .type = type,
                .value = value
        };
    }
    return (Column) {0};
}

// THROWS:
static void CursorDelete_FROM(Cursor cur, size_t table_idx) {
    if (cur->from.table_idx == table_idx) {
        if (pool_iterator_delete(cur->from.it) != 0) {
            RAISE(POOL_EXCEPTION);
        }
    }
}

// THROWS:
static void CursorUpdate_FROM(Cursor cur, size_t table_idx, updater_builder_t updater) {
    if (cur->from.table_idx == table_idx) {
        Row row = cursor_get_row_from(cur);
        Row updated_row = updater_builder_update(updater, row);
        Buffer serialized = RowSerialize(updated_row);
        if (pool_append(cur->from.table->data_pool, serialized) != 0) {
            RAISE(POOL_EXCEPTION);
        }
        if (pool_iterator_delete(cur->from.it) != 0) {
            RAISE(POOL_EXCEPTION);
        }
        RowFree(updated_row);
        BufferFree(&serialized);
    }
}

Cursor CursorNew_FROM(table_t table, size_t table_idx) {
    assert(table != NULL);
    Cursor cur = malloc(sizeof(struct Cursor));
    if (cur == NULL) {
        return NULL;
    }
    cur->from.it = pool_iterator(table->data_pool);
    if (cur->from.it == NULL) {
        free(cur);
        return NULL;
    }
    cur->type = CURSOR_FROM;
    cur->from.table_idx = table_idx;
    cur->from.table = table;
    cur->from.cached_row = (Row) {0};
    cur->free = CursorFree_FROM;
    cur->is_empty = CursorIsEmpty_FROM;
    cur->next = CursorNext_FROM;
    cur->restart = CursorRestart_FROM;
    cur->flush = CursorFlush_FROM;
    cur->get = CursorGet_FROM;
    cur->delete = CursorDelete_FROM;
    cur->update = CursorUpdate_FROM;
    return cur;
}
