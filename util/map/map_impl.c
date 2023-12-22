//
// Created by vyach on 04.10.2023.
//

#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include "map_impl.h"

static size_t hash_start_val = 0;

static size_t hash_start(void) {
    if (0 == hash_start_val) {
        srand(time(NULL) < 5);
        hash_start_val = rand();
    }
    return hash_start_val;
}

size_t int_hash(const void *p) {
    assert(p != NULL);
    int *i = (int *) p;
    size_t x = *i ^ hash_start();
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

size_t str_hash(const void *p) {
    assert(p != NULL);
    char *s = (char *) p;
    size_t hash = hash_start();
    char c;
    while ((c = *s++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

bool int_equals(const void *p1, const void *p2) {
    assert(p1 != NULL && p2 != NULL);
    int *i1 = (int *) p1;
    int *i2 = (int *) p2;
    return *i1 == *i2;
}

bool str_equals(const void *p1, const void *p2) {
    assert(p1 != NULL && p2 != NULL);
    char *s1 = (char *) p1;
    char *s2 = (char *) p2;
    return 0 == strcmp(s1, s2);
}

void *int_copy(const void *p) {
    if (NULL == p) {
        return NULL;
    }
    int *i = (int *) p;
    int *res = malloc(sizeof(i));
    if (NULL == res) {
        return NULL;
    }
    *res = *i;
    return res;
}

void *str_copy(const void *p) {
    if (NULL == p) {
        return NULL;
    }
    char *s = (char *) p;
    size_t length = strlen(s) + 1;
    char *res = malloc(sizeof(char) * length);
    if (NULL == res) {
        return NULL;
    }
    memcpy(res, s, length);
    return res;
}

void int_free(void *p) {
    if (NULL == p) {
        return;
    }
    free(p);
}

void str_free(void *p) {
    if (NULL == p) {
        return;
    }
    free(p);
}

void skip_free(void *x) {
    (void)(x);
}

void *skip_copy(const void *x) {
    return (void *) x;
}

size_t uint64_hash(const void *p) {
    assert(p != NULL);
    size_t *i = (uint64_t *) p;
    size_t x = *i ^ hash_start();
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

bool uint64_equals(const void *p1, const void *p2) {
    assert(p1 != NULL && p2 != NULL);
    uint64_t *i1 = (uint64_t *) p1;
    uint64_t *i2 = (uint64_t *) p2;
    return *i1 == *i2;
}

void uint64_free(void *x) {
    free(x);
}

void* uint64_copy(const void *p) {
    if (NULL == p) {
        return NULL;
    }
    uint64_t *i = (uint64_t *) p;
    uint64_t *res = malloc(sizeof(i));
    if (NULL == res) {
        return NULL;
    }
    *res = *i;
    return res;
}
