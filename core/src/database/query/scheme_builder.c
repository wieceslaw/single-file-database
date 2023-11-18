//
// Created by vyach on 11.10.2023.
//

#include <assert.h>
#include "scheme_builder.h"
#include "list/list.h"
#include "exceptions/exceptions.h"
#include "util_string.h"

struct scheme_builder {
    char *name;
    list_t column_list;
};

/// THROWS: [MALLOC_EXCEPTION]
scheme_builder_t scheme_builder_init(char *name) {
    assert(name != NULL);
    scheme_builder_t builder = rmalloc(sizeof(struct scheme_builder));
    builder->column_list = list_init();
    builder->name = string_copy(name);
    return builder;
}

void scheme_builder_free(scheme_builder_t *builder_ptr) {
    assert(builder_ptr != NULL);
    scheme_builder_t builder = *builder_ptr;
    if (NULL == builder) {
        return;
    }
    FOR_LIST(builder->column_list, it, {
        free(list_it_get(it));
    })
    list_free(&builder->column_list);
    free(builder->name);
    free(builder);
    *builder_ptr = NULL;
}

/// THROWS: [MALLOC_EXCEPTION]
void scheme_builder_add_column(scheme_builder_t builder, char *name, column_type type) {
    assert(builder != NULL && name != NULL);
    table_scheme_column *column = rmalloc(sizeof(table_scheme_column));
    column->type = type;
    column->name = name;
    list_append_tail(builder->column_list, column);
}

/// THROWS: [MALLOC_EXCEPTION]
table_scheme *scheme_builder_build(scheme_builder_t builder) {
    assert(builder != NULL);
    table_scheme *scheme = rmalloc(sizeof(table_scheme));
    scheme->size = list_size(builder->column_list);
    scheme->pool_offset = 0;
    scheme->name = string_copy(builder->name);
    scheme->columns = rmalloc(sizeof(table_scheme_column) * scheme->size);
    int i = 0;
    FOR_LIST(builder->column_list, it, {
        table_scheme_column *column = list_it_get(it);
        scheme->columns[i].type = column->type;
        scheme->columns[i].name = string_copy(column->name);
        i++;
    })
    return scheme;
}
