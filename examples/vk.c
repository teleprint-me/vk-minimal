/**
 * @file examples/vk.c
 * @ref https://vulkan-tutorial.com
 * @ref https://docs.vulkan.org
 * @ref https://vulkan.gpuinfo.org
 * @ref https://registry.khronos.org/vulkan
 */

#include "core/posix.h"
#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "utf8/raw.h"

#include "vk/allocator.h"
#include <vulkan/vulkan.h>

#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    /**
     * @name Page Allocator
     * @{
     */

    PageAllocator* pager = page_allocator_create(1024);
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

    uint32_t vkInstanceLayerPropertyCount = 0;
    result = vkEnumerateInstanceLayerProperties(&vkInstanceLayerPropertyCount, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[InstanceLayerProperties] Failed to enumerate instance layer property count.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkLayerProperties* vkInstanceLayerProperties = page_malloc(
        pager,
        vkInstanceLayerPropertyCount * sizeof(VkLayerProperties),
        alignof(VkLayerProperties)
    );
    if (NULL == vkInstanceLayerProperties) {
        LOG_ERROR(
            "[InstanceLayerProperties] Failed to allocate %u instance layer property objects.", 
            vkInstanceLayerPropertyCount
        );
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }
    memset(vkInstanceLayerProperties, 0, vkInstanceLayerPropertyCount * sizeof(VkLayerProperties));

    result = vkEnumerateInstanceLayerProperties(
        &vkInstanceLayerPropertyCount, vkInstanceLayerProperties
    );
    if (VK_SUCCESS != result) {
        LOG_ERROR("[InstanceLayerProperties] Failed to enumerate instance layer properties.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Log the results to standard output
    LOG_INFO("[InstanceLayerProperties] Found %u instance layer properties.", vkInstanceLayerPropertyCount);
    for (uint32_t i = 0; i < vkInstanceLayerPropertyCount; i++) {
        LOG_INFO("[InstanceLayerProperties] i=%u, name=%s, description=%s", 
            i, vkInstanceLayerProperties[i].layerName, vkInstanceLayerProperties[i].description
        );
    }

    bool vkInstanceLayerPropertyFound = false;
    char const* vkInstanceLayerPropertyNames[] = {"VK_LAYER_KHRONOS_validation"};
    for (uint32_t i = 0; i < vkInstanceLayerPropertyCount; i++) {
        if (0 == utf8_raw_compare(vkInstanceLayerProperties[i].layerName, vkInstanceLayerPropertyNames[0])) {
            vkInstanceLayerPropertyFound = true;
            LOG_INFO("[VkCreateInfo] Enabling Layer: %s", vkInstanceLayerProperties[i].layerName);
            break;
        }
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
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkExtensionProperties* vkInstanceExtensionProperties = page_malloc(
        pager,
        vkInstanceExtensionPropertyCount * sizeof(VkExtensionProperties),
        alignof(VkExtensionProperties)
    );
    if (NULL == vkInstanceExtensionProperties) {
        LOG_ERROR("[InstanceExtensionProperties] Failed to allocate %u instance extension property objects.", vkInstanceExtensionPropertyCount);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }
    memset(vkInstanceExtensionProperties, 0, vkInstanceExtensionPropertyCount * sizeof(VkExtensionProperties));

    result = vkEnumerateInstanceExtensionProperties(NULL, &vkInstanceExtensionPropertyCount, vkInstanceExtensionProperties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[InstanceExtensionProperties] Failed to enumerate instance extension properties.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Log the results to standard output
    LOG_INFO("[InstanceExtensionProperties] Found %u instance extension properties.", vkInstanceExtensionPropertyCount);
    for (uint32_t i = 0; i < vkInstanceExtensionPropertyCount; i++) {
        LOG_INFO("[InstanceExtensionProperties] i=%u, name=%s, version=%u", 
            i, vkInstanceExtensionProperties[i].extensionName, vkInstanceExtensionProperties[i].specVersion
        );
    }

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
                LOG_INFO("[VkCreateInfo] Enabling Extension: %s", vkInstanceExtensionPropertyNames[i]);
                break;
            }
        }

        if (!found) {
            LOG_WARN("[VkCreateInfo] Extension not available: %s", vkInstanceExtensionPropertyNames[i]);
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
        .pApplicationName = "compute",
        .applicationVersion = vkInstanceAPIVersion,
        .pEngineName = "compute engine",
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

    if (vkInstanceLayerPropertyFound) {
        vkInstanceCreateInfo.enabledLayerCount = 1;
        vkInstanceCreateInfo.ppEnabledLayerNames = vkInstanceLayerPropertyNames;
    }

    if (vkInstanceExtensionPropertyFound) {
        vkInstanceCreateInfo.enabledExtensionCount = vkInstanceExtensionPropertyNameCount;
        vkInstanceCreateInfo.ppEnabledExtensionNames = vkInstanceExtensionPropertyNames;
    }

    VkInstance vkInstance = VK_NULL_HANDLE;
    result = vkCreateInstance(&vkInstanceCreateInfo, &vkAllocationCallback, &vkInstance);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkInstance] Failed to create instance object: %d", result);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Free dead weight
    page_free(pager, vkInstanceLayerProperties);
    page_free(pager, vkInstanceExtensionProperties);

    /** @} */

    /**
     * @name Enumerate Physical Device List
     * @{
     */

    /// @todo Look into VK_KHR_device_group and vkEnumeratePhysicalDeviceGroups
    /// @note Multi-GPU support is postponed until single device support is stable.

    uint32_t vkPhysicalDeviceCount = 0;
    result = vkEnumeratePhysicalDevices(vkInstance, &vkPhysicalDeviceCount, NULL);
    if (VK_SUCCESS != result || 0 == vkPhysicalDeviceCount) {
        LOG_ERROR(
            "[VkPhysicalDevice] No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            vkPhysicalDeviceCount
        );
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkPhysicalDevice* vkPhysicalDeviceList = page_malloc(
        pager,
        vkPhysicalDeviceCount * sizeof(VkPhysicalDevice),
        alignof(VkPhysicalDevice)
    );
    if (NULL == vkPhysicalDeviceList) {
        LOG_ERROR("[VkPhysicalDevice] Failed to allocate device list.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    result = vkEnumeratePhysicalDevices(vkInstance, &vkPhysicalDeviceCount, vkPhysicalDeviceList);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkPhysicalDevice] Failed to enumerate devices (VkResult: %d)", result);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Log entire device list
    for (uint32_t i = 0; i < vkPhysicalDeviceCount; i++) {
        VkPhysicalDevice device = vkPhysicalDeviceList[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);    
        LOG_INFO("[VkPhysicalDevice] i=%u, name=%s, type=%d", 
            i, properties.deviceName, (int) properties.deviceType
        );
    }

    /** @} */

    /**
     * @name Select Compute Device
     */

    uint32_t vkQueueFamilyIndex = UINT32_MAX;
    VkPhysicalDevice vkPhysicalDevice = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < vkPhysicalDeviceCount; i++) {
        VkPhysicalDevice device = vkPhysicalDeviceList[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

        VkQueueFamilyProperties* queue_families = page_malloc(
            pager,
            queue_family_count * sizeof(VkQueueFamilyProperties),
            alignof(VkQueueFamilyProperties)
        );

        if (NULL == queue_families) {
            LOG_ERROR("[VkPhysicalDevice] Failed to allocate queue families.");
            vkDestroyInstance(vkInstance, &vkAllocationCallback);
            page_allocator_free(pager);
            return EXIT_FAILURE;
        }

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

        for (uint32_t j = 0; j < queue_family_count; j++) {
            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == properties.deviceType) {
                    vkPhysicalDevice = device;
                    vkQueueFamilyIndex = j;
                    break;
                }
            }
        }

        // Free dead weight
        page_free(pager, queue_families);
        if (VK_NULL_HANDLE != vkPhysicalDevice) {
            LOG_INFO(
                "[VkPhysicalDevice] Selected name=%s, type=%d, queue=%u, api=%u.%u.%u, driver=%u.%u.%u",
                properties.deviceName,
                properties.deviceType,
                vkQueueFamilyIndex,
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion),
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion)
            );
            break;
        }
    }

    /** @} */

    /**
     * @name Select Fallback Compute Device
     * @{
     */

    if (VK_NULL_HANDLE == vkPhysicalDevice) {
        for (uint32_t i = 0; i < vkPhysicalDeviceCount; i++) {
            VkPhysicalDevice device = vkPhysicalDeviceList[i];
            VkPhysicalDeviceProperties properties = {0};
            vkGetPhysicalDeviceProperties(device, &properties);

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

            VkQueueFamilyProperties* queue_families = page_malloc(
                pager,
                queue_family_count * sizeof(VkQueueFamilyProperties),
                alignof(VkQueueFamilyProperties)
            );

            if (NULL == queue_families) {
                LOG_ERROR("[VkPhysicalDevice] Failed to allocate queue families.");
                vkDestroyInstance(vkInstance, &vkAllocationCallback);
                page_allocator_free(pager);
                return EXIT_FAILURE;
            }

            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

            for (uint32_t j = 0; j < queue_family_count; j++) {
                if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    vkPhysicalDevice = device;
                    vkQueueFamilyIndex = j;
                    break;
                }
            }

            // Free dead weight
            page_free(pager, queue_families);
            if (VK_NULL_HANDLE != vkPhysicalDevice) {
                LOG_INFO(
                    "[VkPhysicalDevice] Selected name=%s, type=%d, queue=%u, api=%u.%u.%u, driver=%u.%u.%u",
                    properties.deviceName,
                    properties.deviceType,
                    vkQueueFamilyIndex,
                    VK_VERSION_MAJOR(properties.apiVersion),
                    VK_VERSION_MINOR(properties.apiVersion),
                    VK_VERSION_PATCH(properties.apiVersion),
                    VK_VERSION_MAJOR(properties.driverVersion),
                    VK_VERSION_MINOR(properties.driverVersion),
                    VK_VERSION_PATCH(properties.driverVersion)
                );
                break;
            }
        }
    }

    if (VK_NULL_HANDLE == vkPhysicalDevice) {
        LOG_ERROR("[VkPhysicalDevice] No suitable compute device found.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    /** @} */

    /**
     * @name Enumerate Device Layers
     * @{
     */

    uint32_t vkDeviceLayerPropertyCount = 0;
    result = vkEnumerateDeviceLayerProperties(vkPhysicalDevice, &vkDeviceLayerPropertyCount, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[DeviceLayerProperties] Failed to enumerate device layer property count.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    bool vkDeviceLayerPropertyFound = false;
    char const* vkDeviceLayerPropertyNames[] = {"VK_LAYER_KHRONOS_validation"};
    if (0 == vkDeviceLayerPropertyCount) {
        LOG_INFO("[DeviceLayerProperties] Device layer properties are unavailable. Skipping.");
    } else {
        VkLayerProperties* vkDeviceLayerProperties = page_malloc(
            pager,
            vkDeviceLayerPropertyCount * sizeof(VkLayerProperties),
            alignof(VkLayerProperties) 
        );

        if (NULL == vkDeviceLayerProperties) {
            LOG_ERROR("[DeviceLayerProperties] Failed to allocate device layer properties.");
            vkDestroyInstance(vkInstance, &vkAllocationCallback);
            page_allocator_free(pager);
            return EXIT_FAILURE;
        }

        result = vkEnumerateDeviceLayerProperties(vkPhysicalDevice, &vkDeviceLayerPropertyCount, vkDeviceLayerProperties);
        if (VK_SUCCESS != result) {
            LOG_ERROR("[DeviceLayerProperties] Failed to enumerate device layer properties.");
            vkDestroyInstance(vkInstance, &vkAllocationCallback);
            page_allocator_free(pager);
            return EXIT_FAILURE;
        }

        // Log the results to standard output
        LOG_INFO("[DeviceLayerProperties] Found %u device layer properties.", vkDeviceLayerPropertyCount);
        for (uint32_t i = 0; i < vkDeviceLayerPropertyCount; i++) {
            LOG_INFO("[DeviceLayerProperties] i=%u, name=%s, description=%s", 
                i, vkDeviceLayerProperties[i].layerName, vkDeviceLayerProperties[i].description
            );
        }

        for (uint32_t i = 0; i < vkDeviceLayerPropertyCount; i++) {
            if (0 == utf8_raw_compare(vkDeviceLayerProperties[i].layerName, vkDeviceLayerPropertyNames[0])) {
                vkDeviceLayerPropertyFound = true;
                LOG_INFO("[VkCreateInfo] Enabling Layer: %s", vkDeviceLayerProperties[i].layerName);
                break;
            }
        }

        // Free dead weight
        page_free(pager, vkDeviceLayerProperties);
    }

    /** @} */

    /**
     * @name Enumerate Device Extensions
     * @{
     */

    uint32_t vkDeviceExtensionCount = 0;
    result = vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &vkDeviceExtensionCount, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkPhysicalDevice] Failed to enumerate device extension property count.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkExtensionProperties* vkDeviceExtensionProperties = page_malloc(
        pager,
        vkDeviceExtensionCount * sizeof(VkExtensionProperties),
        alignof(VkExtensionProperties)
    );

    if (NULL == vkDeviceExtensionProperties) {
        LOG_ERROR("[VkPhysicalDevice] Failed to allocate device extension properties.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
    }

    result = vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &vkDeviceExtensionCount, vkDeviceExtensionProperties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkPhysicalDevice] Failed to enumerate device extension properties.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Log supported extensions
    LOG_INFO("[DeviceExtensionProperties] Found %u device extensions.", vkDeviceExtensionCount);
    for (uint32_t i = 0; i < vkDeviceExtensionCount; i++) {
        LOG_INFO("[DeviceExtensionProperties] i=%u, name=%s", i, vkDeviceExtensionProperties[i].extensionName);
    }

    uint32_t vkDeviceExtensionNameCount = 14;
    char const* vkDeviceExtensionNames[] = {
        "VK_EXT_shader_atomic_float",

        "VK_KHR_8bit_storage",
        "VK_KHR_16bit_storage",
        "VK_KHR_shader_float16_int8",
        "VK_KHR_shader_float_controls",
        "VK_KHR_shader_integer_dot_product",
        "VK_KHR_compute_shader_derivatives",
        "VK_KHR_device_group",
        "VK_KHR_external_fence",
        "VK_KHR_external_memory",
        "VK_KHR_external_semaphore",

        "VK_AMD_shader_core_properties",
        "VK_AMD_gpu_shader_half_float",
        "VK_AMD_gpu_shader_int16",
    };

    bool vkDeviceExtensionPropertyFound = true;
    for (uint32_t i = 0; i < vkDeviceExtensionNameCount; i++) {
        bool found = false;
        for (uint32_t j = 0; j < vkDeviceExtensionCount; j++) {
            if (0 == utf8_raw_compare(
                vkDeviceExtensionNames[i],
                vkDeviceExtensionProperties[j].extensionName)) {
                found = true;
                LOG_INFO("[VkCreateInfo] Enabling Extension: %s", vkDeviceExtensionNames[i]);
                break;
            }
        }

        if (!found) {
            LOG_WARN("[VkCreateInfo] Extension not available: %s", vkDeviceExtensionNames[i]);
            vkInstanceExtensionPropertyFound = false;
        }
    }

    /** @} */

    /**
     * @name Physical Device Features
     */

    /// @todo Look into physical device features
    /// @ref https://docs.vulkan.org/guide/latest/enabling_features.html
    VkPhysicalDeviceFeatures features = {0};
    vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &features);

    /**
     * @name Logical Device
     * @{
     */

    // The compute queue family we want access to.
    static const float vkDeviceQueuePriorities[1] = {1.0f};
    VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = vkQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = vkDeviceQueuePriorities,
    };

    VkDeviceCreateInfo vkDeviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &vkDeviceQueueCreateInfo,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .pEnabledFeatures = &features,
    };

    if (vkDeviceLayerPropertyFound) {
        vkDeviceCreateInfo.enabledLayerCount = vkDeviceLayerPropertyCount;
        vkDeviceCreateInfo.ppEnabledLayerNames = vkDeviceLayerPropertyNames;
    }

    if (vkDeviceExtensionPropertyFound) {
        vkDeviceCreateInfo.enabledExtensionCount = vkDeviceExtensionNameCount;
        vkDeviceCreateInfo.ppEnabledExtensionNames = vkDeviceExtensionNames;
    }

    VkDevice vkDevice = VK_NULL_HANDLE;
    result = vkCreateDevice(vkPhysicalDevice, &vkDeviceCreateInfo, &vkAllocationCallback, &vkDevice);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to create logical device: %d", result);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("Logical device created.");

    // VkQueue compute_queue = VK_NULL_HANDLE;
    // vkGetDeviceQueue(device, compute_queue_family_index, 0, &compute_queue);
    // LOG_INFO("Retrieved compute queue.");

    // /**
    //  * Read SPIR-V Binary File
    //  */

    // size_t code_size = 0;
    // const char* filepath = "build/shaders/mean.spv";

    // FILE* file = fopen(filepath, "rb");
    // if (!file) {
    //     LOG_ERROR("Failed to open SPIR-V file: %s", filepath);
    //     goto cleanup_device;
    // }

    // fseek(file, 0, SEEK_END);
    // long length = ftell(file);
    // rewind(file);

    // // Track the buffer because Vulkan won't free this later on for us.
    // char* code = (char*) lease_alloc_owned_address(
    //     vk_alloc_owner,
    //     (size_t) length + 1,
    //     alignof(char)
    // );
    // if (!code) {
    //     fclose(file);
    //     LOG_ERROR("Failed to allocate memory for shader file");
    //     goto cleanup_device;
    // }

    // fread(code, 1, length, file); // assuming fread null terminates buffer for us
    // fclose(file);

    // /**
    //  * Create a Vulkan Shader Module
    //  */

    // VkShaderModule shader_module;
    // VkShaderModuleCreateInfo shader_info = {0};

    // shader_info = (VkShaderModuleCreateInfo) {
    //     .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    //     .codeSize = code_size,
    //     .pCode = (uint32_t*) code,
    // };

    // result = vkCreateShaderModule(device, &shader_info, &vk_alloc, &shader_module);
    // if (result != VK_SUCCESS) {
    //     LOG_ERROR("Failed to create shader module from %s", filepath);
    //     goto cleanup_device;
    // }

    // // stub
    // if (!device) {
    //     goto cleanup_shader;
    // }

    /**
     * Clean up
     */

    // vkDestroyShaderModule(device, shader_module, NULL);
    // vkDestroyDevice(device, NULL);

    vkDestroyDevice(vkDevice, &vkAllocationCallback);
    vkDestroyInstance(vkInstance, &vkAllocationCallback);
    page_allocator_free(pager);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
