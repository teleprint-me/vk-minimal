/**
 * @file src/vk/device.c
 */

#include "core/posix.h"
#include "core/logger.h"
#include "vk/allocator.h"
#include "vk/device.h"

/**
 * @section Private Methods
 */

static bool
vkc_select_physical_device_candidate(VkcDevice* device, VkPhysicalDevice* list, uint32_t index) {
    VkPhysicalDevice candidate = list[index];
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(candidate, &props);

    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, NULL);

    VkQueueFamilyProperties* families = page_malloc(
        device->pager,
        queue_count * sizeof(VkQueueFamilyProperties),
        alignof(VkQueueFamilyProperties)
    );

    if (!families) {
        LOG_WARN("Skipping device %u: failed to allocate queue family buffer", index);
        return false;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_count, families);

    for (uint32_t j = 0; j < queue_count; j++) {
        if (families[j].queueFlags & VK_QUEUE_COMPUTE_BIT
            && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {

            // Store selected device info
            device->physical = candidate;
            device->properties = props;
            vkGetPhysicalDeviceFeatures(candidate, &device->features);

#if defined(DEBUG) && (1 == DEBUG)
            LOG_DEBUG(
                "Selected device %u: %s (type=%d)", index, props.deviceName, props.deviceType
            );
#endif

            page_free(device->pager, families);
            return true;
        }
    }

    page_free(device->pager, families);
    return false;
}

// Phase 1: Select physical device
static bool vkc_select_physical_device(VkcInstance* instance, VkcDevice* device) {
    uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance->object, &count, NULL);
    if (VK_SUCCESS != result || 0 == count) {
        LOG_ERROR("Failed to enumerate physical devices (VkResult: %d, Count: %u)", result, count);
        return false;
    }

    VkPhysicalDevice* list
        = page_malloc(device->pager, count * sizeof(VkPhysicalDevice), alignof(VkPhysicalDevice));
    if (!list) {
        LOG_ERROR("Failed to allocate device list.");
        return false;
    }

    result = vkEnumeratePhysicalDevices(instance->object, &count, list);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to populate device list (VkResult: %d)", result);
        return false;
    }

    // Pick first discrete GPU with compute support
    for (uint32_t i = 0; i < count; i++) {
        if (vkc_select_physical_device_candidate(device, list, i)) {
            return true;
        }
    }

    LOG_ERROR("No suitable compute-capable discrete GPU found.");
    return false;
}

/**
 * @section Public Methods
 */

VkcDevice* vkc_device_create(VkcInstance* instance, size_t page_size) {
    PageAllocator* pager = page_allocator_create(page_size);
    if (!pager) {
        return NULL;
    }

    VkcDevice* device = page_malloc(pager, sizeof(VkcDevice), alignof(VkcDevice));
    if (!device) {
        page_allocator_free(pager);
        return NULL;
    }

    device->pager = pager;
    device->allocator = vkc_hash_callbacks(pager);

    // Phase 1: Select physical device
    if (!vkc_select_physical_device(instance, device)) {
        page_allocator_free(pager);
        return NULL;
    }

    // // Phase 2: Select compute queue
    // if (!vkc_select_queue(device)) {
    //     page_allocator_free(pager);
    //     return NULL;
    // }

    // // Phase 3: Create logical device + get queue
    // if (!vkc_create_logical_device(device)) {
    //     page_allocator_free(pager);
    //     return NULL;
    // }

    return device;
}

void vkc_device_destroy(VkcDevice* device) {
    if (device && device->pager) {
        page_allocator_free(device->pager);
    }
}
