/**
 * @file include/memory.c
 */

#include "memory.h"

void* memory_aligned_alloc(size_t size, size_t alignment) {
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }

    if (!memory_is_power_of_two(alignment)) {
        return NULL;
    }

    void* address = NULL;
    if (posix_memalign(&address, alignment, size) != 0) {
        return NULL;
    }

    return address;
}
