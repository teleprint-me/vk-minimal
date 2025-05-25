// examples/validate.c
#include "core/logger.h"
#include "core/memory.h"
#include "core/lease.h"

#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

/**
 * Create a Vulkan Instance
 */

#define VALIDATION_LAYER_COUNT 1 /// @warning Cannot be a variable

typedef struct LayerRequest {
    uint32_t count;
    const char* const* names;
} LayerRequest;

typedef struct LayerAvailable {
    uint32_t count;
    VkLayerProperties* properties;
} LayerAvailable;

typedef struct Layer {
    LayerRequest* request;
    LayerAvailable* available;
} Layer;

LayerRequest*
layer_create_request(LeaseOwner* owner, const char* const* names, const uint32_t count) {
    LayerRequest* request
        = lease_alloc_owned_address(owner, sizeof(LayerRequest), alignof(LayerRequest));
    if (!request) {
        return NULL;
    }

    *request = (LayerRequest) {
        .count = count,
        .names = names,
    };

    return request;
}

LayerAvailable* layer_create_available(LeaseOwner* owner, LayerRequest* request) {
    if (!request || !request->names || !(*request->names) || 0 == request->count) {
        LOG_ERROR("Invalid arguments (layers=%p, layer_count=%u)", request->names, request->count);
        return NULL;
    }

    LayerAvailable* available
        = lease_alloc_owned_address(owner, sizeof(LayerAvailable), alignof(LayerAvailable));
    if (!available) {
        return NULL;
    }

    VkResult result;
    result = vkEnumerateInstanceLayerProperties(&available->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer property count (error code: %u)", result);
        return NULL;
    }
    if (0 == available->count) {
        LOG_ERROR("No validation layers available");
        return NULL;
    }

    size_t size = available->count * sizeof(VkLayerProperties);
    available->properties = lease_alloc_owned_address(owner, size, alignof(VkLayerProperties));
    if (!available->properties || !memset(available->properties, 0, size)) {
        LOG_ERROR("Memory allocation failed for layer properties");
        return NULL;
    }

    result = vkEnumerateInstanceLayerProperties(&available->count, available->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer properties (error code: %u)", result);
        return NULL;
    }

    return available;
}

Layer* layer_create(LeaseOwner* owner, const char* const* names, const uint32_t count) {
    if (!owner || !names || !(*names) || 0 == count) {
        LOG_ERROR("Invalid arguments (owner=%p, layers=%p, layer_count=%u)", owner, names, count);
        return NULL;
    }

    LayerRequest* request = layer_create_request(owner, names, count);
    if (!request) {
        return NULL;
    }

    LayerAvailable* available = layer_create_available(owner, request);
    if (!available) {
        return NULL;
    }

    Layer* layer = lease_alloc_owned_address(owner, sizeof(Layer), alignof(Layer));
    if (!layer) {
        return NULL;
    }

    *layer = (Layer) {
        .available = available,
        .request = request,
    };

    return layer;
}

bool layer_validate(Layer* layer) {
    for (uint32_t i = 0; i < layer->request->count; ++i) {
        bool found = false;
        for (uint32_t j = 0; j < layer->available->count; ++j) {
            if (strcmp(layer->request->names[i], layer->available->properties[j].layerName)
                == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            LOG_WARN("Requested layer not available: %s", layer->request->names[i]);
            return false;
        }
    }
    return true;
}

VkLayerProperties* layer_query() {
    return NULL; // stub
}

void layer_log(Layer* layer) {
    LOG_INFO("[VkLayerProperties] request->count=%zu", layer->request->count);
    for (uint32_t i = 0; i < layer->request->count; i++) {
        LOG_INFO("[VkLayerProperties] request->names[%zu]=%s", i, layer->request->names[i]);
    }
    LOG_INFO("Property Count: %zu", layer->available->count);
    for (uint32_t i = 0; i < layer->available->count; i++) {
        LOG_INFO(
            "Property: index=%zu, name=%s, description=%s",
            i,
            layer->available->properties[i].layerName,
            layer->available->properties[i].description
        );
    }
}

int main(void) {
    const char* const layers[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};
    LeaseOwner* owner = lease_create_owner(1024); // size in bytes
    Layer* layer = layer_create(owner, layers, VALIDATION_LAYER_COUNT);
    if (!layer) {
        return EXIT_FAILURE;
    }
    layer_log(layer);
    if (!layer_validate(layer)) {
        LOG_ERROR("One or more requested layers are not available.");
    }
    LOG_INFO("Selected layer: %s", layers[0]);
    lease_free_owner(owner); // free everything
    return EXIT_SUCCESS;
}
