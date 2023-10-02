//
// Created by vyach on 01.10.2023.
//

#include "test.h"
#include <stdio.h>

void print_pool(pool_t *pool) {
    pool_it* it = pool_iterator(pool);
    if (NULL == it) {
        printf("unable to get pool iterator \n");
        return;
    }
    int count = 0;
    while (!pool_iterator_is_empty(it)) {
        count++;
        buffer_t *buffer = pool_iterator_get(it);
        if (NULL == buffer) {
            printf("unable to get pool \n");
            return;
        }
        printf("%s \n", buffer->data);
        buffer_free(buffer);
        if (pool_iterator_next(it) != POOL_OP_OK) {
            printf("unable to go next \n");
            return;
        }
    }
    printf("count: %d \n", count);

    if (pool_iterator_free(it) != POOL_OP_OK) {
        printf("unable to free pool iterator \n");
        return;
    }
}

void test_pool(allocator_t *allocator) {
    offset_t entry_point = pool_create(allocator);
    if (0 == entry_point) {
        printf("unable to create pool \n");
        return;
    }
    pool_t *pool = pool_init(allocator, entry_point);
    if (NULL == pool) {
        printf("unable to init pool \n");
        return;
    }

    for (int i = 1; i < 300; i++) {
        buffer_t *buffer = buffer_init(i * 2);
        if (NULL == buffer) {
            printf("unable to create buffer");
        }
        sprintf(buffer->data, "%d", i);
        if (pool_append(pool, buffer) != POOL_OP_OK) {
            printf("unable to append to pool \n");
        }
    }

    printf("before add flush \n");
    print_pool(pool);

    if (pool_flush(pool) != POOL_OP_OK) {
        printf("unable to flush pool");
        return;
    }

    printf("after add flush \n");
    print_pool(pool);


    pool_it* it = pool_iterator(pool);
    if (NULL == it) {
        printf("unable to get pool iterator \n");
        return;
    }
    int count = 0;
    while (!pool_iterator_is_empty(it)) {
        count++;
        buffer_t *buffer = pool_iterator_get(it);
        if (NULL == buffer) {
            printf("unable to get pool \n");
            return;
        }

        if (*buffer->data == '2') {
            if (pool_iterator_delete(it) != POOL_OP_OK) {
                printf("unable to delete from pool");
                return;
            }
        }

        buffer_free(buffer);
        if (pool_iterator_next(it) != POOL_OP_OK) {
            printf("unable to go next \n");
            return;
        }
    }
    printf("%d \n", count);

    if (pool_iterator_free(it) != POOL_OP_OK) {
        printf("unable to free pool iterator \n");
        return;
    }

    printf("before delete flush");
    print_pool(pool);

    if (pool_flush(pool) != POOL_OP_OK) {
        printf("unable to flush pool");
        return;
    }

    printf("after delete flush \n");
    print_pool(pool);

    if (pool_free(pool) != POOL_OP_OK) {
        printf("unable to free pool \n");
    }
}
