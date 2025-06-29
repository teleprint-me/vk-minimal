# Vulkan Pipeline Task List

## Core Pipeline

- **Instance:** [x] Create a `VkInstance` to initialize Vulkan and interface with the loader and available drivers.
- **Physical Device:** [x] Enumerate and select a `VkPhysicalDevice` that supports compute operations (`VK_QUEUE_COMPUTE_BIT`).
- **Logical Device:** [x] Create a `VkDevice` from the physical device with a compute-capable queue family.
- **Queue:** [x] Retrieve a compute queue (`VkQueue`) from the logical device using `vkGetDeviceQueue`.
- **Shader Module:** [ ] Load your SPIR-V binary and create a `VkShaderModule` using `vkCreateShaderModule`.
- **Descriptor Set Layout:** [ ] Define layout bindings for any resources (e.g., buffers) your shader uses; create `VkDescriptorSetLayout`.
- **Pipeline Layout:** [ ] Create a `VkPipelineLayout` to bind descriptor sets and push constants (if used).
- **Compute Pipeline:** [ ] Define the compute stage using your shader module and create a `VkPipeline` of type `COMPUTE`.
- **Buffer Creation:** [ ] Allocate a `VkBuffer` with usage flag `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT` and map host-visible memory if needed.
- **Descriptor Pool:** [ ] Create a `VkDescriptorPool` to allocate descriptor sets.
- **Descriptor Set:** [ ] Allocate a `VkDescriptorSet` from the pool and write buffer bindings to it with `vkUpdateDescriptorSets`.
- **Command Pool & Buffer:** [ ] Create a `VkCommandPool` and allocate a `VkCommandBuffer`. Begin recording commands.
- **Command Recording:** [ ] Record commands to bind pipeline, descriptor set, and dispatch compute work (`vkCmdDispatch`).
- **Submit & Synchronize:** [ ] Submit the command buffer to the queue. Use a `VkFence` to wait until execution completes.
- **Cleanup:** [ ] Destroy all Vulkan objects and free all allocated memory to clean up the framework.
