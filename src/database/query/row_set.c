//
// Created by vyach on 06.10.2023.
//

#include <string.h>
#include "row_set.h"
#include "util/exceptions/exceptions.h"

// create set with single row
/// THROWS: [MALLOC_EXCEPTION]
row_set_t row_set_from(char *alias, row_value row, table_scheme *scheme) {
    row_set_t map = MAP_NEW_STR_ROW(1);
    table_row_t entry = rmalloc(sizeof(struct table_row));
    *entry = (struct table_row) {.data = row, .scheme = scheme};
    MAP_PUT(map, alias, entry);
    return map;
}

// Will nullify both pointers and free one of sets
row_set_t row_set_merge(row_set_t *first, row_set_t *second) {
    FOR_MAP(*second, entry, {
        MAP_PUT((*first), entry->key, entry->val);
    })
    MAP_FREE(*second);
    *second = NULL;
    row_set_t result = *first;
    *first = NULL;
    return result;
}

// Will modify set
/// THROWS: [MALLOC_EXCEPTION]
row_value row_set_extract(row_set_t set, selector_t selector) {
    return NULL;
}

column table_row_get_column(table_row_t table_row, char *column_name) {
    table_scheme *scheme = table_row->scheme;
    for (uint32_t i = 0; i < scheme->size; i++) {
        table_scheme_column col = scheme->columns[i];
        if (0 == strcmp(col.name, column_name)) {
            return (column) {.type = col.type, .value = table_row->data->values[i]};
        }
    }
    return (column) {0};
}
