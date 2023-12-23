//
// Created by vyach on 12.10.2023.
//

#include <malloc.h>
#include "exceptions/exceptions.h"
#include "ResultView.h"

void ResultViewFree(ResultView view) {
    if (NULL == view) {
        return;
    }
    free(view->view_selector);
    CursorFree(&(view->cursor));
    table_scheme_free(view->view_scheme);
    free(view);
}

bool ResultViewIsEmpty(ResultView view) {
    return CursorIsEmpty(view->cursor);
}

Row ResultViewGetRow(ResultView view) {
    size_t size = view->view_scheme->size;
    Row result;
    result.size = size;
    result.columns = rmalloc(sizeof(Column) * size);
    for (size_t i = 0; i < size; i++) {
        column_description description = view->view_selector[i];
        Column col = CursorGetColumn(view->cursor, description.index.table_idx, description.index.column_idx);
        result.columns[i] = col;
    }
    return result;
}

void ResultViewNext(ResultView view) {
    if (ResultViewIsEmpty(view)) {
        return;
    }
    CursorNext(view->cursor);
}

table_scheme *ResultViewGetScheme(ResultView view) {
    return view->view_scheme;
}
