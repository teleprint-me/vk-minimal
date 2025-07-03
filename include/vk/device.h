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
 * @defgroup DeviceList Physical Device List
 * @{
 */

typedef struct VkcDeviceList {
    VkPhysicalDevice* devices;
    uint32_t count;
} VkcDeviceList;

VkcDeviceList* vkc_device_list_create(VkInstance instance);
void vkc_device_list_free(VkcDeviceList* list);

/** @} */

/**
 * @defgroup DeviceQueueFamily Queue Family Properties
 * @{
 */

typedef struct VkcDeviceQueueFamily {
    VkQueueFamilyProperties* properties;
    uint32_t count;
} VkcDeviceQueueFamily;

VkcDeviceQueueFamily* vkc_device_queue_family_create(VkPhysicalDevice device);
void vkc_device_queue_family_free(VkcDeviceQueueFamily* family);

/** @} */

/**
 * @defgroup DeviceSelection Physical Device Selection
 * @{
 */

typedef struct VkcPhysicalDevice {
    VkPhysicalDevice object;
    uint32_t queue_family_index;
} VkcPhysicalDevice;

VkcPhysicalDevice* vkc_device_physical_create(VkcDeviceList* list);
void vkc_device_physical_free(VkcPhysicalDevice* device);

/** @} */

/**
 * @defgroup DeviceLayerQuery Device Layer Query
 * @{
 */

typedef struct VkcDeviceLayer {
    VkLayerProperties* properties;
    uint32_t count;
} VkcDeviceLayer;

VkcDeviceLayer* vkc_device_layer_create(VkPhysicalDevice device);
void vkc_device_layer_free(VkcDeviceLayer* layer);

/** @} */

/**
 * @defgroup DeviceLayerMatch Device Layer Match
 * @{
 */

typedef struct VkcDeviceLayerMatch {
    char** names;
    uint32_t count;
} VkcDeviceLayerMatch;

VkcDeviceLayerMatch* vkc_device_layer_match_create(
    VkcDeviceLayer* layer, const char* const* names, const uint32_t name_count);
void vkc_device_layer_match_free(VkcDeviceLayerMatch* match);

/** @} */

/**
 * @defgroup DeviceExtensionQuery Device Extension Query
 * @{
 */

typedef struct VkcDeviceExtension {
    VkExtensionProperties* properties;
    uint32_t count;
} VkcDeviceExtension;

VkcDeviceExtension* vkc_device_extension_create(VkPhysicalDevice device);
void vkc_device_extension_free(VkcDeviceExtension* extension);

/** @} */

/**
 * @defgroup DeviceExtensionMatch Device Extension Match
 * @{
 */

typedef struct VkcDeviceExtensionMatch {
    char** names;
    uint32_t count;
} VkcDeviceExtensionMatch;

VkcDeviceExtensionMatch* vkc_device_extension_match_create(
    VkcDeviceExtension* extension, const char* const* names, const uint32_t name_count);
void vkc_device_extension_match_free(VkcDeviceExtensionMatch* match);

/** @} */

/**
 * @defgroup DeviceCore Logical Device Wrapper
 * @{
 */

typedef struct VkcDevice {
    VkDevice object;
    VkAllocationCallbacks callbacks;
} VkcDevice;

VkcDevice* vkc_device_create(VkcInstance* instance);
void vkc_device_destroy(VkcDevice* device);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // VKC_DEVICE_H
