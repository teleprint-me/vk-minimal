// examples/validate.c
#include "core/logger.h"
#include "vk/validation.h"

#include <stdlib.h>
#include <stdio.h>

/**
 * Create a Vulkan Instance
 */

#define VALIDATION_LAYER_COUNT 1 /// @warning Cannot be a variable
const char* const VALIDATION_LAYERS[VALIDATION_LAYER_COUNT] = {"VK_LAYER_KHRONOS_validation"};

int main(void) {
    size_t initial_capacity = 1024;
    VkcValidationLayer* layer = vkc_validation_layer_create(VALIDATION_LAYERS, VALIDATION_LAYER_COUNT, initial_capacity);
    if (!layer) {
        return EXIT_FAILURE;
    }
    vkc_validation_layer_log_info(layer);
    if (!vkc_validation_layer_match_request(layer)) {
        LOG_ERROR("One or more requested layers are not response.");
    }

    // Lookup by name
    VkLayerProperties prop;
    if (vkc_validation_layer_match_name(layer, "VK_LAYER_KHRONOS_validation", &prop)) {
        LOG_INFO("Found property (by name): %s - %s", prop.layerName, prop.description);
    } else {
        LOG_WARN("Property by name not found.");
    }

    // Lookup by index
    if (vkc_validation_layer_match_index(layer, 0, &prop)) {
        LOG_INFO("Found property (by index 0): %s - %s", prop.layerName, prop.description);
    } else {
        LOG_WARN("Property by index not found.");
    }

    vkc_validation_layer_free(layer);
    return EXIT_SUCCESS;
}
