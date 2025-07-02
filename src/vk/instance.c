/**
 * @file src/vk/instance.c
 * @brief Encapsulates the Vulkan instance and associated allocation state.
 */

#include "core/logger.h"
#include "core/memory.h"

#include "vk/validation.h"
#include "vk/extension.h"
#include "vk/allocator.h"
#include "vk/instance.h"

/**
 * @name VkC Instance Layer
 * @{
 */

typedef struct VkcInstanceLayer {
    PageAllocator* pager;
    VkLayerProperties* properties;
    uint32_t count;
} VkcInstanceLayer;

VkcInstanceLayer* vkc_instance_layer_create(size_t initial_size) {
    // Create the page allocator.
    PageAllocator* pager = page_allocator_create(initial_size);
    if (!pager) {
        LOG_ERROR("[VkcInstanceLayer] Failed to create allocator (initial_size=%zu).", initial_size);
        return NULL;
    }

    // Allocate the layer struct.
    VkcInstanceLayer* layer = page_malloc(pager, sizeof(*layer), alignof(*layer));
    if (!layer) {
        page_allocator_free(pager);
        LOG_ERROR("[VkcInstanceLayer] Failed to allocate instance layer structure.");
        return NULL;
    }

    *layer = (VkcInstanceLayer){
        .pager = pager,
        .properties = NULL,
        .count = 0,
    };

    // First call to get the layer count.
    VkResult result = vkEnumerateInstanceLayerProperties(&layer->count, NULL);
    if (VK_SUCCESS != result || 0 == layer->count) {
        LOG_ERROR("[VkcInstanceLayer] Failed to enumerate layer count (VkResult: %d).", result);
        vkc_instance_layer_free(layer);
        return NULL;
    }

    // Allocate space for properties.
    layer->properties = page_malloc(pager, layer->count * sizeof(VkLayerProperties), alignof(VkLayerProperties));
    if (!layer->properties) {
        LOG_ERROR("[VkcInstanceLayer] Failed to allocate %u layer properties.", layer->count);
        vkc_instance_layer_free(layer);
        return NULL;
    }

    // Second call to fill the properties.
    result = vkEnumerateInstanceLayerProperties(&layer->count, layer->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcInstanceLayer] Failed to populate layer properties (VkResult: %d).", result);
        vkc_instance_layer_free(layer);
        return NULL;
    }

    return layer;
}

void vkc_instance_layer_free(VkcInstanceLayer* layer) {
    if (!layer) return;

    PageAllocator* pager = layer->pager;
    if (pager) {
        page_free(pager, layer->properties);
        page_free(pager, layer);
        page_allocator_free(pager);
    }
}

/** @} */

/**
 * @name VkC Instance Layer Names
 * @{
 */

/** @} */
