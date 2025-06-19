/**
 * @file src/vk/allocator.c
 * @brief A wrapper for interfacing with Vulkan Host Memory.
 */

#include "core/logger.h"
#include "core/memory.h"
#include "map/linear.h"
#include "vk/allocator.h"

/**
 * @name Private Interface
 */

/**
 * @brief Internal page metadata for Vulkan host allocations.
 *
 * `VkcHashPage` stores metadata associated with a Vulkan allocation
 * managed by the custom allocator. It is used to track memory size,
 * alignment, and allocation scope for each allocation.
 *
 * This struct is stored in a hash map keyed by allocation address.
 */
typedef struct VkcHashPage {
    size_t size; /**< Size of the allocated memory block in bytes. */
    size_t alignment; /**< Alignment of the allocated memory block in bytes. */
    VkSystemAllocationScope scope; /**< Vulkan-defined allocation scope. */
} VkcHashPage;

/**
 * @brief Allocates and initializes a VkcHashPage.
 *
 * Allocates a `VkcHashPage` on the heap and fills in the provided metadata.
 * The returned pointer should be freed using `vkc_free_page()` when no longer needed.
 *
 * @param size Size of the Vulkan allocation in bytes.
 * @param alignment Alignment in bytes.
 * @param scope Vulkan memory allocation scope.
 * @return Pointer to the initialized `VkcHashPage`, or NULL on failure.
 */
VkcHashPage* vkc_alloc_page(size_t size, size_t alignment, VkSystemAllocationScope scope) {
    VkcHashPage* page = memory_alloc(sizeof(VkcHashPage), alignof(VkcHashPage));
    if (NULL == page) {
        return NULL;
    }

    *page = (VkcHashPage) {
        .size = size,
        .alignment = alignment,
        .scope = scope,
    };

    return page;
}

/**
 * @brief Frees a previously allocated VkcHashPage.
 *
 * This function safely frees the metadata associated with a Vulkan allocation.
 * Passing NULL is safe and has no effect.
 *
 * @param page Pointer to the `VkcHashPage` to free.
 */
void vkc_free_page(VkcHashPage* page) {
    if (NULL == page) {
        return;
    }
    memory_free(page);
}

/**
 * @brief Vulkan-style malloc using aligned memory and page tracking.
 *
 * This function is registered as `pfnAllocation` in `VkAllocationCallbacks`.
 * It allocates aligned memory and registers the allocation metadata
 * in the internal page table (a `HashMap*`).
 *
 * @param pUserData Pointer to the internal `HashMap` page table.
 * @param size Number of bytes to allocate.
 * @param alignment Alignment in bytes (must be a power of two).
 * @param scope Vulkan memory allocation scope.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* VKAPI_CALL vkc_malloc(
    void* pUserData,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    HashMap* map = (HashMap*) pUserData;
    if (NULL == map) {
        LOG_ERROR("[VK_ALLOC] Missing allocation context (page map)");
        return NULL;
    }

    void* address = memory_alloc(size, alignment);
    if (NULL == address) {
        LOG_ERROR("[VK_ALLOC] Allocation failed (size=%zu, align=%zu)", size, alignment);
        return NULL;
    }

    VkcHashPage* page = vkc_alloc_page(size, alignment, scope);
    if (NULL == page) {
        memory_free(address);
        LOG_ERROR("[VK_ALLOC] Failed to allocate page metadata for %p", address);
        return NULL;
    }

    HashMapState state = hash_map_insert(map, address, page);
    if (state != HASH_MAP_STATE_SUCCESS) {
        memory_free(address);
        vkc_free_page(page);
        LOG_ERROR("[VK_ALLOC] Failed to insert %p into page map (state = %d)", address, state);
        return NULL;
    }

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VK_ALLOC] %p → %p (%zu bytes, %zu aligned)", address, page, size, alignment);
#endif

    return address;
}

/**
 * @brief Vulkan-style realloc with tracked page metadata.
 *
 * Reallocates memory with the specified size and alignment. Tracks allocation metadata
 * in the provided `HashMap*` and ensures the old pointer is safely removed.
 *
 * @param pUserData Pointer to the internal `HashMap` page table.
 * @param pOriginal Original memory pointer to reallocate.
 * @param size New allocation size.
 * @param alignment New alignment boundary.
 * @param scope Vulkan memory allocation scope.
 * @return Pointer to the newly allocated memory, or NULL on failure.
 */
