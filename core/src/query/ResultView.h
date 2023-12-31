//
// Created by vyach on 12.10.2023.
//

#ifndef LLP_LAB1_RESULT_VIEW_H
#define LLP_LAB1_RESULT_VIEW_H

#include "database/table/table.h"
#include "database/cursor/Cursor.h"

typedef struct ResultView {
    table_scheme *view_scheme;
    column_description *view_selector;
    Cursor cursor;
} *ResultView;

void ResultViewFree(ResultView view);

bool ResultViewIsEmpty(ResultView view);

Row ResultViewGetRow(ResultView view);

void ResultViewNext(ResultView view);

table_scheme *ResultViewGetScheme(ResultView view);

#endif //LLP_LAB1_RESULT_VIEW_H
