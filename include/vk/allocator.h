/**
 * @file include/vk/allocator.h
 * @brief A wrapper for interfacing with Vulkan Host Memory.
 */

#ifndef VK_ALLOC_H
#define VK_ALLOC_H

#include "map/linear.h"

#include <vulkan/vulkan.h>

VkAllocationCallbacks VKAPI_CALL vkc_hash_callbacks(HashMap* map);

#endif // VK_ALLOC_H
