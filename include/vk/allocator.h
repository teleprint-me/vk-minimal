/**
 * @file include/vk/allocator.h
 * @brief Vulkan Host Memory Allocator using a tracked page map.
 */

#ifndef VKC_ALLOCATOR_H
#define VKC_ALLOCATOR_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>

VkAllocationCallbacks VKAPI_CALL vkc_hash_callbacks(PageAllocator* allocator);

#endif // VKC_ALLOCATOR_H
