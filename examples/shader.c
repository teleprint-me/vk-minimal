/**
 * @file examples/shader.c
 */

#include "core/posix.h"
#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "utf8/raw.h"
#include "vk/instance.h"
#include "vk/device.h"

#include <unistd.h>
#include <stdlib.h>


int main(void) {
    // Use static constant file paths to keep things simple for now
    static char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) {
        LOG_ERROR("Failed to get current working directory.");
        return EXIT_FAILURE;
    }

    char* shader_path = utf8_raw_concat(cwd, "/build/shaders/mean.spv");
    LOG_INFO("[SHADER] shader_path='%s'", shader_path);

    // Create a temporary allocator until skeleton is operational
    PageAllocator* pager = page_allocator_create(1024);

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
    if (!device) {
        LOG_ERROR("Failed to create Vulkan device!");
        return EXIT_FAILURE;
    }

    /**
     * Create a Vulkan Shader
     */

    /// @note Shader path will be a variable later on
    FILE* shader_file = fopen(shader_path, "rb");
    if (!shader_file) {
        LOG_ERROR("Failed to open SPIR-V file: %s", shader_path);
        return EXIT_FAILURE;
    }

    fseek(shader_file, 0, SEEK_END);
    uint32_t shader_size = (uint32_t) ftell(shader_file);
    rewind(shader_file);

    /// @note The byte code pointer is a uint32_t pointer, not a char pointer.
    uint32_t* buffer = page_malloc(pager, shader_size * sizeof(uint32_t), alignof(uint32_t));
    if (!buffer) {
        fclose(shader_file);
        LOG_ERROR("Failed to allocate memory for shader file");
        return EXIT_FAILURE;
    }

    fread(buffer, 1, shader_size, shader_file);
    fclose(shader_file);

    page_allocator_free(pager);
    vkc_device_destroy(device);
    vkc_instance_destroy(instance);
    return EXIT_SUCCESS;
}
