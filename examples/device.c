/**
 * @file examples/instance.c
 * @brief Simple example showcasing how to create and destroy a custom VkcDevice object.
 */

#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "vk/validation.h"
#include "vk/extension.h"
#include "vk/allocator.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct VkcDevice {
    PageAllocator* pager;

    VkPhysicalDevice physical;
    VkDevice logical;
    VkQueue queue;

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties properties;
    VkAllocationCallbacks allocator;

    uint32_t queue_family_index;
} VkcDevice;

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

    for (size_t i = 0; i < sizeof(preference) / sizeof(VkPhysicalDeviceType); ++i) {
        if (vkc_physical_device_select_type(device, list, count, preference[i])) {
            return true;
        }
    }

    return false;
}

VkcDevice* vkc_device_create(VkcInstance* instance, size_t page_size) {
    PageAllocator* pager = page_allocator_create(1024);
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

    return NULL;
}

void vkc_device_destroy(VkcDevice* device) {
    if (device && device->pager) {
        page_allocator_free(device->pager);
    }
}

int main(void) {
    /**
     * Create the Vulkan Instance
     */

    VkcInstance* instance = vkc_instance_create(1024);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        return EXIT_FAILURE;
    }

    /**
     * Create a Vulkan Physical Device
     */

    PageAllocator* pager = page_allocator_create(1024);
    if (!pager) {
        LOG_ERROR("Failed to create device pager.");
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    VkcDevice* device = page_malloc(pager, sizeof(VkcDevice), alignof(VkcDevice));
    if (!device) {
        page_allocator_free(device->pager);
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    device->pager = pager;
    device->allocator = vkc_hash_callbacks(pager);
    device->queue_family_index = UINT32_MAX;
    device->physical = VK_NULL_HANDLE;
    device->logical = VK_NULL_HANDLE;
    device->queue = VK_NULL_HANDLE;

    uint32_t device_count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance->object, &device_count, NULL);
    if (VK_SUCCESS != result || 0 == device_count) {
        LOG_ERROR(
            "No Vulkan-compatible devices found (VkResult: %d, Count: %u)", result, device_count
        );
        page_allocator_free(device->pager);
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    LOG_DEBUG("Vulkan-compatible devices found (VkResult: %d, Count: %u)", result, device_count);

    VkPhysicalDevice* device_list = page_malloc(
        device->pager, device_count * sizeof(VkPhysicalDevice), alignof(VkPhysicalDevice)
    );

    if (!device_list) {
        LOG_ERROR("Failed to allocate physical device list.");
        page_allocator_free(device->pager);
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    result = vkEnumeratePhysicalDevices(instance->object, &device_count, device_list);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to set physical device list.");
        page_allocator_free(device->pager);
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    // Try to find the first discrete GPU with compute support
    for (uint32_t i = 0; i < device_count; ++i) {
        VkPhysicalDevice candidate = device_list[i];
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
                if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == props.deviceType
                    && VK_NULL_HANDLE == device->physical) {
                    device->queue_family_index = j;
                    device->physical = candidate;
                    device->properties = props;
                    vkGetPhysicalDeviceFeatures(candidate, &device->features);
                    break;
                }
            }
        }

        if (VK_NULL_HANDLE != device->physical) {
            break;
        } else {
            page_free(device->pager, queue_families);
        }
    }

    // Fallback: if no discrete GPU with compute, pick *any* device with compute support
    if (VK_NULL_HANDLE == device->physical) {
        for (uint32_t i = 0; i < device_count; ++i) {
            VkPhysicalDevice candidate = device_list[i];
            VkPhysicalDeviceProperties props = {0};
            vkGetPhysicalDeviceProperties(candidate, &props);

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &queue_family_count, NULL);

            VkQueueFamilyProperties* queue_families = page_malloc(
                device->pager,
                sizeof(VkQueueFamilyProperties) * queue_family_count,
                alignof(VkQueueFamilyProperties)
            );
            vkGetPhysicalDeviceQueueFamilyProperties(
                candidate, &queue_family_count, queue_families
            );

            for (uint32_t j = 0; j < queue_family_count; ++j) {
                if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    device->queue_family_index = j;
                    device->physical = candidate;
                    device->properties = props;
                    vkGetPhysicalDeviceFeatures(candidate, &device->features);
                    break;
                }
            }
            if (VK_NULL_HANDLE != device->physical) {
                break;
            } else {
                page_free(device->pager, queue_families);
            }
        }
    }

    if (VK_NULL_HANDLE == device->physical) {
        LOG_ERROR("No suitable device with compute queue found.");
        page_allocator_free(device->pager);
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    LOG_DEBUG(
        "Selected: %s (family index = %u)",
        device->properties.deviceName,
        device->queue_family_index
    );

    /**
     * Create a Vulkan Logical Device
     */

    static const float queue_priorities[1] = {1.0f};
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = device->queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    };

    /// @note Extension returns -7 (VK_ERROR_EXTENSION_NOT_PRESENT)
    VkcValidationLayer* validation
        = vkc_validation_layer_create((const char*[]) {"VK_LAYER_KHRONOS_validation"}, 1, 1024);
    // VkcExtension* extension = vkc_extension_create((const char*[]) {"VK_EXT_debug_utils"}, 1,
    // 1024);

    VkDeviceCreateInfo device_info = {
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

    result = vkCreateDevice(device->physical, &device_info, &device->allocator, &device->logical);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        page_allocator_free(device->pager);
        vkc_instance_destroy(instance);
        return EXIT_FAILURE;
    }

    LOG_DEBUG("Logical device created.");

    vkGetDeviceQueue(device->logical, device->queue_family_index, 0, &device->queue);
    LOG_DEBUG("Logical device queue created.");

    // Cleanup
    vkDestroyDevice(device->logical, &device->allocator);
    page_allocator_free(device->pager);
    vkc_instance_destroy(instance);
    return EXIT_SUCCESS;
}
