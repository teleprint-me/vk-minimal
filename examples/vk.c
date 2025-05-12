// examples/vk.c
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

int main(void) {
    // 1. Fill in application info
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vk-minimal",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 4, 0),
        .pEngineName = "no-engine",
        .engineVersion = VK_MAKE_API_VERSION(0, 1, 4, 0),
        .apiVersion = VK_API_VERSION_1_4,
    };

    // 2. No extensions or validation layers for now
    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
    };

    // 3. Create Vulkan instance
    VkInstance instance;
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        return EXIT_FAILURE;
    }

    LOG_INFO("Vulkan instance created successfully.");

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL); // ASAN is detecting leaks
    if (device_count == 0) {
        LOG_ERROR("No Vulkan-compatible GPUs found.");
        vkDestroyInstance(instance, NULL);
        return EXIT_FAILURE;
    }

    VkPhysicalDevice physical_devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

    // Pick the first device with compute support
    VkPhysicalDevice selected_device = VK_NULL_HANDLE;
    uint32_t compute_queue_family_index = UINT32_MAX;

    for (uint32_t i = 0; i < device_count; ++i) {
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
        vkDestroyInstance(instance, NULL);
        return EXIT_FAILURE;
    }

    LOG_INFO(
        "Selected physical device with compute queue family index: %u", compute_queue_family_index
    );

    // 4. Clean up
    vkDestroyInstance(instance, NULL);
    LOG_INFO("Vulkan instance destroyed.");

    return EXIT_SUCCESS;
}
