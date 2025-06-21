/**
 * @file examples/instance.c
 * @brief Simple example showcasing how to create and destroy a custom VkcDevice object.
 */

#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "vk/allocator.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct VkcDevice {
    HashMap* ctx;
    VkPhysicalDevice* list;
    VkQueueFamilyProperties* queue_families;
    VkPhysicalDevice physical;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkDeviceQueueCreateInfo queue_info;
    VkDeviceCreateInfo info;
    VkAllocationCallbacks allocator;
    VkQueue queue;
    VkDevice logical;
    uint32_t count;
    uint32_t queue_family_index;
} VkcDevice;

int main(void) {
    VkcInstance* instance = vkc_instance_create(1024);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        return EXIT_FAILURE;
    }

    /**
     * Create a Vulkan Physical Device
     *
     * @note
     * We can not allocate array memory to the stack because
     * goto disrupts the control flow.
     */

    VkcDevice device = {0};
    device.queue_family_index = UINT32_MAX;
    device.list = NULL;
    device.physical = VK_NULL_HANDLE;
    device.logical = VK_NULL_HANDLE;
    device.ctx = hash_map_create(1024, HASH_MAP_KEY_TYPE_ADDRESS);
    if (!device.ctx) {
        LOG_ERROR("Failed to create device context.");
        goto cleanup_instance;
    }

    VkResult result = vkEnumeratePhysicalDevices(instance->object, &device.count, NULL);
    if (result != VK_SUCCESS || device.count == 0) {
        LOG_ERROR(
            "No Vulkan-compatible devices found (VkResult: %d, Count: %u)", result, device.count
        );
        goto cleanup_context;
    }
    LOG_DEBUG("Vulkan-compatible devices found (VkResult: %d, Count: %u)", result, device.count);

    device.list = hash_page_malloc(
        device.ctx, sizeof(VkPhysicalDevice) * device.count, alignof(VkPhysicalDevice)
    );
    if (!device.list) {
        LOG_ERROR("Failed to allocate physical device list.");
        goto cleanup_context;
    }

    result = vkEnumeratePhysicalDevices(instance->object, &device.count, device.list);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to set physical device list.");
        goto cleanup_context;
    }
    for (uint32_t i = 0; i < device.count; i++) {
        VkPhysicalDevice tmp = device.list[i];
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(tmp, &props);
        LOG_DEBUG("Device %u: %s", i, props.deviceName);
    }

    // Try to find the first discrete GPU with compute support
    for (uint32_t i = 0; i < device.count; ++i) {
        VkPhysicalDevice tmp = device.list[i];
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(tmp, &props);

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(tmp, &queue_family_count, NULL);

        VkQueueFamilyProperties* queue_families = hash_page_malloc(
            device.ctx,
            sizeof(VkQueueFamilyProperties) * queue_family_count,
            alignof(VkQueueFamilyProperties)
        );
        vkGetPhysicalDeviceQueueFamilyProperties(tmp, &queue_family_count, queue_families);

        LOG_DEBUG("Device %u: %s, type=%d", i, props.deviceName, props.deviceType);

        for (uint32_t j = 0; j < queue_family_count; ++j) {
            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                    && device.physical == VK_NULL_HANDLE) {
                    device.physical = tmp;
                    device.properties = props;
                    device.queue_family_index = j;
                    device.queue_families = queue_families;
                    break;
                }
            }
        }
        if (device.physical != VK_NULL_HANDLE) {
            break;
        }
        if (device.physical == VK_NULL_HANDLE) {
            hash_page_free(device.ctx, queue_families);
        }
    }

    if (device.physical == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable device with compute queue found.");
        goto cleanup_context;
    }
    LOG_DEBUG(
        "Selected: %s (family index = %u)", device.properties.deviceName, device.queue_family_index
    );
    VkQueueFamilyProperties props = device.queue_families[device.queue_family_index];
    LOG_DEBUG("Selected queue flags: 0x%x", props.queueFlags);

    /**
     * Create a Vulkan Logical Device
     */

    static const float queue_priorities[1] = {1.0f};
    device.queue_info = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = device.queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    };

    device.info = (VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device.queue_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = &device.features,
    };

    device.allocator = vkc_hash_callbacks(device.ctx);
    result = vkCreateDevice(device.physical, &device.info, &device.allocator, &device.logical);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        goto cleanup_context;
    }
    LOG_DEBUG("Logical device created.");

    vkGetDeviceQueue(device.logical, device.queue_family_index, 0, &device.queue);

    vkDestroyDevice(device.logical, &device.allocator);
cleanup_context:
    hash_page_free_all(device.ctx);
    hash_map_free(device.ctx);
cleanup_instance:
    vkc_instance_destroy(instance);
    return EXIT_SUCCESS;
}
