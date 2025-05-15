/**
 * @file src/vk/alloc.c
 * @brief A wrapper for interfacing with Vulkan Host Memory.
 */

#include "core/logger.h"
#include "vk/alloc.h"

LeaseAccess VKAPI_CALL vk_lease_access(VkSystemAllocationScope scope) {
    switch (scope) {
        case VK_SYSTEM_ALLOCATION_SCOPE_CACHE:
        case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND:
        case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT:
            return LEASE_ACCESS_LOCAL;

        case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE:
        case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE:
        default:
            return LEASE_ACCESS_GLOBAL;
    }
}

void* VKAPI_CALL vk_lease_alloc(
    void* pUserData,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    LeaseOwner* owner = (LeaseOwner*) pUserData;
    if (!owner) {
        LOG_ERROR("[VK_ALLOC] Owner reference failure: address=%p", pUserData);
        return NULL;
    }

    void* address = lease_alloc_owned_address(owner, size, alignment);
    if (!address) {
        LOG_ERROR(
            "VK_ALLOC] Failed to alloc: "
            "address=%p, size=%zu, align=%zu, scope=%d",
            owner, size, alignment, scope
        );
        return NULL;
    }

    LOG_INFO("[VK_ALLOC] %p (%zu bytes, %zu aligned)", address, size, alignment);
    return address;
}

void* VKAPI_CALL vk_lease_realloc(
    void* pUserData,
    void* pOriginal,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    (void) scope;
    LeaseOwner* owner = (LeaseOwner*) pUserData;
    if (!owner) {
        LOG_ERROR("[VK_REALLOC] Reference failure: owner=%p", pUserData);
        return NULL;
    }

    if (HASH_SUCCESS != lease_realloc(owner, pOriginal, size, alignment)) {
        LOG_ERROR("VK_REALLOC] realloc failure: address=%p, size=%zu", pOriginal, size);
        return NULL;
    }

    LeaseTenant* tenant = lease_get_tenant(owner, pOriginal);
    if (tenant && tenant->object && tenant->object->address) { 
        LOG_INFO("[VK_REALLOC] %p → %p (%zu bytes)", pOriginal, tenant->object->address, size);
        return tenant->object->address;
    }

    LOG_ERROR("VK_REALLOC] Failed to find tenant: address=%p, size=%zu", pOriginal, size);
    return NULL;
}

void VKAPI_CALL vk_lease_free(void* pUserData, void* pMemory) {
    if (!pUserData || !pMemory) { return; }

    LeaseOwner* owner = (LeaseOwner*) pUserData;
    if (!owner) {
        LOG_ERROR("[VK_FREE] Failed to reference owner from %p", pUserData);
        return;
    }

    LeaseTenant* tenant = lease_get_tenant(owner, pMemory);
    if (!tenant) {
        LOG_ERROR("[VK_FREE] Attempted to free untracked or borrowed memory at %p", pMemory);
        return;
    }

    lease_terminate(owner, pMemory);
    LOG_INFO("[VK_FREE] %p", pMemory);
}

void VKAPI_CALL vk_lease_internal_alloc(
    void* pUserData,
    size_t size,
    VkInternalAllocationType type,
    VkSystemAllocationScope scope
) {
    LOG_INFO("[VK_INTERNAL_ALLOC] owner=%p, size=%zu, type=%d, scope=%d", pUserData, size, type, scope);
}

void VKAPI_CALL vk_lease_internal_free(
    void* pUserData,
    size_t size,
    VkInternalAllocationType type,
    VkSystemAllocationScope scope
) {
    LOG_INFO("[VK_INTERNAL_FREE] owner=%p, size=%zu, type=%d, scope=%d", pUserData, size, type, scope);
}

VkAllocationCallbacks VKAPI_CALL vk_lease_callbacks(LeaseOwner* owner) {
    return (VkAllocationCallbacks) {
        .pUserData = owner,  // or pass this dynamically
        .pfnAllocation = vk_lease_alloc,
        .pfnReallocation = vk_lease_realloc,
        .pfnFree = vk_lease_free,
        .pfnInternalAllocation = vk_lease_internal_alloc,
        .pfnInternalFree = vk_lease_internal_free
    };    
}
