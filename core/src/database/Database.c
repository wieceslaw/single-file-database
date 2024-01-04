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
#include "database/cursor/Cursor.h"
#include "map/map_impl.h"
#include "defines.h"

struct Database {
    allocator_t *allocator;
    pool_t *tablesPool;
    StrTableSchemeMap tables;
};

static bool RowIsValid(Row row, table_scheme *scheme);

static bool RowBatchIsValid(RowBatch batch, table_scheme *scheme);

static StrTableSchemeMap LoadTables(pool_t *tablesPool);

static table_t DatabaseFindTable(Database database, char *tableName);

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

DatabaseResult DatabaseCreateTable(Database database, SchemeBuilder builder) {
    assert(database != NULL && builder != NULL);
    StrTableSchemeMap tables = database->tables;
    if (MAP_EXISTS(tables, builder->name)) {
        debug("Table already exists");
        return DB_TABLE_EXISTS;
    }
    table_scheme *newTable = SchemeBuilderBuild(builder);
    if (newTable == NULL) {
        return DB_ERR;
    }
    offset_t offset;
    if (pool_create(database->allocator, &offset)) {
        debug("Unable to create pool");
        table_scheme_free(newTable);
        return DB_ERR;
    }
    newTable->pool_offset = offset;
    Buffer serialized = table_scheme_serialize(newTable);
    if (pool_append(database->tablesPool, serialized) != 0) {
        debug("Unable to append to pool");
        table_scheme_free(newTable);
        BufferFree(&serialized);
        return DB_ERR;
    }
    if (pool_flush(database->tablesPool) != 0) {
        debug("File corruption");
        table_scheme_free(newTable);
        BufferFree(&serialized);
        return DB_ERR;
    }
    MAP_PUT(tables, newTable->name, newTable);
    table_scheme_free(newTable);
    BufferFree(&serialized);
    return DB_OK;
}

DatabaseResult DatabaseDeleteTable(Database database, char *tableName) {
    pool_it it = pool_iterator(database->tablesPool);
    if (it == NULL) {
        return DB_ERR;
    }
    while (!pool_iterator_is_empty(it)) {
        Buffer buffer = pool_iterator_get(it);
        if (buffer == NULL) {
            pool_iterator_free(&it);
            return DB_ERR;
        }
        table_scheme *table = table_scheme_deserialize(buffer);
        BufferFree(&buffer);
        if (table == NULL) {
            pool_iterator_free(&it);
            return DB_ERR;
        }
        if (strcmp(table->name, tableName) == 0) {
            table_scheme_free(table);
            if (pool_iterator_delete(it) != 0) {
                pool_iterator_free(&it);
                debug("Unable to delete table from tables pool");
                return DB_ERR;
            }
            pool_iterator_free(&it);
            if (pool_flush(database->tablesPool) != 0) {
                debug("Unable to flush table delete");
                return DB_ERR;
            }
            MAP_REMOVE(database->tables, tableName);
            return DB_OK;
        }
        table_scheme_free(table);
        if (pool_iterator_next(it) != 0) {
            pool_iterator_free(&it);
            return DB_ERR;
        }
    }
    pool_iterator_free(&it);
    debug("Unable to find table to delete");
    return DB_UNKNOWN_TABLE;
}

StrTableSchemeMap DatabaseGetTablesSchemes(Database database) {
    assert(database != NULL);
    return database->tables;
}

table_scheme *DatabaseFindTableScheme(Database database, char *tableName) {
    assert(database != NULL && tableName != NULL);
    StrTableSchemeMap tables = DatabaseGetTablesSchemes(database);
    return MAP_GET(tables, tableName);
}

DatabaseResult DatabaseInsertQuery(Database database, char *tableName, RowBatch batch) {
    assert(database != NULL && tableName != NULL);
    table_t table = DatabaseFindTable(database, tableName);
    if (table == NULL) {
        debug("Unable to find table to insert");
        return DB_UNKNOWN_TABLE;
    }
    if (!RowBatchIsValid(batch, table->scheme)) {
        debug("Row batch is corrupted");
        return DB_BATCH_CORRUPTED;
    }
    for (size_t i = 0; i < batch.size; i++) {
        Row row = batch.rows[i];
        Buffer buffer = RowSerialize(row);
        if (pool_append(table->data_pool, buffer) != 0) {
            debug("Unsaved inserts. File is corrupted");
            BufferFree(&buffer);
            return DB_ERR;
        }
        BufferFree(&buffer);
    }
    if (pool_flush(table->data_pool) != 0) {
        debug("Unable to save inserts. File is corrupted");
        return DB_ERR;
    }
    table_free(&table);
    return DB_OK;
}

