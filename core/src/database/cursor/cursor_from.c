//
// Created by vyach on 13.10.2023.
//

#include <assert.h>
#include "cursor.h"
#include "util_string.h"

static row_t cursor_get_row_from(cursor_t cur) {
    if (NULL != cur->from.cached_row.columns) {
        return cur->from.cached_row;
    }
    buffer_t buffer = pool_iterator_get(cur->from.it);
    row_t row = row_deserialize(cur->from.table->scheme, buffer);
    buffer_free(&buffer);
    cur->from.cached_row = row;
    return row;
}

static void cursor_free_cached_row(cursor_t cur) {
    if (NULL == cur->from.cached_row.columns) {
        return;
    }
    row_free(cur->from.cached_row);
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
    pool_iterator_next(cur->from.it);
}

static void cursor_restart_from(cursor_t cur) {
    pool_iterator_free(&(cur->from.it));
    cur->from.it = pool_iterator(cur->from.table->data_pool);
    cursor_free_cached_row(cur);
}

static void cursor_flush_from(cursor_t cur) {
    cursor_free_cached_row(cur);
    pool_flush(cur->from.table->data_pool);
}

static column_t cursor_get_from(cursor_t cur, size_t table_idx, size_t column_idx) {
    if (cur->from.table_idx == table_idx) {
        row_t row = cursor_get_row_from(cur);
        column_t column = row.columns[column_idx];
        column_type type = column.type;
        column_value value;
        if (type == COLUMN_TYPE_STRING) {
            value.val_string = string_copy(column.value.val_string);
        } else {
            value = column.value;
        }
        return (column_t) {
                .type = type,
                .value = value
        };
    }
    return (column_t) {0};
}

static void cursor_delete_from(cursor_t cur, size_t table_idx) {
    if (cur->from.table_idx == table_idx) {
        pool_iterator_delete(cur->from.it);
    }
}

static void cursor_update_from(cursor_t cur, size_t table_idx, updater_builder_t updater) {
    if (cur->from.table_idx == table_idx) {
        row_t row = cursor_get_row_from(cur);
        row_t updated_row = updater_builder_update(updater, row);
        buffer_t serialized = row_serialize(updated_row);
        pool_append(cur->from.table->data_pool, serialized);
        pool_iterator_delete(cur->from.it);
        row_free(updated_row);
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
        cur->from.cached_row = (row_t) {0};
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


