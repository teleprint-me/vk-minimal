// examples/instance.c
#include "core/logger.h"
#include "core/memory.h"

#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

/**
 * Create a Vulkan Instance
 */

#define VALIDATION_LAYER_COUNT 1 /// @warning Cannot be a variable
const char* const validation_layers[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};

bool vk_validation_layer_support(const char* const* layers, uint32_t layer_count) {
    if (!layers || !(*layers) || 0 == layer_count) {
        LOG_ERROR("Invalid arguments (layers=%p, layer_count=%u)", layers, layer_count);
        return false;
    }

    VkResult result;
    uint32_t property_count = 0;
    result = vkEnumerateInstanceLayerProperties(&property_count, NULL);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer property count (error code: %u)", result);
        return false;
    }
    if (0 == property_count) {
        LOG_ERROR("No validation layers available");
        return false;
    }

    VkLayerProperties* properties = memory_aligned_calloc(property_count, sizeof(VkLayerProperties), alignof(VkLayerProperties));
    if (!properties) {
        LOG_ERROR("Memory allocation failed for layer properties");
        return false;
    }

    result = vkEnumerateInstanceLayerProperties(&property_count, properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("Failed to enumerate layer properties (error code: %u)", result);
        free(properties);
        return false;
    }

    // Check to see if the available layers match the requested layers.
    for (uint32_t i = 0; i < layer_count; ++i) {
        bool layer_found = false;
        for (uint32_t j = 0; j < property_count; ++j) {
            if (strcmp(layers[i], properties[j].layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            LOG_ERROR("Validation layer not found: %s", layers[i]);
            free(properties);
            return false;
        }
    }

    free(properties);
    return true;
}

uint32_t vk_api_version(void) {
    uint32_t apiVersion;
    if (VK_SUCCESS != vkEnumerateInstanceVersion(&apiVersion)) {
        fprintf(stderr, "Failed to enumerate Vulkan instance version.\n");
        return 0;
    }
    return apiVersion;
}

VkApplicationInfo vk_create_app_info(const char* app_name, const char* app_engine, uint32_t api_version) {
    VkApplicationInfo app_info = {0}; // Zero-initialize the info instance
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = api_version;
    app_info.pEngineName = app_engine;
    app_info.engineVersion = api_version;
    app_info.apiVersion = api_version;
    return app_info;
}

void vk_log_app_info(VkApplicationInfo* app_info) {
    LOG_INFO("Application Name: %s", app_info->pApplicationName);
    LOG_INFO("Application Version: %u.%u.%u",
        VK_VERSION_MAJOR(app_info->applicationVersion),
        VK_VERSION_MINOR(app_info->applicationVersion),
        VK_VERSION_PATCH(app_info->applicationVersion)
    );
    LOG_INFO("Engine Name: %s", app_info->pEngineName);
    LOG_INFO("Engine Version: %u.%u.%u",
        VK_VERSION_MAJOR(app_info->engineVersion),
        VK_VERSION_MINOR(app_info->engineVersion),
        VK_VERSION_PATCH(app_info->engineVersion)
    );
    LOG_INFO(
        "API Version: %u.%u.%u",
        VK_API_VERSION_MAJOR(app_info->apiVersion),
        VK_API_VERSION_MINOR(app_info->apiVersion),
        VK_API_VERSION_PATCH(app_info->apiVersion)
    );
}

VkInstanceCreateInfo vk_create_instance_info(VkApplicationInfo* app_info, const char* const* layers, uint32_t layer_count) {
    VkInstanceCreateInfo instance_info = {0};

    // Create instance info: No extensions, unless required, for now
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = app_info;
    
    if (!vk_validation_layer_support(layers, layer_count)) {
        return instance_info;
    }

    instance_info.enabledLayerCount = layer_count;
    instance_info.ppEnabledLayerNames = layers;
    return instance_info;
}

VkInstance vk_create_instance(const VkAllocationCallbacks* allocator, const char* const* layers, uint32_t layer_count) {
    uint32_t api_version = vk_api_version();
    VkApplicationInfo app_info = vk_create_app_info("instance-app", "No Engine", api_version);
    VkInstanceCreateInfo instance_info = vk_create_instance_info(&app_info, layers, layer_count);
    VkInstance instance = VK_NULL_HANDLE;

    VkResult result = vkCreateInstance(&instance_info, allocator, &instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        return VK_NULL_HANDLE;
    }

    LOG_INFO("Vulkan instance created successfully.");
    return instance;
}

/**
 * @brief Simple example showcasing how to create and destroy a custom VulkanInstance object.
 */
int main(void) {
    VkInstance instance = vk_create_instance(NULL, validation_layers, VALIDATION_LAYER_COUNT);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        return EXIT_FAILURE;
    }

    vkDestroyInstance(instance, NULL);
    LOG_INFO("Successfully destroyed Vulkan instance!");
    return EXIT_SUCCESS;
}
