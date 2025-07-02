/**
 * @file examples/instance.c
 */

#include "core/logger.h"
#include "utf8/raw.h"
#include "vk/allocator.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

// typedef struct VkcInstance {
//     PageAllocator* pager;
//     VkInstance object;
//     VkAllocationCallbacks allocator;
// } VkcInstance;

// void vkc_instance_init(VkcInstance* instance) {
//     uint32_t vkInstanceAPIVersion;
//     if (VK_SUCCESS != vkEnumerateInstanceVersion(&vkInstanceAPIVersion)) {
//         LOG_ERROR("Failed to enumerate instance API version.");
//         return 0;
//     }
    
//     VkApplicationInfo vkInstanceAppInfo = {
//         .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
//         .pApplicationName = "compute",
//         .applicationVersion = vkInstanceAPIVersion,
//         .pEngineName = "compute engine",
//         .engineVersion = vkInstanceAPIVersion,
//         .apiVersion = vkInstanceAPIVersion,
//     };

//     LOG_INFO("[VkApplicationInfo] Name: %s", vkInstanceAppInfo.pApplicationName);
//     LOG_INFO(
//         "[VkApplicationInfo] Version: %u.%u.%u",
//         VK_VERSION_MAJOR(vkInstanceAppInfo.applicationVersion),
//         VK_VERSION_MINOR(vkInstanceAppInfo.applicationVersion),
//         VK_VERSION_PATCH(vkInstanceAppInfo.applicationVersion)
//     );
//     LOG_INFO("[VkApplicationInfo] Engine Name: %s", vkInstanceAppInfo.pEngineName);
//     LOG_INFO(
//         "[VkApplicationInfo] Engine Version: %u.%u.%u",
//         VK_VERSION_MAJOR(vkInstanceAppInfo.engineVersion),
//         VK_VERSION_MINOR(vkInstanceAppInfo.engineVersion),
//         VK_VERSION_PATCH(vkInstanceAppInfo.engineVersion)
//     );
//     LOG_INFO(
//         "[VkApplicationInfo] API Version: %u.%u.%u",
//         VK_API_VERSION_MAJOR(vkInstanceAppInfo.apiVersion),
//         VK_API_VERSION_MINOR(vkInstanceAppInfo.apiVersion),
//         VK_API_VERSION_PATCH(vkInstanceAppInfo.apiVersion)
//     );

//     VkInstanceCreateInfo vkInstanceCreateInfo = {
//         .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
//         .pApplicationInfo = &vkInstanceAppInfo,
//     };

//     if (vkInstanceLayerPropertyFound) {
//         vkInstanceCreateInfo.enabledLayerCount = 1;
//         vkInstanceCreateInfo.ppEnabledLayerNames = vkInstanceLayerPropertyNames;
//     }

//     if (vkInstanceExtensionPropertyFound) {
//         vkInstanceCreateInfo.enabledExtensionCount = vkInstanceExtensionPropertyNameCount;
//         vkInstanceCreateInfo.ppEnabledExtensionNames = vkInstanceExtensionPropertyNames;
//     }

//     instance->object = VK_NULL_HANDLE;
//     result = vkCreateInstance(&vkInstanceCreateInfo, &vkAllocationCallback, &instance->object);
//     if (VK_SUCCESS != result) {
//         LOG_ERROR("[VkInstance] Failed to create instance object: %d", result);
//         return;
//     }

//     // Free dead weight
//     page_free(pager, vkInstanceLayerProperties);
//     page_free(pager, vkInstanceExtensionProperties);

//     LOG_INFO("[VkInstance] Created instance @ %p.", vkInstance);
// }

// VkcInstance* vkc_instance_create(size_t initial_size) {
//     PageAllocator* pager = page_allocator_create(initial_size);
//     if (NULL == pager) {
//         LOG_ERROR("[VkcInstance] Failed to create allocator with %zu initial size.", initial_size);
//         return NULL;
//     }

//     VkcInstance* instance = page_malloc(pager, sizeof(VkcInstance), alignof(VkcInstance));
//     if (NULL == instance) {
//         LOG_ERROR("[VkcInstance] Failed to create VkC instance with allocator @ %p.", pager);
//         page_allocator_free(pager);
//         return NULL;
//     }

