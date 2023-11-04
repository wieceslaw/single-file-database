//
// Created by vyach on 04.10.2023.
//

#ifndef LLP_LAB1_VECTOR_H
#define LLP_LAB1_VECTOR_H

#include <stddef.h>
#include <stdbool.h>

#define EXPANDING_RATIO 2
#define START_CAPACITY 8

typedef void *vector_value;

typedef struct vector *vector_t;

vector_t vector_init(size_t capacity);

void vector_free(vector_t *vec);

bool vector_is_empty(vector_t vec);

void vector_clear(vector_t vec);

vector_value *vector_get(vector_t vec, size_t idx);

size_t vector_size(vector_t vec);

bool vector_append(vector_t vec, vector_value val);

bool vector_extend(vector_t vec, vector_value val, size_t size);

#endif //LLP_LAB1_VECTOR_H
