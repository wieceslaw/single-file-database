//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include "selector_builder.h"
#include "util/list/list.h"
#include "util/exceptions/exceptions.h"
#include "database/table/table.h"

/// THROWS: [MALLOC_EXCEPTION]
selector_builder selector_builder_init() {
    selector_builder selector = rmalloc(sizeof(struct selector_builder));
    selector->columns_list = list_init();
    return selector;
}

void selector_builder_free(selector_builder *selector_ptr) {
    assert(selector_ptr != NULL);
    selector_builder selector = *selector_ptr;
    if (NULL == selector) {
        return;
    }
    FOR_LIST(selector->columns_list, it, {
        free(list_it_get(it));
    })
    list_free(&selector->columns_list);
    free(selector);
    *selector_ptr = NULL;
}

/// THROWS: [MALLOC_EXCEPTION]
void selector_builder_add(selector_builder selector, char* table_name, char* column_name) {
    column_description *column = rmalloc(sizeof(column_description));
    column->type = COLUMN_DESC_NAME;
    column->name.table_name = table_name;
    column->name.column_name = column_name;
    list_append_tail(selector->columns_list, column);
}
