/**
 * @file src/vk/allocator.c
 * @brief A wrapper for interfacing with Vulkan Host Memory.
 */

#include "core/logger.h"
#include "core/memory.h"
#include "allocator/page.h"
#include "vk/allocator.h"

/**
 * @section Private Methods
 * {@
 */

void* VKAPI_CALL
vkc_malloc(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope) {
    (void) scope;

    PageAllocator* allocator = (PageAllocator*) pUserData;
    if (NULL == allocator) {
        LOG_ERROR("[VK_ALLOC] Missing allocation context (PageAllocator)");
        return NULL;
    }

    void* address = page_malloc(allocator, size, alignment);
    if (NULL == address) {
        LOG_ERROR("[VK_ALLOC] Allocation failed (size=%zu, align=%zu)", size, alignment);
        return NULL;
    }

    return address;
}

void* VKAPI_CALL vkc_realloc(
    void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope scope
) {
    (void) scope;

    PageAllocator* allocator = (PageAllocator*) pUserData;
    if (NULL == allocator) {
        LOG_ERROR("[VK_REALLOC] Missing allocation context (PageAllocator)");
        return NULL;
    }

    void* address = page_realloc(allocator, pOriginal, size, alignment);
    if (!address) {
        LOG_ERROR(
            "[VK_REALLOC] Allocation failed (pOriginal=%p, size=%zu, align=%zu)",
            pOriginal,
            size,
            alignment
        );
        return NULL;
    }

    return address;
}

void VKAPI_CALL vkc_free(void* pUserData, void* pMemory) {
    PageAllocator* allocator = (PageAllocator*) pUserData;
    if (NULL == allocator || NULL == pMemory) {
        return;
    }

    page_free(allocator, pMemory);
}

/**
 * @name VkcInternal
 */

void VKAPI_CALL vkc_internal_malloc(
    void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope
) {
    (void) pUserData;
    (void) size;
    (void) type;
    (void) scope;
#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG(
        "[VK_INTERNAL_ALLOC] owner=%p, size=%zu, type=%d, scope=%d", pUserData, size, type, scope
    );
#endif
}

void VKAPI_CALL vkc_internal_free(
    void* pUserData, size_t size, VkInternalAllocationType type, VkSystemAllocationScope scope
) {
    (void) pUserData;
    (void) size;
    (void) type;
    (void) scope;
#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG(
        "[VK_INTERNAL_FREE] owner=%p, size=%zu, type=%d, scope=%d", pUserData, size, type, scope
    );
#endif
}

/** @} */

/**
 * @name Public Methods
 * {@
 */

VkAllocationCallbacks VKAPI_CALL vkc_page_callbacks(PageAllocator* allocator) {
    return (VkAllocationCallbacks) {
        .pUserData = allocator,
        .pfnAllocation = vkc_malloc,
        .pfnReallocation = vkc_realloc,
        .pfnFree = vkc_free,
        .pfnInternalAllocation = vkc_internal_malloc,
        .pfnInternalFree = vkc_internal_free,
    };
}

/** @} */
