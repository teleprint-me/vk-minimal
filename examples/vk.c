/**
 * @file examples/vk.c
 * @brief A headless compute pipeline using Vulkan in C.
 *
 * VkC is designed for raw, low-level GPU compute on Unix-like systems.
 * It emphasizes transparency, portability, and simplicity—no rendering, no windowing, no BS.
 * All GPU work is performed via compute shaders.
 *
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
#include "numeric/lehmer.h"

#include <vulkan/vulkan.h>

#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @name Allocator Callbacks
 * @note These must be defined because the allocator expects function pointers.
 * @{
 */

void* VKAPI_CALL
vkc_malloc(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope) {
    (void) scope;

    PageAllocator* allocator = (PageAllocator*) pUserData;
    if (NULL == allocator) {
        LOG_ERROR("[VK_ALLOC] Missing allocation context (PageAllocator)");
        return NULL;
    }

    void* address = page_malloc(allocator, size, alignment);
    if (NULL == address) {
        LOG_ERROR("[VK_ALLOC] Allocation failed (size=%zu, align=%zu)", size, alignment);
        return NULL;
    }

    return address;
}

void* VKAPI_CALL vkc_realloc(
    void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope
) {
    (void) scope;

    PageAllocator* allocator = (PageAllocator*) pUserData;
    if (NULL == allocator) {
        LOG_ERROR("[VK_REALLOC] Missing allocation context (PageAllocator)");
        return NULL;
    }

    void* address = page_realloc(allocator, pOriginal, size, alignment);
    if (!address) {
        LOG_ERROR(
            "[VK_REALLOC] Allocation failed (pOriginal=%p, size=%zu, align=%zu)",
            pOriginal,
            size,
            alignment
        );
        return NULL;
    }

    return address;
}

