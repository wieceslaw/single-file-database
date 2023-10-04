//
// Created by vyach on 04.10.2023.
//

#include <stdlib.h>
#include <time.h>
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

size_t int_hash(const int *i) {
    int x = *i ^ hash_start();
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

size_t str_hash(const char *s) {
    size_t hash = hash_start();
    char c;
    while ((c = *s++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

bool int_equals(const int *i1, const int *i2) {
    return *i1 == *i2;
}

bool str_equals(const char *s1, const char *s2) {
    return 0 == strcmp(s1, s2);
}

int *int_copy(const int *i) {
    int *res = malloc(sizeof(i));
    if (NULL == res) {
        return NULL;
    }
    *res = *i;
    return res;
}

char *str_copy(const char *s) {
    size_t length = strlen(s) + 1;
    char *res = malloc(sizeof(char) * length);
    if (NULL == res) {
        return NULL;
    }
    memcpy(res, s, length);
    return res;
}

void int_free(int *i) {
    free(i);
}

void str_free(char *s) {
    free(s);
}
