/**
 * @file src/vk/device.c
 * 
 * VkcDevice Setup Flow:
 *
 *   - VkcDeviceList         ← Enumerate VkPhysicalDevice
 *   - VkcDeviceQueueFamily  ← For each VkPhysicalDevice, find usable queues
 *   - VkcDeviceLayer        ← Optional: enumerate & match device validation layers
 *   - VkcDeviceExtension    ← Optional: enumerate & match device extensions
 *   - vkc_physical_device_select() ← Selects one based on VK_QUEUE_COMPUTE_BIT
 */

#include "core/posix.h"
#include "core/memory.h"
#include "core/logger.h"
#include "utf8/raw.h"

#include "vk/allocator.h"
#include "vk/instance.h"
#include "vk/device.h"

/**
 * @name DeviceList Physical Device List
 * @{
 */

VkcDeviceList* vkc_device_list_create(VkInstance instance) {
    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcDeviceList] Failed to get global allocator.");
        return NULL;
    }

    VkcDeviceList* list = page_malloc(allocator, sizeof(*list), alignof(*list));
    if (!list) {
        LOG_ERROR("[VkcDeviceList] Failed to allocate memory to list.");
        return NULL;
    }

    *list = (VkcDeviceList) {
        .devices = NULL,
        .count = 0,
    };

    VkResult result = vkEnumeratePhysicalDevices(instance, &list->count, NULL);
    if (VK_SUCCESS != result || 0 == list->count) {
        LOG_ERROR(
            "[VkcDeviceList] No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            list->count
        );
        page_free(allocator, list);
        return NULL;
    }

    list->devices = page_malloc(
        allocator,
        list->count * sizeof(VkPhysicalDevice),
        alignof(VkPhysicalDevice)
    );

    if (!list->devices) {
        LOG_ERROR("[VkcDeviceList] Failed to allocate device list.");
        page_free(allocator, list);
        return NULL;
    }

    result = vkEnumeratePhysicalDevices(instance, &list->count, list->devices);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceList] Failed to enumerate devices (VkResult: %d)", result);
        page_free(allocator, list->devices);
        page_free(allocator, list);
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
    if (list && list->devices) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, list->devices);
        page_free(allocator, list);
    }
}

/** @} */

/**
 * @name DeviceQueueFamily Queue Family Properties
 * @{
 */

VkcDeviceQueueFamily* vkc_device_queue_family_create(VkPhysicalDevice device) {
    if (!device) {
        LOG_ERROR("[VkcDeviceQueueFamily] Invalid physical device.");
        return NULL;
    }

    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to get global allocator.");
        return NULL;
    }

    VkcDeviceQueueFamily* family = page_malloc(allocator, sizeof(*family), alignof(*family));
    if (!family) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to allocate memory to family structure.");
        return NULL;
    }

    *family = (VkcDeviceQueueFamily) {
        .properties = NULL,
        .count = 0,
    };

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family->count, NULL);
    if (0 == family->count) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to query family count.");
        page_free(allocator, family);
        return NULL;
    }

    family->properties = page_malloc(
        allocator,
        family->count * sizeof(VkQueueFamilyProperties),
        alignof(VkQueueFamilyProperties)
    );

    if (!family->properties) {
        LOG_ERROR("[VkcDeviceQueueFamily] Failed to allocate memory for %u properties.", family->count);
        page_free(allocator, family);
        return NULL;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family->count, family->properties);
    return family;
}

void vkc_device_queue_family_free(VkcDeviceQueueFamily* family) {
    if (family && family->properties) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, family->properties);
        page_free(allocator, family);
    }
}

/** @} */

/**
 * @name DeviceSelection Physical Device Selection
 * @{
 */

