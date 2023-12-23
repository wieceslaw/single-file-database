//
// Created by wieceslaw on 23.12.23.
//

#include "database/Database.h"

static void deleteTable(Database db) {
    char* tableName = "test";
    if (DatabaseDeleteTable(db, tableName) != 0) {
        printf("Unsuccessful table delete \n");
    }
}

int main(void) {
    file_open_mode mode = FILE_OPEN_EXIST;
    char* file = "/home/wieceslaw/CLionProjects/single-file-database/test.db";

    file_settings settings = {.path = file, .open_mode = mode};
    Database db = DatabaseNew(&settings);
    if (db == NULL) {
        printf("Unable to init database \n");
    }
    deleteTable(db);
    if (DatabaseFree(db) != 0) {
        printf("Unable to close database \n");
    }
    return 0;
}
