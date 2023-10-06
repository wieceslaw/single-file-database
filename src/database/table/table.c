//
// Created by vyach on 01.10.2023.
//

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "table.h"
#include "util/exceptions/exceptions.h"

static void free_column(table_scheme_column *column) {
    assert(column != NULL);
    free(column->name);
    column->name = NULL;
}

void scheme_free(table_scheme *scheme) {
    assert(scheme != NULL);
    for (uint32_t i = 0; i < scheme->size; i++) {
        free_column(&scheme->columns[i]);
    }
    free(scheme->columns);
    scheme->columns = NULL;
    free(scheme);
}

/// THROWS: [POOL_EXCEPTION]
void table_free(table_t *table_ptr) {
    assert(table_ptr != NULL);
    table_t table = *table_ptr;
    if (NULL == table) {
        return;
    }
    TRY ({
        pool_free(table->data_pool);
    }) FINALLY({
        scheme_free(table->scheme);
        table->scheme = NULL;
        free(table);
        *table_ptr = NULL;
    })
}