VkcPhysicalDevice* vkc_device_physical_create(VkcDeviceList* list) {
    if (!list) {
        LOG_ERROR("[VkcPhysicalDevice] Invalid parameters given.");
        return VK_NULL_HANDLE;
    }

    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcPhysicalDevice] Failed to get global allocator.");
        return NULL;
    }

    VkcPhysicalDevice* device = page_malloc(allocator, sizeof(*device), alignof(*device));
    if (!device) {
        LOG_ERROR("[VkcPhysicalDevice] Failed to create physical device wrapper.");
        return NULL;
    }

    *device = (VkcPhysicalDevice) {
        .object = VK_NULL_HANDLE,
        .queue_family_index = 0,
    };

    static const VkPhysicalDeviceType types[] = {
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
    };

    static const uint32_t type_count = sizeof(types) / sizeof(VkPhysicalDeviceType);

    for (uint32_t i = 0; i < type_count; i++) {
        for (uint32_t j = 0; j < list->count; j++) {
            VkPhysicalDevice candidate = list->devices[j];
            VkPhysicalDeviceProperties properties = {0};
            vkGetPhysicalDeviceProperties(candidate, &properties);

            VkcDeviceQueueFamily* family = vkc_device_queue_family_create(candidate);
            if (!family) {
                page_free(allocator, device);
                return NULL;
            }

            for (uint32_t k = 0; k < family->count; k++) {
                if (family->properties[k].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    if (types[i] == properties.deviceType) {
#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
                        LOG_DEBUG(
                            "[VkPhysicalDevice] Selected name=%s, type=%d, queue=%u, api=%u.%u.%u, driver=%u.%u.%u",
                            properties.deviceName,
                            properties.deviceType,
                            k,
                            VK_VERSION_MAJOR(properties.apiVersion),
                            VK_VERSION_MINOR(properties.apiVersion),
                            VK_VERSION_PATCH(properties.apiVersion),
                            VK_VERSION_MAJOR(properties.driverVersion),
                            VK_VERSION_MINOR(properties.driverVersion),
                            VK_VERSION_PATCH(properties.driverVersion)
                        );
#endif
                        vkc_device_queue_family_free(family);
                        device->queue_family_index = k;
                        device->object = candidate;
                        return device;
                    }
                }
            }

            vkc_device_queue_family_free(family);
        }
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkPhysicalDevice] No suitable compute-capable device found.");
#endif

    page_free(allocator, device);
    return NULL;
}

void vkc_device_physical_free(VkcPhysicalDevice* device) {
    if (device && device->object) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, device->object);
        page_free(allocator, device);
    }
}

/** @} */

/**
 * @name DeviceLayerQuery Device Layer Query
 * @{
 */

VkcDeviceLayer* vkc_device_layer_create(VkPhysicalDevice device) {
    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcDeviceLayer] Failed to get global allocator.");
        return NULL;
    } 

    VkcDeviceLayer* layer = page_malloc(allocator, sizeof(*layer), alignof(*layer));
    if (!layer) {
        LOG_ERROR("[VkcDeviceLayer] Failed to allocate device layer structure.");
        return NULL;
    }

    *layer = (VkcDeviceLayer) {
        .properties = NULL,
        .count = 0,
    };

    VkResult result = vkEnumerateDeviceLayerProperties(device, &layer->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceLayer] Failed to enumerate device layer property count.");
        page_free(allocator, layer);
        return NULL;
    }

    if (0 == layer->count) {
        LOG_ERROR("[VkcDeviceLayer] Device layer properties are unavailable.");
        page_free(allocator, layer);
        return NULL;
    }

    layer->properties = page_malloc(
        allocator,
        layer->count * sizeof(VkLayerProperties),
        alignof(VkLayerProperties) 
    );

    if (NULL == layer->properties) {
        LOG_ERROR("[VkcDeviceLayer] Failed to allocate %u device layer properties.", layer->count);
        page_free(allocator, layer);
        return NULL;
    }

    result = vkEnumerateDeviceLayerProperties(device, &layer->count, layer->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceLayer] Failed to enumerate device layer properties.");
        page_free(allocator, layer->properties);
        page_free(allocator, layer);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
        LOG_DEBUG("[VkcDeviceLayer] Found %u device layer properties.", layer->count);
        for (uint32_t i = 0; i < layer->count; i++) {
            LOG_DEBUG("[VkcDeviceLayer] i=%u, name=%s, description=%s", 
                i, layer->properties[i].layerName, layer->properties[i].description
            );
        }
