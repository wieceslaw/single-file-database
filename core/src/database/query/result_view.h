//
// Created by vyach on 12.10.2023.
//

#ifndef LLP_LAB1_RESULT_VIEW_H
#define LLP_LAB1_RESULT_VIEW_H

#include "database/table/table.h"
#include "database/cursor/cursor.h"

typedef struct result_view {
    table_scheme *view_scheme;
    column_description *view_selector;
    cursor_t cursor;
} *result_view_t;

void result_view_free(result_view_t *view_ptr);

bool result_view_is_empty(result_view_t view);

row_t result_view_get(result_view_t view);

void result_view_next(result_view_t view);

table_scheme *result_view_scheme(result_view_t view);

#endif //LLP_LAB1_RESULT_VIEW_H
