/**
 * @file include/vk/instance.h
 * @brief High-level interface for creating and managing a Vulkan instance.
 *
 * This module encapsulates the creation, allocation, and destruction of a Vulkan `VkInstance`
 * using a custom internal memory allocator. It provides a minimal, self-contained interface
 * suitable for integrating into engines, compute frameworks, or debugging layers.
 *
 * Internally, all Vulkan host allocations are tracked using a linear-probing hash map.
 * Allocation metadata (size, alignment, scope) is preserved per pointer and released automatically
 * on destruction. This simplifies debugging, leak detection, and allocator introspection.
 */

#ifndef VKC_INSTANCE_H
#define VKC_INSTANCE_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque wrapper around a Vulkan instance and its allocator state.
 *
 * This structure encapsulates:
 * - A tracked hash map of host allocations (`pager`)
 * - The actual Vulkan instance object (`object`)
 * - The active allocation callbacks (`allocator`)
 * - Application and instance creation metadata
 *
 * Users typically interact only with `vkc_instance_create()` and `vkc_instance_destroy()`.
 * The struct contents are exposed for introspection, but should not be modified directly.
 */
typedef struct VkcInstance {
    PageAllocator* pager; /**< Internal allocation map (address → metadata). */
    VkInstance object; /**< Vulkan instance handle. */
    VkAllocationCallbacks allocator; /**< Vulkan allocator callbacks for tracked memory. */
    VkApplicationInfo app_info; /**< Application-level metadata used during creation. */
    VkInstanceCreateInfo info; /**< VkInstanceCreateInfo used to initialize the instance. */
    uint32_t version; /**< Vulkan API version negotiated at runtime. */
} VkcInstance;

/**
 * @brief Creates a Vulkan instance with internal memory tracking.
 *
 * This function:
 * - Creates a new hash map for allocation tracking
 * - Registers the custom allocator with Vulkan
 * - Initializes application metadata and extensions
 * - Creates the Vulkan instance
 *
 * @param page_size Initial capacity of the allocation map.
 * @return A pointer to a fully initialized `VkcInstance`, or NULL on failure.
 *
 * @note On success, the returned instance must be destroyed via `vkc_instance_destroy()`.
 * @note The hash map is owned internally and freed automatically.
 */
VkcInstance* vkc_instance_create(size_t page_size);

/**
 * @brief Destroys the Vulkan instance and releases all associated resources.
 *
 * This function:
 * - Calls `vkDestroyInstance()` with the internal allocator
 * - Frees all tracked memory metadata in the allocation map
 * - Releases internal storage used by the `VkcInstance` object
 *
 * @param instance Pointer to a `VkcInstance` created with `vkc_instance_create()`.
 */
void vkc_instance_destroy(VkcInstance* instance);

#ifdef __cplusplus
}
#endif

#endif // VKC_INSTANCE_H