#endif

    return layer;
}

void vkc_device_layer_free(VkcDeviceLayer* layer) {
    if (layer && layer->properties) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, layer->properties);
        page_free(allocator, layer);
    }
}

/** @} */

/**
 * @name DeviceLayerMatch Device Layer Match
 * @{
 */

VkcDeviceLayerMatch* vkc_device_layer_match_create(
    VkcDeviceLayer* layer, const char* const* names, const uint32_t name_count
) {
    if (!layer || !names || name_count == 0) return NULL;

    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcDeviceLayerMatch] Failed to get global allocator.");
        return NULL;
    }

    VkcDeviceLayerMatch* match = page_malloc(allocator, sizeof(*match), alignof(*match)); 
    if (!match) {
        LOG_ERROR("[VkcDeviceLayerMatch] Failed to allocate result.");
        return NULL;
    }

    *match = (VkcDeviceLayerMatch){
        .names = NULL,
        .count = 0,
    };

    // First pass: count matching layers
    for (uint32_t i = 0; i < layer->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], layer->properties[i].layerName)) {
                match->count++;
            }
        }
    }

    if (match->count == 0) {
        LOG_ERROR("[VkcDeviceLayerMatch] No requested layers were available:");
        for (uint32_t i = 0; i < name_count; i++) {
            LOG_ERROR("  - %s", names[i]);
        }
        LOG_INFO("Available device layers:");
        for (uint32_t i = 0; i < layer->count; i++) {
            LOG_INFO("  - %s", layer->properties[i].layerName);
        }
        page_free(allocator, match);
        return NULL;
    }

    match->names = page_malloc(allocator, match->count * sizeof(char*), alignof(char*));
    if (!match->names) {
        LOG_ERROR("[VkcDeviceLayerMatch] Failed to allocate name pointer array.");
        page_free(allocator, match);
        return NULL;
    }

    // Second pass: copy the matching names
    uint32_t k = 0;
    for (uint32_t i = 0; i < layer->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], layer->properties[i].layerName)) {
                char* copy = utf8_raw_copy(layer->properties[i].layerName);
                if (!copy) {
                    LOG_ERROR("[VkcDeviceLayerMatch] Failed to copy name.");
                    page_free(allocator, match->names);
                    page_free(allocator, match);
                    return NULL;
                }

                page_add(allocator, copy, utf8_raw_byte_count(copy), alignof(char));
                match->names[k++] = copy;
            }
        }
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log the results to standard output
    LOG_DEBUG("[VkcDeviceLayerMatch] Matched %u device layer properties.", match->count);
    for (uint32_t i = 0; i < match->count; i++) {
        LOG_DEBUG("[VkcDeviceLayerMatch] i=%u, name=%s", i, match->names[i]);
    }
#endif

    return match;
}

void vkc_device_layer_match_free(VkcDeviceLayerMatch* match) {
    if (match && match->names) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, match->names);
        page_free(allocator, match);
    }
}

/** @} */

/**
 * @name DeviceExtensionQuery Device Extension Query
 * @{
 */

