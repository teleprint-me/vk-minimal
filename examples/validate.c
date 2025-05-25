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

typedef struct Layer {
    uint32_t count;
    const char* const* names;
} Layer;

typedef struct LayerProperty {
    uint32_t count;
    VkLayerProperties* layers;
} LayerProperty;

Layer* layer_create(LeaseOwner* owner, const char* const* names, const uint32_t count) {
    Layer* layer = lease_alloc_owned_address(owner, sizeof(Layer), alignof(Layer));
    if (!layer) {
        return NULL;
    }

    *layer = (Layer) {
        .count = count,
        .names = names,
    };

    return layer;
}

LayerProperty* layer_create_property(LeaseOwner* owner, Layer* layer) {
    if (!layer || !layer->names || !(*layer->names) || 0 == layer->count) {
        LOG_ERROR("Invalid arguments (layers=%p, layer_count=%u)", layer->names, layer->count);
        return NULL;
    }

    LayerProperty* property
        = lease_alloc_owned_address(owner, sizeof(LayerProperty), alignof(LayerProperty));
    if (!property) {
        return NULL;
    }

    VkResult result;
    result = vkEnumerateInstanceLayerProperties(&property->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer property count (error code: %u)", result);
        return NULL;
    }
    if (0 == property->count) {
        LOG_ERROR("No validation layers available");
        return NULL;
    }

    size_t size = property->count * sizeof(VkLayerProperties);
    property->layers = lease_alloc_owned_address(owner, size, alignof(VkLayerProperties));
    if (!property->layers || !memset(property->layers, 0, size)) {
        LOG_ERROR("Memory allocation failed for layer properties");
        return NULL;
    }

    result = vkEnumerateInstanceLayerProperties(&property->count, property->layers);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer properties (error code: %u)", result);
        return NULL;
    }

    return property;
}

void layer_log(LayerProperty* property) {
    LOG_INFO("Supported Property Count: %zu", property->count);
    for (uint32_t i = 0; i < property->count; i++) {
        LOG_INFO(
            "Supported Property: index=%zu, name=%s, description=%s",
            i,
            property->layers[i].layerName,
            property->layers[i].description
        );
    }
}

int main(void) {
    const char* const validation_layers[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};
    LeaseOwner* owner = lease_create_owner(1024); // size in bytes
    Layer* layer = layer_create(owner, validation_layers, VALIDATION_LAYER_COUNT);
    LayerProperty* property = layer_create_property(owner, layer);
    layer_log(property);
    lease_free_owner(owner); // free everything
    return 0;
}
