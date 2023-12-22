//
// Created by vyach on 18.10.2023.
//

#include <stdio.h>
#include <time.h>
#include "database/Database.h"

static void print_row(row_t row) {
    for (size_t i = 0; i < row.size; i++) {
        column_t column = row.columns[i];
        switch (column.type) {
            case COLUMN_TYPE_INT:
                printf("%d ", column.value.val_int);
                break;
            case COLUMN_TYPE_FLOAT:
                printf("%f ", column.value.val_float);
                break;
            case COLUMN_TYPE_STRING:
                printf("%s ", column.value.val_string);
                break;
            case COLUMN_TYPE_BOOL:
                printf("%d ", column.value.val_bool);
                break;
        }
    }
    printf(" \n");
}

static void selecting(Database db) {
    JoinBuilder join_builder = JoinBuilderNew();
    JoinBuilderAddCondition(join_builder,
                            table_column_of(
                                    "test",
                                    "int"
                            ),
                            table_column_of(
                                    "test2",
                                    "int"
                            )
    );
    query_t query = {
            .table = "test",
            .where = NULL,
            .joins = join_builder
    };
    SelectorBuilder selector = SelectorBuilderNew();
    SelectorBuilderAdd(selector, "test", "int");
    SelectorBuilderAdd(selector, "test", "string");
    SelectorBuilderAdd(selector, "test", "bool");
    SelectorBuilderAdd(selector, "test", "float");
    SelectorBuilderAdd(selector, "test2", "int");
    SelectorBuilderAdd(selector, "test2", "string");
    SelectorBuilderAdd(selector, "test2", "bool");
    SelectorBuilderAdd(selector, "test2", "float");

    clock_t begin = clock();

    ResultView view = DatabaseSelectQuery(db, query, selector);
    if (NULL == view) {
        printf("Can't create select \n");
        return;
    }
    while (!ResultViewIsEmpty(view)) {
        row_t row = ResultViewGetRow(view);
        print_row(row);
        row_free(row);
        ResultViewNext(view);
    }
    ResultViewFree(view);
    clock_t end = clock();
    double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
    printf("%f", time_spent);
    SelectorBuilderFree(selector);
    JoinBuilderFree(join_builder);
}

int main(void) {
    file_settings settings = {.path = "C:\\Users\\vyach\\CLionProjects\\llp-lab1\\test.bin", .open_mode = FILE_OPEN_EXIST};
    Database db = DatabaseNew(&settings);
    selecting(db);
    DatabaseFree(db);
    return 0;
}
