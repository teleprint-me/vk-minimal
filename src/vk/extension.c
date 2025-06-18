/**
 * @file src/vk/extension.c
 */

#include "core/logger.h"
#include "core/memory.h"
#include "vk/extension.h"

#include <string.h>
#include <stdlib.h>

/**
 * @section Internal Functions
 */

static VkcExtensionRequest*
vkc_extension_request(LeaseOwner* owner, const char* const* names, uint32_t count) {
    if (!owner || !names || !(*names) || count == 0) {
        LOG_ERROR("Invalid arguments (owner=%p, names=%p, count=%u)", owner, names, count);
        return NULL;
    }
    VkcExtensionRequest* req = lease_alloc_owned_address(
        owner, sizeof(VkcExtensionRequest), alignof(VkcExtensionRequest)
    );
    if (!req) {
        return NULL;
    }
    *req = (VkcExtensionRequest) {.count = count, .names = names};
    return req;
}

static VkcExtensionResponse* vkc_extension_response(LeaseOwner* owner) {
    if (!owner) {
        return NULL;
    }
    VkcExtensionResponse* res = lease_alloc_owned_address(
        owner, sizeof(VkcExtensionResponse), alignof(VkcExtensionResponse)
    );
    if (!res) {
        return NULL;
    }
    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &res->count, NULL);
    if (result != VK_SUCCESS || res->count == 0) {
        LOG_ERROR("Failed to enumerate extension properties (error: %u)", result);
        return NULL;
    }
    size_t size = res->count * sizeof(VkExtensionProperties);
    res->properties = lease_alloc_owned_address(owner, size, alignof(VkExtensionProperties));
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
    LeaseOwner* owner = lease_create_owner(capacity);
    if (!owner) {
        return NULL;
    }
    VkcExtensionRequest* req = vkc_extension_request(owner, names, count);
    if (!req) {
        lease_free_owner(owner);
        return NULL;
    }
    VkcExtensionResponse* res = vkc_extension_response(owner);
    if (!res) {
        lease_free_owner(owner);
        return NULL;
    }
    VkcExtension* ext
        = lease_alloc_owned_address(owner, sizeof(VkcExtension), alignof(VkcExtension));
    if (!ext) {
        lease_free_owner(owner);
        return NULL;
    }
    *ext = (VkcExtension) {.owner = owner, .request = req, .response = res};
    return ext;
}

void vkc_extension_free(VkcExtension* ext) {
    if (ext && ext->owner) {
        lease_free_owner(ext->owner);
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
        if (strcmp(name, ext->response->properties[i].extensionName) == 0) {
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
            LOG_DEBUG(
                "[VkExtensionProperties] Supported: %s (specVersion=%u)",
                prop.extensionName,
                prop.specVersion
            );
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
