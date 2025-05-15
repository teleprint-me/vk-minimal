/**
 * Copyright © 2024 Austin Berrio
 *
 * @file include/core/memory.h
 * @brief Utility functions for computing memory alignment and padding.
 *
 * This module provides functions to check for power-of-two alignments,
 * determine whether an address is aligned, round addresses up to a given alignment,
 * and calculate the padding needed.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdalign.h>
#include <malloc.h>
#include <assert.h>

#define MEMORY_ALIGNMENT 8

/**
 * @brief Computes x modulo y, optimized for y being a power of two.
 *
 * @param x The number (or address) to compute the offset from.
 * @param y The alignment value; must be greater than 0.
 * @return The remainder when x is divided by y.
 */
static inline uintptr_t memory_bitwise_offset(uintptr_t x, uintptr_t y) {
    assert(y > 0);
    return x & (y - 1);
}

/**
 * @brief Checks if a number is a power of two.
 *
 * @param x The value to check.
 * @return true if x is a nonzero power of two, false otherwise.
 */
static inline bool memory_is_power_of_two(uintptr_t x) {
    return x != 0 && memory_bitwise_offset(x, x) == 0;
}

/**
 * @brief Determines if x is aligned to a given alignment.
 *
 * @param x The address or value to check.
 * @param alignment The alignment boundary (must be a power of two).
 * @return true if x is aligned to alignment, false otherwise.
 */
static inline bool memory_is_aligned(uintptr_t x, uintptr_t alignment) {
    assert(memory_is_power_of_two(alignment));
    return memory_bitwise_offset(x, alignment) == 0;
}

/**
 * @brief Rounds up an address to the next multiple of alignment.
 *
 * @param address The starting address.
 * @param alignment The alignment boundary (must be a power of two).
 * @return The next address that is aligned to the specified boundary.
 */
static inline uintptr_t memory_next_aligned_address(uintptr_t address, uintptr_t alignment) {
    assert(memory_is_power_of_two(alignment));
    uintptr_t offset = memory_bitwise_offset(address, alignment);
    return (offset != 0) ? address + alignment - offset : address;
}

/**
 * @brief Computes the number of bytes needed to pad address up to alignment.
 *
 * @param address The current address.
 * @param alignment The alignment boundary (must be a power of two).
 * @return The number of bytes to add to address to achieve alignment.
 */
static inline size_t memory_padding_needed(uintptr_t address, size_t alignment) {
    assert(memory_is_power_of_two(alignment));
    size_t offset = memory_bitwise_offset(address, alignment);
    return (offset != 0) ? alignment - offset : 0;
}

/**
 * @brief Computes the size of x rounded up to the nearest multiple of alignment.
 *
 * @param x The size (or address) to be rounded up.
 * @param alignment The alignment boundary (must be a power of two).
 * @return The size rounded up to the next multiple of alignment.
 */
static inline uintptr_t memory_aligned_size(uintptr_t x, uintptr_t alignment) {
    assert(memory_is_power_of_two(alignment));
    return (x + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief Allocates aligned memory using posix_memalign.
 *
 * The returned pointer must be freed using free().
 *
 * @param size The size of the memory block in bytes.
 * @param alignment The alignment boundary (must be a power of two).
 * @return A pointer to the allocated memory, or NULL if allocation fails.
 */
void* memory_aligned_alloc(size_t size, size_t alignment);

#endif // MEMORY_H
