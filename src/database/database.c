//
// Created by vyach on 02.10.2023.
//

#include <stddef.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "allocator/allocator.h"
#include "pool/pool.h"
#include "table/table.h"
#include "database.h"

struct database {
    allocator_t *allocator;
    pool_t *table_pool;
};

static void read_columns(table_schema_t *schema, buffer_t *buffer) {
    for (uint32_t i = 0; i < schema->size; i++) {
        column_type_t col_type = buffer_read_u32(buffer).ui32;
        char *col_name = buffer_read_string(buffer);
        schema->columns[i] = (column_t) {.type = col_type, .name = col_name};
    }
}

static table_schema_t *table_schema_deserialize(buffer_t *buffer) {
    assert(buffer != NULL);
    // table[offset_t pool_offset, name*, schema[size, columns[type, name*]*]*]*
    buffer_reset(buffer);
    table_schema_t *schema = malloc(sizeof(table_schema_t));
    if (NULL == schema) {
        return NULL;
    }
    schema->pool_offset = buffer_read_u64(buffer).ui64;
    schema->name = buffer_read_string(buffer);
    schema->size = buffer_read_u32(buffer).ui32;
    schema->columns = malloc(sizeof(column_t) * schema->size);
    read_columns(schema, buffer);
    return schema;
}

static uint64_t table_schema_length(const table_schema_t *const table_schema) {
    assert(table_schema != NULL);
    uint64_t size = sizeof(table_schema->pool_offset) + (strlen(table_schema->name) + 1) + sizeof(table_schema->size);
    for (uint32_t i = 0; i < table_schema->size; i++) {
        column_t col = table_schema->columns[size];
        size += (strlen(col.name) + 1) + sizeof(col.type);
    }
    return size;
}

static buffer_t *table_serialize(const table_schema_t *const table_schema) {
    assert(table_schema != NULL);
    uint64_t size = table_schema_length(table_schema);
    buffer_t *buffer = buffer_init(size);
    buffer_reset(buffer);
    if (NULL == buffer) {
        return NULL;
    }
    buffer_write_u64(buffer, (b64_t) {.ui64 = table_schema->pool_offset});
    buffer_write_string(buffer, table_schema->name);
    buffer_write_u32(buffer, (b32_t) {.ui32 = table_schema->size});
    for (uint32_t i = 0; i < table_schema->size; i++) {
        column_t col = table_schema->columns[i];
        buffer_write_u32(buffer, (b32_t) {.i32 = col.type});
        buffer_write_string(buffer, col.name);
    }
    return buffer;
}

database_t *database_init(file_settings *settings) {
    database_t *database = malloc(sizeof(database_t));
    if (NULL == database) {
        return NULL;
    }
    allocator_t *allocator;
    if (allocator_init(settings, &allocator) != FILE_ST_OK) {
        free(allocator);
        free(database);
        return NULL;
    }
    if (settings->open_type == FILE_OPEN_CLEAR || settings->open_type == FILE_OPEN_CREATE) {
        offset_t offset = pool_create(allocator);
        if (0 == offset) {
            allocator_free(allocator);
            free(database);
            return NULL;
        }
        allocator_set_entrypoint(allocator, offset);
    }
    pool_t *table_pool = pool_init(allocator, allocator_get_entrypoint(allocator));
    if (NULL == table_pool) {
        allocator_free(allocator);
        free(database);
        return NULL;
    }
    database->table_pool = table_pool;
    database->allocator = allocator;
    return database;
}

database_result database_free(database_t *database) {
    if (allocator_free(database->allocator) != ALLOCATOR_SUCCESS) {
        pool_free(database->table_pool);
        return DATABASE_OP_ERR;
    }
    if (pool_free(database->table_pool) != POOL_OP_OK) {
        return DATABASE_OP_ERR;
    }
    free(database);
    return DATABASE_OP_OK;
}

database_result database_create_table(database_t *database, const table_schema_t *const table_schema) {
    assert(database != NULL && table_schema != NULL);
    pool_it *it = pool_iterator(database->table_pool);
    if (NULL == it) {
        return DATABASE_OP_ERR;
    }
    while (!pool_iterator_is_empty(it)) {
        buffer_t *buffer = pool_iterator_get(it);
        if (NULL == buffer) {
            pool_iterator_free(it);
            return DATABASE_OP_ERR;
        }
        table_schema_t *schema = table_schema_deserialize(buffer);
        if (NULL == schema) {
            pool_iterator_free(it);
            return DATABASE_OP_ERR;
        }
        if (0 == strcmp(schema->name, table_schema->name)) {
            pool_iterator_free(it);
            return DATABASE_OP_ERR;
        }
        free_schema(schema);
        buffer_free(buffer);
    }
    if (pool_iterator_free(it) != POOL_OP_OK) {
        return DATABASE_OP_ERR;
    }
    it = NULL;
    buffer_t *serialized = table_serialize(table_schema);
    if (NULL == serialized) {
        return DATABASE_OP_ERR;
    }
    if (pool_append(database->table_pool, serialized) != POOL_OP_OK) {
        buffer_free(serialized);
        return DATABASE_OP_ERR;
    }
    buffer_free(serialized);
    return DATABASE_OP_OK;
}

database_result database_delete_table(database_t *database, char *name) {
    pool_it *it = pool_iterator(database->table_pool);
    if (NULL == it) {
        return DATABASE_OP_ERR;
    }
    while (!pool_iterator_is_empty(it)) {
        buffer_t *buffer = pool_iterator_get(it);
        if (NULL == buffer) {
            pool_iterator_free(it);
            return DATABASE_OP_ERR;
        }
        table_schema_t *schema = table_schema_deserialize(buffer);
        if (NULL == schema) {
            pool_iterator_free(it);
            return DATABASE_OP_ERR;
        }
        if (0 == strcmp(schema->name, name)) {
            if (pool_iterator_delete(it) != POOL_OP_OK) {
                pool_iterator_free(it);
                return DATABASE_OP_ERR;
            }
            return DATABASE_OP_OK;
        }
        free_schema(schema);
        buffer_free(buffer);
    }
    if (pool_iterator_free(it) != POOL_OP_OK) {
        return DATABASE_OP_ERR;
    }
    return DATABASE_OP_OK;
}

table_t *database_find_table(database_t *database, char *name) {
    assert(database != NULL && name != NULL);
    pool_it *it = pool_iterator(database->table_pool);
    if (NULL == it) {
        return NULL;
    }
    while (!pool_iterator_is_empty(it)) {
        buffer_t *buffer = pool_iterator_get(it);
        if (NULL == buffer) {
            pool_iterator_free(it);
            return NULL;
        }
        table_schema_t *schema = table_schema_deserialize(buffer);
        if (NULL == schema) {
            pool_iterator_free(it);
            return NULL;
        }
        if (0 == strcmp(schema->name, name)) {
            pool_iterator_free(it);
            table_t *table = malloc(sizeof(table_t));
            table->schema = schema;
            table->data_pool = pool_init(database->allocator, schema->pool_offset);
            if (NULL == table->data_pool) {
                pool_iterator_free(it);
                free(table);
                return NULL;
            }
            return table;
        }
        free_schema(schema);
        buffer_free(buffer);
    }
    if (pool_iterator_free(it) != POOL_OP_OK) {
        return NULL;
    }
    return NULL;
}