VkcDeviceExtension* vkc_device_extension_create(VkPhysicalDevice device) {
    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcDeviceExtension] Failed to get global allocator.");
        return NULL;
    }

    VkcDeviceExtension* extension = page_malloc(allocator, sizeof(*extension), alignof(*extension));
    if (!extension) {
        LOG_ERROR("[VkcDeviceExtension] Failed to allocate extension structure.");
        return NULL;
    }

    *extension = (VkcDeviceExtension) {
        .properties = NULL,
        .count = 0,
    };

    VkResult result = vkEnumerateDeviceExtensionProperties(device, NULL, &extension->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceExtension] Failed to enumerate device extension property count.");
        page_free(allocator, extension);
        return NULL;
    }

    extension->properties = page_malloc(
        allocator,
        extension->count * sizeof(VkExtensionProperties),
        alignof(VkExtensionProperties)
    );

    if (!extension->properties) {
        LOG_ERROR(
            "[VkcDeviceExtension] Failed to allocate %u device extension properties.", extension->count);
        page_free(allocator, extension);
        return NULL;
    }

    result = vkEnumerateDeviceExtensionProperties(device, NULL, &extension->count, extension->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceExtension] Failed to enumerate device extension properties.");
        page_free(allocator, extension->properties);
        page_free(allocator, extension);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log supported extensions
    LOG_DEBUG("[VkcDeviceExtension] Found %u device extensions.", extension->count);
    for (uint32_t i = 0; i < extension->count; i++) {
        LOG_DEBUG("[VkcDeviceExtension] i=%u, name=%s", i, extension->properties[i].extensionName);
    }
#endif

    return extension;
}

void vkc_device_extension_free(VkcDeviceExtension* extension) {
    if (extension && extension->properties) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, extension->properties);
        page_free(allocator, extension);
    }
}

/** @} */

/**
 * @name DeviceExtensionMatch Device Extension Match
 * @{
 */

VkcDeviceExtensionMatch* vkc_device_extension_match_create(
    VkcDeviceExtension* extension, const char* const* names, const uint32_t name_count
) {
    if (!extension || !names || name_count == 0) return NULL;

    PageAllocator* allocator = vkc_allocator_get();
    if (!allocator) {
        LOG_ERROR("[VkcDeviceExtensionMatch] Failed to get global allocator.");
        return NULL;
    }

    VkcDeviceExtensionMatch* match = page_malloc(allocator, sizeof(*match), alignof(*match)); 
    if (!match) {
        LOG_ERROR("[VkcDeviceExtensionMatch] Failed to allocate result.");
        return NULL;
    }

    *match = (VkcDeviceExtensionMatch){
        .names = NULL,
        .count = 0,
    };

    // First pass: count matching extensions
    for (uint32_t i = 0; i < extension->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], extension->properties[i].extensionName)) {
                match->count++;
            }
        }
    }

    if (0 == match->count) {
        LOG_ERROR("[VkcDeviceExtensionMatch] No requested extensions were available:");
        for (uint32_t i = 0; i < name_count; i++) {
            LOG_ERROR("  - %s", names[i]);
        }
        LOG_INFO("Available device extensions:");
        for (uint32_t i = 0; i < extension->count; i++) {
            LOG_INFO("  - %s", extension->properties[i].extensionName);
        }
        page_free(allocator, match);
        return NULL;
    }

    match->names = page_malloc(allocator, match->count * sizeof(char*), alignof(char*));
    if (!match->names) {
        LOG_ERROR("[VkcDeviceExtensionMatch] Failed to allocate name pointer array.");
        page_free(allocator, match);
        return NULL;
    }

    // Second pass: copy the matching names
    uint32_t k = 0;
    for (uint32_t i = 0; i < extension->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], extension->properties[i].extensionName)) {
                char* copy = utf8_raw_copy(extension->properties[i].extensionName);
                if (!copy) {
                    LOG_ERROR("[VkcDeviceExtensionMatch] Failed to copy name.");
                    page_free(allocator, match->names);
                    page_free(allocator, match);
                    return NULL;
                }

                page_add(allocator, copy, utf8_raw_byte_count(copy), alignof(char));
                match->names[k++] = copy;
            }
        }
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log the results to standard output
    LOG_DEBUG("[VkcDeviceExtensionMatch] Matched %u device extension properties.", match->count);
    for (uint32_t i = 0; i < match->count; i++) {
        LOG_DEBUG("[VkcDeviceExtensionMatch] i=%u, name=%s", i, match->names[i]);
    }
