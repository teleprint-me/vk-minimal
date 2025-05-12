// examples/vk.c
#include "logger.h"
#include "lease.h"

#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

int main(void) {
    /**
     * Lease Allocator to sanely track memory allocations. 
     */

    // Scope is Local and Type is Owned
    LeasePolicy policy = {
        .scope = LEASE_ACCESS_LOCAL,
        .type = LEASE_CONTRACT_OWNED,
    };

    // Assume 8 objects are allocated.
    // We can realloc on demand if we get errors.
    LeaseOwner* owner = lease_create(8);

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
        .pEngineName = "no-engine",
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

    result = vkCreateInstance(&instance_info, NULL, &instance);
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
    uint32_t compute_queue_family_index = UINT32_MAX;
    VkPhysicalDevice* physical_devices = VK_NULL_HANDLE;
    VkPhysicalDevice selected_device = VK_NULL_HANDLE;

    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL); // ASAN is detecting leaks
    if (physical_device_count == 0) {
        LOG_ERROR("No Vulkan-compatible GPUs found.");
        goto cleanup_instance;
    }

    // Track the allocated device list
    physical_devices
        = lease_address(owner, policy, sizeof(VkPhysicalDevice) * physical_device_count);
    if (!physical_devices) {
        LOG_ERROR("Failed to allocate physical device list.");
        goto cleanup_instance;
    }

    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    // Pick the first device with compute support
    for (uint32_t i = 0; i < physical_device_count; ++i) {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);

        VkQueueFamilyProperties queue_families[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_devices[i], &queue_family_count, queue_families
        );

        for (uint32_t j = 0; j < queue_family_count; ++j) {
            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                selected_device = physical_devices[i];
                compute_queue_family_index = j;
                break;
            }
        }

        if (selected_device != VK_NULL_HANDLE) {
            break;
        }
    }

    if (selected_device == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found with compute capabilities.");
        goto cleanup_instance;
    }

    LOG_INFO(
        "Selected physical device with compute queue family index: %u", compute_queue_family_index
    );

    /**
     * Create a Vulkan Logical Device
     */

    static float queue_priority = 1.0f;
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

    result = vkCreateDevice(selected_device, &device_info, NULL, &device);
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
    char* code = (char*) lease_address(owner, policy, (size_t) length + 1);
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

    result = vkCreateShaderModule(device, &shader_info, NULL, &shader_module);
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
    lease_free(owner);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
