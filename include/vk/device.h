/**
 * @file include/vk/device.h
 */

#ifndef VKC_DEVICE_H
#define VKC_DEVICE_H

#include "allocator/page.h"
#include "vk/instance.h"
#include <vulkan/vulkan.h>

typedef struct VkcDevice {
    PageAllocator* pager;

    VkPhysicalDevice physical;
    VkDevice logical;
    VkQueue queue;

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkAllocationCallbacks allocator;

    uint32_t queue_family_index;
} VkcDevice;

VkcDevice* vkc_device_create(VkcInstance* instance, size_t page_size);
void vkc_device_destroy(VkcDevice* device);

#endif // VKC_DEVICE_H
