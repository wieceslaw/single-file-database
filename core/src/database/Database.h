//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "allocator/allocator.h"
#include "map/map_impl.h"
#include "table/table.h"
#include "query/where_condition.h"
#include "query/RowBatch.h"
#include "query/updater_builder.h"
#include "query/ResultView.h"
#include "query/SchemeBuilder.h"
#include "query/SelectorBuilder.h"

typedef struct indexed_maps {
    str_map_str_int_map_t columns_maps;
    str_int_map_t table_maps;
} indexed_maps;

typedef struct query {
    char *table;
    JoinBuilder joins;
    where_condition *where;
} query_t;

typedef struct Database *Database;

void indexed_maps_free(indexed_maps maps);

column_description indexed_maps_translate(indexed_maps maps, column_description description);

where_condition *where_condition_translate(where_condition *condition, indexed_maps maps);

/// THROWS: [DATABASE_INTERNAL_ERROR]
Database DatabaseNew(file_settings *settings);

/// THROWS: [DATABASE_INTERNAL_ERROR]
void DatabaseFree(Database database);

/// THROWS: [DATABASE_INTERNAL_ERROR]
void DatabaseDeleteTable(Database database, char *tableName);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
void DatabaseInsertQuery(Database database, char *tableName, RowBatch batch);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
void DatabaseDeleteQuery(Database database, query_t query);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
void DatabaseUpdateQuery(Database database, query_t query, updater_builder_t updater);

/// THROWS: [DATABASE_INTERNAL_ERROR, DATABASE_QUERY_EXCEPTION]
ResultView DatabaseSelectQuery(Database database, query_t query, SelectorBuilder selector);

list_t DatabaseGetTables(Database database);

int DatabaseCreateTable(Database database, SchemeBuilder builder);

#endif //LLP_LAB1_DATABASE_H
