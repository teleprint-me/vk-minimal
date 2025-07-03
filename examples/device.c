/**
 * @file examples/device.c
 */

#include "core/logger.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * @name Device Structures
 * @{
 */

typedef struct VkcDeviceList {
    PageAllocator* allocator;
    VkPhysicalDevice* devices;
    uint32_t count;
} VkcDeviceList;

typedef struct VkcDeviceQueueFamily {
    PageAllocator* allocator;
    VkQueueFamilyProperties* properties;
    uint32_t count;
} VkcDeviceQueueFamily;

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

/** @} */

/**
 * @name Physical Device List
 * @{
 */

VkcDeviceList* vkc_device_list_create(VkcInstance* instance) {
    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        return NULL;
    }

    VkcDeviceList* list = page_malloc(allocator, sizeof(*list), alignof(*list));
    if (!list) {
        return NULL;
    }

    *list = (VkcDeviceList) {
        .allocator = allocator,
        .devices = NULL,
        .count = 0,
    };

    VkResult result = vkEnumeratePhysicalDevices(instance->object, &list->count, NULL);
    if (VK_SUCCESS != result || 0 == list->count) {
        LOG_ERROR(
            "[VkcDeviceList] No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            list->count
        );
        return NULL;
    }

    list->devices = page_malloc(
        allocator,
        list->count * sizeof(VkPhysicalDevice),
        alignof(VkPhysicalDevice)
    );
    if (NULL == list->devices) {
        LOG_ERROR("[VkcDeviceList] Failed to allocate device list.");
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(instance->object, &list->count, list->devices);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceList] Failed to enumerate devices (VkResult: %d)", result);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcDeviceList] Found %u devices.", list->count);
    for (uint32_t i = 0; i < list->count; i++) {
        VkPhysicalDevice device = list->devices[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);    
        LOG_DEBUG("[VkcDeviceList] i=%u, name=%s, type=%d", 
            i, properties.deviceName, (int) properties.deviceType
        );
    }
#endif

    return list;
}

void vkc_device_list_free(VkcDeviceList* list) {
    if (list && list->allocator) {
        page_allocator_free(list->allocator);
    }
}

/** @} */

/**
 * @name Queue Family Properties
 * @{
 */

VkcDeviceQueueFamily* vkc_device_queue_family_create(VkPhysicalDevice device) {
    if (!device) {
        LOG_ERROR("[VkcDeviceQueueFamily] Invalid physical device.");
        return NULL;
    }

    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to create allocator context.");
        return NULL;
    }

    VkcDeviceQueueFamily* family = page_malloc(allocator, sizeof(*family), alignof(*family));
    if (!family) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to allocate memory to family structure.");
        page_allocator_free(allocator);
        return NULL;
    }

    *family = (VkcDeviceQueueFamily) {
        .allocator = allocator,
        .properties = NULL,
        .count = 0,
    };

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family->count, NULL);

    if (0 == family->count) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to query family count.");
        page_allocator_free(allocator);
        return NULL;
    }

    family->properties = page_malloc(
        allocator,
        family->count * sizeof(VkQueueFamilyProperties),
        alignof(VkQueueFamilyProperties)
    );

    if (!family->properties) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to allocate memory for %u properties.", family->count);
        page_allocator_free(allocator);
        return NULL;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family->count, family->properties);
    return family;
}

void vkc_device_queue_family_free(VkcDeviceQueueFamily* family) {
    if (family && family->allocator) {
        page_allocator_free(family->allocator);
    }
}

/** @} */

/**
 * @name Physical Device Selection
 * @{
 */

