//
// Created by vyach on 01.10.2023.
//

#include <malloc.h>
#include <assert.h>
#include "allocator/buffer.h"
#include "table.h"

static void read_schema(schema_t *schema, buffer_t *buffer) {
    for (uint32_t i = 0; i < schema->size; i++) {
        column_type_t col_type = buffer_read_u32(buffer).ui32;
        char *col_name = buffer_read_string(buffer);
        schema->columns[i] = (column_t) {.type = col_type, .name = col_name};
    }
}

table_t *table_deserialize(buffer_t *buffer) {
    assert(buffer != NULL);
    // table[name*, schema[size, columns[type, name*]*]*]*
    buffer_reset(buffer);
    table_t *table = malloc(sizeof(table_t));
    table->name = buffer_read_string(buffer);
    table->schema = malloc(sizeof(schema_t));
    uint32_t size = buffer_read_u32(buffer).ui32;
    table->schema->size = size;
    table->schema->columns = malloc(sizeof(column_t) * size);
    read_schema(table->schema, buffer);
    return table;
}

buffer_t *table_serialize(table_t *table) {
    assert(table != NULL);
    // TODO: implement
    return NULL;
}

static void free_column(column_t *column) {
    free(column->name);
    column->name = NULL;
}

static void free_schema(schema_t *schema) {
    for (uint32_t i = 0; i < schema->size; i++) {
        free_column(&schema->columns[i]);
    }
    free(schema->columns);
    schema->columns = NULL;
    free(schema);
}

void table_free(table_t *table) {
    assert(table != NULL);
    free_schema(table->schema);
    table->schema = NULL;
    free(table);
}
