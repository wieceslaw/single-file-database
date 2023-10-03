//
// Created by vyach on 01.10.2023.
//

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "table.h"

static void free_column(scheme_column_t *column) {
    assert(column != NULL);
    free(column->name);
    column->name = NULL;
}

void scheme_free(scheme_t *scheme) {
    assert(scheme != NULL);
    for (uint32_t i = 0; i < scheme->size; i++) {
        free_column(&scheme->columns[i]);
    }
    free(scheme->columns);
    scheme->columns = NULL;
    free(scheme);
}

table_result_type table_free(table_t *table) {
    assert(table != NULL);
    if (pool_free(table->data_pool) != POOL_OP_OK) {
        return TABLE_OP_ERR;
    }
    scheme_free(table->scheme);
    table->scheme = NULL;
    free(table);
    return TABLE_OP_OK;
}
