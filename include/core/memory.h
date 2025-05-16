/**
 * Copyright © 2024 Austin Berrio
 *
 * @file include/core/memory.h
 * @brief Utility functions for memory alignment, padding, and allocation.
 *
 * Provides helper functions to:
 * - Check power-of-two properties
 * - Determine alignment of addresses or sizes
 * - Calculate padding and aligned sizes
 * - Allocate aligned memory blocks with posix_memalign
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

/**
 * @brief Default memory alignment (8 bytes).
 */
#define MEMORY_ALIGNMENT 8

/**
 * @name Alignment Utilities
 * @{
 */

/**
 * @brief Computes x modulo y, optimized for when y is a power of two.
 *
 * @param x The value (or address) to compute offset for.
 * @param y The alignment boundary; must be greater than zero.
 * @return The remainder of x divided by y (x % y).
 */
uintptr_t memory_bitwise_offset(uintptr_t x, uintptr_t y);

/**
 * @brief Checks if a value is a nonzero power of two.
 *
 * @param x Value to test.
 * @return true if x is a power of two and not zero, false otherwise.
 */
bool memory_is_power_of_two(uintptr_t x);

/**
 * @brief Checks if a value or address is aligned to the given boundary.
 *
 * @param x The value or address to check.
 * @param alignment Alignment boundary; must be a power of two.
 * @return true if x is aligned to alignment, false otherwise.
 */
bool memory_is_aligned(uintptr_t x, uintptr_t alignment);

/**
 * @brief Rounds up an address to the next aligned boundary.
 *
 * If the address is already aligned, it is returned unchanged.
 *
 * @param address Starting address.
 * @param alignment Alignment boundary; must be a power of two.
 * @return The next aligned address >= address.
 */
uintptr_t memory_next_aligned_address(uintptr_t address, uintptr_t alignment);

/**
 * @brief Computes the number of bytes needed to pad an address up to alignment.
 *
 * @param address The current address.
 * @param alignment Alignment boundary; must be a power of two.
 * @return Number of bytes to add to address to align it.
 */
size_t memory_padding_needed(uintptr_t address, size_t alignment);

/**
 * @brief Rounds a size up to the nearest multiple of alignment.
 *
 * @param x Size (or address) to round.
 * @param alignment Alignment boundary; must be a power of two.
 * @return Rounded size, the smallest multiple of alignment >= x.
 */
uintptr_t memory_aligned_size(uintptr_t x, uintptr_t alignment);

/** @} */

/**
 * @name Aligned Memory Allocation
 * @{
 */

/**
 * @brief Allocates size bytes of memory aligned to alignment bytes.
 *
 * Uses posix_memalign internally.
 * The returned pointer must be freed with free().
 *
 * @param size Number of bytes to allocate.
 * @param alignment Alignment boundary; must be a power of two and >= sizeof(void *).
 * @return Pointer to allocated memory on success, NULL on failure.
 */
void* memory_aligned_alloc(size_t size, size_t alignment);

/**
 * @brief Allocates zero-initialized memory for an array with alignment.
 *
 * Equivalent to calloc but aligned.
 *
 * @param n Number of elements.
 * @param size Size of each element.
 * @param alignment Alignment boundary; must be a power of two.
 * @return Pointer to zeroed memory on success, NULL on failure.
 */
void* memory_aligned_calloc(size_t n, size_t size, size_t alignment);

/** @} */

#endif // MEMORY_H
