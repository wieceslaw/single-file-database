//
// Created by vyach on 13.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "database/Database.h"

static void print_row(Row row) {
    for (size_t i = 0; i < row.size; i++) {
        Column column = row.columns[i];
        switch (column.type) {
            case COLUMN_TYPE_INT:
                printf("%d ", column.value.i32);
                break;
            case COLUMN_TYPE_FLOAT:
                printf("%f ", column.value.f32);
                break;
            case COLUMN_TYPE_STRING:
                printf("%s ", column.value.str);
                break;
            case COLUMN_TYPE_BOOL:
                printf("%d ", column.value.b8);
                break;
        }
    }
    printf(" \n");
}

static void selecting(Database db) {
    query_t query = {.table = "test", .where = NULL, .joins = NULL};
    SelectorBuilder selector = SelectorBuilderNew();
    SelectorBuilderAdd(selector, "test", "int");
    SelectorBuilderAdd(selector, "test", "string");
    SelectorBuilderAdd(selector, "test", "bool");
    SelectorBuilderAdd(selector, "test", "float");

    clock_t begin = clock();

    ResultView view = DatabaseSelectQuery(db, query, selector);
    if (NULL == view) {
        printf("Can't create select \n");
        return;
    }
    while (!ResultViewIsEmpty(view)) {
        Row row = ResultViewGetRow(view);
        RowFree(row);
        ResultViewNext(view);
    }
    ResultViewFree(view);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);
    SelectorBuilderFree(selector);
}

int main(void) {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    Database db = DatabaseNew(&settings);
    selecting(db);
    DatabaseFree(db);
    return 0;
}
