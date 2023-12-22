#include "allocator/allocator.h"
#include "database/Database.h"

static char *columnTypeToStr(column_type type) {
    switch (type) {
        case COLUMN_TYPE_INT:
            return "COLUMN_TYPE_INT";
        case COLUMN_TYPE_FLOAT:
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
    list_t tables = DatabaseGetTables(db);
    list_it it = list_head_iterator(tables);
    while (!list_it_is_empty(it)) {
        table_scheme *scheme = list_it_get(it);
        printTable(scheme);
        list_it_next(it);
    }
    list_it_free(&(it));
    DatabaseFree(db);
    return 0;
}
