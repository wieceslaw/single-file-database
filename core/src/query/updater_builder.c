//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include "updater_builder.h"
#include "exceptions/exceptions.h"

/// THROWS: [MALLOC_EXCEPTION]
column_updater *column_updater_of(char* target, column_t new_value) {
    column_updater *updater = rmalloc(sizeof(column_updater));
    updater->new_value = new_value;
    updater->target.name = target;
    updater->translated = false;
    return updater;
}

/// THROWS: [MALLOC_EXCEPTION]
updater_builder_t updater_builder_init(void) {
    updater_builder_t updater = rmalloc(sizeof(struct updater_builder));
    updater->column_updaters = list_init();
    return updater;
}

void updater_builder_free(updater_builder_t* updater_ptr) {
    assert(updater_ptr != NULL);
    updater_builder_t updater = *updater_ptr;
    if (NULL == updater) {
        return;
    }
    list_clear(updater->column_updaters, free);
    list_free(&(updater->column_updaters));
    free(updater);
    *updater_ptr = NULL;
}

/// THROWS: [MALLOC_EXCEPTION]
void updater_builder_add(updater_builder_t updater, column_updater *col_updater) {
    list_append_tail(updater->column_updaters, col_updater);
}

/// THROWS: [MALLOC_EXCEPTION]
row_t updater_builder_update(updater_builder_t updater, row_t row) {
    row_t copy = row_copy(row);
    FOR_LIST(updater->column_updaters, it, {
        column_updater *col_updater = list_it_get(it);
        assert(col_updater->translated);
        size_t column_idx = col_updater->target.idx;
        if (copy.columns[column_idx].type == COLUMN_TYPE_STRING) {
            free(copy.columns[column_idx].value.val_string);
            copy.columns[column_idx].value.val_string = NULL;
        }
        copy.columns[column_idx] = column_copy(col_updater->new_value);
    })
    return copy;
}

/// THROWS: [DATABASE_TRANSLATION_EXCEPTION]
updater_builder_t updater_builder_translate(updater_builder_t old_updater, str_int_map_t map) {
    updater_builder_t new_updater = rmalloc(sizeof(struct updater_builder));
    new_updater->column_updaters = list_init();
    FOR_LIST(old_updater->column_updaters, it, {
        column_updater *col_upd = list_it_get(it);
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
