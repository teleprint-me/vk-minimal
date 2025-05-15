// src/shader.c
#include "core/logger.h"
#include "vk/shader.h"

#include <stdio.h>
#include <stdlib.h>

char* shader_read(const char* filepath, size_t* size_out) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        LOG_ERROR("Failed to open SPIR-V file: %s", filepath);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    rewind(file);

    char* buffer = malloc(length);
    if (!buffer) {
        fclose(file);
        LOG_ERROR("Failed to allocate memory for shader file");
        return NULL;
    }

    fread(buffer, 1, length, file);
    fclose(file);

    *size_out = length;
    return buffer;
}

VkShaderModule shader_load_module(VkDevice device, const char* filepath) {
    size_t code_size;
    char* code = shader_read(filepath, &code_size);
    if (!code) return VK_NULL_HANDLE;

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code_size,
        .pCode = (uint32_t*)code,
    };

    VkShaderModule shader_module;
    VkResult result = vkCreateShaderModule(device, &create_info, NULL, &shader_module);
    free(code);

    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create shader module from %s", filepath);
        return VK_NULL_HANDLE;
    }

    return shader_module;
}

void shader_destroy_module(VkDevice device, VkShaderModule module) {
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, module, NULL);
    }
}
