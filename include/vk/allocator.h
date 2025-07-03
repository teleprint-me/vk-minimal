/**
 * @file include/vk/allocator.h
 * @brief Vulkan Host Memory Allocator using a tracked page map.
 *
 * This interface manages Vulkan host memory allocations using an internal,
 * thread-safe PageAllocator. It provides a global allocation context which
 * can be safely passed to Vulkan interfaces.
 *
 * Use `vkc_allocator_callbacks()` to obtain a Vulkan-compatible callback struct.
 * Use `vkc_allocator_get()` to manually allocate through the internal allocator.
 */

#ifndef VKC_ALLOCATOR_H
#define VKC_ALLOCATOR_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the global Vulkan allocation context.
 *
 * Allocates and initializes the internal PageAllocator and callback bindings.
 * Must be called before using vkc_allocator_get() or vkc_allocator_callbacks().
 *
 * @return true on success, false on failure
 */
bool vkc_allocator_create(void);

/**
 * @brief Destroy the global allocation context and free all memory.
 */
bool vkc_allocator_destroy(void);

/**
 * @brief Get the internal PageAllocator used for Vulkan memory.
 */
PageAllocator* vkc_allocator_get(void);

/**
 * @brief Get the Vulkan-compatible allocation callbacks.
 */
const VkAllocationCallbacks* vkc_allocator_callbacks(void);

#ifdef __cplusplus
}
#endif

#endif // VKC_ALLOCATOR_H
