/**
 * @file src/vk/instance.c
 * @brief Encapsulates the Vulkan instance and associated allocation state.
 */

#include "core/posix.h"
#include "core/memory.h"
#include "core/logger.h"
#include "utf8/raw.h"
#include "vk/allocator.h"
#include "vk/instance.h"

/**
 * @name VkC Instance Layer Properties
 * @{
 */

VkcInstanceLayer* vkc_instance_layer_create(void) {
    PageAllocator* pager = page_allocator_create(8);
    if (!pager) {
        LOG_ERROR("[VkcInstanceLayer] Failed to create allocator.");
        return NULL;
    }

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

    VkResult result = vkEnumerateInstanceLayerProperties(&layer->count, NULL);
    if (VK_SUCCESS != result || 0 == layer->count) {
        LOG_ERROR("[VkcInstanceLayer] Failed to enumerate layer count (VkResult: %d).", result);
        page_allocator_free(pager);
        return NULL;
    }

    layer->properties = page_malloc(
        pager, layer->count * sizeof(VkLayerProperties), alignof(VkLayerProperties));
    if (!layer->properties) {
        LOG_ERROR("[VkcInstanceLayer] Failed to allocate %u layer properties.", layer->count);
        page_allocator_free(pager);
        return NULL;
    }

    result = vkEnumerateInstanceLayerProperties(&layer->count, layer->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR(
            "[VkcInstanceLayer] Failed to populate layer properties (VkResult: %d).", result);
        page_allocator_free(pager);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcInstanceLayer] Found %u instance layer properties.", layer->count);
    for (uint32_t i = 0; i < layer->count; i++) {
        LOG_DEBUG("[VkcInstanceLayer] i=%u, name=%s, description=%s", 
            i, layer->properties[i].layerName, layer->properties[i].description
        );
    }
#endif

    return layer;
}

void vkc_instance_layer_free(VkcInstanceLayer* layer) {
    if (layer && layer->pager) {
        page_allocator_free(layer->pager);
    }
}

/** @} */

/**
 * @name VkC Instance Layer Property Matches
 * @{
 */

VkcInstanceLayerMatch* vkc_instance_layer_match_create(
    VkcInstanceLayer* layer, const char* const* names, const uint32_t name_count
) {
    if (!layer || !names || name_count == 0) return NULL;

    PageAllocator* pager = page_allocator_create(8);
    if (!pager) {
        LOG_ERROR("[VkcInstanceLayerMatch] Failed to create allocator.");
        return NULL;
    }

    VkcInstanceLayerMatch* match = page_malloc(pager, sizeof(*match), alignof(*match)); 
    if (!match) {
        LOG_ERROR("[VkcInstanceLayerMatch] Failed to allocate result.");
        page_allocator_free(pager);
        return NULL;
    }

    *match = (VkcInstanceLayerMatch){
        .pager = pager,
        .names = NULL,
        .count = 0,
    };

    // First pass: count matching layers
    for (uint32_t i = 0; i < layer->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], layer->properties[i].layerName)) {
                match->count++;
            }
        }
    }

    if (match->count == 0) {
        LOG_ERROR("[VkcInstanceLayerMatch] No requested layers were available:");
        for (uint32_t i = 0; i < name_count; i++) {
            LOG_ERROR("  - %s", names[i]);
        }
        LOG_INFO("Available instance layers:");
        for (uint32_t i = 0; i < layer->count; i++) {
            LOG_INFO("  - %s", layer->properties[i].layerName);
        }
        page_allocator_free(pager);
        return NULL;
    }

    match->names = page_malloc(pager, match->count * sizeof(char*), alignof(char*));
    if (!match->names) {
        LOG_ERROR("[VkcInstanceLayerMatch] Failed to allocate name pointer array.");
        page_allocator_free(pager);
        return NULL;
    }

    // Second pass: copy the matching names
    uint32_t k = 0;
    for (uint32_t i = 0; i < layer->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], layer->properties[i].layerName)) {
                char* copy = utf8_raw_copy(layer->properties[i].layerName);
                if (!copy) {
                    LOG_ERROR("[VkcInstanceLayerMatch] Failed to copy name.");
                    page_allocator_free(pager);
                    return NULL;
                }

                page_add(pager, copy, utf8_raw_byte_count(copy), alignof(char));
                match->names[k++] = copy;
            }
        }
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log the results to standard output
    LOG_DEBUG("[VkcInstanceLayerMatch] Matched %u instance layer properties.", match->count);
    for (uint32_t i = 0; i < match->count; i++) {
        LOG_DEBUG("[VkcInstanceLayerMatch] i=%u, name=%s", i, match->names[i]);
    }
