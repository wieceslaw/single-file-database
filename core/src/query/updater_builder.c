//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include "updater_builder.h"
#include "exceptions/exceptions.h"

/// THROWS: [MALLOC_EXCEPTION]
column_updater *column_updater_of(char* target, Column new_value) {
    column_updater *updater = rmalloc(sizeof(column_updater));
    updater->new_value = new_value;
    updater->target.name = target;
    updater->translated = false;
    return updater;
}

/// THROWS: [MALLOC_EXCEPTION]
updater_builder_t updater_builder_init(void) {
    updater_builder_t updater = rmalloc(sizeof(struct updater_builder));
    updater->column_updaters = ListNew();
    return updater;
}

void updater_builder_free(updater_builder_t* updater_ptr) {
    assert(updater_ptr != NULL);
    updater_builder_t updater = *updater_ptr;
    if (NULL == updater) {
        return;
    }
    ListApply(updater->column_updaters, free);
    ListFree(updater->column_updaters);
    updater->column_updaters = NULL;
    free(updater);
    *updater_ptr = NULL;
}

/// THROWS: [MALLOC_EXCEPTION]
void updater_builder_add(updater_builder_t updater, column_updater *col_updater) {
    ListAppendTail(updater->column_updaters, col_updater);
}

/// THROWS: [MALLOC_EXCEPTION]
Row updater_builder_update(updater_builder_t updater, Row row) {
    Row copy = RowCopy(row);
    FOR_LIST(updater->column_updaters, it, {
        column_updater *col_updater = ListIteratorGet(it);
        assert(col_updater->translated);
        size_t column_idx = col_updater->target.idx;
        if (copy.columns[column_idx].type == COLUMN_TYPE_STRING) {
            free(copy.columns[column_idx].value.str);
            copy.columns[column_idx].value.str = NULL;
        }
        copy.columns[column_idx] = ColumnCopy(col_updater->new_value);
    })
    return copy;
}

/// THROWS: [DATABASE_TRANSLATION_EXCEPTION]
updater_builder_t updater_builder_translate(updater_builder_t old_updater, str_int_map_t map) {
    updater_builder_t new_updater = rmalloc(sizeof(struct updater_builder));
    new_updater->column_updaters = ListNew();
    FOR_LIST(old_updater->column_updaters, it, {
        column_updater *col_upd = ListIteratorGet(it);
        assert(!col_upd->translated);
        int *idx = MAP_GET(map, col_upd->target.name);
        if (idx == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        column_updater *updater = rmalloc(sizeof(column_updater));
        updater->new_value = col_upd->new_value;
        updater->target.idx = *idx;
        free(idx);
        updater->translated = true;
        updater_builder_add(new_updater, updater);
    })
    return new_updater;
}
