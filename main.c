#include <stdio.h>

#ifdef _WIN32

#include <windows.h>


typedef SIZE_T size_t;

void* alloc(size_t size);

#else // UNIX

#include <sys/mman.h>


typedef size_t sizeT;

void* alloc(sizeT size);

#endif

int main() {
    size_t size = 2000;
    void* data = alloc(size);
    printf("Hello, World!\n");
    return 0;
}
