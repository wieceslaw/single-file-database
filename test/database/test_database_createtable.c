//
// Created by vyach on 18.10.2023.
//

#include <assert.h>
#include "allocator/allocator.h"
#include "database/database.h"
#include "database/query/scheme_builder.h"

void create_table(database_t db, char* table_name) {
    scheme_builder_t scheme_builder = scheme_builder_init(table_name);
    scheme_builder_add_column(scheme_builder, "int", COLUMN_TYPE_INT);
    scheme_builder_add_column(scheme_builder, "string", COLUMN_TYPE_STRING);
    scheme_builder_add_column(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    scheme_builder_add_column(scheme_builder, "float", COLUMN_TYPE_FLOAT);
    database_create_table(db, scheme_builder);
    scheme_builder_free(&scheme_builder);
}

int main(int argc, char *argv[]) {
    assert(argc == 3);
    file_open_mode mode = atoi(argv[1]);
    char* table_name = argv[2];
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = mode};
    database_t db = database_init(&settings);
    create_table(db, table_name);
    database_free(db);
    return 0;
}

