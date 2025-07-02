/**
 * @file include/vk/instance.h
 * @brief High-level interface for creating and managing a Vulkan instance.
 *
 * This module encapsulates the creation, allocation, and destruction of a Vulkan `VkInstance`
 * using a custom internal memory allocator. It provides a minimal, self-contained interface
 * suitable for integrating into engines, compute frameworks, or debugging layers.
 *
 * Internally, all Vulkan host allocations are tracked using a linear-probing hash map.
 * Allocation metadata (size, alignment, scope) is preserved per pointer and released automatically
 * on destruction. This simplifies debugging, leak detection, and allocator introspection.
 */

#ifndef VKC_INSTANCE_H
#define VKC_INSTANCE_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name VkC Instance Layer Properties
 * @{
 */

typedef struct VkcInstanceLayer {
    PageAllocator* pager;
    VkLayerProperties* properties;
    uint32_t count;
} VkcInstanceLayer;

VkcInstanceLayer* vkc_instance_layer_create(void);
void vkc_instance_layer_free(VkcInstanceLayer* layer);

/** @} */

/**
 * @name VkC Instance Layer Property Matches
 * @{
 */

typedef struct VkcInstanceLayerMatch {
    PageAllocator* pager;
    char** names;
    uint32_t count;
} VkcInstanceLayerMatch;

VkcInstanceLayerMatch* vkc_instance_layer_match_create(
    VkcInstanceLayer* layer, const char* const* names, const uint32_t name_count);
void vkc_instance_layer_match_free(VkcInstanceLayerMatch* match);

/** @} */

/**
 * @name VkC Instance Extension Properties
 * @{
 */

typedef struct VkcInstanceExtension {
    PageAllocator* pager;
    VkExtensionProperties* properties;
    uint32_t count;
} VkcInstanceExtension;

VkcInstanceExtension* vkc_instance_extension_create(void);
void vkc_instance_extension_free(VkcInstanceExtension* extension);

/** @} */

/**
 * @name VkC Instance Extension Property Matches
 * @{
 */

typedef struct VkcInstanceExtensionMatch {
    PageAllocator* pager;
    char** names;
    uint32_t count;
} VkcInstanceExtensionMatch;

VkcInstanceExtensionMatch* vkc_instance_extension_match_create(
    VkcInstanceExtension* extension, const char* const* names, const uint32_t name_count
);
void vkc_instance_extension_match_free(VkcInstanceExtensionMatch* match);

/** @} */

typedef struct VkcInstance {
    PageAllocator* pager; /**< Internal allocation map (address → metadata). */
    VkInstance object; /**< Vulkan instance handle. */
    VkAllocationCallbacks allocator; /**< Vulkan allocator callbacks for tracked memory. */
} VkcInstance;

VkcInstance* vkc_instance_create(size_t page_size);
void vkc_instance_destroy(VkcInstance* instance);

#ifdef __cplusplus
}
#endif

#endif // VKC_INSTANCE_H
