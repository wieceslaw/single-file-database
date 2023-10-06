//
// Created by vyach on 06.10.2023.
//

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "util/map/map_impl.h"

void test_map(int n) {
    str_str_map_t map = MAP_NEW_STR_STR(8);
    for (int i = 0; i < n; i++) {
        char *str = (char*)malloc(32 * sizeof(char));
        sprintf(str, "%d", i);
        MAP_PUT(map, str, str);
        free(str);
    }
    printf("added \n");
    int c = 0;
    for (int i = 0; i < n; i++) {
        char *str = (char*)malloc(32 * sizeof(char));
        sprintf(str, "%d", i);
        char* value = MAP_GET(map, str);
        if (strcmp(value, str) != 0) {
            exit(i);
        }
        free(str);
        free(value);
    }
    printf("checked \n");
    FOR_MAP(map, e, {
            free(e->val);
            free(e->key);
            free(e);
    })
    printf("removed \n");
    MAP_FREE(map);
}

/*
 *     for (int i = 1; i < 10; i++) {
        clock_t begin = clock();
        test_map(i * 1000);
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("n: %d, time: %f \n", i * 1000, time_spent);
        printf("time for one op: %f \n", time_spent / (double) i);
    }
 */