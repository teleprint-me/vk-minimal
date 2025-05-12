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
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "no-engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
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

    // 4. Clean up
    vkDestroyInstance(instance, NULL);
    LOG_INFO("Vulkan instance destroyed.");

    return EXIT_SUCCESS;
}
