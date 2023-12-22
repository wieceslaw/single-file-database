//
// Created by vyach on 11.10.2023.
//

#include <malloc.h>
#include "SelectorBuilder.h"
#include "list/list.h"
#include "database/table/table.h"

SelectorBuilder SelectorBuilderNew(void) {
    SelectorBuilder selector = malloc(sizeof(struct SelectorBuilder));
    if (selector == NULL) {
        return NULL;
    }
    selector->columns = list_init();
    return selector;
}

void SelectorBuilderFree(SelectorBuilder selector) {
    if (NULL == selector) {
        return;
    }
    FOR_LIST(selector->columns, it, {
        free(list_it_get(it));
    })
    list_free(&selector->columns);
    free(selector);
}

int SelectorBuilderAdd(SelectorBuilder selector, char *tableName, char *columnName) {
    column_description *column = malloc(sizeof(column_description));
    if (column == NULL) {
        return -1;
    }
    column->type = COLUMN_DESC_NAME;
    column->name.table_name = tableName;
    column->name.column_name = columnName;
    list_append_tail(selector->columns, column);
    return 0;
}
