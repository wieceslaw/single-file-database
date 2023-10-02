//
// Created by vyach on 01.10.2023.
//

#include <malloc.h>
#include <assert.h>
#include "buffer/buffer.h"
#include "table.h"

static void free_column(column_t *column) {
    free(column->name);
    column->name = NULL;
}

void schema_free(table_schema_t *schema) {
    assert(schema != NULL);
    for (uint32_t i = 0; i < schema->size; i++) {
        free_column(&schema->columns[i]);
    }
    free(schema->columns);
    schema->columns = NULL;
    free(schema);
}

table_result table_free(table_t *table) {
    assert(table != NULL);
    if (pool_free(table->data_pool) != POOL_OP_OK) {
        return TABLE_OP_ERR;
    }
    schema_free(table->schema);
    table->schema = NULL;
    free(table);
    return TABLE_OP_OK;
}