static Cursor DatabaseBuildBaseCursor(Database database, char *tableName, size_t tableIdx) {
    table_t table = DatabaseFindTable(database, tableName);
    if (table == NULL) {
        debug("Unable to find table for base cursor");
        return NULL;
    }
    Cursor cur = CursorNew_FROM(table, tableIdx);
    if (cur == NULL) {
        debug("Unable to create base cursor");
        table_free(&table);
        return NULL;
    }
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
static int query_mapping(Database database, query_t query, indexed_maps* result) {
    assert(query.table != NULL);
    size_t size = 1;
    if (query.joins != NULL) {
        size += ListSize(query.joins->conditions);
    }
    str_map_str_int_map_t columns_maps = MAP_NEW_STR_MAP_STR_INT(size);
    str_int_map_t table_maps = MAP_NEW_STR_INT(size);
    int count = 0;
    str_int_map_t columns_map = table_scheme_mapping(database, query.table);
    if (columns_map == NULL) {
        return -1;
    }
    MAP_PUT(columns_maps, query.table, columns_map);
    MAP_PUT(table_maps, query.table, &count);
    if (query.joins != NULL) {
        FOR_LIST(query.joins->conditions, it, {
            count++;
            join_condition *condition = ListIteratorGet(it);
            assert(condition != NULL);
            assert(condition->left.type == COLUMN_DESC_NAME && condition->right.type == COLUMN_DESC_NAME);
            str_int_map_t left_columns_map = MAP_GET(columns_maps, condition->left.name.table_name);
            if (NULL == left_columns_map) { // table should exists in join set
                return -1;
            }
            if (!MAP_EXISTS(left_columns_map, condition->left.name.column_name)) { // table should have such column
                return -1;
            }
            if (MAP_EXISTS(columns_maps, condition->right.name.table_name)) { // can't self-join
                return -1;
            }
            str_int_map_t right_columns_map = table_scheme_mapping(database, condition->right.name.table_name);
            if (right_columns_map == NULL) { // table should exist in Database
                return -1;
            }
            if (!MAP_EXISTS(right_columns_map, condition->right.name.column_name)) { // right table has such column
                return -1;
            }
            MAP_PUT(columns_maps, condition->right.name.table_name, right_columns_map);
            MAP_PUT(table_maps, condition->right.name.table_name, &count);
        })
    }
    *result = (indexed_maps) {
            .columns_maps = columns_maps,
            .table_maps = table_maps
    };
    return 0;
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
static Cursor query_cursor(Database database, query_t query, indexed_maps maps) {
    assert(database != NULL && query.table != NULL);
    Cursor result = DatabaseBuildBaseCursor(database, query.table, 0);
    if (query.joins != NULL) {
        FOR_LIST(query.joins->conditions, it, {
            TRY({
                join_condition *condition = ListIteratorGet(it);
                assert(condition != NULL);
                join_condition translated_condition;
                translated_condition.right = indexed_maps_translate(maps, condition->right);
                translated_condition.left = indexed_maps_translate(maps, condition->left);
                Cursor right = DatabaseBuildBaseCursor(
                        database,
                        condition->right.name.table_name,
                        translated_condition.right.index.table_idx
                );
                Cursor joined = CursorNew_JOIN(result, right, translated_condition);
                if (joined == NULL) {
                    RAISE(DATABASE_INTERNAL_ERROR);
                }
                result = joined;
            }) CATCH(exception >= EXCEPTION, {
                ListIteratorFree(&it);
                CursorFree(&result);
                indexed_maps_free(maps);
                RAISE(exception);
            }) FINALLY()
        })
    }
    if (query.where != NULL) {
        where_condition *translated_condition = NULL;
        TRY({
            translated_condition = where_condition_translate(query.where, maps);
            result = CursorNew_WHERE(result, translated_condition);
            if (result == NULL) {
                RAISE(DATABASE_INTERNAL_ERROR);
            }
        }) CATCH(exception >= EXCEPTION, {
            where_condition_free(translated_condition);
            CursorFree(&result);
            RAISE(exception);
        }) FINALLY()
    }
    return result;
}

DatabaseResult DatabaseDeleteQuery(Database database, query_t query, int *result) {
    assert(database != NULL);
    indexed_maps maps = {0};
    Cursor cur = NULL;
    int count = 0;
    DatabaseResult res = DB_OK;
    TRY({
        if (query_mapping(database, query, &maps) != 0) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        cur = query_cursor(database, query, maps);
        while (!CursorIsEmpty(cur)) {
            CursorDeleteRow(cur, 0);
            CursorNext(cur);
            count++;
        }
        CursorFlush(cur);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        res = DB_INVALID_QUERY;
    }) CATCH (exception >= EXCEPTION, {
        res = DB_ERR;
    }) FINALLY({
        CursorFree(&cur);
        indexed_maps_free(maps);
    })
    *result = count;
    return res;
}

DatabaseResult DatabaseUpdateQuery(Database database, query_t query, updater_builder_t updater, int *result) {
    assert(database != NULL);
    indexed_maps maps = {0};
    Cursor cur = NULL;
    str_int_map_t map = NULL;
    updater_builder_t translated_updater = NULL;
    DatabaseResult res = DB_OK;
    int count = 0;
    TRY({
        if (query_mapping(database, query, &maps) != 0) {
            RAISE(DATABASE_TRANSLATION_EXCEPTION);
        }
        cur = query_cursor(database, query, maps);
        map = MAP_GET(maps.columns_maps, query.table);
        translated_updater = updater_builder_translate(updater, map);
        while (!CursorIsEmpty(cur)) {
            CursorUpdateRow(cur, 0, translated_updater);
            CursorNext(cur);
            count++;
        }
        CursorFlush(cur);
    }) CATCH(exception == DATABASE_TRANSLATION_EXCEPTION, {
        res = DB_INVALID_QUERY;
    }) CATCH (exception >= EXCEPTION, {
        res = DB_ERR;
    }) FINALLY({
        CursorFree(&cur);
        indexed_maps_free(maps);
        updater_builder_free(&translated_updater);
    })
    *result = count;
    return res;
}

static table_scheme *create_view_scheme(Database database, SelectorBuilder selector, indexed_maps maps) {
    SchemeBuilder view_scheme_builder = SchemeBuilderNew("");
    if (view_scheme_builder == NULL) {
        RAISE(MALLOC_EXCEPTION);
    }
    FOR_LIST(selector->columns, it, {
        column_description *description = ListIteratorGet(it);
        assert(description != NULL);
        column_description indexed_description = indexed_maps_translate(maps, *description);
        table_scheme *scheme = DatabaseFindTableScheme(database, description->name.table_name);
        table_scheme_column column = scheme->columns[indexed_description.index.column_idx];
        char *label = malloc(sizeof(char) * (2 + strlen(scheme->name) + strlen(column.name)));
        label[0] = '\0';
        strcat(label, scheme->name);
        strcat(label, ".");
        strcat(label, column.name);
        if (SchemeBuilderAddColumn(view_scheme_builder, label, column.type) != 0) {
            RAISE(MALLOC_EXCEPTION);
        }
        free(label);
        table_scheme_free(scheme);
    })
    table_scheme *result = SchemeBuilderBuild(view_scheme_builder);
    SchemeBuilderFree(view_scheme_builder);
    return result;
}

static column_description *create_view_selector(SelectorBuilder selector, indexed_maps maps) {
    column_description *result = rmalloc(sizeof(column_description) * ListSize(selector->columns));
    int i = 0;
    FOR_LIST(selector->columns, it, {
        column_description *description = ListIteratorGet(it);
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
    Cursor cur = NULL;
    result = malloc(sizeof(struct ResultView));
    if (result == NULL) {
        return NULL;
    }
    if (query_mapping(database, query, &maps) != 0) {
        return NULL;
    }
    TRY({
        cur = query_cursor(database, query, maps);
        result->cursor = cur;
        result->view_scheme = create_view_scheme(database, selector, maps);
        result->view_selector = create_view_selector(selector, maps);
    }) CATCH(exception >= EXCEPTION, {
        free(result);
        result = NULL;
    }) FINALLY({
       indexed_maps_free(maps);
    })
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

static ColumnType operand_type(operand op) {
    if (op.type == OPERAND_VALUE_LITERAL) {
        return op.literal.type;
    } else {
        assert(op.column.type == COLUMN_DESC_INDEX);
        // TODO: access operand type? add into mappings? Same problem with joins
    }
}

static bool column_operands_have_same_type(operand first, operand second) {
    return operand_type(first) != operand_type(second);
}

static where_condition *where_condition_translate_indexed(where_condition *condition, indexed_maps maps) {
    assert(condition->type == CONDITION_COMPARE);
    where_condition *result = rmalloc(sizeof(where_condition));
    result->type = CONDITION_COMPARE;
    result->compare.type = condition->compare.type;
    result->compare.first = column_operand_translate(condition->compare.first, maps);
    result->compare.second = column_operand_translate(condition->compare.second, maps);
    if (!column_operands_have_same_type(result->compare.first, result->compare.second)) {
        free(result);
        RAISE(DATABASE_TRANSLATION_EXCEPTION);
    }
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
        Buffer buffer = pool_iterator_get(it);
        if (buffer == NULL) {
            pool_iterator_free(&it);
            MAP_FREE(tables);
            return NULL;
        }
        table_scheme *table = table_scheme_deserialize(buffer);
        BufferFree(&buffer);
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

static table_t DatabaseFindTable(Database database, char *tableName) {
    assert(database != NULL && tableName != NULL);
    table_t table = malloc(sizeof(struct table));
    if (table == NULL) {
        return NULL;
    }
    table->scheme = DatabaseFindTableScheme(database, tableName);
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
