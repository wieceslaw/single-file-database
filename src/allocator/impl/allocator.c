//
// Created by vyach on 15.09.2023.
//

#include <stdbool.h>
#include <stddef.h>
#include <malloc.h>
#include "allocator/allocator.h"
#include "blocklist.h"

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

PACK (
    typedef struct {
        offset_t next;
    } free_page_h;
)

static bool header_is_valid(file_h *header) {
    return MAGIC == header->magic;
}

static offset_t granularity = 0;

static offset_t sys_granularity(void);

static offset_t block_size(void) {
    if (granularity == 0) {
        granularity = sys_granularity();
    }
    return granularity;
}

static offset_t granular_offset(offset_t offset) {
    if (granularity == 0) {
        granularity = block_size();
    }
    return (offset / granularity) * granularity;
}

static allocator_result unmap_block(block *block);

static block *map_block(allocator_t *allocator, offset_t file_offset);

static block_list *allocator_get_list(allocator_t *allocator);

static file_h *allocator_get_header(allocator_t *allocator);

static allocator_result allocator_clear_list(allocator_t *allocator) {
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

static block_list_node *block_list_find(block_list *list, offset_t offset) {
    block_list_it it;
    block_list_iterator(list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        if (it.node->file_offset == offset) {
            return it.node;
        }
        block_list_iterator_next(&it);
    }
    return NULL;
}

char *page_ptr(page_t *p) {
    if (NULL == p) {
        return NULL;
    }
    return p->block->ptr + p->offset;
}

offset_t page_offset(page_t *p) {
    return p->block->file_offset + p->offset;
}

page_t *page_copy(allocator_t *allocator, page_t *page) {
    return allocator_map_page(allocator, page_offset(page));
}

page_t *allocator_map_page(allocator_t *allocator, offset_t offset) {
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

allocator_result allocator_unmap_page(allocator_t *allocator, page_t *p) {
    if (NULL == allocator || NULL == p) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    block_list *list = allocator_get_list(allocator);
    block_list_node *node = block_list_find(list, p->block->file_offset);
    if (NULL == node) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    node->used -= 1;
    if (0 == node->used) {
        if (0 != unmap_block(p->block)) {
            block_list_delete(list, node);
            free(p);
            return ALLOCATOR_UNABLE_UNMAP;
        }
        if (!block_list_delete(list, node)) {
            free(p);
            return ALLOCATOR_UNABLE_UNMAP;
        }
    }
    free(p);
    return ALLOCATOR_SUCCESS;
}

allocator_result allocator_return_page(allocator_t *allocator, offset_t offset) {
    page_t *p = allocator_map_page(allocator, offset);
    if (NULL == p) {
        return ALLOCATOR_UNABLE_MAP;
    }
    free_page_h *free_page = (free_page_h *) page_ptr(p);
    file_h *header = allocator_get_header(allocator);
    free_page->next = header->free_pages_next;
    header->free_pages_next = offset;
    if (ALLOCATOR_SUCCESS != allocator_unmap_page(allocator, p)) {
        return ALLOCATOR_UNABLE_UNMAP;
    }
    header->free_pages_count += 1;
    return ALLOCATOR_SUCCESS;
}

page_t *allocator_get_page(allocator_t *allocator) {
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

#include "windows.h"

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
    if (ptr == NULL) {
        return NULL;
    }
    block *block_ptr = malloc(sizeof(block));
    if (block_ptr == NULL) {
        UnmapViewOfFile(ptr);
        return NULL;
    }
    block_ptr->file_offset = offset;
    block_ptr->ptr = ptr;
    return block_ptr;
}

static allocator_result unmap_block(block *block) {
    if (block == NULL) {
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
    if (*allocator_ptr == NULL) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator_t) {0};
    allocator_t *allocator_t = *allocator_ptr;
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            allocator_t->hFile = CreateFile(settings->path,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_EXISTING,
                                            FILE_ATTRIBUTE_NORMAL,
                                            0);
            if (INVALID_HANDLE_VALUE == allocator_t->hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
        case FILE_OPEN_CLEAR: {
            allocator_t->hFile = CreateFile(settings->path,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL,
                                            0);
            if (INVALID_HANDLE_VALUE == allocator_t->hFile) {
                return FILE_ST_UNABLE_OPEN;
            }
            break;
        }
        case FILE_OPEN_CREATE: {
            allocator_t->hFile = CreateFile(settings->path,
                                            GENERIC_READ | GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            CREATE_NEW,
                                            FILE_ATTRIBUTE_NORMAL,
                                            0);
            if (INVALID_HANDLE_VALUE == allocator_t->hFile) {
                return FILE_ST_ALREADY_EXISTS;
            }
            break;
        }
    }
    if (!GetFileSizeEx((*allocator_ptr)->hFile, &allocator_t->liFileSize)) {
        CloseHandle((*allocator_ptr)->hFile);
        return FILE_ST_ERROR;
    }
    switch (settings->open_type) {
        case FILE_OPEN_EXIST: {
            if ((*allocator_ptr)->liFileSize.QuadPart < PAGE_SIZE) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_WRONG_FORMAT;
            }
            allocator_t->hMap = CreateFileMapping(
                    allocator_t->hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == allocator_t->hMap) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            allocator_t->header_page = allocator_map_page(allocator_t, 0);
            if (allocator_t->header_page == NULL) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            file_h *header = allocator_get_header(allocator_t);
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
            allocator_t->liFileSize.QuadPart = new_size;
            if (!SetEndOfFile((*allocator_ptr)->hFile)) {
                return FILE_ST_ERROR;
            }
            allocator_t->hMap = CreateFileMapping(
                    allocator_t->hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    0,
                    NULL);
            if (0 == allocator_t->hMap) {
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            allocator_t->header_page = allocator_map_page(allocator_t, 0);
            if (allocator_t->header_page == NULL) {
                CloseHandle((*allocator_ptr)->hMap);
                CloseHandle((*allocator_ptr)->hFile);
                return FILE_ST_ERROR;
            }
            file_h *header = allocator_get_header(*allocator_ptr);
            *header = (file_h) {0};
            header->magic = MAGIC;
            for (offset_t file_offset = PAGE_SIZE; file_offset < new_size; file_offset += PAGE_SIZE) {
                if (ALLOCATOR_SUCCESS != allocator_return_page(allocator_t, file_offset)) {
                    return FILE_ST_ERROR;
                }
            }
            break;
        }
    }
    return FILE_ST_OK;
}

file_status allocator_free(allocator_t *allocator_t) {
    if (allocator_unmap_page(allocator_t, allocator_t->header_page) != ALLOCATOR_SUCCESS) {
        allocator_clear_list(allocator_t);
        CloseHandle(allocator_t->hMap);
        CloseHandle(allocator_t->hFile);
        free(allocator_t);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (allocator_clear_list(allocator_t) != ALLOCATOR_SUCCESS) {
        CloseHandle(allocator_t->hMap);
        CloseHandle(allocator_t->hFile);
        free(allocator_t);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (!CloseHandle(allocator_t->hMap)) {
        CloseHandle(allocator_t->hFile);
        free(allocator_t);
        return FILE_ST_UNABLE_RELEASE;
    }
    if (!CloseHandle(allocator_t->hFile)) {
        free(allocator_t);
        return FILE_ST_UNABLE_RELEASE;
    }
    free(allocator_t);
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
    if (allocator_t->hMap == INVALID_HANDLE_VALUE) {
        return ALLOCATOR_UNABLE_MAP;
    }
    block_list_it it;
    block_list_iterator(&allocator_t->block_list, &it);
    while (!block_list_iterator_is_empty(&it)) {
        block *block_ptr = map_block(allocator_t, it.node->file_offset);
        if (block_ptr == NULL) {
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
    uint32_t added = n - count;
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
    if (fill_mapping(allocator_t) != ALLOCATOR_SUCCESS) {
        return ALLOCATOR_UNABLE_MAP;
    }
    allocator_t->liFileSize.QuadPart = new_size;
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

static offset_t sys_granularity() {
    return getpagesize();
}

static allocator_result unmap_block(block *block) {
    if (munmap(block->ptr, granularity) == 0) {
        return ALLOCATOR_SUCCESS;
    }
    return ALLOCATOR_UNABLE_UNMAP;
}

static block *map_block(allocator_t *allocator, offset_t offset) {
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
    return &(allocator->list);
}

static file_h *allocator_get_header(allocator_t *allocator) {
    return (file_h *) page_ptr(allocator->header_page);
}

file_status allocator_init(file_settings *settings, allocator_t **allocator_ptr) {
    *allocator_ptr = malloc(PAGE_SIZE);
    if (NULL == allocator_ptr) {
        return FILE_ST_ERROR;
    }
    **allocator_ptr = (allocator_t) {0};
    allocator_t *allocator = *allocator_ptr;
    switch (settings->open_type) {
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
    switch (settings->open_type) {
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
    free(allocator);
    return FILE_ST_OK;
}

allocator_result allocator_reserve_pages(allocator_t *allocator, uint32_t n) {
    if (allocator_get_header(allocator)->free_pages_count >= n) {
        return ALLOCATOR_SUCCESS;
    }
    uint32_t added = n - allocator_get_header(allocator)->free_pages_count;
    offset_t old_size = allocator->file_size;
    offset_t new_size = old_size + (PAGE_SIZE * added);
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
