//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include "cursor.h"

void cursor_free(cursor_t *cur_ptr) {
    assert(cur_ptr != NULL);
    cursor_t cur = *cur_ptr;
    if (NULL == cur) {
        return;
    }
    cur->free(cur);
    *cur_ptr = NULL;
}

bool cursor_is_empty(cursor_t cur) {
    assert(cur != NULL);
    return cur->is_empty(cur);
}

void cursor_next(cursor_t cur) {
    assert(cur != NULL);
    cur->next(cur);
}

void cursor_restart(cursor_t cur) {
    assert(cur != NULL);
    cur->restart(cur);
}

void cursor_flush(cursor_t cur) {
    assert(cur != NULL);
    cur->flush(cur);
}

column_t cursor_get(cursor_t cur, size_t table_idx, size_t column_idx) {
    assert(cur != NULL);
    assert(!cursor_is_empty(cur));
    return cur->get(cur, table_idx, column_idx);
}

void cursor_delete(cursor_t cur, size_t table_idx) {
    assert(cur != NULL);
    cur->delete(cur, table_idx);
}

void cursor_update(cursor_t cur, size_t table_idx, updater_builder_t updater) {
    assert(cur != NULL);
    cur->update(cur, table_idx, updater);
}
