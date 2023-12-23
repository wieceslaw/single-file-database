//
// Created by vyach on 18.10.2023.
//

#include "allocator/allocator.h"
#include "database/Database.h"
#include "query/SchemeBuilder.h"

static void createTable(Database db) {
    char* tableName = "test";
    SchemeBuilder scheme_builder = SchemeBuilderNew(tableName);
    SchemeBuilderAddColumn(scheme_builder, "int", COLUMN_TYPE_INT);
    SchemeBuilderAddColumn(scheme_builder, "string", COLUMN_TYPE_STRING);
    SchemeBuilderAddColumn(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    SchemeBuilderAddColumn(scheme_builder, "float", COLUMN_TYPE_FLOAT);
    if (DatabaseCreateTable(db, scheme_builder) != 0) {
        printf("Unsuccessful table create \n");
    }
    SchemeBuilderFree(scheme_builder);
}

int main(void) {
    file_open_mode mode = FILE_OPEN_EXIST;
    char* file = "/home/wieceslaw/CLionProjects/single-file-database/test.db";

    file_settings settings = {.path = file, .open_mode = mode};
    Database db = DatabaseNew(&settings);
    if (db == NULL) {
        printf("Unable to init database \n");
    }
    createTable(db);
    if (DatabaseFree(db) != 0) {
        printf("Unable to close database \n");
    }
    return 0;
}

