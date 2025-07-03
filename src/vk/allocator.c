/**
 * @file src/vk/allocator.c
 * @brief Vulkan Host Memory Allocator using a tracked page map.
 */

#include "core/posix.h"
#include "core/memory.h"
#include "core/logger.h"
#include "allocator/page.h"
#include "vk/allocator.h"

/**
 * @section Private
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

/** @} */

/**
 * @name Public
 * {@
 */

static PageAllocator* _vkc_allocator = NULL;
static VkAllocationCallbacks _vkc_callbacks = {0};

bool vkc_allocator_create(void) {
    if (_vkc_allocator) {
        return true; // Already initialized
    }

    _vkc_allocator = page_allocator_create(1);
    if (!_vkc_allocator) {
        LOG_ERROR("[VkcAllocator] Failed to create global PageAllocator.");
        return false;
    }

    _vkc_callbacks = (VkAllocationCallbacks) {
        .pUserData = _vkc_allocator,
        .pfnAllocation = vkc_malloc,
        .pfnReallocation = vkc_realloc,
        .pfnFree = vkc_free,
        .pfnInternalAllocation = NULL,
        .pfnInternalFree = NULL,
    };

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcAllocator] Initialized global Vulkan allocator.");
#endif

    return true;
}

bool vkc_allocator_destroy(void) {
    if (_vkc_allocator) {
        page_allocator_free(_vkc_allocator);
        _vkc_allocator = NULL;
        memset(&_vkc_callbacks, 0, sizeof(_vkc_callbacks));

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
        LOG_DEBUG("[VkcAllocator] Global Vulkan allocator destroyed.");
#endif

        return true;
    }

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
    LOG_DEBUG("[VkcAllocator] Failed to destory global Vulkan allocator.");
#endif

    return false;
}

PageAllocator* vkc_allocator_get(void) {
    if (!_vkc_allocator) {
        LOG_ERROR("[VkcAllocator] Global Vulkan allocator is unintialized!");

#if defined(VKC_DEBUG) && (1 == VKC_DEBUG)
        LOG_DEBUG("[VkcAllocator] Use `vkc_allocator_create()` to initialize the allocator.");
#endif

        return NULL;
    }

    return _vkc_allocator;
}

const VkAllocationCallbacks* vkc_allocator_callbacks(void) {
    return _vkc_allocator ? &_vkc_callbacks : NULL;
}

/** @} */
