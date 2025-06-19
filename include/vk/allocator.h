/**
 * @file include/vk/allocator.h
 * @brief Vulkan Host Memory Allocator using a tracked page map.
 *
 * This interface provides a custom Vulkan-compatible memory allocator
 * that tracks all host allocations using a linear probing hash map.
 *
 * The allocator is designed for introspection, debugging, and compliance
 * with Vulkan's `VkAllocationCallbacks` system. Each allocation is
 * tracked via address → metadata mapping (size, alignment, scope),
 * enabling safe `realloc()` and memory lifecycle analysis.
 *
 * Usage:
 *   1. Create a HashMap with key type `HASH_MAP_KEY_TYPE_ADDRESS`.
 *   2. Pass the map to `vkc_hash_callbacks()` to generate a valid
 *      `VkAllocationCallbacks` structure.
 *   3. Use the returned callbacks in Vulkan instance or device creation.
 *
 * Example:
 *   HashMap* map = hash_map_create(1024, HASH_MAP_KEY_TYPE_ADDRESS);
 *   VkAllocationCallbacks alloc = vkc_hash_callbacks(map);
 *   vkCreateInstance(&info, &alloc, &instance);
 *
 * The user is responsible for freeing the map after all Vulkan resources
 * using the allocator have been destroyed.
 *
 * Example:
 *   hash_map_free(map);
 */

#ifndef VKC_ALLOCATOR_H
#define VKC_ALLOCATOR_H

#include "map/linear.h"
#include <vulkan/vulkan.h>

/**
 * @brief Returns a VkAllocationCallbacks structure backed by a hash map.
 *
 * This function creates a Vulkan-compliant allocation interface that tracks
 * all allocation, reallocation, and deallocation operations using a custom
 * page table built on a hash map. Each memory allocation is logged and
 * metadata is maintained for safety and debugging.
 *
 * @param map A pointer to a `HashMap*` created with `HASH_MAP_KEY_TYPE_ADDRESS`.
 *            This map is used to store per-allocation metadata.
 * @return A fully initialized `VkAllocationCallbacks` structure. The map is
 *         passed through as `pUserData` and used internally by all callbacks.
 *
 * @warning The caller must ensure the provided map remains valid for the
 *          entire lifetime of the Vulkan object(s) using the allocator.
 * @warning Do not share the same map across multiple Vulkan instances or
 *          devices unless you implement external synchronization.
 */
VkAllocationCallbacks VKAPI_CALL vkc_hash_callbacks(HashMap* map);

#endif // VKC_ALLOCATOR_H
