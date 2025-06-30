# Vulkan Compute Pipeline: Task Checklist

A step-by-step overview for building a minimal headless Vulkan **compute pipeline**.

## 🔧 Core Setup

* [x] **Instance**: Create a `VkInstance` to initialize Vulkan and connect to the loader and drivers.
* [x] **Physical Device**: Enumerate and select a `VkPhysicalDevice` that supports `VK_QUEUE_COMPUTE_BIT`.
* [x] **Logical Device**: Create a `VkDevice` with a compute-capable queue family.
* [x] **Queue**: Retrieve the compute queue using `vkGetDeviceQueue`.

## 📦 Shader & Pipeline

* [x] **Shader Module**: Load your SPIR-V binary and create a `VkShaderModule`.
* [x] **Descriptor Set Layout**: Define resource bindings (e.g., buffers), then create `VkDescriptorSetLayout`.
* [x] **Pipeline Layout**: Create a `VkPipelineLayout` to combine descriptor sets and optional push constants.
* [x] **Compute Pipeline**: Create a `VkPipeline` (type `COMPUTE`) using your shader and layout.

## 🧱 Resource Allocation

* [x] **Buffer Creation**: Allocate `VkBuffer`(s) with usage `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT`. Bind and map memory if host-visible access is needed.
* [ ] **Descriptor Pool**: Create a `VkDescriptorPool` to manage descriptor set allocation.
* [ ] **Descriptor Set**: Allocate `VkDescriptorSet` and bind buffers using `vkUpdateDescriptorSets`.

## 🎮 Command & Execution

* [ ] **Command Pool & Buffer**: Create a `VkCommandPool` and allocate a `VkCommandBuffer`.
* [ ] **Record Commands**: Begin recording. Bind pipeline and descriptor sets, then call `vkCmdDispatch`.
* [ ] **Submit & Sync**: Submit the command buffer. Use a `VkFence` to block until execution finishes.

## 🧹 Cleanup

* [ ] Destroy all Vulkan objects.
* [ ] Free all allocated memory and resources.
