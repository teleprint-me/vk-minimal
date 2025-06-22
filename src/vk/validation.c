/**
 * @file src/vk/validation.c
 */

#include "core/memory.h"
#include "core/logger.h"
#include "utf8/raw.h"
#include "vk/validation.h"

/**
 * @section Internal functions
 */

static VkcValidationLayerRequest* vkc_validation_layer_request(
    PageAllocator* allocator, const char* const* names, const uint32_t count
) {
    if (!allocator || !names || !(*names) || 0 == count) {
        LOG_ERROR(
            "Invalid arguments (allocator=%p, layers=%p, layer_count=%u)", allocator, names, count
        );
        return NULL;
    }

    VkcValidationLayerRequest* request = page_malloc(
        allocator, sizeof(VkcValidationLayerRequest), alignof(VkcValidationLayerRequest)
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
vkc_validation_layer_response(PageAllocator* allocator, VkcValidationLayerRequest* request) {
    if (!allocator || !request || !request->names || !(*request->names) || 0 == request->count) {
        LOG_ERROR("Invalid arguments (layers=%p, layer_count=%u)", request->names, request->count);
        return NULL;
    }

    VkcValidationLayerResponse* response = page_malloc(
        allocator, sizeof(VkcValidationLayerResponse), alignof(VkcValidationLayerResponse)
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
    response->properties = page_malloc(allocator, size, alignof(VkLayerProperties));
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

    PageAllocator* allocator = page_allocator_create(capacity);
    if (!allocator) {
        return NULL;
    }

    VkcValidationLayerRequest* req = vkc_validation_layer_request(allocator, names, count);
    if (!req) {
        page_allocator_free(allocator);
        return NULL;
    }

    VkcValidationLayerResponse* res = vkc_validation_layer_response(allocator, req);
    if (!res) {
        page_allocator_free(allocator);
        return NULL;
    }

    VkcValidationLayer* layer
        = page_malloc(allocator, sizeof(VkcValidationLayer), alignof(VkcValidationLayer));
    if (!layer) {
        page_allocator_free(allocator);
        return NULL;
    }

    *layer = (VkcValidationLayer) {
        .allocator = allocator,
        .request = req,
        .response = res,
    };

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VL_CREATE] %u requested validation layers, %u found", count, res->count);
#endif

    return layer;
}

void vkc_validation_layer_free(VkcValidationLayer* layer) {
    if (layer && layer->allocator) {
        page_allocator_free(layer->allocator); // free everything
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
        if (0 == utf8_raw_compare(name, layer->response->properties[i].layerName)) {
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
#if defined(DEBUG) && (1 == DEBUG)
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
