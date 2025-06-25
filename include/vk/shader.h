/**
 * @file include/vk/shader.h
 * @brief A wrapper for interfacing with SPIR-V Compute Shaders.
 */

#ifndef SHADER_H
#define SHADER_H

#include <vulkan/vulkan.h>

char* shader_read(const char* filepath, size_t* size_out);
VkShaderModule shader_load_module(VkDevice device, const char* filepath);
void shader_destroy_module(VkDevice device, VkShaderModule module);

#endif // SHADER_H
