// examples/vk.c
#include "core/logger.h"
#include "core/lease.h"
#include "vk/alloc.h"

#include <vulkan/vulkan.h>

#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    /**
     * Lease Allocator for sanely tracking memory allocations.
     */

    LeaseOwner* vk_alloc_owner = lease_create_owner(2048); // scoped
    VkAllocationCallbacks vk_alloc = vk_lease_callbacks(vk_alloc_owner);

    // Result codes and handles
    VkResult result;

    /**
     * Create a Vulkan Instance
     */

    VkApplicationInfo app_info = {0};
    VkInstanceCreateInfo instance_info = {0};
    VkInstance instance = VK_NULL_HANDLE;

    app_info = (VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vk-minimal",
        .applicationVersion = VK_API_VERSION_1_4,
        .pEngineName = "vk-engine",
        .engineVersion = VK_API_VERSION_1_4,
        .apiVersion = VK_API_VERSION_1_4,
    };

    // No extensions or validation layers for now
    instance_info = (VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
    };

    result = vkCreateInstance(&instance_info, &vk_alloc, &instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        goto cleanup_lease;
    }

    LOG_INFO("Vulkan instance created successfully.");

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

    static const float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {0};
    VkDeviceCreateInfo device_info = {0};

    VkDevice device = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;

    // The compute queue family we want access to.
    queue_info = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = compute_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    device_info = (VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = NULL, // VkPhysicalDeviceFeatures later if needed
    };

    result = vkCreateDevice(selected_physical_device, &device_info, &vk_alloc, &device);
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
cleanup_shader:
    vkDestroyShaderModule(device, shader_module, NULL);
cleanup_device:
    vkDestroyDevice(device, NULL);
cleanup_instance:
    vkDestroyInstance(instance, NULL);
cleanup_lease:
    lease_free_owner(vk_alloc_owner);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
