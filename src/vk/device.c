/**
 * @file src/vk/device.c
 */

#include "core/posix.h"
#include "core/logger.h"
#include "vk/allocator.h"
#include "vk/validation.h"
#include "vk/extension.h"
#include "vk/device.h"

/**
 * @name Private methods
 * {@
 */

uint32_t vkc_physical_device_count(VkcInstance* instance) {
    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance->object, &device_count, NULL);
    if (VK_SUCCESS != result || 0 == device_count) {
        LOG_ERROR(
            "No Vulkan-compatible devices found (VkResult: %d, Count: %u)", result, device_count
        );
    }
    return device_count;
}

VkPhysicalDevice*
vkc_physical_device_list(PageAllocator* pager, VkcInstance* instance, uint32_t device_count) {
    VkPhysicalDevice* device_list
        = page_malloc(pager, device_count * sizeof(VkPhysicalDevice), alignof(VkPhysicalDevice));

    if (!device_list) {
        LOG_ERROR("Failed to allocate physical device list.");
        return NULL;
    }

    VkResult result = vkEnumeratePhysicalDevices(instance->object, &device_count, device_list);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to set physical device list.");
        page_free(pager, device_list);
        return NULL;
    }

    return device_list;
}

bool vkc_physical_device_select_candidate(
    VkcDevice* device, VkPhysicalDevice* list, uint32_t i, VkPhysicalDeviceType type
) {
    VkPhysicalDevice candidate = list[i];
    VkPhysicalDeviceProperties props = {0};
    vkGetPhysicalDeviceProperties(candidate, &props);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_family_count, NULL);

    VkQueueFamilyProperties* queue_families = page_malloc(
        device->pager,
        queue_family_count * sizeof(VkQueueFamilyProperties),
        alignof(VkQueueFamilyProperties)
    );

    vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_family_count, queue_families);
    LOG_DEBUG("Device %u: %s, type=%d", i, props.deviceName, props.deviceType);

    for (uint32_t j = 0; j < queue_family_count; ++j) {
        if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            if (type == props.deviceType && VK_NULL_HANDLE == device->physical) {
                device->queue_family_index = j;
                device->physical = candidate;
                device->properties = props;
                vkGetPhysicalDeviceFeatures(candidate, &device->features);
                break;
            }
        }
    }

    if (VK_NULL_HANDLE != device->physical) {
        return true;
    }

    page_free(device->pager, queue_families);
    return false;
}

bool vkc_physical_device_select_type(
    VkcDevice* device, VkPhysicalDevice* list, uint32_t count, VkPhysicalDeviceType type
) {
    // Pick first discrete GPU with compute support
    for (uint32_t i = 0; i < count; i++) {
        if (vkc_physical_device_select_candidate(device, list, i, type)) {
            return true;
        }
    }

    LOG_ERROR("No suitable compute-capable discrete GPU found.");
    return false;
}

bool vkc_physical_device_select(VkcDevice* device, VkPhysicalDevice* list, uint32_t count) {
    VkPhysicalDeviceType preference[] = {
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
    };

    for (size_t i = 0; i < sizeof(preference) / sizeof(VkPhysicalDeviceType); i++) {
        if (vkc_physical_device_select_type(device, list, count, preference[i])) {
            return true;
        }
    }

    return false;
}

VkDeviceQueueCreateInfo vkc_physical_device_queue_create_info(VkcDevice* device) {
    static const float queue_priorities[1] = {1.0f};
    return (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = device->queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    };
}

VkDeviceCreateInfo vkc_logical_device_create_info(VkcDevice* device, VkDeviceQueueCreateInfo queue_info) {
    /// @note Extension returns -7 (VK_ERROR_EXTENSION_NOT_PRESENT)
    VkcValidationLayer* validation
        = vkc_validation_layer_create((const char*[]) {"VK_LAYER_KHRONOS_validation"}, 1, 1024);
    // VkcExtension* extension = vkc_extension_create((const char*[]) {"VK_EXT_debug_utils"}, 1,
    // 1024);

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        // .enabledExtensionCount = extension->request->count,
        // .ppEnabledExtensionNames = extension->request->names,
        .enabledLayerCount = validation->request->count,
        .ppEnabledLayerNames = validation->request->names,
        .pEnabledFeatures = &device->features,
    };

    vkc_validation_layer_free(validation);
    // vkc_extension_free(extension);

    return create_info;
}

/**
 * @name Public methods
 * {@
 */

VkcDevice* vkc_device_create(VkcInstance* instance, size_t page_size) {
    PageAllocator* pager = page_allocator_create(page_size);
    if (!pager) {
        LOG_ERROR("Failed to create device pager.");
        return NULL;
    }

    VkcDevice* device = page_malloc(pager, sizeof(VkcDevice), alignof(VkcDevice));
    if (!device) {
        page_allocator_free(pager);
        return NULL;
    }

    device->queue_family_index = UINT32_MAX;
    device->physical = VK_NULL_HANDLE;
    device->logical = VK_NULL_HANDLE;
    device->queue = VK_NULL_HANDLE;
    device->pager = pager;
    device->allocator = vkc_hash_callbacks(pager);

    uint32_t device_count = vkc_physical_device_count(instance);
    if (0 == device_count) {
        page_allocator_free(pager);
        return NULL;
    }

    VkPhysicalDevice* device_list = vkc_physical_device_list(pager, instance, device_count);
    if (!device_list) {
        page_allocator_free(pager);
        return NULL;
    }

    if (!vkc_physical_device_select(device, device_list, device_count)) {
        LOG_ERROR("No suitable Vulkan device found.");
        page_allocator_free(pager);
        return NULL;
    }

    page_free(pager, device_list); // Prevent silent leaks

    VkDeviceQueueCreateInfo queue_info = vkc_physical_device_queue_create_info(device);
    VkDeviceCreateInfo logical_info = vkc_logical_device_create_info(device, queue_info);

    VkResult result = vkCreateDevice(device->physical, &logical_info, &device->allocator, &device->logical);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        page_allocator_free(device->pager);
        return NULL;
    }

    vkGetDeviceQueue(device->logical, device->queue_family_index, 0, &device->queue);
    return device;
}

void vkc_device_destroy(VkcDevice* device) {
    if (device && device->pager) {
        if (device->logical) {
            vkDestroyDevice(device->logical, &device->allocator);
        }
        page_allocator_free(device->pager);
    }
}