void* VKAPI_CALL vkc_realloc(
    void* pUserData,
    void* pOriginal,
    size_t size,
    size_t alignment,
    VkSystemAllocationScope scope
) {
    HashMap* map = (HashMap*) pUserData;

    // Case: Fresh allocation
    if (NULL == pOriginal) {
        return vkc_malloc(pUserData, size, alignment, scope);
    }

    // Case: Null context
    if (NULL == map) {
        LOG_ERROR("[VK_REALLOC] Missing allocation context (page map)");
        return NULL;
    }

    // Case: Lookup existing metadata
    VkcHashPage* page = hash_map_search(map, pOriginal);
    if (NULL == page) {
        LOG_ERROR("[VK_REALLOC] Unknown pointer %p", pOriginal);
        return NULL;
    }

    // Case: Vulkan signals free via realloc with size == 0
    if (0 == size) {
        if (HASH_MAP_STATE_SUCCESS != hash_map_delete(map, pOriginal)) {
            LOG_ERROR("[VK_REALLOC] Failed to remove page for %p", pOriginal);
        }
        vkc_free_page(page);
        memory_free(pOriginal);
        return NULL;
    }

    void* address = memory_realloc(pOriginal, page->size, size, alignment);
    if (NULL == address) {
        LOG_ERROR(
            "[VK_REALLOC] Failed to realloc %p (%zu → %zu bytes)", pOriginal, page->size, size
        );
        return NULL;
    }

    // Update page metadata in-place
    *page = (VkcHashPage) {
        .size = size,
        .alignment = alignment,
        .scope = scope,
    };

    // Re-map page metadata to new address
    if (HASH_MAP_STATE_SUCCESS != hash_map_delete(map, pOriginal)) {
        LOG_ERROR("[VK_REALLOC] Failed to remove old mapping for %p", pOriginal);
        return NULL;
    }

    if (HASH_MAP_STATE_SUCCESS != hash_map_insert(map, address, page)) {
        LOG_ERROR("[VK_REALLOC] Failed to insert new mapping for %p", address);
        return NULL;
    }

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VK_REALLOC] %p → %p (%zu bytes)", pOriginal, address, size);
#endif

    return address;
}

/**
 * @brief Vulkan-style free with tracked page cleanup.
 *
 * Frees memory allocated via `vkc_malloc` or `vkc_realloc`, and removes
 * its tracking metadata from the internal `HashMap*`.
 *
 * @param pUserData Pointer to the internal page map (`HashMap*`).
 * @param pMemory Pointer to the memory to be freed.
 */
void VKAPI_CALL vkc_free(void* pUserData, void* pMemory) {
    HashMap* map = (HashMap*) pUserData;
    if (NULL == map || NULL == pMemory) {
        return;
    }

    VkcHashPage* page = (VkcHashPage*) hash_map_search(map, pMemory);
    if (NULL == page) {
        LOG_ERROR("[VK_FREE] Attempted to free untracked memory %p", pMemory);
        return;
    }

    if (HASH_MAP_STATE_SUCCESS != hash_map_delete(map, pMemory)) {
        LOG_ERROR("[VK_FREE] Failed to remove page for %p", pMemory);
        return;
    }

#if defined(DEBUG) && (1 == DEBUG)
    LOG_DEBUG("[VK_FREE] %p (%zu bytes, %zu aligned)", pMemory, page->size, page->alignment);
#endif

    vkc_free_page(page);
    memory_free(pMemory);
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

/**
 * @name Public Interface
 */

VkAllocationCallbacks VKAPI_CALL vkc_hash_callbacks(HashMap* map) {
    return (VkAllocationCallbacks) {
        .pUserData = map,
        .pfnAllocation = vkc_malloc,
        .pfnReallocation = vkc_realloc,
        .pfnFree = vkc_free,
        .pfnInternalAllocation = vkc_internal_malloc,
        .pfnInternalFree = vkc_internal_free,
    };
}
