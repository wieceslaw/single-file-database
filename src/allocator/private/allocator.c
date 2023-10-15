//
// Created by vyach on 15.09.2023.
//

#include <stdbool.h>
#include <stddef.h>
#include <malloc.h>
#include <assert.h>
#include "allocator/allocator.h"
#include "blocklist.h"

#define NEW_SIZE(SIZE) (SIZE + 10)

typedef PACK(
        struct {
            offset_t next;
        }
) free_page_h;

static offset_t granularity = 0;

static allocator_result unmap_block(block *block);

static block *map_block(allocator_t *allocator, offset_t file_offset);

static block_list *allocator_get_list(allocator_t *allocator);

static file_h *allocator_get_header(allocator_t *allocator);

static offset_t sys_granularity(void);

static bool header_is_valid(file_h *header) {
    assert(NULL != header);
    return MAGIC == header->magic;
}

static offset_t block_size(void) {
    if (0 == granularity) {
        granularity = sys_granularity();
    }
    return granularity;
}

static offset_t granular_offset(offset_t offset) {
    if (0 == granularity) {
        granularity = block_size();
    }
    return (offset / granularity) * granularity;
}

static allocator_result allocator_clear_list(allocator_t *allocator) {
    assert(NULL != allocator);
    block_list_it it;
    block_list_iterator(allocator_get_list(allocator), &it);
    while (!block_list_iterator_is_empty(&it)) {
        block_list_node *node = it.node;
        block_list_iterator_next(&it);
        if (unmap_block(node->block) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_UNMAP;
        }
        if (!block_list_delete(allocator_get_list(allocator), node)) {
            return ALLOCATOR_UNABLE_UNMAP;
        }
    }
    return ALLOCATOR_SUCCESS;
}

void allocator_set_entrypoint(allocator_t* allocator, offset_t entrypoint) {
    assert(NULL != allocator && NULL != allocator_get_header(allocator));
    allocator_get_header(allocator)->entrypoint = entrypoint;
}

offset_t allocator_get_entrypoint(allocator_t *allocator) {
    assert(NULL != allocator && NULL != allocator_get_header(allocator));
    return allocator_get_header(allocator)->entrypoint;
}

char *page_ptr(page_t *page) {
    assert(NULL != page);
    return page->block->ptr + page->offset;
}

offset_t page_offset(page_t *page) {
    assert(NULL != page);
    return page->block->file_offset + page->offset;
}

page_t *page_copy(allocator_t *allocator, page_t *page) {
    assert(NULL != page && NULL != allocator);
    return allocator_map_page(allocator, page_offset(page));
}

page_t *allocator_map_page(allocator_t *allocator, offset_t offset) {
    assert(NULL != allocator);
    block_list *list = allocator_get_list(allocator);
    offset_t fixed_offset = granular_offset(offset);
    block_list_node *node = block_list_find(list, fixed_offset);
    page_t *p = malloc(sizeof(page_t));
    if (NULL == p) {
        return NULL;
    }
    if (NULL == node) {
        block *block = map_block(allocator, fixed_offset);
        if (NULL == block) {
            free(p);
            return NULL;
        }
        node = block_list_append(list, block);
        if (NULL == node) {
            free(p);
            return NULL;
        }
    }
    node->used++;
    *p = (page_t) {.offset = offset - fixed_offset, .block = node->block};
    return p;
}

allocator_result allocator_unmap_page(allocator_t *allocator, page_t *page) {
    assert(NULL != page && NULL != allocator);
    block_list *list = allocator_get_list(allocator);
    block_list_node *node = page->block->node;
    if (NULL == node) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    node->used -= 1;
    if (list->size >= BLOCK_LIST_CAPACITY) {
        if (0 == node->used) {
            if (0 != unmap_block(page->block)) {
                block_list_delete(list, node);
                free(page);
                return ALLOCATOR_UNABLE_UNMAP;
            }
            if (!block_list_delete(list, node)) {
                free(page);
                return ALLOCATOR_UNABLE_UNMAP;
            }
        }
    }
    free(page);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_return_page(allocator_t *allocator, offset_t offset) {
    assert(NULL != allocator);
    page_t *page = allocator_map_page(allocator, offset);
    if (NULL == page) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_page_h *free_page_header = (free_page_h *) page_ptr(page);
    file_h *file_header = allocator_get_header(allocator);
    free_page_header->next = file_header->free_pages_next;
    file_header->free_pages_next = offset;
    if (ALLOCATOR_SUCCESS != allocator_unmap_page(allocator, page)) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    file_header->free_pages_count += 1;
    return ALLOCATOR_SUCCESS;
}

page_t *allocator_get_page(allocator_t *allocator) {
    assert(NULL != allocator);
    if (allocator_get_header(allocator)->free_pages_count == 0) {
        if (ALLOCATOR_SUCCESS != allocator_reserve_pages(allocator, 1)) {
            return NULL;
        }
    }
    offset_t next_block_offset = allocator_get_header(allocator)->free_pages_next;
    page_t *ref = allocator_map_page(allocator, next_block_offset);
    if (NULL == ref) {
        return NULL;
    }
    allocator_get_header(allocator)->free_pages_count -= 1;
    allocator_get_header(allocator)->free_pages_next = ((free_page_h *) page_ptr(ref))->next;
    return ref;
}

#ifdef _WIN32

#include "Windows.h"

struct allocator_t {
    HANDLE hFile;
    HANDLE hMap;
    LARGE_INTEGER liFileSize;
    block_list block_list;
    page_t *header_page;
};

offset_t sys_granularity() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwAllocationGranularity;
}

