//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "SchemeBuilder.h"
#include "list/List.h"
#include "util_string.h"

SchemeBuilder SchemeBuilderNew(char *tableName) {
    assert(tableName != NULL);
    SchemeBuilder builder = malloc(sizeof(struct SchemeBuilder));
    if (builder == NULL) {
        return NULL;
    }
    builder->column_list = ListNew();
    builder->name = string_copy(tableName);
    return builder;
}

void SchemeBuilderFree(SchemeBuilder builder) {
    if (NULL == builder) {
        return;
    }
    ListApply(builder->column_list, (Applier) free_column);
    ListApply(builder->column_list, free);
    ListFree(builder->column_list);
    builder->column_list = NULL;
    free(builder->name);
    free(builder);
}

int SchemeBuilderAddColumn(SchemeBuilder builder, char *name, ColumnType type) {
    assert(builder != NULL && name != NULL);
    table_scheme_column *column = malloc(sizeof(table_scheme_column));
    if (column == NULL) {
        return -1;
    }
    column->type = type;
    column->name = string_copy(name);
    ListAppendTail(builder->column_list, column);
    return 0;
}

table_scheme *SchemeBuilderBuild(SchemeBuilder builder) {
    assert(builder != NULL);
    table_scheme *scheme = malloc(sizeof(table_scheme));
    if (scheme == NULL) {
        assert(0);
    }
    scheme->size = ListSize(builder->column_list);
    scheme->pool_offset = 0;
    scheme->name = string_copy(builder->name);
    if (scheme->name == NULL) {
        free(scheme);
        assert(0);
    }
    scheme->columns = malloc(sizeof(table_scheme_column) * scheme->size);
    if (scheme->columns == NULL) {
        free(scheme->name);
        free(scheme);
        assert(0);
    }
    int i = 0;
    FOR_LIST(builder->column_list, it, {
        table_scheme_column *column = ListIteratorGet(it);
        scheme->columns[i].type = column->type;
        scheme->columns[i].name = string_copy(column->name);
        i++;
    })
    return scheme;
}
