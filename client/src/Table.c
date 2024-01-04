//
// Created by wieceslaw on 04.01.24.
//

#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "Table.h"
#include "util_string.h"
#include "defines.h"

struct Table *TableNew(size_t nrows, size_t ncols) {
    struct Table *table = malloc(sizeof(struct Table));
    table->nrows = nrows;
    table->ncols = ncols;
    table->data = malloc(sizeof(char *) * nrows * ncols);
    memset(table->data, 0, sizeof(char *) * nrows * ncols);
    table->columnMaxes = malloc(sizeof(size_t) * table->ncols);
    memset(table->columnMaxes, 0, sizeof(char *) * table->ncols);
    return table;
}

void TableFree(struct Table *table) {
    for (size_t i = 0; i < table->ncols * table->nrows; i++) {
        free(table->data[i]);
        table->data[i] = NULL;
    }
    table->data = NULL;
    table->ncols = 0;
    table->nrows = 0;
    free(table);
}

void TableSet(struct Table *table, size_t row, size_t col, char *value) {
    assert(col < table->ncols && row < table->nrows);
    char **cell = table->data + row * table->ncols + col;
    *cell = string_copy(value);
    size_t length = strlen(value);
    table->columnMaxes[col] = MAX(table->columnMaxes[col], length);
}

char *TableGet(struct Table *table, size_t row, size_t col) {
    char **cell = table->data + row * table->ncols + col;
    return *cell;
}

static void printChars(size_t n, char chr) {
    for (size_t i = 0; i < n; i++) {
        printf("%c", chr);
    }
}

static size_t barLength(size_t ncols, const size_t *columnMaxes) {
    size_t sum = 4 + (ncols - 1) * 3;
    for (size_t i = 0; i < ncols; i++) {
        sum += columnMaxes[i];
    }
    return sum;
}

void TablePrintRow(struct Table *table, size_t row) {
    if (table->ncols == 0 || table->nrows == 0) {
        return;
    }
    printf("| ");
    for (size_t col = 0; col < table->ncols; col++) {
        char *str = TableGet(table, row, col);
        size_t length = 0;
        if (str != NULL) {
            length = strlen(str);
        }
        size_t nspaces = table->columnMaxes[col] - length;
        printf("%s", str);
        printChars(nspaces, ' ');
        printf(" | ");
    }
}

void TablePrintBar(struct Table *table) {
    if (table->ncols == 0 || table->nrows == 0) {
        return;
    }
    size_t barLen = barLength(table->ncols, table->columnMaxes);
    printChars(barLen, '-');
}

void TablePrintInterBar(struct Table *table) {
    if (table->ncols == 0 || table->nrows == 0) {
        return;
    }
    printf("+");
    for (size_t i = 0; i < table->ncols; i++) {
        printChars(table->columnMaxes[i], '-');
        printf("--+");
    }
}

void TablePrint(struct Table *table, bool withHeader) {
    if (table->ncols == 0 || table->nrows == 0) {
        return;
    }
    TablePrintBar(table);
    printf("\n");
    for (size_t row = 0; row < table->nrows; row++) {
        if (withHeader && row == 1) {
            TablePrintInterBar(table);
            printf("\n");
        }
        TablePrintRow(table, row);
        printf("\n");
    }
    TablePrintBar(table);
    printf("\n");
}