static block *map_block(allocator_t *allocator_t, offset_t offset) {
    ULARGE_INTEGER ulOffset;
    ulOffset.QuadPart = offset;
    offset_t mapping_size = min(block_size(), allocator_t->liFileSize.QuadPart - offset);
    void *ptr = MapViewOfFile(
            allocator_t->hMap,
            FILE_MAP_READ | FILE_MAP_WRITE,
            ulOffset.HighPart,
            ulOffset.LowPart,
            mapping_size);
    if (NULL == ptr) {
        return NULL;
    }
    block *block_ptr = malloc(sizeof(block));
    if (NULL == block_ptr) {
        UnmapViewOfFile(ptr);
        return NULL;
    }
    block_ptr->file_offset = offset;
    block_ptr->ptr = ptr;
    return block_ptr;
}

static allocator_result unmap_block(block *block) {
    if (NULL == block) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    if (!UnmapViewOfFile(block->ptr)) {
        free(block);
        return ALLOCATOR_UNABLE_UNMAP;
    }
    free(block);
    return ALLOCATOR_SUCCESS;
}

static block_list *allocator_get_list(allocator_t *allocator_t) {
    return &(allocator_t->block_list);
}

static file_h *allocator_get_header(allocator_t *allocator_t) {
    return (file_h *) page_ptr(allocator_t->header_page);
}

