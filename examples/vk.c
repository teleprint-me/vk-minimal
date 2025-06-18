// examples/vk.c
#include "core/logger.h"
#include "core/memory.h"
#include "allocator/lease.h"
#include "vk/alloc.h"

#include <vulkan/vulkan.h>

#include <stdalign.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Create a Vulkan Instance
 */

uint32_t vk_api_version(void) {
    uint32_t apiVersion;
    if (vkEnumerateInstanceVersion(&apiVersion) != VK_SUCCESS) {
        fprintf(stderr, "Failed to enumerate Vulkan instance version.\n");
        return 0;
    }
    return apiVersion;
}

VkApplicationInfo vk_create_app_info(void) {
    VkApplicationInfo app_info = {0}; // Zero-initialize the info instance
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vk-minimal";
    app_info.applicationVersion = VK_API_VERSION_1_4;
    app_info.pEngineName = "vk-compute";
    app_info.engineVersion = VK_API_VERSION_1_4;
    app_info.apiVersion = vk_api_version();
    return app_info;
}

VkInstanceCreateInfo vk_create_instance_info(VkApplicationInfo* app_info) {
    VkInstanceCreateInfo instance_info = {0};
    // Create instance info
    // No extensions or validation layers for now
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = app_info;
    return instance_info;
}

