//
// Created by vyach on 02.10.2023.
//

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "allocator/allocator.h"
#include "pool/pool.h"
#include "table/table.h"
#include "Database.h"
#include "exceptions/exceptions.h"
#include "database/cursor/cursor.h"
#include "map/map_impl.h"
#include "defines.h"

struct Database {
    allocator_t *allocator;
    pool_t *tablesPool;
    StrTableSchemeMap tables;
};

static StrTableSchemeMap LoadTables(pool_t *tablesPool) {
    assert(tablesPool != NULL);
    StrTableSchemeMap tables = MAP_NEW_STR_TABLE_SCHEME(8);
    if (tables == NULL) {
        return NULL;
    }
    pool_it it = pool_iterator(tablesPool);
    if (it == NULL) {
        MAP_FREE(tables);
        return NULL;
    }
    while (!pool_iterator_is_empty(it)) {
        buffer_t buffer = pool_iterator_get(it);
        if (buffer == NULL) {
            pool_iterator_free(&it);
            MAP_FREE(tables);
            return NULL;
        }
        table_scheme *table = table_scheme_deserialize(buffer);
        buffer_free(&buffer);
        if (table == NULL) {
            pool_iterator_free(&it);
            MAP_FREE(tables);
            return NULL;
        }
        MAP_PUT(tables, table->name, table);
        table_scheme_free(table);
        if (pool_iterator_next(it) != 0) {
            pool_iterator_free(&it);
            MAP_FREE(tables);
            return NULL;
        }
    }
    pool_iterator_free(&it);
    return tables;
}

StrTableSchemeMap DatabaseGetTablesSchemes(Database database) {
    assert(database != NULL);
    return database->tables;
}

Database DatabaseNew(file_settings *settings) {
    Database database;
    database = malloc(sizeof(struct Database));
    if (database == NULL) {
        return NULL;
    }
    allocator_t *allocator;
    if (allocator_init(settings, &allocator) != FILE_ST_OK) {
        free(database);
        return NULL;
    }
    if (settings->open_mode == FILE_OPEN_CLEAR || settings->open_mode == FILE_OPEN_CREATE) {
        offset_t offset;
        if (pool_create(allocator, &offset) != 0) {
            free(database);
            allocator_free(allocator);
            return NULL;
        }
        allocator_set_entrypoint(allocator, offset);
    }
    database->tablesPool = pool_init(allocator, allocator_get_entrypoint(allocator));
    if (database->tablesPool == NULL) {
        free(database);
        allocator_free(allocator);
        return NULL;
    }
    database->allocator = allocator;
    database->tables = LoadTables(database->tablesPool);
    if (database->tables == NULL) {
        pool_free(database->tablesPool);
        free(database);
        allocator_free(allocator);
        return NULL;
    }
    return database;
}

int DatabaseFree(Database database) {
    if (pool_free(database->tablesPool) != 0) {
        allocator_free(database->allocator);
        free(database);
        return -1;
    }
    if (allocator_free(database->allocator) != 0) {
        free(database);
        return -1;
    }
    free(database);
    return 0;
}

int DatabaseCreateTable(Database database, SchemeBuilder builder) {
    assert(database != NULL && builder != NULL);
    StrTableSchemeMap tables = database->tables;
    if (MAP_EXISTS(tables, builder->name)) {
        debug("Table already exists");
        return -1;
    }
    table_scheme *newTable = SchemeBuilderBuild(builder);
    if (newTable == NULL) {
        return -1;
    }
    offset_t offset;
    if (pool_create(database->allocator, &offset)) {
        debug("Unable to create pool");
        table_scheme_free(newTable);
        return -1;
    }
    newTable->pool_offset = offset;
    buffer_t serialized = table_scheme_serialize(newTable);
    if (pool_append(database->tablesPool, serialized) != 0) {
        debug("Unable to append to pool");
        table_scheme_free(newTable);
        buffer_free(&serialized);
        return -1;
    }
    if (pool_flush(database->tablesPool) != 0) {
        debug("File corruption");
        table_scheme_free(newTable);
        buffer_free(&serialized);
        return -1;
    }
    MAP_PUT(tables, newTable->name, newTable);
    table_scheme_free(newTable);
    buffer_free(&serialized);
    return 0;
}

