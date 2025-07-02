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
    PageAllocator* pager = page_allocator_create(initial_size);
    if (NULL == pager) {
        LOG_ERROR("[VkcInstanceLayer] Failed to create allocator (initial_size=%zu).", initial_size);
        return NULL;
    }

    VkcInstanceLayer* layer = page_malloc(pager, sizeof(VkcInstanceLayer), alignof(VkcInstanceLayer));
    if (NULL == layer) {
        return NULL;
    }

    layer->pager = pager;
    layer->properties = NULL;
    layer->count = 0;

    VkResult result;
    result = vkEnumerateInstanceLayerProperties(&layer->count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcInstanceLayer] Failed to enumerate instance layer property count.");
        return NULL;
    }

    layer->properties = page_malloc(
        pager, layer->count * sizeof(VkLayerProperties), alignof(VkLayerProperties));
    if (NULL == layer->properties) {
        LOG_ERROR("[VkcInstanceLayer] Failed to allocate %u instance layer properties.", layer->count);
        return NULL;
    }
    memset(layer->properties, 0, layer->count * sizeof(VkLayerProperties));

    result = vkEnumerateInstanceLayerProperties(&layer->count, layer->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcInstanceLayer] Failed to enumerate instance layer properties.");
        return NULL;
    }

    layer->pager = pager;
    return layer;
}

void vkc_instance_layer_free(VkcInstanceLayer* layer) {
    if (NULL == layer || NULL == layer->pager) return;
    PageAllocator* pager = layer->pager;
    page_free(pager, layer->properties);
    page_free(pager, layer);
    page_allocator_free(pager);
}

/** @} */

/**
 * @name VkC Instance Layer Names
 * @{
 */

/** @} */
