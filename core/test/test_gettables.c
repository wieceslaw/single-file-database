#include "allocator/allocator.h"
#include "database/Database.h"

static char *columnTypeToStr(ColumnType type) {
    switch (type) {
        case COLUMN_TYPE_INT32:
            return "COLUMN_TYPE_INT";
        case COLUMN_TYPE_FLOAT32:
            return "COLUMN_TYPE_FLOAT";
        case COLUMN_TYPE_STRING:
            return "COLUMN_TYPE_STRING";
        case COLUMN_TYPE_BOOL:
            return "COLUMN_TYPE_BOOL";
    }
    return NULL;
}

static void printTable(table_scheme *scheme) {
    printf("Table [name: \"%s\"] \n", scheme->name);
    for (size_t i = 0; i < scheme->size; i++) {
        printf("Column [name: \"%s\", type: %s] \n", scheme->columns[i].name, columnTypeToStr(scheme->columns[i].type));
    }
}

int main(void) {
    file_open_mode mode = FILE_OPEN_EXIST;
    file_settings settings = {.path = "/home/wieceslaw/CLionProjects/single-file-database/test.db", .open_mode = mode};
    Database db = DatabaseNew(&settings);
    if (db == NULL) {
        printf("Unable to init database \n");
    }
    StrTableSchemeMap tables = DatabaseGetTablesSchemes(db);
    FOR_MAP(tables, entry, {
        printTable(entry->val);
        free(entry->key);
        table_scheme_free(entry->val);
    })
    if (DatabaseFree(db) != 0) {
        printf("Unable to close database \n");
    }
    return 0;
}
