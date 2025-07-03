/**
 * @file examples/instance.c
 */

#include "core/logger.h"
#include "vk/allocator.h"
#include "vk/instance.h"

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
     * @name Initialize Global Allocators
     * @{
     */

    if (!vkc_allocator_create()) {
        return EXIT_FAILURE;
    }

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
        goto cleanup_properties;
    }

    /** @} */


    /**
     * @name Clean up on Success
     * @{
     */

    vkc_instance_extension_match_free(extension_match);
    vkc_instance_extension_free(extension);
    vkc_instance_layer_match_free(layer_match);
    vkc_instance_layer_free(layer);
    vkc_instance_free(instance);

    if (!vkc_allocator_destroy()) {
        return EXIT_FAILURE;
    }

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

cleanup_properties:
    vkc_instance_extension_match_free(extension_match);
    vkc_instance_extension_free(extension);
    vkc_instance_layer_match_free(layer_match);
    vkc_instance_layer_free(layer);
    vkc_allocator_destroy();

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkCompute] Debug Mode: Exit Failure");
#else
    LOG_INFO("[VkCompute] Release Mode: Exit Failure");
#endif

    return EXIT_FAILURE;

    /** @} */
}