#endif

    return match;
}

void vkc_instance_layer_match_free(VkcInstanceLayerMatch* match) {
    if (match && match->pager) {
        page_allocator_free(match->pager);
    }
}

/** @} */

/**
 * @name VkC Instance Extension Properties
 * @{
 */

VkcInstanceExtension* vkc_instance_extension_create(void) {
    PageAllocator* pager = page_allocator_create(8);
    if (!pager) {
        LOG_ERROR("[VkcInstanceExtension] Failed to create allocator.");
        return NULL;
    }

    VkcInstanceExtension* ext = page_malloc(pager, sizeof(*ext), alignof(*ext));
    if (!ext) {
        LOG_ERROR("[VkcInstanceExtension] Failed to allocate instance extension structure.");
        page_allocator_free(pager);
        return NULL;
    }

    *ext = (VkcInstanceExtension){
        .pager = pager,
        .properties = NULL,
        .count = 0,
    };

    VkResult result = vkEnumerateInstanceExtensionProperties(NULL, &ext->count, NULL);
    if (VK_SUCCESS != result || 0 == ext->count) {
        LOG_ERROR("[VkcInstanceExtension] Failed to enumerate extension count (VkResult: %d).", result);
        page_allocator_free(pager);
        return NULL;
    }

    ext->properties = page_malloc(
        pager, ext->count * sizeof(VkExtensionProperties), alignof(VkExtensionProperties));
    if (!ext->properties) {
        LOG_ERROR("[VkcInstanceExtension] Failed to allocate %u extension properties.", ext->count);
        page_allocator_free(pager);
        return NULL;
    }

    result = vkEnumerateInstanceExtensionProperties(NULL, &ext->count, ext->properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcInstanceExtension] Failed to populate extension properties (VkResult: %d).", result);
        page_allocator_free(pager);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcInstanceExtension] Found %u instance extension properties.", ext->count);
    for (uint32_t i = 0; i < ext->count; i++) {
        LOG_DEBUG("[VkcInstanceExtension] i=%u, name=%s, version=%u",
            i, ext->properties[i].extensionName, ext->properties[i].specVersion);
    }
#endif

    return ext;
}

void vkc_instance_extension_free(VkcInstanceExtension* extension) {
    if (extension && extension->pager) {
        page_allocator_free(extension->pager);
    }
}

/** @} */

/**
 * @name VkC Instance Extension Property Matches
 * @{
 */

VkcInstanceExtensionMatch* vkc_instance_extension_match_create(
    VkcInstanceExtension* extension, const char* const* names, const uint32_t name_count
) {
    if (!extension || !names || name_count == 0) return NULL;

    PageAllocator* pager = page_allocator_create(8);
    if (!pager) {
        LOG_ERROR("[VkcInstanceExtensionMatch] Failed to create allocator.");
        return NULL;
    }

    VkcInstanceExtensionMatch* match = page_malloc(pager, sizeof(*match), alignof(*match)); 
    if (!match) {
        LOG_ERROR("[VkcInstanceExtensionMatch] Failed to allocate result.");
        page_allocator_free(pager);
        return NULL;
    }

    *match = (VkcInstanceExtensionMatch){
        .pager = pager,
        .names = NULL,
        .count = 0,
    };

    // First pass: count matching extensions
    for (uint32_t i = 0; i < extension->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], extension->properties[i].extensionName)) {
                match->count++;
            }
        }
    }

    if (match->count == 0) {
        LOG_ERROR("[VkcInstanceExtensionMatch] No requested extensions were available:");
        for (uint32_t i = 0; i < name_count; i++) {
            LOG_ERROR("  - %s", names[i]);
        }
        LOG_INFO("Available instance extensions:");
        for (uint32_t i = 0; i < extension->count; i++) {
            LOG_INFO("  - %s", extension->properties[i].extensionName);
        }
        page_allocator_free(pager);
        return NULL;
    }

    match->names = page_malloc(pager, match->count * sizeof(char*), alignof(char*));
    if (!match->names) {
        LOG_ERROR("[VkcInstanceExtensionMatch] Failed to allocate name pointer array.");
        page_allocator_free(pager);
        return NULL;
    }

    // Second pass: copy the matching names
    uint32_t k = 0;
    for (uint32_t i = 0; i < extension->count; i++) {
        for (uint32_t j = 0; j < name_count; j++) {
            if (0 == utf8_raw_compare(names[j], extension->properties[i].extensionName)) {
                char* copy = utf8_raw_copy(extension->properties[i].extensionName);
                if (!copy) {
                    LOG_ERROR("[VkcInstanceExtensionMatch] Failed to copy name.");
                    page_allocator_free(pager);
                    return NULL;
                }

                page_add(pager, copy, utf8_raw_byte_count(copy), alignof(char));
                match->names[k++] = copy;
            }
        }
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    // Log the results to standard output
    LOG_DEBUG("[VkcInstanceExtensionMatch] Matched %u instance extension properties.", match->count);
    for (uint32_t i = 0; i < match->count; i++) {
        LOG_DEBUG("[VkcInstanceExtensionMatch] i=%u, name=%s", i, match->names[i]);
    }
#endif

    return match;
}

