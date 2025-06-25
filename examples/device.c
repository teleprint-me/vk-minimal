/**
 * @file examples/device.c
 */

#include "core/logger.h"
#include "vk/instance.h"
#include "vk/device.h"

#include <stdlib.h>

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

    vkc_device_destroy(device);
    vkc_instance_destroy(instance);
    return EXIT_SUCCESS;
}
