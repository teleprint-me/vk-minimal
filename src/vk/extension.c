/**
 * @file src/vk/extension.c
 */

#include "core/memory.h"
#include "core/logger.h"
#include "utf8/raw.h"
#include "vk/extension.h"

#include <stdlib.h>

/**
 * @section Private Extension Request and Response Interface
 */

static VkcExtensionRequest*
vkc_extension_request(PageAllocator* allocator, const char* const* names, uint32_t count) {
    if (!allocator || !names || !(*names) || count == 0) {
        LOG_ERROR("Invalid arguments (allocator=%p, names=%p, count=%u)", allocator, names, count);
        return NULL;
    }
    VkcExtensionRequest* req
        = page_malloc(allocator, sizeof(VkcExtensionRequest), alignof(VkcExtensionRequest));
    if (!req) {
        return NULL;
    }
    *req = (VkcExtensionRequest) {.count = count, .names = names};
    return req;
}

static VkcExtensionResponse* vkc_extension_response(PageAllocator* allocator) {
    if (!allocator) {
        return NULL;
    }
    VkcExtensionResponse* res
        = page_malloc(allocator, sizeof(VkcExtensionResponse), alignof(VkcExtensionResponse));
    if (!res) {
        return NULL;
    }
    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &res->count, NULL);
    if (result != VK_SUCCESS || res->count == 0) {
        LOG_ERROR("Failed to enumerate extension properties (error: %u)", result);
        return NULL;
    }
    size_t size = res->count * sizeof(VkExtensionProperties);
    res->properties = page_malloc(allocator, size, alignof(VkExtensionProperties));
    if (!res->properties) {
        LOG_ERROR("Failed to allocate extension properties array");
        return NULL;
    }
    memset(res->properties, 0, size);
    result = vkEnumerateInstanceExtensionProperties(NULL, &res->count, res->properties);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to enumerate extension properties (error: %u)", result);
        return NULL;
    }
    return res;
}

/**
 * @section Public Function
 */

VkcExtension* vkc_extension_create(const char* const* names, uint32_t count, size_t capacity) {
    if (!names || !(*names) || count == 0 || capacity == 0) {
        LOG_ERROR("Invalid arguments to vkc_extension_create");
        return NULL;
    }

    PageAllocator* allocator = page_allocator_create(capacity);
    if (!allocator) {
        return NULL;
    }

    VkcExtensionRequest* req = vkc_extension_request(allocator, names, count);
    if (!req) {
        page_allocator_free(allocator);
        return NULL;
    }

    VkcExtensionResponse* res = vkc_extension_response(allocator);
    if (!res) {
        page_allocator_free(allocator);
        return NULL;
    }

    VkcExtension* ext = page_malloc(allocator, sizeof(VkcExtension), alignof(VkcExtension));
    if (!ext) {
        page_allocator_free(allocator);
        return NULL;
    }

    *ext = (VkcExtension) {
        .allocator = allocator,
        .request = req,
        .response = res,
    };

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[EXT_CREATE] %u requested extensions, %u found", count, res->count);
#endif

    return ext;
}

void vkc_extension_free(VkcExtension* ext) {
    if (!ext) {
        return;
    }

    if (ext->allocator) {
        page_allocator_free(ext->allocator);
    }
}

bool vkc_extension_match_index(VkcExtension* ext, uint32_t index, VkExtensionProperties* out) {
    if (!ext || !ext->response || !out) {
        return false;
    }
    if (index >= ext->response->count) {
        return false;
    }
    *out = ext->response->properties[index];
    return true;
}

bool vkc_extension_match_name(VkcExtension* ext, const char* name, VkExtensionProperties* out) {
    if (!ext || !ext->response || !name || !out) {
        return false;
    }
    for (uint32_t i = 0; i < ext->response->count; ++i) {
        if (0 == utf8_raw_compare(name, ext->response->properties[i].extensionName)) {
            *out = ext->response->properties[i];
            return true;
        }
    }
    return false;
}

bool vkc_extension_match_request(VkcExtension* ext) {
    for (uint32_t i = 0; i < ext->request->count; ++i) {
        VkExtensionProperties prop = {0};
        if (vkc_extension_match_name(ext, ext->request->names[i], &prop)) {
#if defined(DEBUG) && (1 == DEBUG)
            LOG_DEBUG(
                "[VkExtensionProperties] Supported: %s (specVersion=%u)",
                prop.extensionName,
                prop.specVersion
            );
#endif
            return true;
        }
    }
    LOG_WARN("[VkExtensionProperties] One or more requested extensions are not present.");
    return false;
}

void vkc_extension_log_info(VkcExtension* ext) {
    LOG_INFO("[VkExtensionProperties] [Request] count=%u", ext->request->count);
    for (uint32_t i = 0; i < ext->request->count; ++i) {
        LOG_INFO("[VkExtensionProperties] [Request] index=%u, name=%s", i, ext->request->names[i]);
    }
    LOG_INFO("[VkExtensionProperties] [Response] count=%u", ext->response->count);
    for (uint32_t i = 0; i < ext->response->count; ++i) {
        LOG_INFO(
            "[VkExtensionProperties] [Response] index=%u, name=%s, specVersion=%u",
            i,
            ext->response->properties[i].extensionName,
            ext->response->properties[i].specVersion
        );
    }
}
