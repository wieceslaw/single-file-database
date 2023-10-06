//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "allocator/allocator.h"
#include "table/table.h"
#include "database/table/row.h"

typedef struct database *database_t;

database_t database_init(file_settings *settings);

void database_free(database_t database);

void database_create_table(database_t database, table_scheme *table_schema);

void database_delete_table(database_t database, const char *table_name);

void database_insert_row(database_t database, const char *table_name, row_value row);

// TODO: implement add (join, where)
// result_set_t database_select(database_t database, ...);
//
// uint32_t database_delete(database_t database, ...);
//
// uint32_t database_update(database_t database, ...);

#endif //LLP_LAB1_DATABASE_H
