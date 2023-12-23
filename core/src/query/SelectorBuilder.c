//
// Created by vyach on 11.10.2023.
//

#include <malloc.h>
#include "SelectorBuilder.h"
#include "list/List.h"
#include "database/table/table.h"

SelectorBuilder SelectorBuilderNew(void) {
    SelectorBuilder selector = malloc(sizeof(struct SelectorBuilder));
    if (selector == NULL) {
        return NULL;
    }
    selector->columns = ListNew();
    return selector;
}

void SelectorBuilderFree(SelectorBuilder selector) {
    if (NULL == selector) {
        return;
    }
    ListApply(selector->columns, free);
    ListFree(selector->columns);
    selector->columns = NULL;
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
    ListAppendTail(selector->columns, column);
    return 0;
}
