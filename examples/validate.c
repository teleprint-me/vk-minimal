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

LayerRequest* layer_create_request(
    LeaseOwner* owner, const char* const* names, const uint32_t count
) {
    if (!owner || !names || !(*names) || 0 == count) {
        LOG_ERROR("Invalid arguments (owner=%p, layers=%p, layer_count=%u)", owner, names, count);
        return NULL;
    }

    LayerRequest* request = lease_alloc_owned_address(
        owner, sizeof(LayerRequest), alignof(LayerRequest)
    );
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
    if (!owner || !request || !request->names || !(*request->names) || 0 == request->count) {
        LOG_ERROR("Invalid arguments (layers=%p, layer_count=%u)", request->names, request->count);
        return NULL;
    }

    LayerAvailable* available = lease_alloc_owned_address(
        owner, sizeof(LayerAvailable), alignof(LayerAvailable)
    );
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

bool layer_match_index(Layer* layer, uint32_t index, VkLayerProperties* out) {
    if (!layer || !layer->available || !out) {
        return false;
    }
    if (index >= layer->available->count) {
        return false;
    }
    *out = layer->available->properties[index];
    return true;
}

bool layer_match_name(Layer* layer, const char* name, VkLayerProperties* out) {
    if (!layer || !layer->available || !name || !out) {
        return false;
    }
    for (uint32_t i = 0; i < layer->available->count; ++i) {
        if (0 == strcmp(name, layer->available->properties[i].layerName)) {
            *out = layer->available->properties[i];
            return true;
        }
    }
    return false;
}

bool layer_match_request(Layer* layer) {
    for (uint32_t i = 0; i < layer->request->count; ++i) {
        VkLayerProperties props = {0};
        if (layer_match_name(layer, layer->request->names[i], &props)) {
            LOG_DEBUG("Requested layer(s) available: name=%s, desc=%s", props.layerName, props.description);
            return true;
        }
    }
    LOG_WARN("Requested layer(s) not available.");
    return false;
}

void layer_log_info(Layer* layer) {
    LOG_INFO("[VkLayerProperties] [Request] count=%zu", layer->request->count);
    for (uint32_t i = 0; i < layer->request->count; i++) {
        LOG_INFO("[VkLayerProperties] [Request] index=%zu, name=%s", i, layer->request->names[i]);
    }
    LOG_INFO("[VkLayerProperties] [Available] count=%zu", layer->available->count);
    for (uint32_t i = 0; i < layer->available->count; i++) {
        LOG_INFO(
            "[VkLayerProperties] [Available] index=%zu, name=%s, description=%s",
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
    layer_log_info(layer);
    if (!layer_match_request(layer)) {
        LOG_ERROR("One or more requested layers are not available.");
    }

    // Lookup by name
    VkLayerProperties prop;
    if (layer_match_name(layer, "VK_LAYER_KHRONOS_validation", &prop)) {
        LOG_INFO("Found property (by name): %s - %s", prop.layerName, prop.description);
    } else {
        LOG_WARN("Property by name not found.");
    }

    // Lookup by index
    if (layer_match_index(layer, 0, &prop)) {
        LOG_INFO("Found property (by index 0): %s - %s", prop.layerName, prop.description);
    } else {
        LOG_WARN("Property by index not found.");
    }

    lease_free_owner(owner); // free everything
    return EXIT_SUCCESS;
}
