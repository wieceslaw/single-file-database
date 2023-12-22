//
// Created by vyach on 18.10.2023.
//

#include "allocator/allocator.h"
#include "database/Database.h"
#include "query/SchemeBuilder.h"

static void createTable(Database db, char* tableName) {
    SchemeBuilder scheme_builder = SchemeBuilderNew(tableName);
    SchemeBuilderAddColumn(scheme_builder, "int", COLUMN_TYPE_INT);
    SchemeBuilderAddColumn(scheme_builder, "string", COLUMN_TYPE_STRING);
    SchemeBuilderAddColumn(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    SchemeBuilderAddColumn(scheme_builder, "float", COLUMN_TYPE_FLOAT);
    DatabaseCreateTable(db, scheme_builder);
    SchemeBuilderFree(scheme_builder);
}

int main(void) {
    file_open_mode mode = FILE_OPEN_CLEAR;
    char* tableName = "test";
    file_settings settings = {.path = "/home/wieceslaw/CLionProjects/single-file-database/test.db", .open_mode = mode};
    Database db = DatabaseNew(&settings);
    createTable(db, tableName);
    DatabaseFree(db);
    return 0;
}

