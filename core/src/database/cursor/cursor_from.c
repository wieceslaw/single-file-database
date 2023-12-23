//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "cursor.h"
#include "util_string.h"

static Row cursor_get_row_from(cursor_t cur) {
    if (NULL != cur->from.cached_row.columns) {
        return cur->from.cached_row;
    }
    buffer_t buffer = pool_iterator_get(cur->from.it);
    Row row = row_deserialize(cur->from.table->scheme, buffer);
    buffer_free(&buffer);
    cur->from.cached_row = row;
    return row;
}

static void cursor_free_cached_row(cursor_t cur) {
    if (NULL == cur->from.cached_row.columns) {
        return;
    }
    RowFree(cur->from.cached_row);
    cur->from.cached_row.size = 0;
    cur->from.cached_row.columns = NULL;
}

static void cursor_free_from(cursor_t cur) {
    cursor_free_cached_row(cur);
    table_free(&(cur->from.table));
    pool_iterator_free(&(cur->from.it));
    free(cur);
}

static bool cursor_is_empty_from(cursor_t cur) {
    return pool_iterator_is_empty(cur->from.it);
}

static void cursor_next_from(cursor_t cur) {
    if (cursor_is_empty(cur)) {
        return;
    }
    cursor_free_cached_row(cur);
    if (pool_iterator_next(cur->from.it) != 0) {
        RAISE(POOL_EXCEPTION);
    }
}

static void cursor_restart_from(cursor_t cur) {
    pool_iterator_free(&(cur->from.it));
    cur->from.it = pool_iterator(cur->from.table->data_pool);
    cursor_free_cached_row(cur);
}

static void cursor_flush_from(cursor_t cur) {
    cursor_free_cached_row(cur);
    if (pool_flush(cur->from.table->data_pool) != 0) {
        RAISE(POOL_EXCEPTION);
    }
}

static Column cursor_get_from(cursor_t cur, size_t table_idx, size_t column_idx) {
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

static void cursor_delete_from(cursor_t cur, size_t table_idx) {
    if (cur->from.table_idx == table_idx) {
        if (pool_iterator_delete(cur->from.it) != 0) {
            RAISE(POOL_EXCEPTION);
        }
    }
}

static void cursor_update_from(cursor_t cur, size_t table_idx, updater_builder_t updater) {
    if (cur->from.table_idx == table_idx) {
        Row row = cursor_get_row_from(cur);
        Row updated_row = updater_builder_update(updater, row);
        buffer_t serialized = RowSerialize(updated_row);
        if (pool_append(cur->from.table->data_pool, serialized) != 0) {
            RAISE(POOL_EXCEPTION);
        }
        if (pool_iterator_delete(cur->from.it) != 0) {
            RAISE(POOL_EXCEPTION);
        }
        RowFree(updated_row);
        buffer_free(&serialized);
    }
}

/// THROWS: [MALLOC_EXCEPTION, POOL_EXCEPTION]
cursor_t cursor_init_from(table_t table, size_t table_idx) {
    assert(table != NULL);
    cursor_t cur = NULL;
    TRY({
        cur = rmalloc(sizeof(struct cursor));
        cur->type = CURSOR_FROM;
        cur->from.table_idx = table_idx;
        cur->from.table = table;
        cur->from.it = pool_iterator(table->data_pool);
        cur->from.cached_row = (Row) {0};
    }) CATCH(exception >= EXCEPTION, {
        free(cur);
        RAISE(exception);
    }) FINALLY()

    cur->free = cursor_free_from;
    cur->is_empty = cursor_is_empty_from;
    cur->next = cursor_next_from;
    cur->restart = cursor_restart_from;
    cur->flush = cursor_flush_from;
    cur->get = cursor_get_from;
    cur->delete = cursor_delete_from;
    cur->update = cursor_update_from;
    return cur;
}


