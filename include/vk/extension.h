/**
 * @file include/vk/extension.h
 * @note Validation and Extensions are similar in shape and form.
 * The interfaces are duplicated for clarity and simplicity.
 */

#ifndef VKC_EXTENSION_H
#define VKC_EXTENSION_H

#include "allocator/page.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct VkcExtensionRequest {
    uint32_t count;
    const char* const* names;
} VkcExtensionRequest;

typedef struct VkcExtensionResponse {
    uint32_t count;
    VkExtensionProperties* properties;
} VkcExtensionResponse;

typedef struct VkcExtension {
    PageAllocator* allocator;
    VkcExtensionRequest* request;
    VkcExtensionResponse* response;
} VkcExtension;

VkcExtension* vkc_extension_create(const char* const* names, uint32_t count, size_t capacity);
void vkc_extension_free(VkcExtension* extension);

bool vkc_extension_match_index(VkcExtension* extension, uint32_t index, VkExtensionProperties* out);
bool vkc_extension_match_name(VkcExtension* extension, const char* name, VkExtensionProperties* out);
bool vkc_extension_match_request(VkcExtension* extension);

void vkc_extension_log_info(VkcExtension* extension);

#endif // VKC_EXTENSION_H