void vkc_instance_extension_match_free(VkcInstanceExtensionMatch* match) {
    if (match && match->pager) {
        page_allocator_free(match->pager);
    }
}

/** @} */

/**
 * @name VkC Instance
 * @{
 */

VkcInstance* vkc_instance_create(
    VkcInstanceLayerMatch* layer_match,
    VkcInstanceExtensionMatch* extension_match
) {
    uint32_t version;
    VkResult result = vkEnumerateInstanceVersion(&version);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkInstance] Failed to enumerate instance API version (VkResult=%d).", result);
        return NULL;
    }
    
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vkc",
        .applicationVersion = version,
        .pEngineName = "vkc engine",
        .engineVersion = version,
        .apiVersion = version,
    };

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcInstance] Name: %s", app_info.pApplicationName);
    LOG_DEBUG(
        "[VkcInstance] Version: %u.%u.%u",
        VK_VERSION_MAJOR(app_info.applicationVersion),
        VK_VERSION_MINOR(app_info.applicationVersion),
        VK_VERSION_PATCH(app_info.applicationVersion)
    );
    LOG_DEBUG("[VkcInstance] Engine Name: %s", app_info.pEngineName);
    LOG_DEBUG(
        "[VkcInstance] Engine Version: %u.%u.%u",
        VK_VERSION_MAJOR(app_info.engineVersion),
        VK_VERSION_MINOR(app_info.engineVersion),
        VK_VERSION_PATCH(app_info.engineVersion)
    );
    LOG_DEBUG(
        "[VkcInstance] API Version: %u.%u.%u",
        VK_API_VERSION_MAJOR(app_info.apiVersion),
        VK_API_VERSION_MINOR(app_info.apiVersion),
        VK_API_VERSION_PATCH(app_info.apiVersion)
    );
#endif

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    if (layer_match && layer_match->names) {
        create_info.enabledLayerCount = layer_match->count;
        create_info.ppEnabledLayerNames = (const char* const*) layer_match->names;
    }

    if (extension_match && extension_match->names) {
        create_info.enabledExtensionCount = extension_match->count;
        create_info.ppEnabledExtensionNames = (const char* const*) extension_match->names;
    }

    PageAllocator* pager = page_allocator_create(8);
    if (!pager) {
        LOG_ERROR("[VkcInstance] Failed to create page allocator.");
        return NULL;
    }

    VkcInstance* instance = page_malloc(pager, sizeof(*instance), alignof(*instance));
    if (!instance) {
        LOG_ERROR("[VkcInstance] Failed to allocate instance wrapper.");
        page_allocator_free(pager);
        return NULL;
    }

    *instance = (VkcInstance){
        .pager = pager,
        .object = VK_NULL_HANDLE,
        .allocator = vkc_page_callbacks(pager),
    };

    result = vkCreateInstance(&create_info, &instance->allocator, &instance->object);
    if (VK_SUCCESS != result) {
        LOG_ERROR("[VkcInstance] Failed to create instance object (VkResult=%d)", result);
        page_allocator_free(pager);
        return NULL;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcInstance] Successfully created Vulkan instance @ %p.", (void*)instance->object);
#endif

    return instance;
}

void vkc_instance_free(VkcInstance* instance) {
    if (instance && instance->pager && instance->object) {
        vkDestroyInstance(instance->object, &instance->allocator);
        page_allocator_free(instance->pager);
    }
}

/** @} */
