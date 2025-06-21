/**
 * @file examples/instance.c
 * @brief Simple example showcasing how to create and destroy a custom VkcDevice object.
 */

#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct VkcPhysicalDevice {
    VkPhysicalDevice* list;
    VkPhysicalDevice selected;
    VkPhysicalDeviceProperties properties;
    uint32_t count;
    uint32_t queue_family_index;
} VkcPhysicalDevice;

int main(void) {
    VkcInstance* instance = vkc_instance_create(1024);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        return EXIT_FAILURE;
    }

    // Create the allocator context
    HashMap* ctx = hash_map_create(1024, HASH_MAP_KEY_TYPE_ADDRESS);
    if (!ctx) {
        LOG_ERROR("Failed to create device context.");
        goto cleanup_instance;
    }

    VkcPhysicalDevice device = {0};
    device.queue_family_index = UINT32_MAX;
    device.list = NULL;
    device.selected = VK_NULL_HANDLE;

    VkResult result = vkEnumeratePhysicalDevices(instance->object, &device.count, NULL);
    if (result != VK_SUCCESS || device.count == 0) {
        LOG_ERROR(
            "No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            device.count
        );
        goto cleanup_context;
    }
    LOG_DEBUG("Vulkan-compatible devices found (VkResult: %d, Count: %u)", result, device.count);

    device.list = hash_page_malloc(
        ctx, sizeof(VkPhysicalDevice) * device.count, alignof(VkPhysicalDevice)\
    );
    if (!device.list) {
        LOG_ERROR("Failed to allocate physical device list.");
        goto cleanup_context;
    }

cleanup_context:
    hash_page_free_all(ctx);
    hash_map_free(ctx);
cleanup_instance:
    vkc_instance_destroy(instance);
    return EXIT_SUCCESS;
}
