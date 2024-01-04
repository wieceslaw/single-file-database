//
// Created by wieceslaw on 04.01.24.
//

#ifndef SINGLE_FILE_DATABASE_TABLE_H
#define SINGLE_FILE_DATABASE_TABLE_H

#include <stddef.h>
#include <malloc.h>

struct Table {
    size_t nrows;
    size_t ncols;
    char** data;
    size_t *columnMaxes;
};

struct Table *TableNew(size_t nrows, size_t ncols);

void TableFree(struct Table * table);

void TableSet(struct Table * table, size_t row, size_t col, char* value);

char* TableGet(struct Table * table, size_t row, size_t col);

void TablePrintRow(struct Table *table, size_t row);

void TablePrintBar(struct Table *table);

void TablePrintInterBar(struct Table *table);

void TablePrint(struct Table *table, bool withHeader);

#endif //SINGLE_FILE_DATABASE_TABLE_H
