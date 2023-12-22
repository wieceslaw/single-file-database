//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include <malloc.h>
#include "SchemeBuilder.h"
#include "list/list.h"
#include "util_string.h"

struct SchemeBuilder {
    char *name;
    list_t column_list;
};

SchemeBuilder SchemeBuilderNew(char *tableName) {
    assert(tableName != NULL);
    SchemeBuilder builder = malloc(sizeof(struct SchemeBuilder));
    if (builder == NULL) {
        return NULL;
    }
    builder->column_list = list_init();
    builder->name = string_copy(tableName);
    return builder;
}

void SchemeBuilderFree(SchemeBuilder builder) {
    if (NULL == builder) {
        return;
    }
    FOR_LIST(builder->column_list, it, {
        free(list_it_get(it));
    })
    list_free(&builder->column_list);
    free(builder->name);
    free(builder);
}

int SchemeBuilderAddColumn(SchemeBuilder builder, char *name, column_type type) {
    assert(builder != NULL && name != NULL);
    table_scheme_column *column = malloc(sizeof(table_scheme_column));
    if (column == NULL) {
        return -1;
    }
    column->type = type;
    column->name = name;
    list_append_tail(builder->column_list, column);
    return 0;
}

table_scheme *SchemeBuilderBuild(SchemeBuilder builder) {
    assert(builder != NULL);
    table_scheme *scheme = malloc(sizeof(table_scheme));
    if (scheme == NULL) {
        assert(0);
    }
    scheme->size = list_size(builder->column_list);
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
        table_scheme_column *column = list_it_get(it);
        scheme->columns[i].type = column->type;
        scheme->columns[i].name = string_copy(column->name);
        i++;
    })
    return scheme;
}