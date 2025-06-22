/**
 * @file include/vk/allocator.h
 * @brief Vulkan Host Memory Allocator using a tracked page map.
 *
 * Provides a minimal Vulkan-compatible allocation interface using the internal
 * PageAllocator for tracked host memory. This wrapper allows Vulkan to
 * allocate, reallocate, and free host-visible memory using your own memory system.
 *
 * The interface is intended to be passed to Vulkan via `VkInstanceCreateInfo`
 * and `VkDeviceCreateInfo` through the `pAllocator` field. It tracks allocations
 * internally and allows for introspection, debugging, or leak detection
 * using your existing page allocator.
 */

#ifndef VKC_ALLOCATOR_H
#define VKC_ALLOCATOR_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns a Vulkan allocation callback structure using a custom page allocator.
 *
 * This function constructs a `VkAllocationCallbacks` struct which Vulkan can use
 * for all host memory allocations. All allocations are routed through the provided
 * `PageAllocator` instance, allowing you to track and manage memory usage explicitly.
 *
 * @param allocator A pointer to an initialized `PageAllocator` instance.
 * @return A fully populated `VkAllocationCallbacks` struct referencing your allocator.
 */
VkAllocationCallbacks VKAPI_CALL vkc_hash_callbacks(PageAllocator* allocator);

#ifdef __cplusplus
}
#endif

#endif // VKC_ALLOCATOR_H
