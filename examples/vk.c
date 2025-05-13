// examples/vk.c
#include "logger.h"
#include "lease.h"

#include <vulkan/vulkan.h>

#include <stdlib.h>
#include <stdio.h>

void* VKAPI_CALL vk_lease_alloc(
    void* pUserData,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    (void) alignment;
    LeaseOwner* lease = (LeaseOwner*) pUserData;
    if (!lease) return NULL;
    LeasePolicy policy = {
        .type = LEASE_CONTRACT_OWNED,
        .scope = scope == VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE ? LEASE_ACCESS_GLOBAL : LEASE_ACCESS_LOCAL
    };

    void* ptr = lease_address(lease, policy, size);
    if (!ptr) return NULL;
    LOG_INFO("[VK_ALLOC] %p (%zu bytes)", ptr, size);
    return ptr;
}

void* VKAPI_CALL vk_lease_realloc(
    void* pUserData,
    void* pOriginal,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    (void) alignment;
    (void) scope;
    LeaseOwner* lease = (LeaseOwner*) pUserData;
    LeaseState state = lease_realloc(lease, pOriginal, size);
    if (state != HASH_SUCCESS) {
        return NULL;
    }

    LeaseTenant* tenant = lease_find(lease, pOriginal);
    if (!tenant) return NULL;
    LOG_INFO("[VK_REALLOC] %p → %p (%zu bytes)", pOriginal, tenant->address, size);
    return tenant->address;
}

void VKAPI_CALL vk_lease_free(void* pUserData, void* pMemory) {
    LeaseOwner* lease = (LeaseOwner*) pUserData;
    if (!pMemory) return;

    LeaseTenant* tenant = lease_find(lease, pMemory);
    if (tenant && tenant->policy.type == LEASE_CONTRACT_OWNED) {
        lease_terminate(lease, pMemory);
        LOG_INFO("[VK_FREE] %p", pMemory);
    } else {
        LOG_ERROR("[VK_FREE] Attempted to free untracked or borrowed memory at %p", pMemory);
    }
}

void VKAPI_CALL vk_lease_internal_alloc(
    void* pUserData,
    size_t size,
    VkInternalAllocationType type,
    VkSystemAllocationScope scope
) {
    (void) pUserData;
    LOG_INFO("[VK_INTERNAL_ALLOC] Size: %zu Type: %d Scope: %d", size, type, scope);
}

void VKAPI_CALL vk_lease_internal_free(
    void* pUserData,
    size_t size,
    VkInternalAllocationType type,
    VkSystemAllocationScope scope
) {
    (void) pUserData;
    LOG_INFO("[VK_INTERNAL_FREE] Size: %zu Type: %d Scope: %d", size, type, scope);
}

VkAllocationCallbacks VKAPI_CALL vk_make_lease_callbacks(LeaseOwner* owner) {
    return (VkAllocationCallbacks) {
        .pUserData = owner,  // or pass this dynamically
        .pfnAllocation = vk_lease_alloc,
        .pfnReallocation = vk_lease_realloc,
        .pfnFree = vk_lease_free,
        .pfnInternalAllocation = vk_lease_internal_alloc,
        .pfnInternalFree = vk_lease_internal_free
    };    
}

int main(void) {
    /**
     * Lease Allocator to sanely track memory allocations.
     */
    
    LeasePolicy g_lease_policy = {
        .type = LEASE_CONTRACT_OWNED,
        .scope = LEASE_ACCESS_LOCAL
    };
    LeaseOwner* g_lease_owner = lease_create(1024); // local
    LeaseOwner* vk_lease_owner = lease_create(2048); // scoped
    VkAllocationCallbacks vk_alloc = vk_make_lease_callbacks(vk_lease_owner);

    // Result codes and handles
    VkResult result;

    /**
     * Create a Vulkan Instance
     */

    VkApplicationInfo app_info = {0};
    VkInstanceCreateInfo instance_info = {0};
    VkInstance instance = VK_NULL_HANDLE;

    app_info = (VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vk-minimal",
        .applicationVersion = VK_API_VERSION_1_4,
        .pEngineName = "vk-engine",
        .engineVersion = VK_API_VERSION_1_4,
        .apiVersion = VK_API_VERSION_1_4,
    };

    // No extensions or validation layers for now
    instance_info = (VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
    };

    result = vkCreateInstance(&instance_info, &vk_alloc, &instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create Vulkan instance: %d", result);
        goto cleanup_lease;
    }

    LOG_INFO("Vulkan instance created successfully.");

    /**
     * Create a Vulkan Physical Device
     *
     * @note
     * We can not allocate array memory to the stack because
     * goto disrupts the control flow.
     */

    uint32_t physical_device_count = 0;
    uint32_t compute_queue_family_index = UINT32_MAX;
    VkPhysicalDevice* physical_devices = VK_NULL_HANDLE;
    VkPhysicalDevice selected_device = VK_NULL_HANDLE;

    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL); // ASAN is detecting leaks
    if (physical_device_count == 0) {
        LOG_ERROR("No Vulkan-compatible GPUs found.");
        goto cleanup_instance;
    }

    // Track the allocated device list
    physical_devices = lease_address(
            g_lease_owner,
            g_lease_policy,
            sizeof(VkPhysicalDevice) * physical_device_count
    );
    if (!physical_devices) {
        LOG_ERROR("Failed to allocate physical device list.");
        goto cleanup_instance;
    }

    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);

    // Pick the first device with compute support
    for (uint32_t i = 0; i < physical_device_count; ++i) {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_family_count, NULL);

        VkQueueFamilyProperties queue_families[queue_family_count];
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_devices[i], &queue_family_count, queue_families
        );

        for (uint32_t j = 0; j < queue_family_count; ++j) {
            if (queue_families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                selected_device = physical_devices[i];
                compute_queue_family_index = j;
                break;
            }
        }

        if (selected_device != VK_NULL_HANDLE) {
            break;
        }
    }

    if (selected_device == VK_NULL_HANDLE) {
        LOG_ERROR("No suitable GPU found with compute capabilities.");
        goto cleanup_instance;
    }

    LOG_INFO(
        "Selected physical device with compute queue family index: %u", compute_queue_family_index
    );

    /**
     * Create a Vulkan Logical Device
     */

    static float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {0};
    VkDeviceCreateInfo device_info = {0};

    VkDevice device = VK_NULL_HANDLE;
    VkQueue compute_queue = VK_NULL_HANDLE;

    // The compute queue family we want access to.
    queue_info = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = compute_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    device_info = (VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL,
        .pEnabledFeatures = NULL, // VkPhysicalDeviceFeatures later if needed
    };

    result = vkCreateDevice(selected_device, &device_info, &vk_alloc, &device);
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
    char* code = (char*) lease_address(
        g_lease_owner,
        g_lease_policy,
        (size_t) length + 1
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
    lease_free(g_lease_owner);
    lease_free(vk_lease_owner);

    LOG_INFO("Vulkan application destroyed.");
    return EXIT_SUCCESS;
}
