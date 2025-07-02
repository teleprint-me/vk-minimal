/**
 * @file examples/device.c
 */

#include "core/logger.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct VkcPhysicalDeviceList {
    PageAllocator* pager;
    VkPhysicalDevice* devices;
    uint32_t count;
} VkcPhysicalDeviceList;

VkcPhysicalDeviceList* vkc_physical_device_list_create(VkcInstance* instance) {
    PageAllocator* pager = page_allocator_create(8);
    if (!pager) {
        return NULL;
    }

    VkcPhysicalDeviceList* list = page_malloc(pager, sizeof(*list), alignof(*list));
    if (!list) {
        return NULL;
    }

    *list = (VkcPhysicalDeviceList) {
        .pager = pager,
        .devices = NULL,
        .count = 0,
    };

    VkResult result = vkEnumeratePhysicalDevices(instance->object, &list->count, NULL);
    if (VK_SUCCESS != result || 0 == list->count) {
        LOG_ERROR(
            "[VkPhysicalDevice] No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            list->count
        );
        return NULL;
    }

    list->devices = page_malloc(
        pager,
        list->count * sizeof(VkPhysicalDevice),
        alignof(VkPhysicalDevice)
    );
    if (NULL == list->devices) {
        LOG_ERROR("[VkPhysicalDevice] Failed to allocate device list.");
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(instance->object, &list->count, list->devices);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkPhysicalDevice] Failed to enumerate devices (VkResult: %d)", result);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcPhysicalDeviceList] Found %u devices.", list->count);
    for (uint32_t i = 0; i < list->count; i++) {
        VkPhysicalDevice device = list->devices[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);    
        LOG_DEBUG("[VkcPhysicalDeviceList] i=%u, name=%s, type=%d", 
            i, properties.deviceName, (int) properties.deviceType
        );
    }
#endif

    return list;
}

void vkc_physical_device_list_free(VkcPhysicalDeviceList* list) {
    if (list && list->pager) {
        page_allocator_free(list->pager);
    }
}

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

    VkcPhysicalDeviceList* device_list = vkc_physical_device_list_create(instance);
    if (!device_list) {
        goto cleanup_instance;
    }

    /**
     * @name Clean up on Success
     * @{
     */

    // Device List
    vkc_physical_device_list_free(device_list);

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
