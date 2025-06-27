/**
 * @file examples/vk.c
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
     * @section Page Allocator to tack memory allocations.
     * @{
     */

    PageAllocator* pager = page_allocator_create(1024);
    VkAllocationCallbacks vkAllocationCallback = vkc_page_callbacks(pager);

    /** @} */

    /**
     * @section Global result variable
     * @{
     */

    VkResult result; // Result codes and handles

    /** @} */

    /**
     * @section Get the instance layer properties
     * @{
     */

    uint32_t vkInstanceLayerPropertyCount = 0;
    result = vkEnumerateInstanceLayerProperties(&vkInstanceLayerPropertyCount, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkLayerProperties] Failed to enumerate instance layer property count.");
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
            "[VkLayerProperties] Failed to allocate %u instance layer property objects.", 
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
        LOG_ERROR("[VkLayerProperties] Failed to enumerate instance layer properties.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Log the results to standard output
    LOG_INFO("[VkLayerProperties] Found %u instance layer properties.", vkInstanceLayerPropertyCount);
    for (uint32_t i = 0; i < vkInstanceLayerPropertyCount; i++) {
        LOG_INFO("[VkLayerProperties] i=%u, name=%s, description=%s", 
            i, vkInstanceLayerProperties[i].layerName, vkInstanceLayerProperties[i].description
        );
    }

    char const** vkInstanceLayerPropertyNames = page_malloc(
        pager,
        vkInstanceLayerPropertyCount * sizeof(char*),
        alignof(char*)
    );
    if (NULL == vkInstanceLayerPropertyNames) {
        LOG_ERROR("[VkLayerProperties] Failed to allocate instance layer property names.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    for (uint32_t i = 0; i < vkInstanceLayerPropertyCount; i++) {
        vkInstanceLayerPropertyNames[i] = vkInstanceLayerProperties[i].layerName;
        LOG_INFO("[VkCreateInfo] Enabling Layer: %s", vkInstanceLayerPropertyNames[i]);
    }

    /** @} */

    /**
     * @section Get the extension layer properties
     * @{
     */

    uint32_t vkInstanceExtensionPropertyCount = 0;
    result = vkEnumerateInstanceExtensionProperties(NULL, &vkInstanceExtensionPropertyCount, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkExtensionProperties] Failed to enumerate instance extension property count.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    VkExtensionProperties* vkInstanceExtensionProperties = page_malloc(
        pager,
        vkInstanceExtensionPropertyCount * sizeof(VkExtensionProperties),
        alignof(VkExtensionProperties)
    );
    if (NULL == vkInstanceExtensionProperties) {
        LOG_ERROR("[VkExtensionProperties] Failed to allocate %u instance extension property objects.", vkInstanceExtensionPropertyCount);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }
    memset(vkInstanceExtensionProperties, 0, vkInstanceExtensionPropertyCount * sizeof(VkExtensionProperties));

    result = vkEnumerateInstanceExtensionProperties(NULL, &vkInstanceExtensionPropertyCount, vkInstanceExtensionProperties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkExtensionProperties] Failed to enumerate instance extension properties.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Log the results to standard output
    LOG_INFO("[VkExtensionProperties] Found %u instance extension properties.", vkInstanceExtensionPropertyCount);
    for (uint32_t i = 0; i < vkInstanceExtensionPropertyCount; i++) {
        LOG_INFO("[VkExtensionProperties] i=%u, name=%s, version=%u", 
            i, vkInstanceExtensionProperties[i].extensionName, vkInstanceExtensionProperties[i].specVersion
        );
    }

    char const** vkInstanceExtensionPropertyNames = page_malloc(
        pager,
        vkInstanceExtensionPropertyCount * sizeof(char*),
        alignof(char*)
    );
    if (NULL == vkInstanceExtensionPropertyNames) {
        LOG_ERROR("[VkExtensionProperties] Failed to allocate instance extension names.");
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    for (uint32_t i = 0; i < vkInstanceExtensionPropertyCount; i++) {
        vkInstanceExtensionPropertyNames[i] = vkInstanceExtensionProperties[i].extensionName;
        LOG_INFO("[VkCreateInfo] Enabling Extension: %s", vkInstanceExtensionPropertyNames[i]);
    }

    /** @} */

    /**
     * @section Create a Vulkan Instance
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

    LOG_INFO("Application Name: %s", vkInstanceAppInfo.pApplicationName);
    LOG_INFO(
        "Application Version: %u.%u.%u",
        VK_VERSION_MAJOR(vkInstanceAppInfo.applicationVersion),
        VK_VERSION_MINOR(vkInstanceAppInfo.applicationVersion),
        VK_VERSION_PATCH(vkInstanceAppInfo.applicationVersion)
    );
    LOG_INFO("Engine Name: %s", vkInstanceAppInfo.pEngineName);
    LOG_INFO(
        "Engine Version: %u.%u.%u",
        VK_VERSION_MAJOR(vkInstanceAppInfo.engineVersion),
        VK_VERSION_MINOR(vkInstanceAppInfo.engineVersion),
        VK_VERSION_PATCH(vkInstanceAppInfo.engineVersion)
    );
    LOG_INFO(
        "API Version: %u.%u.%u",
        VK_API_VERSION_MAJOR(vkInstanceAppInfo.apiVersion),
        VK_API_VERSION_MINOR(vkInstanceAppInfo.apiVersion),
        VK_API_VERSION_PATCH(vkInstanceAppInfo.apiVersion)
    );

    VkInstanceCreateInfo vkInstanceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &vkInstanceAppInfo,
        .enabledLayerCount = vkInstanceLayerPropertyCount,
        .ppEnabledLayerNames = vkInstanceLayerPropertyNames,
        .enabledExtensionCount = vkInstanceExtensionPropertyCount,
        .ppEnabledExtensionNames = vkInstanceExtensionPropertyNames,
    };

    VkInstance vkInstance = VK_NULL_HANDLE;
    result = vkCreateInstance(&vkInstanceCreateInfo, &vkAllocationCallback, &vkInstance);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkInstance] Failed to create instance object: %d", result);
        page_allocator_free(pager);
        return EXIT_FAILURE;
    }

    // Free all instance properties
    page_free(pager, vkInstanceLayerPropertyNames);
    page_free(pager, vkInstanceLayerProperties);
    page_free(pager, vkInstanceExtensionPropertyNames);
    page_free(pager, vkInstanceExtensionProperties);

    /** @} */

    /**
     * Create a Vulkan Physical Device
     *
     * @note
     * We can not allocate array memory to the stack because
     * goto disrupts the control flow.
     */

    uint32_t physical_device_count = 0;
    VkPhysicalDevice* physical_device_list = NULL;
    VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties selected_properties = {0};
    uint32_t compute_queue_family_index = UINT32_MAX;

    result = vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
    if (result != VK_SUCCESS || physical_device_count == 0) {
        LOG_ERROR(
            "No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            physical_device_count
        );
        goto cleanup_instance;
    }

    physical_device_list = lease_alloc_owned_address(
        vk_alloc_owner,
        sizeof(VkPhysicalDevice) * physical_device_count,
        alignof(VkPhysicalDevice)
    );
    if (!physical_device_list) {
        LOG_ERROR("Failed to allocate device list.");
        goto cleanup_instance;
    }

    result = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_device_list);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to enumerate devices (VkResult: %d)", result);
        goto cleanup_instance;
    }

    // Try to find the first discrete GPU with compute support
    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDevice device = physical_device_list[i];
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(device, &props);

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

        VkQueueFamilyProperties* queue_families = lease_alloc_owned_address(
            vk_alloc_owner,
            sizeof(VkQueueFamilyProperties) * queue_family_count,
            alignof(VkQueueFamilyProperties)
        );
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

        LOG_INFO("Device %u: %s, type=%d", i, props.deviceName, props.deviceType);

        for (uint32_t j = 0; j < queue_family_count; ++j) {
            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                    && selected_physical_device == VK_NULL_HANDLE) {
                    selected_physical_device = device;
                    selected_properties = props;
                    compute_queue_family_index = j;
                    break;
                }
            }
        }
        if (selected_physical_device != VK_NULL_HANDLE) {
            break;
        }
    }

    // Fallback: if no discrete GPU with compute, pick *any* device with compute support
    if (selected_physical_device == VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < physical_device_count; ++i) {
            VkPhysicalDevice device = physical_device_list[i];
            VkPhysicalDeviceProperties props = {0};
            vkGetPhysicalDeviceProperties(device, &props);

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

            VkQueueFamilyProperties* queue_families = lease_alloc_owned_address(
                vk_alloc_owner,
                sizeof(VkQueueFamilyProperties) * queue_family_count,
                alignof(VkQueueFamilyProperties)
            );
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

            for (uint32_t j = 0; j < queue_family_count; ++j) {
                if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    selected_physical_device = device;
                    selected_properties = props;
                    compute_queue_family_index = j;
                    break;
                }
            }
            if (selected_physical_device != VK_NULL_HANDLE) {
                break;
            }
        }
    }

    if (selected_physical_device == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable device with compute queue found.");
        goto cleanup_instance;
    }

    LOG_INFO(
        "Selected device: %s (type=%d), compute queue index: %u",
        selected_properties.deviceName,
        selected_properties.deviceType,
        compute_queue_family_index
    );

    /**
     * Create a Vulkan Logical Device
     */

    static const float queue_priorities[1] = {1.0f};
    VkDeviceQueueCreateInfo queue_info = {0};
    VkPhysicalDeviceFeatures features = {0};

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = &features, // or see note below
    };

    VkDevice device = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;

    // The compute queue family we want access to.
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = compute_queue_family_index;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;

    result = vkCreateDevice(selected_physical_device, &device_info, NULL, &device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        goto cleanup_instance;
    }
    LOG_INFO("Logical device created.");

    vkGetDeviceQueue(device, compute_queue_family_index, 0, &compute_queue);
    LOG_INFO("Retrieved compute queue.");

    /**
     * Read SPIR-V Binary File
     */

    size_t code_size = 0;
    const char* filepath = "build/shaders/mean.spv";

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        LOG_ERROR("Failed to open SPIR-V file: %s", filepath);
        goto cleanup_device;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Track the buffer because Vulkan won't free this later on for us.
    char* code = (char*) lease_alloc_owned_address(
        vk_alloc_owner,
        (size_t) length + 1,
        alignof(char)
    );
    if (!code) {
        fclose(file);
        LOG_ERROR("Failed to allocate memory for shader file");
        goto cleanup_device;
    }

    fread(code, 1, length, file); // assuming fread null terminates buffer for us
    fclose(file);

    /**
     * Create a Vulkan Shader Module
     */

    VkShaderModule shader_module;
    VkShaderModuleCreateInfo shader_info = {0};

    shader_info = (VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = (uint32_t*) code,
    };

    result = vkCreateShaderModule(device, &shader_info, &vk_alloc, &shader_module);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create shader module from %s", filepath);
        goto cleanup_device;
    }

    // stub
    if (!device) {
        goto cleanup_shader;
    }

    /**
     * Clean up
     */

    vkDestroyShaderModule(device, shader_module, NULL);
    vkDestroyDevice(device, NULL);

    vkDestroyInstance(vkInstance, &vkAllocationCallback);
    page_allocator_free(pager);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
