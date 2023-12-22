//
// Created by vyach on 12.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "exceptions/exceptions.h"
#include "result_view.h"

void result_view_free(result_view_t *view_ptr) {
    assert(view_ptr != NULL);
    result_view_t view = *view_ptr;
    if (NULL == view) {
        return;
    }
    free(view->view_selector);
    cursor_free(&(view->cursor));
    table_scheme_free(view->view_scheme);
    free(view);
    *view_ptr = NULL;
}

bool result_view_is_empty(result_view_t view) {
    return cursor_is_empty(view->cursor);
}

row_t result_view_get(result_view_t view) {
    size_t size = view->view_scheme->size;
    row_t result;
    result.size = size;
    result.columns = rmalloc(sizeof(column_t) * size);
    for (size_t i = 0; i < size; i++) {
        column_description description = view->view_selector[i];
        column_t col = cursor_get(view->cursor, description.index.table_idx, description.index.column_idx);
        result.columns[i] = col;
    }
    return result;
}

void result_view_next(result_view_t view) {
    if (result_view_is_empty(view)) {
        return;
    }
    cursor_next(view->cursor);
}

table_scheme *result_view_scheme(result_view_t view) {
    return view->view_scheme;
}
