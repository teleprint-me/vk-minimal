/**
 * @file include/vk/validation.h
 * @note Validation and Extensions are similar in shape and form.
 * The interfaces are duplicated for clarity and simplicity.
 */

#ifndef VKC_VALIDATION_H
#define VKC_VALIDATION_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct VkcValidationLayerRequest {
    uint32_t count;
    const char* const* names;
} VkcValidationLayerRequest;

typedef struct VkcValidationLayerResponse {
    uint32_t count;
    VkLayerProperties* properties;
} VkcValidationLayerResponse;

typedef struct VkcValidationLayer {
    PageAllocator* allocator;
    VkcValidationLayerRequest* request;
    VkcValidationLayerResponse* response;
} VkcValidationLayer;

VkcValidationLayer* vkc_validation_layer_create(const char* const* names, const uint32_t count, size_t capacity);
void vkc_validation_layer_free(VkcValidationLayer* layer);

bool vkc_validation_layer_match_index(VkcValidationLayer* layer, uint32_t index, VkLayerProperties* out);
bool vkc_validation_layer_match_name(VkcValidationLayer* layer, const char* name, VkLayerProperties* out);
bool vkc_validation_layer_match_request(VkcValidationLayer* layer);

void vkc_validation_layer_log_info(VkcValidationLayer* layer);

#endif // VKC_VALIDATION_H
