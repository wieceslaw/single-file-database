#include <stddef.h>
#include <stdio.h>
#include "allocator/allocator.h"
#include "database/database.h"

static char *col_type_string(column_type_t type) {
    switch (type) {
        case TABLE_COLUMN_INT:
            return "INT";
        case TABLE_COLUMN_FLOAT:
            return "FLOAT";
        case TABLE_COLUMN_STRING:
            return "STR";
        case TABLE_COLUMN_BOOL:
            return "BOOL";
    }
}

static void print_table(table_schema_t *schema) {
    printf("====== %s ====== \n", schema->name);
    for (int i = 0; i < schema->size; i++) {
        column_t col = schema->columns[i];
        printf("| %s - %s \n", col.name, col_type_string(col.type));
    }
    printf("================== \n");
}

int main(void) {
    column_t columns[4] = {
            {.name = "id", .type = TABLE_COLUMN_INT},
            {.name = "name", .type = TABLE_COLUMN_STRING},
            {.name = "age", .type = TABLE_COLUMN_INT},
            {.name = "password", .type = TABLE_COLUMN_STRING}
    };
    table_schema_t schema = {.name = "user", .size = sizeof(columns) / sizeof(columns[0]), .columns = columns};

    file_settings settings = {.path = "test.bin", .open_type = FILE_OPEN_EXIST};
    database_t *db = database_init(&settings);
    if (NULL == db) {
        printf("unable to init database");
        return -1;
    }
//    if (database_create_table(db, &schema) != DATABASE_OP_OK) {
//        printf("unable to create table");
//        database_free(db);
//        return -1;
//    }
    table_t *table = database_find_table(db, "user");
    if (NULL == table) {
        printf("unable to find table");
        database_free(db);
        return -1;
    }

    print_table(table->schema);

    if (table_free(table) != TABLE_OP_OK) {
        printf("unable to free table");
        database_free(db);
        return -1;
    }
    if (database_free(db) != DATABASE_OP_OK) {
        printf("unable to free database");
        return -1;
    }
    return 0;
}
