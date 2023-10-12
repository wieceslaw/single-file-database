//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include "updater_builder.h"
#include "util/exceptions/exceptions.h"
#include "util/string.h"

/// THROWS: [MALLOC_EXCEPTION]
column_updater *column_updater_of(char* target, column new_value) {
    column_updater *updater = rmalloc(sizeof(column_updater));
    updater->new_value = new_value;
    updater->target.name = target;
    updater->translated = false;
    return updater;
}

/// THROWS: [MALLOC_EXCEPTION]
updater_builder_t updater_builder_init() {
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
    FOR_LIST(updater->column_updaters, it, {
        free(list_it_get(it));
    })
    list_free(&(updater->column_updaters));
    free(updater);
    *updater_ptr = NULL;
}

/// THROWS: [MALLOC_EXCEPTION]
void updater_builder_add(updater_builder_t updater, column_updater *col_updater) {
    list_append_tail(updater->column_updaters, col_updater);
}

void updater_builder_update(updater_builder_t updater, table_scheme *scheme, row_value value) {
    FOR_LIST(updater->column_updaters, it, {
        column_updater *col_upd = list_it_get(it);
        size_t idx = col_upd->target.idx;
        assert(col_upd->translated);
        column_value new_value;
        if (scheme->columns[idx].type == COLUMN_TYPE_STRING) {
            free(value->values[col_upd->target.idx].val_string);
            new_value.val_string = string_copy(col_upd->new_value.value.val_string);
        } else {
            new_value = col_upd->new_value.value;
        }
        value->values[col_upd->target.idx] = new_value;
    })
}

updater_builder_t updater_builder_translate(updater_builder_t old_updater, str_int_map_t map) {
    updater_builder_t new_updater = rmalloc(sizeof(struct updater_builder));
    new_updater->column_updaters = list_init();
    FOR_LIST(old_updater->column_updaters, it, {
        column_updater *col_upd = list_it_get(it);
        assert(!col_upd->translated);
        int *idx = MAP_GET(map, col_upd->target.name);
        if (idx == NULL) {
            RAISE(TRANSLATION_EXCEPTION);
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
