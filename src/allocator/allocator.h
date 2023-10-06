//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_ALLOCATOR_H
#define LLP_LAB1_ALLOCATOR_H

#include <stdint.h>
#include "util/defines.h"

#define PAGE_SIZE 4096
#define PAGE_CAPACITY (PAGE_SIZE - sizeof(page_list_node_h))
#define MAGIC 0xABCD

typedef uint64_t offset_t;

typedef enum {
    ALLOCATOR_SUCCESS = 0,
    ALLOCATOR_UNABLE_MAP = 1,
    ALLOCATOR_UNABLE_UNMAP = 2,
    ALLOCATOR_UNABLE_EXTEND = 3,
} allocator_result;

typedef enum {
    FILE_ST_OK = 0,
    FILE_ST_ALREADY_EXISTS = 1,
    FILE_ST_NOT_EXIST = 2,
    FILE_ST_WRONG_FORMAT = 3,
    FILE_ST_UNABLE_OPEN = 4,
    FILE_ST_UNABLE_RELEASE = 5,
    FILE_ST_ERROR = 6,
} file_status;

typedef enum {
    FILE_OPEN_CREATE,
    FILE_OPEN_EXIST,
    FILE_OPEN_CLEAR
} file_open_type;

typedef struct {
    char *path;
    file_open_type open_type;
} file_settings;

typedef PACK(
        struct {
            int32_t magic;
            uint32_t free_pages_count;
            offset_t free_pages_next;
            offset_t entrypoint;
        }
) file_h;

typedef struct allocator_t allocator_t;

typedef struct page_t page_t;

char *page_ptr(page_t *page);

page_t *page_copy(allocator_t *allocator, page_t *page);

offset_t page_offset(page_t *page);

file_status allocator_init(file_settings *settings, allocator_t **allocator_ptr);

file_status allocator_free(allocator_t *allocator);

void allocator_set_entrypoint(allocator_t *allocator, offset_t entrypoint);

offset_t allocator_get_entrypoint(allocator_t *allocator);

allocator_result allocator_return_page(allocator_t *allocator, offset_t offset);

page_t *allocator_get_page(allocator_t *allocator);

allocator_result allocator_reserve_pages(allocator_t *allocator, uint32_t n);

page_t *allocator_map_page(allocator_t *allocator, offset_t offset);

allocator_result allocator_unmap_page(allocator_t *allocator, page_t *page);

#endif //LLP_LAB1_ALLOCATOR_H
