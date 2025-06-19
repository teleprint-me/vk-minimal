// examples/instance.c
#include "core/logger.h"
#include "vk/allocator.h"
#include "vk/validation.h"
#include "vk/extension.h"

#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

#define VALIDATION_LAYER_COUNT 1
#define EXTENSION_COUNT 1

const char* const VALIDATION_LAYERS[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};
const char* const INSTANCE_EXTENSIONS[EXTENSION_COUNT] = {"VK_EXT_debug_utils"};

/**
 * Create a Vulkan Instance
 */

uint32_t vkc_instance_version(void) {
    uint32_t apiVersion;
    if (VK_SUCCESS != vkEnumerateInstanceVersion(&apiVersion)) {
        LOG_ERROR("Failed to enumerate Vulkan instance version.");
        return 0;
    }
    return apiVersion;
}

VkApplicationInfo
vkc_instance_app_info(const char* app_name, const char* app_engine, uint32_t api_version) {
    VkApplicationInfo app_info = {0}; // Zero-initialize the info instance
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = api_version;
    app_info.pEngineName = app_engine;
    app_info.engineVersion = api_version;
    app_info.apiVersion = api_version;
    return app_info;
}

void vkc_instance_app_info_log(VkApplicationInfo* app_info) {
    LOG_DEBUG("Application Name: %s", app_info->pApplicationName);
    LOG_DEBUG(
        "Application Version: %u.%u.%u",
        VK_VERSION_MAJOR(app_info->applicationVersion),
        VK_VERSION_MINOR(app_info->applicationVersion),
        VK_VERSION_PATCH(app_info->applicationVersion)
    );
    LOG_DEBUG("Engine Name: %s", app_info->pEngineName);
    LOG_DEBUG(
        "Engine Version: %u.%u.%u",
        VK_VERSION_MAJOR(app_info->engineVersion),
        VK_VERSION_MINOR(app_info->engineVersion),
        VK_VERSION_PATCH(app_info->engineVersion)
    );
    LOG_DEBUG(
        "API Version: %u.%u.%u",
        VK_API_VERSION_MAJOR(app_info->apiVersion),
        VK_API_VERSION_MINOR(app_info->apiVersion),
        VK_API_VERSION_PATCH(app_info->apiVersion)
    );
}

VkInstanceCreateInfo vkc_instance_info(VkApplicationInfo* app_info) {
    VkInstanceCreateInfo instance_info = {0};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = app_info;

#if defined(DEBUG) && (1 == DEBUG)
    VkcValidationLayer* validation
        = vkc_validation_layer_create(VALIDATION_LAYERS, VALIDATION_LAYER_COUNT, 1024);
    if (validation && vkc_validation_layer_match_request(validation)) {
        LOG_DEBUG("Validation layer check succeeded");
        instance_info.enabledLayerCount = validation->request->count;
        instance_info.ppEnabledLayerNames = validation->request->names;
    }

    VkcExtension* extension = vkc_extension_create(INSTANCE_EXTENSIONS, EXTENSION_COUNT, 1024);
    if (extension && vkc_extension_match_request(extension)) {
        LOG_DEBUG("Extension check succeeded");
        instance_info.enabledExtensionCount = extension->request->count;
        instance_info.ppEnabledExtensionNames = extension->request->names;
    }

    vkc_extension_free(extension);
    vkc_validation_layer_free(validation);
#endif

    return instance_info;
}

VkInstance vkc_instance_create(const VkAllocationCallbacks* allocator) {
    uint32_t api_version = vkc_instance_version();
    VkApplicationInfo app_info = vkc_instance_app_info("instance-app", "No Engine", api_version);

#if defined(DEBUG) && (1 == DEBUG)
    vkc_instance_app_info_log(&app_info);
#endif

    VkInstanceCreateInfo instance_info = vkc_instance_info(&app_info);
    VkInstance instance = VK_NULL_HANDLE;

    VkResult result = vkCreateInstance(&instance_info, allocator, &instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        return VK_NULL_HANDLE;
    }

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VkInstance] Vulkan instance created successfully.");
#endif

    return instance;
}

/**
 * @brief Simple example showcasing how to create and destroy a custom VulkanInstance object.
 */
int main(void) {
    HashMap* map = hash_map_create(1024, HASH_MAP_KEY_TYPE_ADDRESS);
    if (!map) {
        LOG_ERROR("Failed to create HashMap.");
        return EXIT_FAILURE;
    }

    VkAllocationCallbacks allocator = vkc_hash_callbacks(map);
    VkInstance instance = vkc_instance_create(&allocator);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        hash_map_free(map);
        return EXIT_FAILURE;
    }

    vkDestroyInstance(instance, &allocator);
    hash_map_free(map);
    LOG_INFO("Successfully destroyed Vulkan instance!");

    return EXIT_SUCCESS;
}
