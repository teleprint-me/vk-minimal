// examples/vk.c
#include "logger.h"

#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

int main(void) {
    // Result codes and handles
    VkResult result;

    /**
     * Create VkInstance
     */

    VkApplicationInfo app_info = {0};
    VkInstanceCreateInfo create_info = {0};
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
    create_info = (VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
    };

    result = vkCreateInstance(&create_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        return EXIT_FAILURE;
    }

    LOG_INFO("Vulkan instance created successfully.");

    /**
     * Create VkPhysicalDevice
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
    physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
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
    free(physical_devices); // Cleaup to prevent leaks!

    if (selected_device == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found with compute capabilities.");
        goto cleanup_instance;
    }

    LOG_INFO(
        "Selected physical device with compute queue family index: %u", compute_queue_family_index
    );

    /**
     * Create VkLogicalDevice
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

    // stub to silence the compiler about unused variables
    // we'll need this later regardless.
    if (!device) {
        goto cleanup_device;
    }

    // Clean up
cleanup_device:
    vkDestroyDevice(device, NULL);
cleanup_instance:
    vkDestroyInstance(instance, NULL);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
