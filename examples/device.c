/**
 * @file examples/device.c
 * 
 * VkcDevice Setup Flow:
 *
 *   - VkcDeviceList         ← Enumerate VkPhysicalDevice
 *   - VkcDeviceQueueFamily  ← For each VkPhysicalDevice, find usable queues
 *   - VkcDeviceLayer        ← Optional: enumerate & match device validation layers
 *   - VkcDeviceExtension    ← Optional: enumerate & match device extensions
 *   - vkc_physical_device_select() ← Selects one based on VK_QUEUE_COMPUTE_BIT
 */

#include "core/logger.h"

#include "vk/instance.h"
#include "vk/device.h"

#include <stdlib.h>
#include <stdio.h>

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

    VkcDeviceList* device_list = vkc_device_list_create(instance->object);
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
