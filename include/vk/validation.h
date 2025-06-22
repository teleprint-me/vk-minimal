/**
 * @file include/vk/validation.h
 * @brief Interface for querying and validating Vulkan instance validation layers.
 *
 * This module allows:
 * - Requesting a specific set of validation layers
 * - Querying available validation layers at runtime
 * - Matching by name or index
 * - Logging for debugging and diagnostics
 *
 * @note Validation and Extension interfaces are intentionally similar in structure.
 */

#ifndef VKC_VALIDATION_H
#define VKC_VALIDATION_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Represents a user-specified list of requested Vulkan validation layers.
 */
typedef struct VkcValidationLayerRequest {
    uint32_t count; /**< Number of requested layer names. */
    const char* const* names; /**< Array of null-terminated validation layer names. */
} VkcValidationLayerRequest;

/**
 * @brief Represents the list of validation layers provided by the Vulkan runtime.
 */
typedef struct VkcValidationLayerResponse {
    uint32_t count; /**< Number of available layer properties. */
    VkLayerProperties* properties; /**< Array of Vulkan validation layer properties. */
} VkcValidationLayerResponse;

/**
 * @brief Encapsulates validation layer request/response metadata and memory state.
 */
typedef struct VkcValidationLayer {
    PageAllocator* allocator; /**< Internal allocator tracking all allocations. */
    VkcValidationLayerRequest* request; /**< Pointer to requested layer names. */
    VkcValidationLayerResponse* response; /**< Pointer to Vulkan-reported layers. */
} VkcValidationLayer;

/**
 * @brief Creates a validation layer tracker from a list of requested names.
 *
 * @param names Array of requested layer names (must remain valid for the duration of the object).
 * @param count Number of requested layers.
 * @param capacity Initial capacity for the internal page allocator.
 * @return Pointer to a new VkcValidationLayer object, or NULL on failure.
 */
VkcValidationLayer* vkc_validation_layer_create(const char* const* names, const uint32_t count, size_t capacity);

/**
 * @brief Frees all memory associated with the validation layer object.
 *
 * @param layer Pointer to a VkcValidationLayer object. Safe to pass NULL.
 */
void vkc_validation_layer_free(VkcValidationLayer* layer);

/**
 * @brief Retrieves a validation layer by index from the Vulkan response set.
 *
 * @param layer Pointer to the VkcValidationLayer object.
 * @param index Index into the response list.
 * @param out Output pointer to receive the matched layer properties.
 * @return true if the index is valid and the layer is found, false otherwise.
 */
bool vkc_validation_layer_match_index(VkcValidationLayer* layer, uint32_t index, VkLayerProperties* out);

/**
 * @brief Finds a validation layer by name from the response set.
 *
 * @param layer Pointer to the VkcValidationLayer object.
 * @param name Null-terminated string of the layer name.
 * @param out Output pointer to receive the matched layer properties.
 * @return true if the layer is found by name, false otherwise.
 */
bool vkc_validation_layer_match_name(VkcValidationLayer* layer, const char* name, VkLayerProperties* out);

/**
 * @brief Checks if any of the requested layers are supported by the runtime.
 *
 * @param layer Pointer to the VkcValidationLayer object.
 * @return true if a match is found for at least one requested layer, false otherwise.
 */
bool vkc_validation_layer_match_request(VkcValidationLayer* layer);

/**
 * @brief Logs detailed information about requested and available validation layers.
 *
 * @param layer Pointer to the VkcValidationLayer object.
 */
void vkc_validation_layer_log_info(VkcValidationLayer* layer);

#endif // VKC_VALIDATION_H
