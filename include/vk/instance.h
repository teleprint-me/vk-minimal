/**
 * @file include/vk/instance.h
 * @brief High-level interface for creating and managing a Vulkan instance.
 *
 * This module encapsulates the creation, allocation, and destruction of a Vulkan `VkInstance`
 * using a custom internal memory allocator. It provides a minimal, self-contained interface
 * suitable for integrating into engines, compute frameworks, or debugging layers.
 *
 * Internally, all Vulkan host allocations are tracked using a linear-probing hash map.
 * Allocation metadata (size and alignment) is preserved per pointer and released automatically
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
 * @defgroup InstanceLayer VkC Instance Layer Properties
 * @{
 */

/**
 * @brief Container for enumerated Vulkan instance layers.
 *
 * Allocated using a tracked page allocator.
 */
typedef struct VkcInstanceLayer {
    PageAllocator* pager; /**< Internal allocator context. */
    VkLayerProperties* properties; /**< Array of enumerated layer properties. */
    uint32_t count; /**< Number of available layer properties. */
} VkcInstanceLayer;

/**
 * @brief Enumerate available Vulkan instance layers.
 *
 * @return Allocated layer property container, or NULL on failure.
 */
VkcInstanceLayer* vkc_instance_layer_create(void);

/**
 * @brief Free a previously allocated VkcInstanceLayer.
 *
 * @param layer Pointer returned by vkc_instance_layer_create().
 */
void vkc_instance_layer_free(VkcInstanceLayer* layer);

/** @} */

/**
 * @defgroup InstanceLayerMatch VkC Instance Layer Property Matches
 * @{
 */

/**
 * @brief Filtered list of matching Vulkan instance layer names.
 *
 * Used for populating `VkInstanceCreateInfo::ppEnabledLayerNames`.
 */
typedef struct VkcInstanceLayerMatch {
    PageAllocator* pager; /**< Internal allocator context. */
    char** names; /**< Array of UTF-8 layer name strings. */
    uint32_t count; /**< Number of matched layer names. */
} VkcInstanceLayerMatch;

/**
 * @brief Match a list of requested layer names against available layers.
 *
 * Invalid or unavailable names are skipped automatically.
 *
 * @param layer       Layer container returned by vkc_instance_layer_create().
 * @param names       Array of requested layer names.
 * @param name_count  Number of entries in `names`.
 * @return Filtered match structure, or NULL if no matches found.
 */
VkcInstanceLayerMatch* vkc_instance_layer_match_create(
    VkcInstanceLayer* layer, const char* const* names, const uint32_t name_count);

/**
 * @brief Free a previously allocated VkcInstanceLayerMatch.
 *
 * @param match Pointer returned by vkc_instance_layer_match_create().
 */
void vkc_instance_layer_match_free(VkcInstanceLayerMatch* match);

/** @} */

/**
 * @defgroup InstanceExtension VkC Instance Extension Properties
 * @{
 */

/**
 * @brief Container for enumerated Vulkan instance extensions.
 *
 * Allocated using a tracked page allocator.
 */
typedef struct VkcInstanceExtension {
    PageAllocator* pager; /**< Internal allocator context. */
    VkExtensionProperties* properties; /**< Array of enumerated extension properties. */
    uint32_t count; /**< Number of available extension properties. */
} VkcInstanceExtension;

/**
 * @brief Enumerate available Vulkan instance extensions.
 *
 * @return Allocated extension property container, or NULL on failure.
 */
VkcInstanceExtension* vkc_instance_extension_create(void);

/**
 * @brief Free a previously allocated VkcInstanceExtension.
 *
 * @param extension Pointer returned by vkc_instance_extension_create().
 */
void vkc_instance_extension_free(VkcInstanceExtension* extension);

/** @} */

/**
 * @defgroup InstanceExtensionMatch VkC Instance Extension Property Matches
 * @{
 */

/**
 * @brief Filtered list of matching Vulkan instance extension names.
 *
 * Used for populating `VkInstanceCreateInfo::ppEnabledExtensionNames`.
 */
typedef struct VkcInstanceExtensionMatch {
    PageAllocator* pager; /**< Internal allocator context. */
    char** names; /**< Array of UTF-8 extension name strings. */
    uint32_t count; /**< Number of matched extension names. */
} VkcInstanceExtensionMatch;

/**
 * @brief Match a list of requested instance extensions against available extensions.
 *
 * Invalid or unavailable names are skipped automatically.
 *
 * @param extension   Extension container returned by vkc_instance_extension_create().
 * @param names       Array of requested extension names.
 * @param name_count  Number of entries in `names`.
 * @return Filtered match structure, or NULL if no matches found.
 */
VkcInstanceExtensionMatch* vkc_instance_extension_match_create(
    VkcInstanceExtension* extension, const char* const* names, const uint32_t name_count);

/**
 * @brief Free a previously allocated VkcInstanceExtensionMatch.
 *
 * @param match Pointer returned by vkc_instance_extension_match_create().
 */
void vkc_instance_extension_match_free(VkcInstanceExtensionMatch* match);

/** @} */

/**
 * @defgroup Instance VkC Instance
 * @{
 */

/**
 * @brief Encapsulated Vulkan instance object with internal allocator tracking.
 */
typedef struct VkcInstance {
    PageAllocator* pager; /**< Internal allocation map (address → metadata). */
    VkInstance object; /**< Vulkan instance handle. */
    VkAllocationCallbacks allocator; /**< Allocator callbacks used for Vulkan object creation. */
} VkcInstance;

/**
 * @brief Create a Vulkan instance with the specified enabled layers and extensions.
 *
 * @param layer_match     Optional matched layer list (may be NULL).
 * @param extension_match Optional matched extension list (may be NULL).
 * @return Allocated Vulkan instance wrapper, or NULL on failure.
 */
VkcInstance* vkc_instance_create(
    VkcInstanceLayerMatch* layer_match, VkcInstanceExtensionMatch* extension_match);

/**
 * @brief Destroy a Vulkan instance and free associated tracked memory.
 *
 * @param instance Pointer returned by vkc_instance_create().
 */
void vkc_instance_free(VkcInstance* instance);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // VKC_INSTANCE_H