int DatabaseDeleteTable(Database database, char *tableName) {
    pool_it it = pool_iterator(database->tablesPool);
    if (it == NULL) {
        return -1;
    }
    while (!pool_iterator_is_empty(it)) {
        buffer_t buffer = pool_iterator_get(it);
        if (buffer == NULL) {
            pool_iterator_free(&it);
            return -1;
        }
        table_scheme *table = table_scheme_deserialize(buffer);
        buffer_free(&buffer);
        if (table == NULL) {
            pool_iterator_free(&it);
            return -1;
        }
        if (strcmp(table->name, tableName) == 0) {
            table_scheme_free(table);
            if (pool_iterator_delete(it) != 0) {
                pool_iterator_free(&it);
                debug("Unable to delete table from tables pool");
                return -1;
            }
            pool_iterator_free(&it);
            if (pool_flush(database->tablesPool) != 0) {
                debug("Unable to flush table delete");
                return -1;
            }
            MAP_REMOVE(database->tables, tableName);
            return 0;
        }
        table_scheme_free(table);
        if (pool_iterator_next(it) != 0) {
            pool_iterator_free(&it);
            return -1;
        }
    }
    pool_iterator_free(&it);
    debug("Unable to find table to delete");
    return -1;
}

table_scheme *DatabaseFindTableScheme(Database database, char *tableName) {
    assert(database != NULL && tableName != NULL);
    StrTableSchemeMap tables = DatabaseGetTablesSchemes(database);
    return MAP_GET(tables, tableName);
}

static table_t DatabaseFindTable(Database database, char *name) {
    assert(database != NULL && name != NULL);
    table_t table = malloc(sizeof(struct table));
    if (table == NULL) {
        return NULL;
    }
    table->scheme = DatabaseFindTableScheme(database, name);
    if (table->scheme == NULL) {
        free(table);
        return NULL;
    }
    table->data_pool = pool_init(database->allocator, table->scheme->pool_offset);
    if (table->data_pool == NULL) {
        table_scheme_free(table->scheme);
        free(table);
        return NULL;
    }
    return table;
}

static bool RowIsValid(Row row, table_scheme *scheme) {
    for (size_t i = 0; i < row.size; i++) {
        if (scheme->columns[i].type != row.columns[i].type) {
            return false;
        }
    }
    return true;
}

static bool RowBatchIsValid(RowBatch batch, table_scheme *scheme) {
    for (size_t i = 0; i < batch.size; i++) {
        if (!RowIsValid(batch.rows[i], scheme)) {
            return false;
        }
    }
    return true;
}

int DatabaseInsertQuery(Database database, char *tableName, RowBatch batch) {
    assert(database != NULL && tableName != NULL);
    table_t table = DatabaseFindTable(database, tableName);
    if (table == NULL) {
        debug("Unable to find table to insert");
        return -1;
    }
    if (!RowBatchIsValid(batch, table->scheme)) {
        debug("Row batch is corrupted");
        return -1;
    }
    for (size_t i = 0; i < batch.size; i++) {
        Row row = batch.rows[i];
        buffer_t buffer = RowSerialize(row);
        if (pool_append(table->data_pool, buffer) != 0) {
            debug("Unsaved inserts. File is corrupted");
            buffer_free(&buffer);
            return -1;
        }
        buffer_free(&buffer);
    }
    if (pool_flush(table->data_pool) != 0) {
        debug("Unable to save inserts. File is corrupted");
        return -1;
    }
    table_free(&table);
    return 0;
}

static cursor_t database_build_base_cursor(Database database, char* table_name, size_t table_idx) {
    cursor_t cur = NULL;
    TRY({
        table_t table = DatabaseFindTable(database, table_name);
        cur = cursor_init_from(table, table_idx);
    }) CATCH(exception >= EXCEPTION, {
        cursor_free(&cur);
        RAISE(exception);
    }) FINALLY()
    return cur;
}

