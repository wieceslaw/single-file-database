//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "allocator/allocator.h"
#include "database/Database.h"
#include "query/RowBuilder.h"

static void insert(Database db, int n, char* table_name) {
    RowBatch batch = RowBatchNew(n);
    for (int i = 0; i < n; i++) {
        int integer;
        float floating;
        int boolean;
        char string[64];
        scanf("%d", &integer);
        scanf("%s", string);
        scanf("%d", &boolean);
        scanf("%f", &floating);
        RowBuilder builder = RowBuilderNew(4);
        RowBuilderAdd(&builder, ColumnOfInt32(integer));
        RowBuilderAdd(&builder, ColumnOfString(string));
        RowBuilderAdd(&builder, ColumnOfBool(!!boolean));
        RowBuilderAdd(&builder, ColumnOfFloat32(floating));
        RowBatchAddRow(&batch, RowBuilderToRow(&builder));
        RowBuilderFree(&builder);
    }

    clock_t begin = clock();
    DatabaseInsertQuery(db, table_name, batch);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);

    RowBatchFree(&batch);
}

static void create_table(Database db, char* table_name) {
    SchemeBuilder scheme_builder = SchemeBuilderNew(table_name);
    SchemeBuilderAddColumn(scheme_builder, "int", COLUMN_TYPE_INT32);
    SchemeBuilderAddColumn(scheme_builder, "string", COLUMN_TYPE_STRING);
    SchemeBuilderAddColumn(scheme_builder, "bool", COLUMN_TYPE_BOOL);
    SchemeBuilderAddColumn(scheme_builder, "float", COLUMN_TYPE_FLOAT32);
    DatabaseCreateTable(db, scheme_builder);
    SchemeBuilderFree(scheme_builder);
}

int main(int argc, char *argv[]) {
    assert(argc == 4);
    int n = atoi(argv[1]);
    file_open_mode mode = atoi(argv[2]);
    char* table_name = argv[3];
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = mode};
    Database db = DatabaseNew(&settings);
    if (mode == FILE_OPEN_CLEAR) {
        createTable(db, table_name);
    }

    insert(db, n, table_name);

    if (DatabaseFree(db) != 0) {
        printf("Unable to close database \n");
    }
    return 0;
}
