/**
 * Copyright © 2024 Austin Berrio
 *
 * @file src/core/memory.c
 * @brief Utility functions for memory alignment, padding, and allocation.
 *
 * Provides helper functions to:
 * - Check power-of-two properties
 * - Determine alignment of addresses or sizes
 * - Calculate padding and aligned sizes
 * - Allocate aligned memory blocks with posix_memalign
 */

#include "core/memory.h"

#include <string.h>

uintptr_t memory_bitwise_offset(uintptr_t x, uintptr_t y) {
    assert(y > 0);
    return x & (y - 1);
}

bool memory_is_power_of_two(uintptr_t x) {
    return x != 0 && memory_bitwise_offset(x, x) == 0;
}

bool memory_is_aligned(uintptr_t x, uintptr_t alignment) {
    assert(memory_is_power_of_two(alignment));
    return memory_bitwise_offset(x, alignment) == 0;
}

uintptr_t memory_next_aligned_address(uintptr_t address, uintptr_t alignment) {
    assert(memory_is_power_of_two(alignment));
    uintptr_t offset = memory_bitwise_offset(address, alignment);
    return (offset != 0) ? address + alignment - offset : address;
}

size_t memory_padding_needed(uintptr_t address, size_t alignment) {
    assert(memory_is_power_of_two(alignment));
    size_t offset = memory_bitwise_offset(address, alignment);
    return (offset != 0) ? alignment - offset : 0;
}

uintptr_t memory_aligned_size(uintptr_t x, uintptr_t alignment) {
    assert(memory_is_power_of_two(alignment));
    return (x + alignment - 1) & ~(alignment - 1);
}

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

void* memory_aligned_calloc(size_t n, size_t size, size_t alignment) {
    size_t total = n * size;
    void* address = memory_aligned_alloc(total, alignment);
    if (address && memset(address, 0, total)) {
        return address;
    }

    return NULL;
}
