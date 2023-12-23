//
// Created by vyach on 02.10.2023.
//

#include <assert.h>
#include "Cursor.h"

void CursorFree(Cursor *pCur) {
    assert(pCur != NULL);
    Cursor cur = *pCur;
    if (NULL == cur) {
        return;
    }
    cur->free(cur);
    *pCur = NULL;
}

bool CursorIsEmpty(Cursor cur) {
    assert(cur != NULL);
    return cur->is_empty(cur);
}

void CursorNext(Cursor cur) {
    assert(cur != NULL);
    cur->next(cur);
}

void CursorRestart(Cursor cur) {
    assert(cur != NULL);
    cur->restart(cur);
}

void CursorFlush(Cursor cur) {
    assert(cur != NULL);
    cur->flush(cur);
}

Column CursorGetColumn(Cursor cur, size_t table_idx, size_t column_idx) {
    assert(cur != NULL);
    assert(!CursorIsEmpty(cur));
    return cur->get(cur, table_idx, column_idx);
}

void CursorDeleteRow(Cursor cur, size_t table_idx) {
    assert(cur != NULL);
    cur->delete(cur, table_idx);
}

void CursorUpdateRow(Cursor cur, size_t table_idx, updater_builder_t updater) {
    assert(cur != NULL);
    cur->update(cur, table_idx, updater);
}
