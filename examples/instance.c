/**
 * @file examples/instance.c
 * @brief Simple example showcasing how to create and destroy a custom VkcInstance object.
 */

#include "core/logger.h"
#include "vk/instance.h"

#include <stdlib.h>
#include <stdio.h>

int main(void) {
    VkcInstance* instance = vkc_instance_create(1024);
    if (!instance) {
        LOG_ERROR("Failed to create Vulkan instance!");
        return EXIT_FAILURE;
    }

    vkc_instance_destroy(instance);
    LOG_INFO("Successfully destroyed Vulkan instance!");
    return EXIT_SUCCESS;
}