bool vkc_physical_device_select(VkcDevice* device, VkcDeviceList* list) {
    static const VkPhysicalDeviceType type[] = {
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
    };

    static const uint32_t type_count = sizeof(type) / sizeof(VkPhysicalDeviceType);

    for (uint32_t i = 0; i < type_count; i++) {
        for (uint32_t j = 0; j < list->count; j++) {
            VkPhysicalDevice candidate = list->devices[j];
            VkPhysicalDeviceProperties properties = {0};
            vkGetPhysicalDeviceProperties(candidate, &properties);

            VkcDeviceQueueFamily* family = vkc_device_queue_family_create(candidate);
            if (!family) {
                return false;
            }

    #if defined(DEBUG) && (1 == DEBUG)
            LOG_DEBUG("[VkcPhysicalDevice] Candidate %u: %s, type=%d", j, properties.deviceName, properties.deviceType);
    #endif

            bool selected = false;
            for (uint32_t k = 0; k < family->count; k++) {
                if (family->properties[k].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    if (type[i] == properties.deviceType) {
                        device->queue->family_index = k;
                        device->queue->family_properties = family->properties[k];
                        device->physical->object = candidate;
                        device->physical->properties = properties;
                        device->physical->type = type[i];
                        vkGetPhysicalDeviceFeatures(candidate, &device->physical->features);
                        selected = true;
                        break;
                    }
                }
            }

            vkc_device_queue_family_free(family);
            if (selected) {
    #if defined(DEBUG) && (1 == DEBUG)
                LOG_DEBUG("[VkcPhysicalDevice] Selected %u: %s, type=%d", j, properties.deviceName, properties.deviceType);
    #endif
                return true;
            }
        }
    }

    LOG_ERROR("No suitable compute-capable discrete GPU found.");
    return false;
}

/** @} */

int main(void) {
    /**
     * @name Debug Environment
     * @brief Enables verbose logging when VKC_DEBUG=1 is set.
     * @{
     */

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkCompute] Debug mode.");
#else
    LOG_INFO("[VkCompute] Release mode.");
#endif

    /** @} */

    /**
     * @name Instance Layer Properties
     * @{
     */

    static const char* const validation_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    VkcInstanceLayer* layer = vkc_instance_layer_create();
    VkcInstanceLayerMatch* layer_match = vkc_instance_layer_match_create(
        layer, validation_layers, 1
    );

    /** @} */

    /**
     * @name Instance Extension Properties
     * @{
     */

    static const char* const extension_names[] = {
        "VK_KHR_device_group_creation",
        "VK_KHR_external_fence_capabilities",
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_external_semaphore_capabilities",
        "VK_KHR_get_physical_device_properties2",
        "VK_EXT_debug_utils",
    };

    VkcInstanceExtension* extension = vkc_instance_extension_create();
    VkcInstanceExtensionMatch* extension_match = vkc_instance_extension_match_create(
        extension, extension_names, 6
    );
    
    /** @} */

    /**
     * @name Vulkan Instance
     * @{
     */

    VkcInstance* instance = vkc_instance_create(layer_match, extension_match);
    if (!instance) {
        goto cleanup_instance_layer;
    }

    /** @} */

    VkcDeviceList* device_list = vkc_device_list_create(instance);
    if (!device_list) {
        goto cleanup_instance;
    }

    /**
     * @name Clean up on Success
     * @{
     */

    // Device List
    vkc_device_list_free(device_list);

    // Instance
    vkc_instance_free(instance);
    // Instance Extensions
    vkc_instance_extension_match_free(extension_match);
    vkc_instance_extension_free(extension);
    // Instance Layers
    vkc_instance_layer_match_free(layer_match);
    vkc_instance_layer_free(layer);

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkCompute] Debug Mode: Exit Success");
#else
    LOG_INFO("[VkCompute] Release Mode: Exit Success");
#endif

    return EXIT_SUCCESS;

    /** {@ */

    /**
     * @name Clean up on Failure
     * @{
     */

cleanup_instance: 
    vkc_instance_free(instance);

cleanup_instance_layer:
    vkc_instance_extension_match_free(extension_match);
    vkc_instance_extension_free(extension);
    vkc_instance_layer_match_free(layer_match);
    vkc_instance_layer_free(layer);

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkCompute] Debug Mode: Exit Failure");
#else
    LOG_INFO("[VkCompute] Release Mode: Exit Failure");
#endif

    return EXIT_FAILURE;

    /** @} */
}