VkInstance vk_create_instance(const VkAllocationCallbacks* allocator) {
    VkApplicationInfo app_info = vk_create_app_info();
    VkInstanceCreateInfo instance_info = vk_create_instance_info(&app_info);
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
 * Create Validation Layers
 */

// Define validation layers
#define VALIDATION_LAYER_COUNT 1 /// @warning Cannot be a variable
const char* const validation_layers[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};

VkResult vk_check_validation_layer_support(const char* const* layers, uint32_t layer_count) {
    if (!layers || !(*layers) || 0 == layer_count) {
        LOG_ERROR("%s: Invalid arguments (layers=%p, layer_count=%u)\n", __func__, layers, layer_count);
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkResult result;
    uint32_t property_count = 0;
    result = vkEnumerateInstanceLayerProperties(&property_count, VK_NULL_HANDLE);
    if (VK_SUCCESS != result) {
        LOG_ERROR("%s: Failed to enumerate layer property count (error code: %u)\n", __func__, result);
        return result;
    }
    if (0 == property_count) {
        LOG_ERROR("%s: No validation layers available\n", __func__);
        return VK_ERROR_LAYER_NOT_PRESENT;
    }

    VkLayerProperties* properties = memory_alloc(property_count, alignof(VkLayerProperties));
    if (!properties) {
        LOG_ERROR("%s: Memory allocation failed for layer properties\n", __func__);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    result = vkEnumerateInstanceLayerProperties(&property_count, properties);
    if (VK_SUCCESS != result) {
        LOG_ERROR("%s: Failed to enumerate layer properties (error code: %u)\n", __func__, result);
        free(properties);
        return result;
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
            LOG_ERROR("%s: Validation layer not found: %s\n", __func__, layers[i]);
            free(properties);
            return VK_ERROR_LAYER_NOT_PRESENT;
        }
    }

    free(properties);
    return VK_SUCCESS;
}

int main(void) {
    /**
     * Lease Allocator for sanely tracking memory allocations.
     */

    LeaseOwner* vk_alloc_owner = lease_create_owner(1024); // memory region tracker
    VkAllocationCallbacks vk_alloc = vk_lease_callbacks(vk_alloc_owner); // attach and set tracker

    // Result codes and handles
    VkResult result;

    /**
     * Create a Vulkan Instance
     */

    VkInstance instance = vk_create_instance(&vk_alloc);
    if (instance == VK_NULL_HANDLE) {
        goto cleanup_lease;
    }

    /**
     * Create a Vulkan Physical Device
     *
     * @note
     * We can not allocate array memory to the stack because
     * goto disrupts the control flow.
     */

    uint32_t physical_device_count = 0;
    VkPhysicalDevice* physical_device_list = NULL;
    VkPhysicalDevice selected_physical_device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties selected_properties = {0};
    uint32_t compute_queue_family_index = UINT32_MAX;

    result = vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
    if (result != VK_SUCCESS || physical_device_count == 0) {
        LOG_ERROR(
            "No Vulkan-compatible devices found (VkResult: %d, Count: %u)",
            result,
            physical_device_count
        );
        goto cleanup_instance;
    }

    physical_device_list = lease_alloc_owned_address(
        vk_alloc_owner,
        sizeof(VkPhysicalDevice) * physical_device_count,
        alignof(VkPhysicalDevice)
    );
    if (!physical_device_list) {
        LOG_ERROR("Failed to allocate device list.");
        goto cleanup_instance;
    }

    result = vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_device_list);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to enumerate devices (VkResult: %d)", result);
        goto cleanup_instance;
    }

    // Try to find the first discrete GPU with compute support
    for (uint32_t i = 0; i < physical_device_count; ++i) {
        VkPhysicalDevice device = physical_device_list[i];
        VkPhysicalDeviceProperties props = {0};
        vkGetPhysicalDeviceProperties(device, &props);

        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

        VkQueueFamilyProperties* queue_families = lease_alloc_owned_address(
            vk_alloc_owner,
            sizeof(VkQueueFamilyProperties) * queue_family_count,
            alignof(VkQueueFamilyProperties)
        );
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

        LOG_INFO("Device %u: %s, type=%d", i, props.deviceName, props.deviceType);

        for (uint32_t j = 0; j < queue_family_count; ++j) {
            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
                    && selected_physical_device == VK_NULL_HANDLE) {
                    selected_physical_device = device;
                    selected_properties = props;
                    compute_queue_family_index = j;
                    break;
                }
            }
        }
        if (selected_physical_device != VK_NULL_HANDLE) {
            break;
        }
    }

    // Fallback: if no discrete GPU with compute, pick *any* device with compute support
    if (selected_physical_device == VK_NULL_HANDLE) {
        for (uint32_t i = 0; i < physical_device_count; ++i) {
            VkPhysicalDevice device = physical_device_list[i];
            VkPhysicalDeviceProperties props = {0};
            vkGetPhysicalDeviceProperties(device, &props);

            uint32_t queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

            VkQueueFamilyProperties* queue_families = lease_alloc_owned_address(
                vk_alloc_owner,
                sizeof(VkQueueFamilyProperties) * queue_family_count,
                alignof(VkQueueFamilyProperties)
            );
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

            for (uint32_t j = 0; j < queue_family_count; ++j) {
                if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    selected_physical_device = device;
                    selected_properties = props;
                    compute_queue_family_index = j;
                    break;
                }
            }
            if (selected_physical_device != VK_NULL_HANDLE) {
                break;
            }
        }
    }

    if (selected_physical_device == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable device with compute queue found.");
        goto cleanup_instance;
    }

    LOG_INFO(
        "Selected device: %s (type=%d), compute queue index: %u",
        selected_properties.deviceName,
        selected_properties.deviceType,
        compute_queue_family_index
    );

    /**
     * Create a Vulkan Logical Device
     */

    static const float queue_priorities[1] = {1.0f};
    VkDeviceQueueCreateInfo queue_info = {0};
    VkPhysicalDeviceFeatures features = {0};

    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = &features, // or see note below
    };

    VkDevice device = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;

    // The compute queue family we want access to.
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = compute_queue_family_index;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;

    result = vkCreateDevice(selected_physical_device, &device_info, NULL, &device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create logical device: %d", result);
        goto cleanup_instance;
    }
    LOG_INFO("Logical device created.");

    vkGetDeviceQueue(device, compute_queue_family_index, 0, &compute_queue);
    LOG_INFO("Retrieved compute queue.");

    /**
     * Read SPIR-V Binary File
     */

    size_t code_size = 0;
    const char* filepath = "build/shaders/mean.spv";

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        LOG_ERROR("Failed to open SPIR-V file: %s", filepath);
        goto cleanup_device;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    // Track the buffer because Vulkan won't free this later on for us.
    char* code = (char*) lease_alloc_owned_address(
        vk_alloc_owner,
        (size_t) length + 1,
        alignof(char)
    );
    if (!code) {
        fclose(file);
        LOG_ERROR("Failed to allocate memory for shader file");
        goto cleanup_device;
    }

    fread(code, 1, length, file); // assuming fread null terminates buffer for us
    fclose(file);

    /**
     * Create a Vulkan Shader Module
     */

    VkShaderModule shader_module;
    VkShaderModuleCreateInfo shader_info = {0};

    shader_info = (VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = (uint32_t*) code,
    };

    result = vkCreateShaderModule(device, &shader_info, &vk_alloc, &shader_module);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create shader module from %s", filepath);
        goto cleanup_device;
    }

    // stub
    if (!device) {
        goto cleanup_shader;
    }

    /**
     * Clean up
     */
cleanup_shader:
    vkDestroyShaderModule(device, shader_module, NULL);
cleanup_device:
    vkDestroyDevice(device, NULL);
cleanup_instance:
    vkDestroyInstance(instance, NULL);
cleanup_lease:
    lease_free_owner(vk_alloc_owner);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
