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
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties; // optional
    VkDevice logical;
    VkQueue queue;
    uint32_t queue_family_index;
    VkAllocationCallbacks allocator;
} VkcDevice;

VkcDevice* vkc_device_create(VkcInstance* instance, size_t page_size);
void vkc_device_destroy(VkcDevice* device);

#endif // VKC_DEVICE_H
