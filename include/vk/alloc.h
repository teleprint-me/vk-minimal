/**
 * @file include/vk/alloc.h
 * @brief A wrapper for interfacing with Vulkan Host Memory.
 */

#ifndef VK_ALLOC_H
#define VK_ALLOC_H

#include "core/lease.h"

#include <vulkan/vulkan.h>

LeaseAccess VKAPI_CALL vk_lease_access(VkSystemAllocationScope scope);
void* VKAPI_CALL vk_lease_alloc(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope);
void* VKAPI_CALL vk_lease_realloc(
    void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope
);
void VKAPI_CALL vk_lease_free(void* pUserData, void* pMemory);
void VKAPI_CALL vk_lease_internal_alloc(
    void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope
);
void VKAPI_CALL vk_lease_internal_free(
    void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope
);
VkAllocationCallbacks VKAPI_CALL vk_lease_callbacks(LeaseOwner* owner);

#endif // VK_ALLOC_H
