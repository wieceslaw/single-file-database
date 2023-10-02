//
// Created by vyach on 01.10.2023.
//

#include <malloc.h>
#include <assert.h>
#include "buffer/buffer.h"
#include "table.h"

//static void free_column(column_t *column) {
//    free(column->name);
//    column->name = NULL;
//}
//
//static void free_schema(table_schema_t *schema) {
//    for (uint32_t i = 0; i < schema->size; i++) {
//        free_column(&schema->columns[i]);
//    }
//    free(schema->columns);
//    schema->columns = NULL;
//    free(schema);
//}
//
//void table_free(table_t *table) {
//    assert(table != NULL);
//    if (pool_free(table->data_pool) != POOL_OP_OK) {
//        return 1;
//    }
//    free_schema(table->schema);
//    table->schema = NULL;
//    free(table);
//}
