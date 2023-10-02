//
// Created by vyach on 02.10.2023.
//

#ifndef LLP_LAB1_JOINSET_H
#define LLP_LAB1_JOINSET_H


#include <stdbool.h>

typedef struct row_set_t row_set_t;
typedef struct row_set_it row_set_it;
typedef struct row_t row_t;

row_set_t *row_set_create(row_t* row);

void row_set_free(row_set_t *);

void row_set_append(row_set_t *);

row_set_it *row_set_iterator(row_set_t *);



#endif //LLP_LAB1_JOINSET_H
