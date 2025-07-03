/**
 * @file include/vk/device.h
 * 
 * VkcDevice Setup Flow:
 *
 *   - VkcDeviceList         ← Enumerate VkPhysicalDevice
 *   - VkcDeviceQueueFamily  ← For each VkPhysicalDevice, find usable queues
 *   - VkcDeviceLayer        ← Optional: enumerate & match device validation layers
 *   - VkcDeviceExtension    ← Optional: enumerate & match device extensions
 *   - vkc_physical_device_select() ← Selects one based on VK_QUEUE_COMPUTE_BIT
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

VkcDeviceList* vkc_device_list_create(VkInstance instance);
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

/**
 * @defgroup Physical Device Selection
 * @{
 */

bool vkc_physical_device_select(VkcDevice* device, VkcDeviceList* list);

/** @} */

/**
 * @defgroup Device Layer Query
 * @{
 */

typedef struct VkcDeviceLayer {
    PageAllocator* allocator;
    VkLayerProperties* properties;
    uint32_t count;
} VkcDeviceLayer;

VkcDeviceLayer* vkc_device_layer_create(VkPhysicalDevice device);
void vkc_device_layer_free(VkcDeviceLayer* layer);

/** @} */

/**
 * @defgroup Device Layer Match and Filter
 * @{
 */

typedef struct VkcDeviceLayerMatch {
    PageAllocator* allocator;
    char** names;
    uint32_t count;
} VkcDeviceLayerMatch;

VkcDeviceLayerMatch* vkc_device_layer_match_create(
    VkcDeviceLayer* layer, const char* const* names, const uint32_t name_count);
void vkc_device_layer_match_free(VkcDeviceLayerMatch* match);

/** @} */

/**
 * @defgroup
 * @{
 */

typedef struct VkcDeviceExtension {
    PageAllocator* allocator;
    VkExtensionProperties* properties;
    uint32_t count;
} VkcDeviceExtension;

VkcDeviceExtension* vkc_device_extension_create(VkPhysicalDevice device);
void vkc_device_extension_free(VkcDeviceExtension* extension);

/** @} */

/**
 * @defgroup
 * @{
 */

typedef struct VkcDeviceExtensionMatch {
    PageAllocator* allocator;
    char** names;
    uint32_t count;
} VkcDeviceExtensionMatch;

VkcDeviceExtensionMatch* vkc_device_extension_match_create(
    VkcDeviceExtension* extension, const char* const* names, const uint32_t name_count);
void vkc_device_extension_match_free(VkcDeviceLayerMatch* match);

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

VkcDevice* vkc_device_create(VkcInstance* instance);
void vkc_device_destroy(VkcDevice* device);

#ifdef __cplusplus
}
#endif

#endif // VKC_DEVICE_H