//     *instance = (VkcInstance) {
//         .pager = pager,
//         .allocator = vkc_page_callbacks(pager),
//     };

//     return instance;
// }

// void vkc_instance_destroy(VkcInstance* instance) {
//     if (NULL == instance || NULL == instance->pager) return;
//     PageAllocator* pager = instance->pager;
//     page_allocator_free(pager);
// }

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
     * @name Memory Allocators
     * @{
     */

    PageAllocator* pager = page_allocator_create(1024);
    if (NULL == pager) {
        LOG_INFO("[PageAllocator] Failed to create allocator with %zu initial size.", 1024);
        return EXIT_FAILURE;
    }

    VkAllocationCallbacks vkAllocationCallback = vkc_page_callbacks(pager);

    /** @} */

    /**
     * @name Global Result
     * @{
     */

    VkResult result; // Result codes and handles

    /** @} */

    /**
     * @name Instance Layer Properties
     * @{
     */

    VkcInstanceLayer* instanceLayer = vkc_instance_layer_create();
    if (!instanceLayer) {
        goto cleanup_pager;
    }

    static const char* const validation_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    VkcInstanceLayerMatch* instanceLayerMatch = vkc_instance_layer_match_create(
        instanceLayer, validation_layers, 1
    );

    if (!instanceLayerMatch) {
        goto cleanup_instance_layer;
    }

    /** @} */

    /**
     * @name Instance Extension Properties
     * @{
     */

    uint32_t vkInstanceExtensionPropertyCount = 0;
    result = vkEnumerateInstanceExtensionProperties(NULL, &vkInstanceExtensionPropertyCount, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[InstanceExtensionProperties] Failed to enumerate instance extension property count.");
        goto cleanup_instance_layer_match;
    }

    VkExtensionProperties* vkInstanceExtensionProperties = page_malloc(
        pager,
        vkInstanceExtensionPropertyCount * sizeof(VkExtensionProperties),
        alignof(VkExtensionProperties)
    );
    if (NULL == vkInstanceExtensionProperties) {
        LOG_ERROR("[InstanceExtensionProperties] Failed to allocate %u instance extension property objects.", vkInstanceExtensionPropertyCount);
        goto cleanup_instance_layer_match;
    }
    memset(vkInstanceExtensionProperties, 0, vkInstanceExtensionPropertyCount * sizeof(VkExtensionProperties));

    result = vkEnumerateInstanceExtensionProperties(NULL, &vkInstanceExtensionPropertyCount, vkInstanceExtensionProperties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[InstanceExtensionProperties] Failed to enumerate instance extension properties.");
        goto cleanup_instance_layer_match;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log the results to standard output
    LOG_DEBUG("[InstanceExtensionProperties] Found %u instance extension properties.", vkInstanceExtensionPropertyCount);
    for (uint32_t i = 0; i < vkInstanceExtensionPropertyCount; i++) {
        LOG_DEBUG("[InstanceExtensionProperties] i=%u, name=%s, version=%u", 
            i, vkInstanceExtensionProperties[i].extensionName, vkInstanceExtensionProperties[i].specVersion
        );
    }
#endif

    uint32_t vkInstanceExtensionPropertyNameCount = 6;
    char const* vkInstanceExtensionPropertyNames[] = {
        "VK_KHR_device_group_creation",
        "VK_KHR_external_fence_capabilities",
        "VK_KHR_external_memory_capabilities",
        "VK_KHR_external_semaphore_capabilities",
        "VK_KHR_get_physical_device_properties2",
        "VK_EXT_debug_utils",
    };

    bool vkInstanceExtensionPropertyFound = true;
    for (uint32_t i = 0; i < vkInstanceExtensionPropertyNameCount; i++) {
        bool found = false;
        for (uint32_t j = 0; j < vkInstanceExtensionPropertyCount; j++) {
            if (0 == utf8_raw_compare(
                vkInstanceExtensionPropertyNames[i],
                vkInstanceExtensionProperties[j].extensionName)) {
                found = true;
                LOG_INFO("[InstanceCreateInfo] Enabling Extension: %s", vkInstanceExtensionPropertyNames[i]);
                break;
            }
        }

        if (!found) {
            LOG_WARN("[InstanceCreateInfo] Extension not available: %s", vkInstanceExtensionPropertyNames[i]);
            vkInstanceExtensionPropertyFound = false;
        }
    }

    /** @} */

    /**
     * @name Vulkan Instance
     * @{
     */

    uint32_t vkInstanceAPIVersion;
    if (VK_SUCCESS != vkEnumerateInstanceVersion(&vkInstanceAPIVersion)) {
        LOG_ERROR("Failed to enumerate instance API version.");
        return 0;
    }
    
    VkApplicationInfo vkInstanceAppInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vkc",
        .applicationVersion = vkInstanceAPIVersion,
        .pEngineName = "vkc engine",
        .engineVersion = vkInstanceAPIVersion,
        .apiVersion = vkInstanceAPIVersion,
    };

    LOG_INFO("[VkApplicationInfo] Name: %s", vkInstanceAppInfo.pApplicationName);
    LOG_INFO(
        "[VkApplicationInfo] Version: %u.%u.%u",
        VK_VERSION_MAJOR(vkInstanceAppInfo.applicationVersion),
        VK_VERSION_MINOR(vkInstanceAppInfo.applicationVersion),
        VK_VERSION_PATCH(vkInstanceAppInfo.applicationVersion)
    );
    LOG_INFO("[VkApplicationInfo] Engine Name: %s", vkInstanceAppInfo.pEngineName);
    LOG_INFO(
        "[VkApplicationInfo] Engine Version: %u.%u.%u",
        VK_VERSION_MAJOR(vkInstanceAppInfo.engineVersion),
        VK_VERSION_MINOR(vkInstanceAppInfo.engineVersion),
        VK_VERSION_PATCH(vkInstanceAppInfo.engineVersion)
    );
    LOG_INFO(
        "[VkApplicationInfo] API Version: %u.%u.%u",
        VK_API_VERSION_MAJOR(vkInstanceAppInfo.apiVersion),
        VK_API_VERSION_MINOR(vkInstanceAppInfo.apiVersion),
        VK_API_VERSION_PATCH(vkInstanceAppInfo.apiVersion)
    );

    VkInstanceCreateInfo vkInstanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &vkInstanceAppInfo,
        .enabledExtensionCount = vkInstanceExtensionPropertyCount,
        .ppEnabledExtensionNames = vkInstanceExtensionPropertyNames,
    };

    if (instanceLayerMatch) {
        vkInstanceCreateInfo.enabledLayerCount = instanceLayerMatch->count;
        vkInstanceCreateInfo.ppEnabledLayerNames = (const char* const*) instanceLayerMatch->names;
    }

    if (vkInstanceExtensionPropertyFound) {
        vkInstanceCreateInfo.enabledExtensionCount = vkInstanceExtensionPropertyNameCount;
        vkInstanceCreateInfo.ppEnabledExtensionNames = vkInstanceExtensionPropertyNames;
    }

    VkInstance vkInstance = VK_NULL_HANDLE;
    result = vkCreateInstance(&vkInstanceCreateInfo, &vkAllocationCallback, &vkInstance);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkInstance] Failed to create instance object: %d", result);
        goto cleanup_instance_layer_match;
    }

    // Free dead weight
    page_free(pager, vkInstanceExtensionProperties);

    LOG_INFO("[VkInstance] Created instance @ %p.", vkInstance);

    /** @} */


    /**
     * @name Clean up on Success
     * @{
     */

    vkDestroyInstance(vkInstance, &vkAllocationCallback);
    page_allocator_free(pager);

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

    vkDestroyInstance(vkInstance, &vkAllocationCallback);
cleanup_instance_layer_match:
    vkc_instance_layer_match_free(instanceLayerMatch);
cleanup_instance_layer:
    vkc_instance_layer_free(instanceLayer);
cleanup_pager:
    page_allocator_free(pager);

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkCompute] Debug Mode: Exit Failure");
#else
    LOG_INFO("[VkCompute] Release Mode: Exit Failure");
#endif

    return EXIT_FAILURE;

    /** @} */
}