#endif

    return match;
}

void vkc_device_extension_match_free(VkcDeviceExtensionMatch* match) {
    if (match && match->names) {
        PageAllocator* allocator = vkc_allocator_get();
        page_free(allocator, match->names);
        page_free(allocator, match);
    }
}

/** @} */

/**
 * @name DeviceCore Logical Device Wrapper
 * @{
 */

// VkcDevice* vkc_device_create(VkcInstance* instance) {
//     PageAllocator* pager = vkc_allocator_get();
//     if (!pager) {
//         LOG_ERROR("Failed to create device pager.");
//         return NULL;
//     }

//     VkcDevice* device = page_malloc(pager, sizeof(VkcDevice), alignof(VkcDevice));
//     if (!device) {
//         page_allocator_free(pager);
//         return NULL;
//     }

//     device->queue_family_index = UINT32_MAX;
//     device->physical = VK_NULL_HANDLE;
//     device->logical = VK_NULL_HANDLE;
//     device->queue = VK_NULL_HANDLE;
//     device->pager = pager;
//     device->allocator = vkc_page_callbacks(pager);

//     uint32_t device_count = vkc_physical_device_count(instance);
//     if (0 == device_count) {
//         page_allocator_free(pager);
//         return NULL;
//     }

// #if defined(DEBUG) && (1 == DEBUG)
//     LOG_DEBUG("[VK_DEVICE] count=%u", device_count);
// #endif

//     VkPhysicalDevice* device_list = vkc_physical_device_list(pager, instance, device_count);
//     if (!device_list) {
//         page_allocator_free(pager);
//         return NULL;
//     }

// #if defined(DEBUG) && (1 == DEBUG)
//     for (size_t i = 0; i < device_count; i++) {
//         VkPhysicalDevice tmp = device_list[i];
//         VkPhysicalDeviceProperties props = {0};
//         vkGetPhysicalDeviceProperties(tmp, &props);
//         LOG_DEBUG("[VK_DEVICE] Option %u: %s, type=%d", i, props.deviceName, props.deviceType);
//     }
// #endif

//     if (!vkc_physical_device_select(device, device_list, device_count)) {
//         LOG_ERROR("No suitable Vulkan device found.");
//         page_allocator_free(pager);
//         return NULL;
//     }

//     page_free(pager, device_list); // Prevent silent leaks

//     VkDeviceQueueCreateInfo queue_info = vkc_physical_device_queue_create_info(device);
//     VkDeviceCreateInfo logical_info = vkc_logical_device_create_info(device, queue_info);

//     VkResult result
//         = vkCreateDevice(device->physical, &logical_info, &device->allocator, &device->logical);
//     if (result != VK_SUCCESS) {
//         LOG_ERROR("Failed to create logical device: %d", result);
//         page_allocator_free(device->pager);
//         return NULL;
//     }

//     vkGetDeviceQueue(device->logical, device->queue_family_index, 0, &device->queue);

// #if defined(DEBUG) && (1 == DEBUG)
//     LOG_DEBUG("[VK_DEVICE] Vulkan physical device created successfully @ %p", device->physical);
//     LOG_DEBUG("[VK_DEVICE] Vulkan logical device created successfully @ %p", device->logical);
//     LOG_DEBUG("[VK_DEVICE] Vulkan device queue created successfully @ %p", device->queue);
// #endif

//     return device;
// }

// void vkc_device_destroy(VkcDevice* device) {
//     if (device && device->pager) {
//         if (device->logical) {
//             vkDestroyDevice(device->logical, &device->allocator);
//         }
//         page_allocator_free(device->pager);
//     }
// }
