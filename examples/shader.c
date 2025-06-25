/**
 * @file examples/shader.c
 */

#include "core/posix.h"
#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "vk/instance.h"
#include "vk/device.h"

#include <stdlib.h>

// Use a static constant file path to keep things simple
static char* shader_path = "build/shaders/mean.spv";

int main(void) {
    /**
     * Create a Vulkan Instance
     */

    VkcInstance* instance = vkc_instance_create(1024);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        return EXIT_FAILURE;
    }

    /**
     * Create a Vulkan Device
     */

    VkcDevice* device = vkc_device_create(instance, 1024);

    /**
     * Create a Vulkan Shader
     */
    (void) shader_path;

    vkc_device_destroy(device);
    vkc_instance_destroy(instance);
    return EXIT_SUCCESS;
}
