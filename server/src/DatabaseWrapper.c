//
// Created by wieceslaw on 22.12.23.
//

#include "database/Database.h"
#include "network.h"

static ColumnType MapColumnType(ColumnType columnType) {
    switch (columnType) {
        case COLUMN_TYPE__COLUMN_TYPE_INT:
            return COLUMN_TYPE_INT;
        case COLUMN_TYPE__COLUMN_TYPE_FLOAT:
            return COLUMN_TYPE_FLOAT;
        case COLUMN_TYPE__COLUMN_TYPE_STRING:
            return COLUMN_TYPE_STRING;
        case COLUMN_TYPE__COLUMN_TYPE_BOOL:
            return COLUMN_TYPE_BOOL;
        default:
            assert(0);
    }
}

//    char *file = NULL;
//    file_settings settings = {.path = file, .open_mode = FILE_OPEN_CLEAR};
//    Database db = DatabaseNew(&settings);
//    DatabaseFree(db);

static SchemeBuilder CreateTableQueryToBuilder(CreateTableQuery query) {
    SchemeBuilder schemeBuilder = SchemeBuilderNew(query.name);
    if (schemeBuilder == NULL) {
        return NULL;
    }
    TableScheme *scheme = query.scheme;
    for (size_t i = 0; i < scheme->n_columns; i++) {
        SchemeColumn *column = scheme->columns[i];
        char* columnName = column->name;
        ColumnType columnType = MapColumnType(column->type);
        if (SchemeBuilderAddColumn(schemeBuilder, columnName, columnType) != 0) {
            SchemeBuilderFree(schemeBuilder);
            return NULL;
        }
    }
    return schemeBuilder;
}

static int CallDatabaseCreateTable(Database db, CreateTableQuery query) {
    SchemeBuilder schemeBuilder = CreateTableQueryToBuilder(query);
    DatabaseCreateTable(db, schemeBuilder);
    SchemeBuilderFree(schemeBuilder);
    return 0;
}
