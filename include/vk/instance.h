/**
 * @file include/vk/instance.h
 * @brief Encapsulates the Vulkan instance and associated allocation state.
 */

#ifndef VKC_INSTANCE_H
#define VKC_INSTANCE_H

#include "map/linear.h"

#include <vulkan/vulkan.h>

typedef struct VkcInstance {
    HashMap* map;
    VkInstance object;
    VkAllocationCallbacks allocator;
    VkApplicationInfo app_info;
    VkInstanceCreateInfo info;
    uint32_t version;
} VkcInstance;

/**
 * @brief Creates a Vulkan instance with internal allocator and metadata.
 *
 * @param map_size Initial size of the allocator's hash map.
 * @return Pointer to the created instance, or NULL on failure.
 */
VkcInstance* vkc_instance_create(size_t map_size);

void vkc_instance_destroy(VkcInstance* instance);

#endif // VKC_INSTANCE_H
