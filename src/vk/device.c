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
 * @name Physical Device List
 * @{
 */

VkcDeviceList* vkc_device_list_create(VkInstance instance) {
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

    VkResult result = vkEnumeratePhysicalDevices(instance, &list->count, NULL);
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

    result = vkEnumeratePhysicalDevices(instance, &list->count, list->devices);
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

    #if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
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
    #if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
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

/**
 * @name Device Layers
 * @{
 */

VkcDeviceLayer* vkc_device_layer_create(VkPhysicalDevice device) {
    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        LOG_ERROR("[VkcDeviceLayer] Failed to create allocator context.");
        return NULL;
    } 

    VkcDeviceLayer* layer = page_malloc(allocator, sizeof(*layer), alignof(*layer));
    if (!layer) {
        LOG_ERROR("[VkcDeviceLayer] Failed to allocate device layer structure.");
        page_allocator_free(allocator);
        return NULL;
    }

    *layer = (VkcDeviceLayer) {
        .allocator = allocator,
        .properties = NULL,
        .count = 0,
    };

    VkResult result = vkEnumerateDeviceLayerProperties(device, &layer->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceLayer] Failed to enumerate device layer property count.");
        page_allocator_free(allocator);
        return NULL;
    }

    if (0 == layer->count) {
        LOG_ERROR("[VkcDeviceLayer] Device layer properties are unavailable.");
        page_allocator_free(allocator);
        return NULL;
    }

    layer->properties = page_malloc(
        allocator,
        layer->count * sizeof(VkLayerProperties),
        alignof(VkLayerProperties) 
    );

    if (NULL == layer->properties) {
        LOG_ERROR("[VkcDeviceLayer] Failed to allocate %u device layer properties.", layer->count);
        return NULL;
    }

    result = vkEnumerateDeviceLayerProperties(device, &layer->count, layer->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceLayer] Failed to enumerate device layer properties.");
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
    if (layer && layer->allocator) {
        page_allocator_free(layer->allocator);
    }
}

/** @} */

/**
 * @name Device Layer Match
 * @{
 */

VkcDeviceLayerMatch* vkc_device_layer_match_create(
    VkcDeviceLayer* layer, const char* const* names, const uint32_t name_count
) {
    if (!layer || !names || name_count == 0) return NULL;

    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        LOG_ERROR("[VkcDeviceLayerMatch] Failed to create allocator.");
        return NULL;
    }

    VkcDeviceLayerMatch* match = page_malloc(allocator, sizeof(*match), alignof(*match)); 
    if (!match) {
        LOG_ERROR("[VkcDeviceLayerMatch] Failed to allocate result.");
        page_allocator_free(allocator);
        return NULL;
    }

    *match = (VkcDeviceLayerMatch){
        .allocator = allocator,
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
        page_allocator_free(allocator);
        return NULL;
    }

    match->names = page_malloc(allocator, match->count * sizeof(char*), alignof(char*));
    if (!match->names) {
        LOG_ERROR("[VkcDeviceLayerMatch] Failed to allocate name pointer array.");
        page_allocator_free(allocator);
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
                    page_allocator_free(allocator);
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
    if (match && match->allocator) {
        page_allocator_free(match->allocator);
    }
}

/** @} */

/**
 * @name Device Extensions
 * @{
 */

VkcDeviceExtension* vkc_device_extension_create(VkPhysicalDevice device) {
    PageAllocator* allocator = page_allocator_create(1);
    if (!allocator) {
        LOG_ERROR("[VkcDeviceExtension] Failed to create allocator.");
        return NULL;
    }

    VkcDeviceExtension* extension = page_malloc(allocator, sizeof(*extension), alignof(*extension));
    if (!extension) {
        LOG_ERROR("[VkcDeviceExtension] Failed to allocate extension structure.");
        page_allocator_free(allocator);
        return NULL;
    }

    *extension = (VkcDeviceExtension) {
        .allocator = allocator,
        .properties = NULL,
        .count = 0,
    };

    VkResult result = vkEnumerateDeviceExtensionProperties(device, NULL, &extension->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceExtension] Failed to enumerate device extension property count.");
        page_allocator_free(allocator);
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
        page_allocator_free(allocator);
        return NULL;
    }

    result = vkEnumerateDeviceExtensionProperties(device, NULL, &extension->count, extension->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcDeviceExtension] Failed to enumerate device extension properties.");
        page_allocator_free(allocator);
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
    if (extension && extension->allocator) {
        page_allocator_free(extension->allocator);
    }
}

/** @} */

/**
 * @name Public methods
 * {@
 */

// VkcDevice* vkc_device_create(VkcInstance* instance) {
//     PageAllocator* pager = page_allocator_create(1);
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
