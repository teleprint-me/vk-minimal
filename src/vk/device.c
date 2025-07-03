/**
 * @file src/vk/device.c
 */

#include "core/posix.h"
#include "core/logger.h"
#include "vk/allocator.h"
#include "vk/device.h"

/**
 * @name Physical Device List
 * @{
 */

VkcDeviceList* vkc_device_list_create(VkcInstance* instance) {
    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        return NULL;
    }

    VkcDeviceList* list = page_malloc(allocator, sizeof(*list), alignof(*list));
    if (!list) {
        return NULL;
    }

    *list = (VkcDeviceList) {
        .allocator = allocator,
        .devices = NULL,
        .count = 0,
    };

    VkResult result = vkEnumeratePhysicalDevices(instance->object, &list->count, NULL);
    if (VK_SUCCESS != result || 0 == list->count) {
        LOG_ERROR(
            "[VkcDeviceList] No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            list->count
        );
        return NULL;
    }

    list->devices = page_malloc(
        allocator,
        list->count * sizeof(VkPhysicalDevice),
        alignof(VkPhysicalDevice)
    );
    if (NULL == list->devices) {
        LOG_ERROR("[VkcDeviceList] Failed to allocate device list.");
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(instance->object, &list->count, list->devices);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceList] Failed to enumerate devices (VkResult: %d)", result);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcDeviceList] Found %u devices.", list->count);
    for (uint32_t i = 0; i < list->count; i++) {
        VkPhysicalDevice device = list->devices[i];
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(device, &properties);    
        LOG_DEBUG("[VkcDeviceList] i=%u, name=%s, type=%d", 
            i, properties.deviceName, (int) properties.deviceType
        );
    }
#endif

    return list;
}

void vkc_device_list_free(VkcDeviceList* list) {
    if (list && list->allocator) {
        page_allocator_free(list->allocator);
    }
}

/** @} */

/**
 * @name Queue Family Properties
 * @{
 */

VkcDeviceQueueFamily* vkc_device_queue_family_create(VkPhysicalDevice device) {
    if (!device) {
        LOG_ERROR("[VkcDeviceQueueFamily] Invalid physical device.");
        return NULL;
    }

    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to create allocator context.");
        return NULL;
    }

    VkcDeviceQueueFamily* family = page_malloc(allocator, sizeof(*family), alignof(*family));
    if (!family) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to allocate memory to family structure.");
        page_allocator_free(allocator);
        return NULL;
    }

    *family = (VkcDeviceQueueFamily) {
        .allocator = allocator,
        .properties = NULL,
        .count = 0,
    };

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family->count, NULL);

    if (0 == family->count) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to query family count.");
        page_allocator_free(allocator);
        return NULL;
    }

    family->properties = page_malloc(
        allocator,
        family->count * sizeof(VkQueueFamilyProperties),
        alignof(VkQueueFamilyProperties)
    );

    if (!family->properties) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to allocate memory for %u properties.", family->count);
        page_allocator_free(allocator);
        return NULL;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family->count, family->properties);
    return family;
}

void vkc_device_queue_family_free(VkcDeviceQueueFamily* family) {
    if (family && family->allocator) {
        page_allocator_free(family->allocator);
    }
}

/** @} */

/**
 * @name Physical Device Selection
 * @{
 */

bool vkc_physical_device_select(VkcDevice* device, VkcDeviceList* list) {
    static const VkPhysicalDeviceType type[] = {
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
    };

    static const uint32_t type_count = sizeof(type) / sizeof(VkPhysicalDeviceType);

    for (uint32_t i = 0; i < type_count; i++) {
        for (uint32_t j = 0; j < list->count; j++) {
            VkPhysicalDevice candidate = list->devices[j];
            VkPhysicalDeviceProperties properties = {0};
            vkGetPhysicalDeviceProperties(candidate, &properties);

            VkcDeviceQueueFamily* family = vkc_device_queue_family_create(candidate);
            if (!family) {
                return false;
            }

    #if defined(DEBUG) && (1 == DEBUG)
            LOG_DEBUG("[VkcPhysicalDevice] Candidate %u: %s, type=%d", j, properties.deviceName, properties.deviceType);
    #endif

            bool selected = false;
            for (uint32_t k = 0; k < family->count; k++) {
                if (family->properties[k].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    if (type[i] == properties.deviceType) {
                        device->queue->family_index = k;
                        device->queue->family_properties = family->properties[k];
                        device->physical->object = candidate;
                        device->physical->properties = properties;
                        device->physical->type = type[i];
                        vkGetPhysicalDeviceFeatures(candidate, &device->physical->features);
                        selected = true;
                        break;
                    }
                }
            }

            vkc_device_queue_family_free(family);
            if (selected) {
    #if defined(DEBUG) && (1 == DEBUG)
                LOG_DEBUG("[VkcPhysicalDevice] Selected %u: %s, type=%d", j, properties.deviceName, properties.deviceType);
    #endif
                return true;
            }
        }
    }

    LOG_ERROR("No suitable compute-capable discrete GPU found.");
    return false;
}

/** @} */

VkDeviceQueueCreateInfo vkc_physical_device_queue_create_info(VkcDevice* device) {
    static const float queue_priorities[1] = {1.0f};
    return (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = device->queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    };
}

VkDeviceCreateInfo
vkc_logical_device_create_info(VkcDevice* device, VkDeviceQueueCreateInfo queue_info) {
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
    device->allocator = vkc_page_callbacks(pager);

    uint32_t device_count = vkc_physical_device_count(instance);
    if (0 == device_count) {
        page_allocator_free(pager);
        return NULL;
    }

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VK_DEVICE] count=%u", device_count);
#endif

    VkPhysicalDevice* device_list = vkc_physical_device_list(pager, instance, device_count);
    if (!device_list) {
        page_allocator_free(pager);
        return NULL;
    }

#if defined(DEBUG) && (1 == DEBUG)
    for (size_t i = 0; i < device_count; i++) {
        VkPhysicalDevice tmp = device_list[i];
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(tmp, &props);
        LOG_DEBUG("[VK_DEVICE] Option %u: %s, type=%d", i, props.deviceName, props.deviceType);
    }
#endif

    if (!vkc_physical_device_select(device, device_list, device_count)) {
        LOG_ERROR("No suitable Vulkan device found.");
        page_allocator_free(pager);
        return NULL;
    }

    page_free(pager, device_list); // Prevent silent leaks

    VkDeviceQueueCreateInfo queue_info = vkc_physical_device_queue_create_info(device);
    VkDeviceCreateInfo logical_info = vkc_logical_device_create_info(device, queue_info);

    VkResult result
        = vkCreateDevice(device->physical, &logical_info, &device->allocator, &device->logical);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        page_allocator_free(device->pager);
        return NULL;
    }

    vkGetDeviceQueue(device->logical, device->queue_family_index, 0, &device->queue);

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VK_DEVICE] Vulkan physical device created successfully @ %p", device->physical);
    LOG_DEBUG("[VK_DEVICE] Vulkan logical device created successfully @ %p", device->logical);
    LOG_DEBUG("[VK_DEVICE] Vulkan device queue created successfully @ %p", device->queue);
#endif

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
