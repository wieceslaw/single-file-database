//
// Created by vyach on 04.10.2023.
//

#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "vector.h"
#include "../defines.h"

struct vector {
    vector_value *array;
    size_t size;
    size_t capacity;
};

vector_t vector_init(size_t capacity) {
    vector_t vector = malloc(sizeof(struct vector));
    if (NULL == vector) {
        return NULL;
    }
    capacity = MAX(capacity, START_CAPACITY);
    vector_value *array = malloc(sizeof(vector_value) * capacity);
    if (NULL == array) {
        free(vector);
        return NULL;
    }
    vector->size = 0;
    vector->array = array;
    vector->capacity = capacity;
    return vector;
}

void vector_free(vector_t *vec) {
    if (NULL == *vec) {
        return;
    }
    free((*vec)->array);
    (*vec)->array = NULL;
    free(*vec);
    *vec = NULL;
}

size_t vector_size(vector_t vec) {
    assert(vec != NULL);
    return vec->size;
}

bool vector_is_empty(vector_t vec) {
    assert(vec != NULL);
    return 0 == vec->size;
}

void vector_clear(vector_t vec) {
    assert(vec != NULL);
    vec->size = 0;
}

vector_value *vector_get(vector_t vec, size_t idx) {
    assert(vec != NULL);
    if (idx > vector_size(vec)) {
        return NULL;
    }
    return vec->array[idx];
}

static bool vector_expand(vector_t vec, size_t size) {
    assert(vec != NULL);
    if (vec->capacity >= size) {
        return true;
    }
    size_t capacity = MAX(vec->capacity * EXPANDING_RATIO, size);
    vector_value *array = malloc(sizeof(vector_value) * capacity);
    if (NULL == array) {
        return false;
    }
    vec->capacity = capacity;
    memcpy(array, vec->array, vector_size(vec));
    free(vec->array);
    vec->array = array;
    return true;
}

bool vector_append(vector_t vec, vector_value val) {
    assert(vec != NULL);
    if (!vector_expand(vec, vec->size + 1)) {
        return false;
    }
    vec->array[vec->size] = val;
    vec->size++;
    return true;
}

bool vector_extend(vector_t vec, vector_value val, size_t size) {
    assert(vec != NULL);
    if (!vector_expand(vec, vec->size + size)) {
        return false;
    }
    for (size_t i = 0; i < vec->size; i++) {
        if (!vector_append(vec, val)) {
            return false;
        }
    }
    return true;
}