file_status allocator_init(file_settings *settings, allocator_t **allocator_ptr) {
    *allocator_ptr = malloc(sizeof(allocator_t));
    if (NULL == *allocator_ptr) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator_t) {0};
    allocator_t *allocator = *allocator_ptr;
    allocator->block_list.map = MAP_NEW_UINT64_VOID(BLOCK_LIST_CAPACITY);
    switch (settings->open_mode) {
        case FILE_OPEN_EXIST: {
            allocator->hFile = CreateFile(settings->path,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL,
                                          0);
            if (INVALID_HANDLE_VALUE == allocator->hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            allocator->hFile = CreateFile(settings->path,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_ALWAYS,
                                          FILE_ATTRIBUTE_NORMAL,
                                          0);
            if (INVALID_HANDLE_VALUE == allocator->hFile) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            allocator->hFile = CreateFile(settings->path,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          CREATE_NEW,
                                          FILE_ATTRIBUTE_NORMAL,
                                          0);
            if (INVALID_HANDLE_VALUE == allocator->hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    if (!GetFileSizeEx((*allocator_ptr)->hFile, &allocator->liFileSize)) {
        CloseHandle((*allocator_ptr)->hFile);
        return FILE_ST_ERROR;
    }
    switch (settings->open_mode) {
        case FILE_OPEN_EXIST: {
            if ((*allocator_ptr)->liFileSize.QuadPart < PAGE_SIZE) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            allocator->hMap = CreateFileMapping(
                    allocator->hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == allocator->hMap) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            allocator->header_page = allocator_map_page(allocator, 0);
            if (allocator->header_page == NULL) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            file_h *header = allocator_get_header(allocator);
            if (!header_is_valid(header)) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            break;
        }
        case FILE_OPEN_CREATE:
        case FILE_OPEN_CLEAR: {
            offset_t new_size = block_size();
            if (INVALID_SET_FILE_POINTER ==
                SetFilePointer(
                        (*allocator_ptr)->hFile,
                        new_size,
                        NULL,
                        FILE_BEGIN)
                    ) {
                return FILE_ST_ERROR;
            }
            allocator->liFileSize.QuadPart = new_size;
            if (!SetEndOfFile((*allocator_ptr)->hFile)) {
                return FILE_ST_ERROR;
            }
            allocator->hMap = CreateFileMapping(
                    allocator->hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == allocator->hMap) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            allocator->header_page = allocator_map_page(allocator, 0);
            if (allocator->header_page == NULL) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            file_h *header = allocator_get_header(*allocator_ptr);
            *header = (file_h) {0};
            header->magic = MAGIC;
            for (offset_t file_offset = PAGE_SIZE; file_offset < new_size; file_offset += PAGE_SIZE) {
                if (ALLOCATOR_SUCCESS != allocator_return_page(allocator, file_offset)) {
                    return FILE_ST_ERROR;
                }
            }
            break;
        }
    }
    return FILE_ST_OK;
}

file_status allocator_free(allocator_t *allocator) {
    if (allocator_unmap_page(allocator, allocator->header_page) != ALLOCATOR_SUCCESS) {
        allocator_clear_list(allocator);
        CloseHandle(allocator->hMap);
        CloseHandle(allocator->hFile);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (allocator_clear_list(allocator) != ALLOCATOR_SUCCESS) {
        CloseHandle(allocator->hMap);
        CloseHandle(allocator->hFile);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (!CloseHandle(allocator->hMap)) {
        CloseHandle(allocator->hFile);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (!CloseHandle(allocator->hFile)) {
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    MAP_FREE(allocator->block_list.map);
    free(allocator);
    return FILE_ST_OK;
}

static allocator_result clear_mapping(allocator_t *allocator_t) {
    block_list_it it;
    block_list_iterator(&allocator_t->block_list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        block *block = block_list_iterator_get(&it);
        if (!FlushViewOfFile(block->ptr, 0)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
        if (!UnmapViewOfFile(block->ptr)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
        block->ptr = NULL;
        block_list_iterator_next(&it);
    }
    if (!FlushFileBuffers(allocator_t->hFile)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    if (!CloseHandle(allocator_t->hMap)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    return ALLOCATOR_SUCCESS;
}

static allocator_result fill_mapping(allocator_t *allocator_t) {
    allocator_t->hMap = CreateFileMapping(
            allocator_t->hFile,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            NULL);
    if (INVALID_HANDLE_VALUE == allocator_t->hMap) {
        return ALLOCATOR_UNABLE_MAP;
    }
    block_list_it it;
    block_list_iterator(&allocator_t->block_list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        block *block_ptr = map_block(allocator_t, it.node->file_offset);
        if (NULL == block_ptr) {
            return ALLOCATOR_UNABLE_MAP;
        }
        it.node->block->ptr = block_ptr->ptr;
        free(block_ptr);
        block_list_iterator_next(&it);
    }
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_reserve_pages(allocator_t *allocator_t, uint32_t n) {
    file_h *header = allocator_get_header(allocator_t);
    uint32_t count = header->free_pages_count;
    if (count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t needed = n - count;
    uint32_t added = MAX(needed, NEW_SIZE(count));
    offset_t old_size = allocator_t->liFileSize.QuadPart;
    offset_t new_size =  PAGE_SIZE * (uint64_t) added + old_size;
    if (clear_mapping(allocator_t) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    union {
        struct {
            LONG LowPart;
            LONG HighPart;
        } u;
        LONGLONG QuadPart;
    } liOffset;
    liOffset.QuadPart = new_size;
    if (SetFilePointer(
            allocator_t->hFile,
            liOffset.u.LowPart,
            &liOffset.u.HighPart,
            FILE_BEGIN
    ) == INVALID_SET_FILE_POINTER) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    if (!SetEndOfFile(allocator_t->hFile)) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator_t->liFileSize.QuadPart = new_size;
    if (fill_mapping(allocator_t) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_MAP;
    }
    for (offset_t file_offset = old_size; file_offset < new_size; file_offset += PAGE_SIZE) {
        if (allocator_return_page(allocator_t, file_offset) != ALLOCATOR_SUCCESS) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

#else

#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>

struct allocator_t {
    int fd;
    page_t *header_page;
    offset_t file_size;
    block_list list;
};

static offset_t sys_granularity(void) {
    return getpagesize();
}

static allocator_result unmap_block(block *block) {
    assert(NULL != block);
    if (munmap(block->ptr, granularity) == 0) {
        return ALLOCATOR_SUCCESS;
    }
    return ALLOCATOR_UNABLE_UNMAP;
}

static block *map_block(allocator_t *allocator, offset_t offset) {
    assert(NULL != allocator);
    offset_t mapping_size = MIN(block_size(), allocator->file_size - offset);
    void *block_mapping = mmap(NULL,
                               mapping_size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               allocator->fd,
                               offset);
    if (MAP_FAILED == block_mapping) {
        return NULL;
    }
    block *b = malloc(sizeof(block));
    if (NULL == b) {
        munmap(block_mapping, mapping_size);
        return NULL;
    }
    b->ptr = block_mapping;
    b->file_offset = offset;
    return b;
}

static block_list *allocator_get_list(allocator_t *allocator) {
    assert(NULL != allocator);
    return &(allocator->list);
}

static file_h *allocator_get_header(allocator_t *allocator) {
    assert(NULL != allocator);
    return (file_h *) page_ptr(allocator->header_page);
}

file_status allocator_init(file_settings *settings, allocator_t **allocator_ptr) {
    *allocator_ptr = malloc(PAGE_SIZE);
    if (NULL == allocator_ptr) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator_t) {0};
    allocator_t *allocator = *allocator_ptr;
    allocator->list.map = MAP_NEW_UINT64_VOID(BLOCK_LIST_CAPACITY);
    switch (settings->open_mode) {
        case FILE_OPEN_EXIST: {
            allocator->fd = open(settings->path, O_RDWR);
            if (-1 == allocator->fd) {
                return FILE_ST_NOT_EXIST;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            allocator->fd = open(settings->path, O_RDWR | O_CREAT);
            if (-1 == allocator->fd) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            allocator->fd = open(settings->path, O_RDWR | O_CREAT | O_EXCL);
            if (-1 == allocator->fd) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    struct stat statbuf;
    int err = fstat(allocator->fd, &statbuf);
    if (err < 0) {
        return FILE_ST_WRONG_FORMAT;
    }
    allocator->file_size = statbuf.st_size;
    switch (settings->open_mode) {
        case FILE_OPEN_EXIST: {
            if (allocator->file_size < PAGE_SIZE) {
                return FILE_ST_WRONG_FORMAT;
            }
            allocator->header_page = allocator_map_page(allocator, 0);
            if (allocator->header_page == NULL) {
                return FILE_ST_ERROR;
            }
            if (!header_is_valid(allocator_get_header(allocator))) {
                allocator_unmap_page(allocator, allocator->header_page);
                return FILE_ST_WRONG_FORMAT;
            }
            break;
        }
        case FILE_OPEN_CREATE:
        case FILE_OPEN_CLEAR: {
            offset_t new_size = block_size();
            err = ftruncate(allocator->fd, new_size);
            allocator->file_size = new_size;
            if (err < 0) {
                return FILE_ST_ERROR;
            }
            allocator->header_page = allocator_map_page(allocator, 0);
            if (allocator->header_page == NULL) {
                return FILE_ST_ERROR;
            }
            *allocator_get_header(allocator) = (file_h) {0};
            allocator_get_header(allocator)->magic = MAGIC;
            for (offset_t offset = PAGE_SIZE; offset < new_size; offset += PAGE_SIZE) {
                if (ALLOCATOR_SUCCESS != allocator_return_page(allocator, offset)) {
                    return FILE_ST_ERROR;
                }
            }
            break;
        }
    }
    return FILE_ST_OK;
}

file_status allocator_free(allocator_t *allocator) {
    assert(NULL != allocator);
    if (ALLOCATOR_SUCCESS != allocator_unmap_page(allocator, allocator->header_page)) {
        allocator_clear_list(allocator);
        close(allocator->fd);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (ALLOCATOR_SUCCESS != allocator_clear_list(allocator)) {
        close(allocator->fd);
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (0 != close(allocator->fd)) {
        free(allocator);
        return FILE_ST_UNABLE_RELEASE;
    }
    MAP_FREE(allocator->list.map);
    free(allocator);
    return FILE_ST_OK;
}

allocator_result allocator_reserve_pages(allocator_t *allocator, uint32_t n) {
    assert(NULL != allocator);
    uint32_t old_count = allocator_get_header(allocator)->free_pages_count;
    if (old_count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t diff = n - old_count;
    uint32_t new_count = MAX(diff, NEW_SIZE(old_count));
    offset_t old_size = allocator->file_size;
    offset_t new_size = old_size + (PAGE_SIZE * new_count);
    if (ftruncate(allocator->fd, new_size) != 0) {
        return ALLOCATOR_UNABLE_EXTEND;
    }
    allocator->file_size = new_size;
    for (offset_t offset = old_size; offset < new_size; offset += PAGE_SIZE) {
        if (ALLOCATOR_SUCCESS != allocator_return_page(allocator, offset)) {
            return ALLOCATOR_UNABLE_EXTEND;
        }
    }
    return ALLOCATOR_SUCCESS;
}

#endif
