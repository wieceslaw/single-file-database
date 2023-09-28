//
// Created by vyach on 13.09.2023.
//

#ifndef LLP_LAB1_ALLOCATOR_H
#define LLP_LAB1_ALLOCATOR_H

#include <stdint.h>

#define PAGE_SIZE 4096
#define MAGIC 0xABCD

#define PAGE_CAPACITY (PAGE_SIZE - sizeof(list_node_h))

#ifdef _WIN32
#define MIN(a, b) (((a)<(b))?(a):(b))
#else
#include <sys/param.h>
#endif

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

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

typedef struct {
    int32_t magic;
    uint32_t free_pages_count;
    offset_t free_pages_next;
    offset_t entry_point_page;
} file_h;

typedef struct allocator_t allocator_t;

typedef struct page_t page_t;

char *page_ptr(page_t *p);

page_t* page_copy(allocator_t *allocator, page_t *page);

offset_t page_offset(page_t *p);

file_status allocator_init(file_settings *settings, allocator_t **allocator_ptr);

file_status allocator_free(allocator_t *allocator);

allocator_result allocator_return_page(allocator_t *allocator, offset_t offset);

page_t *allocator_get_page(allocator_t *allocator);

allocator_result allocator_reserve_pages(allocator_t *allocator, uint32_t n);

page_t *allocator_map_page(allocator_t *allocator, offset_t offset);

allocator_result allocator_unmap_page(allocator_t *allocator, page_t *page);

#endif //LLP_LAB1_ALLOCATOR_H
