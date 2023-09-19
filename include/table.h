//
// Created by vyach on 17.09.2023.
//

#ifndef LLP_LAB1_TABLE_H
#define LLP_LAB1_TABLE_H

typedef struct table table;

typedef struct table_it table_it;

typedef enum table_result {
    TABLE_OP_SUCCESS = 0,
    TABLE_OP_ERROR = 1,
} table_result;

table_result table_append(table*);

table_result table_flush(table*);

table_it* table_get_iterator(table*);

#endif //LLP_LAB1_TABLE_H
