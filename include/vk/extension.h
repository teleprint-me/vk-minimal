/**
 * @file include/vk/extension.h
 * @brief High-level interface for querying and validating Vulkan instance extensions.
 *
 * This module provides functionality for:
 * - Requesting and verifying specific Vulkan instance extensions
 * - Querying available extensions from the Vulkan runtime
 * - Tracking allocations via a custom PageAllocator
 * - Logging supported extensions for diagnostics
 *
 * @note Validation and Extension interfaces follow a similar structure for clarity and separation
 * of concerns.
 */

#ifndef VKC_EXTENSION_H
#define VKC_EXTENSION_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Holds the list of requested Vulkan extension names.
 */
typedef struct VkcExtensionRequest {
    uint32_t count; /**< Number of requested extension names. */
    const char* const* names; /**< Array of null-terminated extension name strings. */
} VkcExtensionRequest;

/**
 * @brief Holds the list of Vulkan extensions reported by the instance.
 */
typedef struct VkcExtensionResponse {
    uint32_t count; /**< Number of available extension properties. */
    VkExtensionProperties* properties; /**< Array of VkExtensionProperties returned from Vulkan. */
} VkcExtensionResponse;

/**
 * @brief Encapsulates the extension request, response, and associated allocator.
 */
typedef struct VkcExtension {
    PageAllocator* allocator; /**< Internal allocator tracking all extension-related allocations. */
    VkcExtensionRequest* request; /**< Pointer to requested extension names. */
    VkcExtensionResponse* response; /**< Pointer to available Vulkan extension properties. */
} VkcExtension;

/**
 * @brief Creates and populates a VkcExtension object with request and response data.
 *
 * @param names Array of requested extension names (must remain valid during use).
 * @param count Number of requested extensions.
 * @param capacity Initial capacity of the internal PageAllocator.
 * @return Pointer to a newly created VkcExtension object, or NULL on failure.
 */
VkcExtension* vkc_extension_create(const char* const* names, uint32_t count, size_t capacity);

/**
 * @brief Destroys a VkcExtension object and frees all tracked memory.
 *
 * @param extension Pointer to a VkcExtension object. Safe to pass NULL.
 */
void vkc_extension_free(VkcExtension* extension);

/**
 * @brief Matches a known extension by its index and returns its properties.
 *
 * @param extension Pointer to a valid VkcExtension object.
 * @param index Index of the extension in the response array.
 * @param out Pointer to receive the matched VkExtensionProperties.
 * @return true if a valid extension exists at the given index, false otherwise.
 */
bool vkc_extension_match_index(VkcExtension* extension, uint32_t index, VkExtensionProperties* out);

/**
 * @brief Searches for a specific extension name in the available response set.
 *
 * @param extension Pointer to a valid VkcExtension object.
 * @param name Null-terminated extension name to search for.
 * @param out Pointer to receive the matched VkExtensionProperties.
 * @return true if the extension is found, false otherwise.
 */
bool vkc_extension_match_name(VkcExtension* extension, const char* name, VkExtensionProperties* out);

/**
 * @brief Verifies if any of the requested extension names are supported.
 *
 * @param extension Pointer to a valid VkcExtension object.
 * @return true if at least one requested extension is supported, false otherwise.
 */
bool vkc_extension_match_request(VkcExtension* extension);

/**
 * @brief Logs the requested and available extension sets to the active logger.
 *
 * @param extension Pointer to a valid VkcExtension object.
 */
void vkc_extension_log_info(VkcExtension* extension);

#endif // VKC_EXTENSION_H
