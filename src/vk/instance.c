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

static uint32_t vkc_instance_init_version(void) {
    uint32_t apiVersion;
    if (VK_SUCCESS != vkEnumerateInstanceVersion(&apiVersion)) {
        LOG_ERROR("Failed to enumerate Vulkan instance version.");
        return 0;
    }
    return apiVersion;
}

/**
 * @brief Initializes the Vulkan application info.
 */
static void
vkc_instance_init_app_info(VkcInstance* instance, const char* name, const char* engine) {
    instance->version = vkc_instance_init_version();

    instance->app_info = (VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = name,
        .applicationVersion = instance->version,
        .pEngineName = engine,
        .engineVersion = instance->version,
        .apiVersion = instance->version,
    };

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("Application Name: %s", instance->app_info.pApplicationName);
    LOG_DEBUG(
        "Application Version: %u.%u.%u",
        VK_VERSION_MAJOR(instance->app_info.applicationVersion),
        VK_VERSION_MINOR(instance->app_info.applicationVersion),
        VK_VERSION_PATCH(instance->app_info.applicationVersion)
    );
    LOG_DEBUG("Engine Name: %s", instance->app_info.pEngineName);
    LOG_DEBUG(
        "Engine Version: %u.%u.%u",
        VK_VERSION_MAJOR(instance->app_info.engineVersion),
        VK_VERSION_MINOR(instance->app_info.engineVersion),
        VK_VERSION_PATCH(instance->app_info.engineVersion)
    );
    LOG_DEBUG(
        "API Version: %u.%u.%u",
        VK_API_VERSION_MAJOR(instance->app_info.apiVersion),
        VK_API_VERSION_MINOR(instance->app_info.apiVersion),
        VK_API_VERSION_PATCH(instance->app_info.apiVersion)
    );
#endif
}

/**
 * @brief Initializes the Vulkan allocator and internal page map.
 */
static bool vkc_instance_init_allocator(VkcInstance* instance, size_t map_size) {
    instance->map = hash_map_create(map_size, HASH_MAP_KEY_TYPE_ADDRESS);
    if (!instance->map) {
        LOG_ERROR("Failed to create allocator hash map.");
        return false;
    }

    instance->allocator = vkc_hash_callbacks(instance->map);
    return true;
}

/**
 * @brief Initializes the VkInstanceCreateInfo structure.
 */
static void vkc_instance_init_info(VkcInstance* instance) {
    instance->info = (VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &instance->app_info,
    };

#if defined(DEBUG) && (1 == DEBUG)
    VkcValidationLayer* validation
        = vkc_validation_layer_create((const char*[]) {"VK_LAYER_KHRONOS_validation"}, 1, 1024);

    if (validation && vkc_validation_layer_match_request(validation)) {
        instance->info.enabledLayerCount = validation->request->count;
        instance->info.ppEnabledLayerNames = validation->request->names;
        LOG_DEBUG("Validation layers enabled.");
    }

    VkcExtension* extension = vkc_extension_create((const char*[]) {"VK_EXT_debug_utils"}, 1, 1024);

    if (extension && vkc_extension_match_request(extension)) {
        instance->info.enabledExtensionCount = extension->request->count;
        instance->info.ppEnabledExtensionNames = extension->request->names;
        LOG_DEBUG("Instance extensions enabled.");
    }

    vkc_validation_layer_free(validation);
    vkc_extension_free(extension);
#endif
}

/**
 * @brief Creates a Vulkan instance with internal allocator and metadata.
 *
 * @param map_size Initial size of the allocator's hash map.
 * @return Pointer to the created instance, or NULL on failure.
 */
VkcInstance* vkc_instance_create(size_t map_size) {
    VkcInstance* instance = memory_alloc(sizeof(VkcInstance), alignof(VkcInstance));
    if (!instance) {
        LOG_ERROR("Failed to allocate VkcInstance.");
        return NULL;
    }

    vkc_instance_init_app_info(instance, "Compute", "Compute Engine");

    if (!vkc_instance_init_allocator(instance, map_size)) {
        memory_free(instance);
        return NULL;
    }

    vkc_instance_init_info(instance);

    VkResult result = vkCreateInstance(&instance->info, &instance->allocator, &instance->object);
    if (result != VK_SUCCESS) {
        LOG_ERROR("vkCreateInstance failed: %d", result);
        hash_map_free(instance->map);
        memory_free(instance);
        return NULL;
    }

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("Vulkan instance created successfully @ %p", (void*) instance);
#endif

    return instance;
}

void vkc_instance_destroy(VkcInstance* instance) {
    if (!instance) {
        return;
    }
    if (instance->object != VK_NULL_HANDLE) {
        vkDestroyInstance(instance->object, &instance->allocator);
    }
    hash_map_free(instance->map);
    memory_free(instance);
}