void VKAPI_CALL vkc_free(void* pUserData, void* pMemory) {
    PageAllocator* allocator = (PageAllocator*) pUserData;
    if (NULL == allocator || NULL == pMemory) {
        return;
    }

    page_free(allocator, pMemory);
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
     * @name Memory Allocators
     * @{
     */

    PageAllocator* pager = page_allocator_create(1024);
    VkAllocationCallbacks vkAllocationCallback = (VkAllocationCallbacks) {
        .pUserData = pager,
        .pfnAllocation = vkc_malloc,
        .pfnReallocation = vkc_realloc,
        .pfnFree = vkc_free,
    };

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

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log the results to standard output
    LOG_DEBUG("[InstanceLayerProperties] Found %u instance layer properties.", vkInstanceLayerPropertyCount);
    for (uint32_t i = 0; i < vkInstanceLayerPropertyCount; i++) {
        LOG_DEBUG("[InstanceLayerProperties] i=%u, name=%s, description=%s", 
            i, vkInstanceLayerProperties[i].layerName, vkInstanceLayerProperties[i].description
        );
    }
#endif

    bool vkInstanceLayerPropertyFound = false;
    char const* vkInstanceLayerPropertyNames[] = {"VK_LAYER_KHRONOS_validation"};
    for (uint32_t i = 0; i < vkInstanceLayerPropertyCount; i++) {
        if (0 == utf8_raw_compare(vkInstanceLayerProperties[i].layerName, vkInstanceLayerPropertyNames[0])) {
            vkInstanceLayerPropertyFound = true;
            LOG_INFO("[InstanceCreateInfo] Enabling Layer: %s", vkInstanceLayerProperties[i].layerName);
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
     * @todo Look into VK_KHR_device_group and vkEnumeratePhysicalDeviceGroups
     * @note Multi-GPU support is postponed until single device support is stable.
     * @{
     */

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

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkPhysicalDevice] Found %u devices.", vkPhysicalDeviceCount);
    for (uint32_t i = 0; i < vkPhysicalDeviceCount; i++) {
        VkPhysicalDevice device = vkPhysicalDeviceList[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);    
        LOG_DEBUG("[VkPhysicalDevice] i=%u, name=%s, type=%d", 
            i, properties.deviceName, (int) properties.deviceType
        );
    }
#endif

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

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
        // Log the results to standard output
        LOG_DEBUG("[DeviceLayerProperties] Found %u device layer properties.", vkDeviceLayerPropertyCount);
        for (uint32_t i = 0; i < vkDeviceLayerPropertyCount; i++) {
            LOG_DEBUG("[DeviceLayerProperties] i=%u, name=%s, description=%s", 
                i, vkDeviceLayerProperties[i].layerName, vkDeviceLayerProperties[i].description
            );
        }
#endif

        for (uint32_t i = 0; i < vkDeviceLayerPropertyCount; i++) {
            if (0 == utf8_raw_compare(vkDeviceLayerProperties[i].layerName, vkDeviceLayerPropertyNames[0])) {
                vkDeviceLayerPropertyFound = true;
                LOG_INFO("[DeviceCreateInfo] Enabling Layer: %s", vkDeviceLayerProperties[i].layerName);
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

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log supported extensions
    LOG_DEBUG("[DeviceExtensionProperties] Found %u device extensions.", vkDeviceExtensionCount);
    for (uint32_t i = 0; i < vkDeviceExtensionCount; i++) {
        LOG_DEBUG("[DeviceExtensionProperties] i=%u, name=%s", i, vkDeviceExtensionProperties[i].extensionName);
    }
#endif

    uint32_t vkDeviceExtensionNameCount = 13;
    char const* vkDeviceExtensionNames[] = {
        "VK_EXT_descriptor_buffer",
        "VK_EXT_shader_atomic_float",
        "VK_EXT_subgroup_size_control",
    
        "VK_KHR_8bit_storage",
        "VK_KHR_16bit_storage",
        "VK_KHR_shader_float16_int8",
        "VK_KHR_shader_float_controls",
        "VK_KHR_compute_shader_derivatives",
        "VK_KHR_uniform_buffer_standard_layout",
        "VK_KHR_device_group",
        "VK_KHR_external_fence",
        "VK_KHR_external_memory",
        "VK_KHR_external_semaphore",
    };

    bool vkDeviceExtensionPropertyFound = true;
    for (uint32_t i = 0; i < vkDeviceExtensionNameCount; i++) {
        bool found = false;
        for (uint32_t j = 0; j < vkDeviceExtensionCount; j++) {
            if (0 == utf8_raw_compare(
                vkDeviceExtensionNames[i],
                vkDeviceExtensionProperties[j].extensionName)) {
                found = true;
                LOG_INFO("[DeviceCreateInfo] Enabling Extension: %s", vkDeviceExtensionNames[i]);
                break;
            }
        }

        if (!found) {
            LOG_WARN("[DeviceCreateInfo] Extension not available: %s", vkDeviceExtensionNames[i]);
            vkInstanceExtensionPropertyFound = false;
        }
    }

    /** @} */

    /**
     * @name Physical Device Features
     * @ref https://docs.vulkan.org/guide/latest/enabling_features.html
     * @note The Vulkan spec states:
     * If the pNext chain includes a
     * VkPhysicalDeviceVulkan12Features structure,
     * then it must not include a
     * VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES structure
     * @{
     */

    /**
     * @section Feature Extensions
     */

    VkPhysicalDeviceDescriptorBufferFeaturesEXT deviceDescriptorBuffer = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .descriptorBufferImageLayoutIgnored = VK_TRUE,
    };

    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT deviceShaderAtomicFloat = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT,
    };

    /**
     * @section Feature Chains
     * @note Chain extensions on an as-needed basis.
     */

    VkPhysicalDeviceVulkan12Features deviceVulkan12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    };

    deviceShaderAtomicFloat.pNext = NULL;
    deviceDescriptorBuffer.pNext = &deviceShaderAtomicFloat;
    deviceVulkan12.pNext = &deviceDescriptorBuffer;

    /** 
     * @section Device Features
     */

    VkPhysicalDeviceFeatures2 deviceFeatures2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &deviceVulkan12,
    };

    vkGetPhysicalDeviceFeatures2(vkPhysicalDevice, &deviceFeatures2);

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    if (deviceDescriptorBuffer.descriptorBuffer) {
        LOG_DEBUG("[VkPhysicalDeviceFeatures2] descriptorBuffer=%s", deviceDescriptorBuffer.descriptorBuffer ? "true" : "false");
        LOG_DEBUG("[VkPhysicalDeviceFeatures2] descriptorBufferImageLayoutIgnored=%s", deviceDescriptorBuffer.descriptorBufferImageLayoutIgnored ? "true" : "false");
    } else {
        LOG_ERROR("[VkPhysialDeviceFeatures2] Descriptor buffer is unsupported for the selected GPU.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    if (deviceShaderAtomicFloat.shaderBufferFloat32Atomics) {
        LOG_DEBUG("[VkPhysicalDeviceFeatures2] shaderBufferFloat32Atomics=%s", deviceShaderAtomicFloat.shaderBufferFloat32Atomics ? "true" : "false");
        LOG_DEBUG("[VkPhysicalDeviceFeatures2] shaderBufferFloat32AtomicAdd=%s", deviceShaderAtomicFloat.shaderBufferFloat32AtomicAdd ? "true" : "false");
    } else {
        LOG_ERROR("[VkPhysialDeviceFeatures2] Atomicity is unsupported for the selected GPU.");
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    if (deviceVulkan12.shaderFloat16) {
        LOG_DEBUG("[VkPhysicalDeviceFeatures2] shaderFloat16=%s", deviceVulkan12.shaderFloat16 ? "true" : "false");
        LOG_DEBUG("[VkPhysicalDeviceFeatures2] shaderInt8=%s", deviceVulkan12.shaderInt8 ? "true" : "false");
    }
#endif

    LOG_INFO("[VkPhysicalDeviceFeatures2] Enabled physical device extensions.");

    /** @} */

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
        .pNext = &deviceFeatures2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &vkDeviceQueueCreateInfo,
        .pEnabledFeatures = NULL, // overridden by pNext chain
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
        LOG_ERROR("[VkDevice] Failed to create logical device: %d", result);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkDevice] Created logical device @ %p.", vkDevice);

    VkQueue vkQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(vkDevice, vkQueueFamilyIndex, 0, &vkQueue);

    LOG_INFO("[VkQueue] Create logical queue @ %p.", vkQueue);

    /** @} */

    /**
     * @name Read SPIR-V File
     * @{
     */

    const char* shaderFilePath = "build/shaders/mean.spv";
    FILE* shaderFile = fopen(shaderFilePath, "rb");
    if (NULL == shaderFile) {
        LOG_ERROR("[VkShaderModule] Failed to open SPIR-V file: %s", shaderFilePath);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderFilelength = ftell(shaderFile);
    rewind(shaderFile);
    if (-1 == shaderFilelength) {
        LOG_ERROR("[VkShaderModule] Failed to inference SPIR-V file size: %s", shaderFilePath);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    uint32_t shaderCodeSize = (uint32_t) shaderFilelength;
    uint32_t* shaderCode = (uint32_t*) page_malloc(
        pager,
        (shaderCodeSize + 1) * sizeof(uint32_t),
        alignof(uint32_t)
    );

    if (NULL == shaderCode) {
        LOG_ERROR("[VkShaderModule] Failed to allocate %u bytes for SPIR-V shader", shaderCodeSize);
        fclose(shaderFile);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Assuming fread null terminates buffer for us
    fread(shaderCode, 1, shaderCodeSize, shaderFile);
    fclose(shaderFile);

    LOG_INFO("[VkShaderModule] Read SPIR-V shader: file=%s, size=%u", shaderFilePath, shaderCodeSize);

    /** @} */

    /**
     * @name Shader Module
     * @{
     */

    VkShaderModuleCreateInfo vkShaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCodeSize,
        .pCode = shaderCode,
    };

    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
    result = vkCreateShaderModule(vkDevice, &vkShaderInfo, &vkAllocationCallback, &vkShaderModule);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkShaderModule] Failed to create shader module from %s (VkResult=%d)", shaderFilePath, result);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkShaderModule] Created shader module @ %p.", vkShaderModule);

    /** @} */

    /**
     * @name Descriptor Set Layout
     * @{
     */

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = NULL,
        },
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = descriptorSetLayoutBindings,
    };

    VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
    result = vkCreateDescriptorSetLayout(vkDevice, &descriptorSetLayoutCreateInfo, &vkAllocationCallback, &vkDescriptorSetLayout);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkDescriptorSetLayout] Failed to create the descriptor set layout (VkResult=%d)", result);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkDescriptorSetLayout] Created descriptor set layout @ %p.", vkDescriptorSetLayout);

    /** @} */

    /**
     * @name Pipeline Layout
     */

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vkDescriptorSetLayout
    };

    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    result = vkCreatePipelineLayout(
        vkDevice,
        &pipelineLayoutCreateInfo,
        &vkAllocationCallback,
        &vkPipelineLayout
    );

    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkPipelineLayout] Failed to create pipeline layout.");
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkPipelineLayout] Created pipeline layout @ %p.", vkPipelineLayout);

    /** @} */

    /**
     * @name Shader Stage
     */

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = vkShaderModule,
        .pName = "main",
    };

    /** @} */

    /**
     * @name Compute Pipeline
     */

    VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
        .stage = pipelineShaderStageCreateInfo,
        .layout = vkPipelineLayout,
    };

    VkPipeline vkPipeline = VK_NULL_HANDLE;
    result = vkCreateComputePipelines(
        vkDevice,
        NULL,
        1,
        &computePipelineCreateInfo,
        &vkAllocationCallback,
        &vkPipeline
    );

    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkPipeline] Failed to create compute pipeline.");
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkPipeline] Created compute pipeline @ %p.", vkPipeline);

    /** @} */

    /**
     * @name Input Storage Buffer
     * @{
     */

    VkBufferCreateInfo inputBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = 64 * sizeof(float),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer inputBuffer = VK_NULL_HANDLE;
    result = vkCreateBuffer(vkDevice, &inputBufferCreateInfo, &vkAllocationCallback, &inputBuffer);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkBuffer] Failed to create input storage buffer (VkSuccess=%d).", result);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkBuffer] Created input storage buffer @ %p.", inputBuffer);

    /** @} */

    /**
     * @name Input Storage Buffer: Allocate Memory
     * @{
     */

    VkMemoryRequirements inputMemoryRequirements = {0};
    vkGetBufferMemoryRequirements(vkDevice, inputBuffer, &inputMemoryRequirements);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties = {0};
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &physicalDeviceMemoryProperties);

    uint32_t memoryType = UINT32_MAX;
    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        if ((inputMemoryRequirements.memoryTypeBits & (1 << i)) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags &
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryType = i;
            break;
        }
    }

    if (UINT32_MAX == memoryType) {
        LOG_ERROR("[VkMemory] Failed to find a suitable memory type for input buffer.");
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkMemoryAllocateInfo inputAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = inputMemoryRequirements.size,
        .memoryTypeIndex = memoryType,
    };

    VkDeviceMemory inputMemory = VK_NULL_HANDLE;
    result = vkAllocateMemory(vkDevice, &inputAllocInfo, &vkAllocationCallback, &inputMemory);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkMemory] Failed to allocate memory for input buffer (VkResult=%d).", result);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    result = vkBindBufferMemory(vkDevice, inputBuffer, inputMemory, 0);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkMemory] Failed to bind input memory to buffer (VkResult=%d).", result);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkMemory] Allocated and bound input buffer to device @ %p.", inputMemory);

    /** @} */

    /**
     * @name Input Storage Buffer: Upload data
     * @{
     */

    void* mapped = NULL;
    result = vkMapMemory(vkDevice, inputMemory, 0, 64 * sizeof(float), 0, &mapped);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkMapMemory] Failed to map input memory (VkResult=%d).", result);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    lehmer_initialize(LEHMER_SEED);
    float* data = (float*) mapped;
    for (uint32_t i = 0; i < 64; i++) {
        data[i] = lehmer_generate_float();
    }
    vkUnmapMemory(vkDevice, inputMemory);

    LOG_INFO("[VkMapMemory] Mapped memory and initialized data @ %p.", mapped);

    /** @} */

    /**
     * @name Output Storage Buffer
     * @note We do this *after* we submit and sync.
     * @{
     */

    VkBufferCreateInfo outputBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = sizeof(float),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer outputBuffer = VK_NULL_HANDLE;
    result = vkCreateBuffer(vkDevice, &outputBufferCreateInfo, &vkAllocationCallback, &outputBuffer);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkBuffer] Failed to create output storage buffer (VkSuccess=%d).", result);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    /** @} */

    /**
     * @name Output Storage Buffer: Allocate Memory
     * @{
     */

    VkMemoryRequirements outputMemoryRequirements = {0};
    vkGetBufferMemoryRequirements(vkDevice, outputBuffer, &outputMemoryRequirements);

    memoryType = UINT32_MAX;
    for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        if ((outputMemoryRequirements.memoryTypeBits & (1 << i)) &&
            (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags &
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            memoryType = i;
            break;
        }
    }

    if (UINT32_MAX == memoryType) {
        LOG_ERROR("[VkMemory] Failed to find a suitable memory type for output buffer.");
        vkDestroyBuffer(vkDevice, outputBuffer, &vkAllocationCallback);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkMemoryAllocateInfo outputAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = outputMemoryRequirements.size,
        .memoryTypeIndex = memoryType,
    };

    VkDeviceMemory outputMemory = VK_NULL_HANDLE;
    result = vkAllocateMemory(vkDevice, &outputAllocInfo, &vkAllocationCallback, &outputMemory);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkMemory] Failed to allocate memory for output buffer (VkResult=%d).", result);
        vkDestroyBuffer(vkDevice, outputBuffer, &vkAllocationCallback);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    result = vkBindBufferMemory(vkDevice, outputBuffer, outputMemory, 0);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkMemory] Failed to bind output memory to buffer (VkResult=%d).", result);
        vkFreeMemory(vkDevice, outputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, outputBuffer, &vkAllocationCallback);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkMemory] Allocated and bound output buffer to device @ %p.", outputMemory);

    /** @} */

    /**
     * @name Descriptor Pool
     * @{
     */

    VkDescriptorPoolSize descriptorPoolSizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 2,
        },
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = descriptorPoolSizes,
        .maxSets = 1,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    };

    VkDescriptorPool vkDescriptorPool = VK_NULL_HANDLE;
    result = vkCreateDescriptorPool(vkDevice, &descriptorPoolCreateInfo, &vkAllocationCallback, &vkDescriptorPool);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkDescriptorPool] Failed to create descriptor pool: %d", result);
        /// @todo Handle error and cleanup
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkDescriptorPool] Created descriptor pool @ %p", vkDescriptorPool);

    /** @} */

    /**
     * @name Descriptor Set
     * @{
     */

    VkDescriptorSetAllocateInfo descriptorSetAllocationInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vkDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vkDescriptorSetLayout,
    };

    VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
    result = vkAllocateDescriptorSets(vkDevice, &descriptorSetAllocationInfo, &vkDescriptorSet);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkDescriptorSet] Failed to allocate descriptor set: %d", result);
        /// @todo Handle error and cleanup
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkDescriptorSet] Created descriptor set @ %p", vkDescriptorSet);

    /** @} */

    /**
     * @name Bind Buffers to Descriptor Set
     * @{
     */

    VkDescriptorBufferInfo inputBufferInfo = {
        .buffer = inputBuffer,
        .offset = 0,
        .range = 64 * sizeof(float),
    };

    VkDescriptorBufferInfo outputBufferInfo = {
        .buffer = outputBuffer,
        .offset = 0,
        .range = sizeof(float),
    };

    VkWriteDescriptorSet descriptorWrites[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vkDescriptorSet,
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &inputBufferInfo,
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = vkDescriptorSet,
            .dstBinding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &outputBufferInfo,
        },
    };

    vkUpdateDescriptorSets(vkDevice, 2, descriptorWrites, 0, NULL);

    LOG_INFO("[VkWriteDescriptorSets] Successfully updated descriptor sets.");

    /** @} */

    /**
     * @name Command Pool
     * @{
     */

    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vkQueueFamilyIndex,
    };

    VkCommandPool vkCommandPool = VK_NULL_HANDLE;
    result = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, &vkAllocationCallback, &vkCommandPool);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkCommandPool] Failed to create command pool (VkResult=%d).", result);
        /// @todo Handle error and cleanup
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkCommandPool] Created command pool @ %p", vkCommandPool);

    /** @} */

    /**
     * @name Command Buffer Allocation
     * @{
     */

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vkCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;
    result = vkAllocateCommandBuffers(vkDevice, &commandBufferAllocateInfo, &vkCommandBuffer);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkAllocateCommandBuffers] Failed to allocate command buffer (VkResult=%d).", result);
        /// @todo Handle error and cleanup
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkCommandBuffer] Created command buffer @ %p.", vkCommandBuffer);

    /** @} */

    /**
     * @name Output Storage Buffer: Download Data
     * @{
     */

    float* out = NULL;
    result = vkMapMemory(vkDevice, outputMemory, 0, sizeof(float), 0, (void**)&out);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkMapMemory] Failed to map output memory.");
        vkFreeMemory(vkDevice, outputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, outputBuffer, &vkAllocationCallback);
        vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
        vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);
        vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
        vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
        vkDestroyDevice(vkDevice, &vkAllocationCallback);
        vkDestroyInstance(vkInstance, &vkAllocationCallback);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    LOG_INFO("[VkMapMemory] Output result: %.6f", (double) *out);
    vkUnmapMemory(vkDevice, outputMemory);

    /** @} */

    /**
     * @name Clean up
     * @{
     */

    vkFreeMemory(vkDevice, outputMemory, &vkAllocationCallback);
    vkDestroyBuffer(vkDevice, outputBuffer, &vkAllocationCallback);

    vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &vkCommandBuffer);
    vkDestroyCommandPool(vkDevice, vkCommandPool, &vkAllocationCallback);

    vkFreeDescriptorSets(vkDevice, vkDescriptorPool, 1, &vkDescriptorSet);
    vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, &vkAllocationCallback);

    vkFreeMemory(vkDevice, inputMemory, &vkAllocationCallback);
    vkDestroyBuffer(vkDevice, inputBuffer, &vkAllocationCallback);

    vkDestroyPipeline(vkDevice, vkPipeline, &vkAllocationCallback);
    vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, &vkAllocationCallback);
    vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, &vkAllocationCallback);
    vkDestroyShaderModule(vkDevice, vkShaderModule, &vkAllocationCallback);
    vkDestroyDevice(vkDevice, &vkAllocationCallback);
    vkDestroyInstance(vkInstance, &vkAllocationCallback);
    page_allocator_free(pager);

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkCompute] Debug Mode: Exit Success");
#else
    LOG_INFO("[VkCompute] Release Mode: Exit Success");
#endif

    return EXIT_SUCCESS;

    /** {@ */
}
