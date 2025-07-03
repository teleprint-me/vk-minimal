/**
 * @file include/vk/device.h
 */

#ifndef VKC_DEVICE_H
#define VKC_DEVICE_H

#include "allocator/page.h"
#include "vk/instance.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Physical Device List
 * @{
 */

typedef struct VkcDeviceList {
    PageAllocator* allocator;
    VkPhysicalDevice* devices;
    uint32_t count;
} VkcDeviceList;

VkcDeviceList* vkc_device_list_create(VkcInstance* instance);
void vkc_device_list_free(VkcDeviceList* list);

/** @} */

/**
 * @defgroup Physical Queue Family Properties
 * @{
 */

typedef struct VkcDeviceQueueFamily {
    PageAllocator* allocator;
    VkQueueFamilyProperties* properties;
    uint32_t count;
} VkcDeviceQueueFamily;

VkcDeviceQueueFamily* vkc_device_queue_family_create(VkPhysicalDevice device);
void vkc_device_queue_family_free(VkcDeviceQueueFamily* family);

/** @} */

typedef struct VkcQueue {
    VkQueue object;
    VkQueueFamilyProperties family_properties;
    float* priorities;
    uint32_t family_index;
    uint32_t index;
    uint32_t count;
} VkcQueue;

typedef struct VkcPhysicalDevice {
    VkPhysicalDevice object;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceType type;
} VkcPhysicalDevice;

typedef struct VkcDevice {
    PageAllocator* allocator;
    VkcQueue* queue;
    VkcPhysicalDevice* physical;
    VkDevice object;
    VkAllocationCallbacks callbacks;
} VkcDevice;

VkcDevice* vkc_device_create(VkcInstance* instance, size_t page_size);
void vkc_device_destroy(VkcDevice* device);

#ifdef __cplusplus
}
#endif

#endif // VKC_DEVICE_H
