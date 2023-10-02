//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_DATABASE_H
#define LLP_LAB1_DATABASE_H

#include "allocator/allocator.h"
#include "table/table.h"

typedef enum {
    DATABASE_OP_OK = 0,
    DATABASE_OP_ERR = 1,
} database_result;

typedef struct database database_t;

database_t *database_init(file_settings *settings);

database_result database_free(database_t *database);

database_result database_create_table(database_t *database, const table_schema_t *table_schema);

// OK - if table existed and was successfully deleted OR didn't exist
// ERR - if something went wrong
database_result database_delete_table(database_t *database, char* name);

// NULL - if something went wrong or didn't find
table_t *database_find_table(database_t *database, char* name);

#endif //LLP_LAB1_DATABASE_H
