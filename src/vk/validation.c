/**
 * @file src/vk/validation.c
 */

#include "core/logger.h"
#include "core/memory.h"
#include "allocator/lease.h"

#include "vk/validation.h"

/**
 * @section Internal functions
 */

static VkcValidationLayerRequest*
vkc_validation_layer_request(LeaseOwner* owner, const char* const* names, const uint32_t count) {
    if (!owner || !names || !(*names) || 0 == count) {
        LOG_ERROR("Invalid arguments (owner=%p, layers=%p, layer_count=%u)", owner, names, count);
        return NULL;
    }

    VkcValidationLayerRequest* request = lease_alloc_owned_address(
        owner, sizeof(VkcValidationLayerRequest), alignof(VkcValidationLayerRequest)
    );
    if (!request) {
        return NULL;
    }

    *request = (VkcValidationLayerRequest) {
        .count = count,
        .names = names,
    };

    return request;
}

static VkcValidationLayerResponse*
vkc_validation_layer_response(LeaseOwner* owner, VkcValidationLayerRequest* request) {
    if (!owner || !request || !request->names || !(*request->names) || 0 == request->count) {
        LOG_ERROR("Invalid arguments (layers=%p, layer_count=%u)", request->names, request->count);
        return NULL;
    }

    VkcValidationLayerResponse* response = lease_alloc_owned_address(
        owner, sizeof(VkcValidationLayerResponse), alignof(VkcValidationLayerResponse)
    );
    if (!response) {
        return NULL;
    }

    VkResult result;
    result = vkEnumerateInstanceLayerProperties(&response->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer property count (error code: %u)", result);
        return NULL;
    }
    if (0 == response->count) {
        LOG_ERROR("No validation layers response");
        return NULL;
    }

    size_t size = response->count * sizeof(VkLayerProperties);
    response->properties = lease_alloc_owned_address(owner, size, alignof(VkLayerProperties));
    if (!response->properties || !memset(response->properties, 0, size)) {
        LOG_ERROR("Memory allocation failed for layer properties");
        return NULL;
    }

    result = vkEnumerateInstanceLayerProperties(&response->count, response->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer properties (error code: %u)", result);
        return NULL;
    }

    return response;
}

/**
 * @section Public functions
 */

VkcValidationLayer*
vkc_validation_layer_create(const char* const* names, const uint32_t count, size_t capacity) {
    if (!names || !(*names) || 0 == count || 0 == capacity) {
        LOG_ERROR("Invalid arguments (layers=%p, count=%u, capacity=%zu", names, count, capacity);
        return NULL;
    }

    LeaseOwner* owner = lease_create_owner(capacity);
    if (!owner) {
        return NULL;
    }

    VkcValidationLayerRequest* request = vkc_validation_layer_request(owner, names, count);
    if (!request) {
        lease_free_owner(owner);
        return NULL;
    }

    VkcValidationLayerResponse* response = vkc_validation_layer_response(owner, request);
    if (!response) {
        lease_free_owner(owner);
        return NULL;
    }

    VkcValidationLayer* layer
        = lease_alloc_owned_address(owner, sizeof(VkcValidationLayer), alignof(VkcValidationLayer));
    if (!layer) {
        lease_free_owner(owner);
        return NULL;
    }

    *layer = (VkcValidationLayer) {
        .owner = owner,
        .request = request,
        .response = response,
    };

    return layer;
}

void vkc_validation_layer_free(VkcValidationLayer* layer) {
    if (layer && layer->owner) {
        LeaseOwner* owner = layer->owner;
        lease_free_owner(owner); // free everything
    }
}

bool vkc_validation_layer_match_index(
    VkcValidationLayer* layer, uint32_t index, VkLayerProperties* out
) {
    if (!layer || !layer->response || !out) {
        return false;
    }
    if (index >= layer->response->count) {
        return false;
    }
    *out = layer->response->properties[index];
    return true;
}

bool vkc_validation_layer_match_name(
    VkcValidationLayer* layer, const char* name, VkLayerProperties* out
) {
    if (!layer || !layer->response || !name || !out) {
        return false;
    }
    for (uint32_t i = 0; i < layer->response->count; ++i) {
        if (0 == strcmp(name, layer->response->properties[i].layerName)) {
            *out = layer->response->properties[i];
            return true;
        }
    }
    return false;
}

bool vkc_validation_layer_match_request(VkcValidationLayer* layer) {
    for (uint32_t i = 0; i < layer->request->count; ++i) {
        VkLayerProperties props = {0};
        if (vkc_validation_layer_match_name(layer, layer->request->names[i], &props)) {
#ifdef NDEBUG
            LOG_DEBUG(
                "[VkLayerProperties] [Response] name=%s, desc=%s",
                props.layerName,
                props.description
            );
#endif
            return true;
        }
    }
    LOG_WARN("[VkLayerProperties] [Request] Failed to discover a valid response.");
    return false;
}

void vkc_validation_layer_log_info(VkcValidationLayer* layer) {
    LOG_INFO("[VkLayerProperties] [Request] count=%zu", layer->request->count);
    for (uint32_t i = 0; i < layer->request->count; i++) {
        LOG_INFO("[VkLayerProperties] [Request] index=%zu, name=%s", i, layer->request->names[i]);
    }
    LOG_INFO("[VkLayerProperties] [Response] count=%zu", layer->response->count);
    for (uint32_t i = 0; i < layer->response->count; i++) {
        LOG_INFO(
            "[VkLayerProperties] [Response] index=%zu, name=%s, description=%s",
            i,
            layer->response->properties[i].layerName,
            layer->response->properties[i].description
        );
    }
}