// THROWS: [MALLOC_EXCEPTION, DATABASE_TRANSLATION_EXCEPTION]
column_description indexed_maps_translate(indexed_maps maps, column_description description) {
    column_description result;
    int *column_idx = NULL;
    int *table_idx = NULL;
    TRY({
        table_idx = MAP_GET(maps.table_maps, description.name.table_name);
        if (table_idx == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        str_int_map_t column_map = MAP_GET(maps.columns_maps, description.name.table_name);
        if (column_map == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        column_idx = MAP_GET(column_map, description.name.column_name);
        if (column_idx == NULL) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        result.type = COLUMN_DESC_INDEX;
        result.index.table_idx = *table_idx;
        result.index.column_idx = *column_idx;
    }) FINALLY({
       free(column_idx);
       free(table_idx);
    })
    return result;
}

// THROWS: [MALLOC_EXCEPTION, POOL_EXCEPTION]
static str_int_map_t table_scheme_mapping(Database database, char* table_name) {
    // MAP[column_name, column_idx]
    table_scheme *scheme = NULL;
    str_int_map_t map = NULL;
    TRY({
        scheme = DatabaseFindTableScheme(database, table_name);
        if (NULL == scheme) {
            return NULL;
        }
        map = MAP_NEW_STR_INT(scheme->size);
        for (int i = 0; i < (int) scheme->size; i++) {
            table_scheme_column col = scheme->columns[i];
            MAP_PUT(map, col.name, &i);
        }
    }) FINALLY({
        table_scheme_free(scheme);
    })
    return map;
}

// THROWS: [TRANSLATION_EXCEPTION, EXCEPTION]
static indexed_maps query_mapping(Database database, query_t query) {
    assert(query.table != NULL);
    size_t size = 1;
    if (query.joins != NULL) {
        size += list_size(query.joins->conditions);
    }
    str_map_str_int_map_t columns_maps = MAP_NEW_STR_MAP_STR_INT(size);
    str_int_map_t table_maps = MAP_NEW_STR_INT(size);
    int count = 0;
    str_int_map_t columns_map = table_scheme_mapping(database, query.table);
    if (NULL == columns_map) {
        RAISE(DATABASE_TRANSLATION_EXCEPTION);
    }
    MAP_PUT(columns_maps, query.table, columns_map);
    MAP_PUT(table_maps, query.table, &count);
    if (query.joins != NULL) {
        FOR_LIST(query.joins->conditions, it, {
            count++;
            join_condition *condition = list_it_get(it);
            assert(condition != NULL);
            assert(condition->left.type == COLUMN_DESC_NAME && condition->right.type == COLUMN_DESC_NAME);
            str_int_map_t left_columns_map = MAP_GET(columns_maps, condition->left.name.table_name);
            if (NULL == left_columns_map) { // table should exists in join set
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            if (!MAP_EXISTS(left_columns_map, condition->left.name.column_name)) { // table should have such column
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            if (MAP_EXISTS(columns_maps, condition->right.name.table_name)) { // can't self-join
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            str_int_map_t right_columns_map = table_scheme_mapping(database, condition->right.name.table_name);
            if (NULL == right_columns_map) { // table should exist in Database
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            if (!MAP_EXISTS(right_columns_map, condition->right.name.column_name)) { // right table has such column
                RAISE(DATABASE_TRANSLATION_EXCEPTION);
            }
            MAP_PUT(columns_maps, condition->right.name.table_name, right_columns_map);
            MAP_PUT(table_maps, condition->right.name.table_name, &count);
        })
    }
    return (indexed_maps) {
        .columns_maps = columns_maps,
        .table_maps = table_maps
    };
}

void indexed_maps_free(indexed_maps maps) {
    if (maps.table_maps != NULL) {
        MAP_FREE(maps.table_maps);
    }
    if (maps.columns_maps != NULL) {
        FOR_MAP(maps.columns_maps, entry, {
            MAP_FREE(entry->val);
            free(entry->key);
            free(entry);
        })
        MAP_FREE(maps.columns_maps);
    }
}

// THROWS: [DATABASE_TRANSLATION_EXCEPTION]
static cursor_t query_cursor(Database database, query_t query, indexed_maps maps) {
    assert(database != NULL && query.table != NULL);
    cursor_t result = database_build_base_cursor(database, query.table, 0);
    if (query.joins != NULL) {
        FOR_LIST(query.joins->conditions, it, {
            TRY({
                join_condition *condition = list_it_get(it);
                assert(condition != NULL);
                join_condition translated_condition;
                translated_condition.right = indexed_maps_translate(maps, condition->right);
                translated_condition.left = indexed_maps_translate(maps, condition->left);
                cursor_t right = database_build_base_cursor(
                        database,
                        condition->right.name.table_name,
                        translated_condition.right.index.table_idx
                );
                cursor_t joined = cursor_init_join(result, right, translated_condition);
                result = joined;
            }) CATCH(exception >= EXCEPTION, {
                list_it_free(&it);
                cursor_free(&result);
                indexed_maps_free(maps);
                RAISE(exception);
            }) FINALLY()
        })
    }
    if (query.where != NULL) {
        where_condition *translated_condition;
        TRY({
            translated_condition = where_condition_translate(query.where, maps);
            cursor_t tmp = cursor_init_where(result, translated_condition);
            result = tmp;
        }) CATCH(exception >= exception, {
            where_condition_free(translated_condition);
            cursor_free(&result);
            RAISE(exception);
        }) FINALLY()
    }
    return result;
}

void DatabaseDeleteQuery(Database database, query_t query) {
    assert(database != NULL);
    indexed_maps maps = {0};
    cursor_t cur = NULL;
    TRY({
        maps = query_mapping(database, query);
        cur = query_cursor(database, query, maps);
        while (!cursor_is_empty(cur)) {
            cursor_delete(cur, 0);
            cursor_next(cur);
        }
        cursor_flush(cur);
        cursor_free(&cur);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        RAISE(DATABASE_QUERY_EXCEPTION);
    }) CATCH (exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        indexed_maps_free(maps);
    })
}

void DatabaseUpdateQuery(Database database, query_t query, updater_builder_t updater) {
    assert(database != NULL);
    indexed_maps maps = {0};
    cursor_t cur = NULL;
    str_int_map_t map = NULL;
    updater_builder_t translated_updater = NULL;
    TRY({
        maps = query_mapping(database, query);
        cur = query_cursor(database, query, maps);
        map = MAP_GET(maps.columns_maps, query.table);
        translated_updater = updater_builder_translate(updater, map);
        while (!cursor_is_empty(cur)) {
            cursor_update(cur, 0, translated_updater);
            cursor_next(cur);
        }
        cursor_flush(cur);
        cursor_free(&cur);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        RAISE(DATABASE_QUERY_EXCEPTION);
    }) CATCH (exception >= EXCEPTION, {
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY({
        indexed_maps_free(maps);
        updater_builder_free(&translated_updater);
    })
}

static table_scheme *create_view_scheme(Database database, SelectorBuilder selector, indexed_maps maps) {
    SchemeBuilder view_scheme_builder = SchemeBuilderNew("");
    if (view_scheme_builder == NULL) {
        RAISE(MALLOC_EXCEPTION);
    }
    FOR_LIST(selector->columns, it, {
        column_description *description = list_it_get(it);
        assert(description != NULL);
        column_description indexed_description = indexed_maps_translate(maps, *description);
        // add schemes cache?
        table_scheme *scheme = DatabaseFindTableScheme(database, description->name.table_name);
        table_scheme_column column = scheme->columns[indexed_description.index.column_idx];
        if (SchemeBuilderAddColumn(view_scheme_builder, column.name, column.type) != 0) {
            RAISE(MALLOC_EXCEPTION);
        }
        table_scheme_free(scheme);
    })
    table_scheme *result = SchemeBuilderBuild(view_scheme_builder);
    SchemeBuilderFree(view_scheme_builder);
    return result;
}

static column_description *create_view_selector(SelectorBuilder selector, indexed_maps maps) {
    column_description *result = rmalloc(sizeof(column_description) * list_size(selector->columns));
    int i = 0;
    FOR_LIST(selector->columns, it, {
        column_description *description = list_it_get(it);
        assert(description != NULL);
        result[i] = indexed_maps_translate(maps, *description);
        i++;
    })
    return result;
}

ResultView DatabaseSelectQuery(Database database, query_t query, SelectorBuilder selector) {
    assert(database != NULL && selector != NULL);
    ResultView result = NULL;
    indexed_maps maps = {0};
    cursor_t cur = NULL;
    TRY({
        result = rmalloc(sizeof(struct ResultView));
        maps = query_mapping(database, query);
        cur = query_cursor(database, query, maps);
        result->cursor = cur;
        result->view_scheme = create_view_scheme(database, selector, maps);
        result->view_selector = create_view_selector(selector, maps);
        indexed_maps_free(maps);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        free(result);
        indexed_maps_free(maps);
        RAISE(DATABASE_QUERY_EXCEPTION);
    }) CATCH(exception >= EXCEPTION, {
        free(result);
        indexed_maps_free(maps);
        RAISE(DATABASE_INTERNAL_ERROR);
    }) FINALLY()
    return result;
}

static operand column_operand_translate(operand op, indexed_maps maps) {
    switch (op.type) {
        case OPERAND_VALUE_LITERAL: {
            return op;
        }
        case OPERAND_VALUE_COLUMN: {
            return (operand) {
                    .type = OPERAND_VALUE_COLUMN,
                    .column = indexed_maps_translate(maps, op.column)
            };
        }
        default:
            assert(0);
    }
}

static where_condition *where_condition_translate_indexed(where_condition *condition, indexed_maps maps) {
    assert(condition->type == CONDITION_COMPARE);
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_COMPARE;
    result->compare.type = condition->compare.type;
    result->compare.first = column_operand_translate(condition->compare.first, maps);
    result->compare.second = column_operand_translate(condition->compare.second, maps);
    return result;
}

where_condition *where_condition_translate(where_condition *condition, indexed_maps maps) {
    switch (condition->type) {
        case CONDITION_AND:
            return where_condition_and(
                    where_condition_translate(condition->and.first, maps),
                    where_condition_translate(condition->and.second, maps)
            );
        case CONDITION_OR:
            return where_condition_or(
                    where_condition_translate(condition->or.first, maps),
                    where_condition_translate(condition->or.second, maps)
            );
        case CONDITION_NOT:
            return where_condition_not(
                    where_condition_translate(condition->not.first, maps)
            );
        case CONDITION_COMPARE:
            return where_condition_translate_indexed(condition, maps);
    }
    assert(0);
}
