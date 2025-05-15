/**
 * Copyright © 2025 Austin Berrio
 *
 * @file src/core/lease.c
 * @brief A dynamic runtime allocator for tracking memory address states.
 *
 * Each object assumes full ownership of its internal components.
 *
 * Structure:
 * - A LeasePolicy defines the access and ownership rules for a memory region.
 * - A LeaseObject represents the allocated address, size, and alignment.
 * - A LeaseTenant combines a Policy with an Object to form a complete allocation contract.
 *
 * Ownership Model:
 * - A LeaseObject owns its address unless specified otherwise by its LeasePolicy.
 * - A LeaseTenant owns both its LeasePolicy and LeaseObject.
 * - Each LeaseTenant must be freed exactly once.
 * - Policies and Objects must not be shared across multiple Tenants.
 * - All internal structures are heap-allocated to prevent stack exhaustion at scale.
 *
 * This design enables clear, deterministic ownership semantics in C,
 * with explicit control over lifetime and tracking of dynamic memory.
 */

#include "core/logger.h"
#include "core/memory.h"
#include "core/lease.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @section Creation
 */

LeaseOwner* lease_create_owner(size_t capacity) {
    return hash_table_create(capacity, HASH_TYPE_ADDRESS);
}

LeasePolicy* lease_create_policy(LeaseAccess access, LeaseContract contract) {
    LeasePolicy* policy = memory_aligned_alloc(sizeof(LeasePolicy), alignof(LeasePolicy));
    if (!policy) {
        return NULL;
    }

    *policy = (LeasePolicy) {
        .access = access,
        .contract = contract,
    };

    return policy;
}

LeaseObject* lease_create_object(void* address, size_t size, size_t alignment) {
    if (!address || size == 0 || alignment == 0) {
        return NULL;
    }

    LeaseObject* object = memory_aligned_alloc(sizeof(LeaseObject), alignof(LeaseObject));
    if (!object) {
        return NULL;
    }

    *object = (LeaseObject) {
        .alignment = alignment,
        .size = size,
        .address = address,
    };

    return object;
}

LeaseTenant* lease_create_tenant(LeasePolicy* policy, LeaseObject* object) {
    if (!policy || !object) {
        return NULL;
    }

    LeaseTenant* tenant = memory_aligned_alloc(sizeof(LeaseTenant), alignof(LeaseTenant));
    if (!tenant) {
        return NULL;
    }

    *tenant = (LeaseTenant) {
        .policy = policy,
        .object = object,
    };

    return tenant;
}

/**
 * @section Destruction
 */

void lease_free_owner(LeaseOwner* owner) {
    if (owner) {
        for (uint64_t i = 0; i < owner->size; i++) {
            HashTableEntry* entry = &owner->entries[i];
            if (!entry || !entry->key || !entry->value) {
                continue; // Skip empty or invalid entries
            }

            LeaseTenant* tenant = (LeaseTenant*) entry->value;
            if (tenant) {
                lease_free_tenant(tenant);
            }
        }

        hash_table_free(owner);
    }
}

void lease_free_policy(LeasePolicy* policy) {
    if (policy) {
        free(policy);
    }
}

void lease_free_object(LeasePolicy* policy, LeaseObject* object) {
    if (policy && object) { // object depends upon policy
        if (object->address && policy->contract == LEASE_CONTRACT_OWNED) {
            free(object->address); // policy determines freedom
        }
        free(object);
    }
}

void lease_free_tenant(LeaseTenant* tenant) {
    if (tenant) {
        lease_free_object(tenant->policy, tenant->object);
        lease_free_policy(tenant->policy);
        free(tenant);
    }
}

/**
 * @section Tenant Allocation
 */

static LeaseTenant* lease_alloc_internal(
    LeaseAccess access, LeaseContract contract, void* address, size_t size, size_t alignment
) {
    LeasePolicy* policy = lease_create_policy(access, contract);
    if (!policy) {
        return NULL;
    }

    LeaseObject* object = lease_create_object(address, size, alignment);
    if (!object) {
        if (policy->contract == LEASE_CONTRACT_OWNED) {
            free(address);
        }
        lease_free_policy(policy);
        return NULL;
    }

    LeaseTenant* tenant = lease_create_tenant(policy, object);
    if (!tenant) {
        lease_free_object(policy, object);
        lease_free_policy(policy);
        return NULL;
    }

    return tenant;
}

LeaseTenant* lease_alloc_owned_tenant(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) {
        return NULL;
    }

    void* address = memory_aligned_alloc(size, alignment);
    if (!address) {
        return NULL;
    }

    return (LeaseTenant*) lease_alloc_internal(
        LEASE_ACCESS_LOCAL, LEASE_CONTRACT_OWNED, address, size, alignment
    );
}

LeaseTenant* lease_alloc_borrowed_tenant(void* address, size_t size, size_t alignment) {
    if (!address || size == 0 || alignment == 0) {
        return NULL;
    }

    return (LeaseTenant*) lease_alloc_internal(
        LEASE_ACCESS_LOCAL, LEASE_CONTRACT_BORROWED, address, size, alignment
    );
}

LeaseTenant* lease_alloc_static_tenant(void* address, size_t size, size_t alignment) {
    if (!address || size == 0 || alignment == 0) {
        return NULL;
    }

    return (LeaseTenant*) lease_alloc_internal(
        LEASE_ACCESS_STATIC, LEASE_CONTRACT_STATIC, address, size, alignment
    );
}

/**
 * @section Address Allocation
 */

void* lease_alloc_owned_address(LeaseOwner* owner, size_t size, size_t alignment) {
    if (!owner || size == 0 || alignment == 0) {
        return NULL;
    }

    LeaseTenant* tenant = lease_alloc_owned_tenant(size, alignment);
    if (!tenant) {
        return NULL;
    }

    const void* address = tenant->object->address;
    if (!address) {
        lease_free_tenant(tenant);
        return NULL;
    }

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        lease_free_tenant(tenant);
        return NULL;
    }

    return (void*) address;
}

void* lease_alloc_borrowed_address(
    LeaseOwner* owner, void* address, size_t size, size_t alignment
) {
    if (!owner || !address || size == 0 || alignment == 0) {
        return NULL;
    }

    LeaseTenant* tenant = lease_alloc_borrowed_tenant(address, size, alignment);
    if (!tenant) {
        return NULL;
    }

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        lease_free_tenant(tenant);
        return NULL;
    }

    return address;
}

void* lease_alloc_static_address(LeaseOwner* owner, void* address, size_t size, size_t alignment) {
    if (!owner || !address || size == 0 || alignment == 0) {
        return NULL;
    }

    LeaseTenant* tenant = lease_alloc_static_tenant(address, size, alignment);
    if (!tenant) {
        return NULL;
    }

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        lease_free_tenant(tenant);
        return NULL;
    }

    return address;
}

/**
 * @section String Allocation
 */

char* lease_alloc_owned_string(LeaseOwner* owner, const char* address) {
    size_t size = strlen(address);
    char* string = lease_alloc_owned_address(owner, size + 1, alignof(char));
    if (!string) {
        return NULL; // Alloc failed
    }

    memcpy(string, (const void*) address, size); // copy up to size bytes
    string[size] = '\0'; // null terminate string
    return string;
}

char* lease_alloc_borrowed_string(LeaseOwner* owner, const char* address) {
    return lease_alloc_borrowed_address(owner, (void*) address, strlen(address), alignof(char));
}

char* lease_alloc_static_string(LeaseOwner* owner, const char* address) {
    return lease_alloc_static_address(owner, (void*) address, strlen(address), alignof(char));
}

/**
 * @section Metadata Access
 */

LeaseTenant* lease_get_tenant(LeaseOwner* owner, void* address) {
    if (!owner || !address) {
        return NULL;
    }
    return (LeaseTenant*) hash_table_search(owner, address);
}

LeaseObject* lease_get_object(LeaseOwner* owner, void* address) {
    LeaseTenant* tenant = lease_get_tenant(owner, address);
    return tenant ? tenant->object : NULL;
}

LeasePolicy* lease_get_policy(LeaseOwner* owner, void* address) {
    LeaseTenant* tenant = lease_get_tenant(owner, address);
    return tenant ? tenant->policy : NULL;
}

LeaseAccess lease_get_access(LeaseOwner* owner, void* address) {
    LeasePolicy* policy = lease_get_policy(owner, address);
    return policy ? policy->access : LEASE_ACCESS_LOCAL; // default fallback
}

LeaseContract lease_get_contract(LeaseOwner* owner, void* address) {
    LeasePolicy* policy = lease_get_policy(owner, address);
    return policy ? policy->contract : LEASE_CONTRACT_BORROWED; // safe fallback
}

/**
 * @section Mutation and Transfer
 */

LeaseState lease_realloc(LeaseOwner* owner, void* address, size_t size, size_t alignment) {
    if (!owner || !address || size == 0 || alignment == 0) {
        return HASH_ERROR;
    }

    LeaseTenant* old_tenant = lease_get_tenant(owner, address);
    if (!old_tenant || !old_tenant->policy) {
        return HASH_KEY_NOT_FOUND;
    }

    if (old_tenant->policy->contract != LEASE_CONTRACT_OWNED) {
        return HASH_ERROR; // illegal operation on borrowed/static memory
    }

    size_t old_size = old_tenant->object->size;
    void* old_address = old_tenant->object->address;

    if (size <= old_size) {
        return HASH_SUCCESS; // no need to realloc — caller's request already satisfied
    }

    // Allocate a new tenant
    LeaseTenant* new_tenant = lease_alloc_owned_tenant(size, alignment);
    if (!new_tenant || !new_tenant->object) {
        return HASH_ERROR;
    }

    void* new_address = new_tenant->object->address;

    // Copy old data into new region
    memcpy(new_address, old_address, old_size);

    // Remove old address from table
    if (HASH_SUCCESS != hash_table_delete(owner, address)) {
        lease_free_tenant(new_tenant);
        return HASH_ERROR;
    }

    // Insert new address/tenant pair
    if (HASH_SUCCESS != hash_table_insert(owner, new_address, new_tenant)) {
        lease_free_tenant(new_tenant);
        return HASH_ERROR;
    }

    // Free the old tenant
    lease_free_tenant(old_tenant);

    return HASH_SUCCESS;
}

LeaseState lease_transfer(LeaseOwner* from, LeaseOwner* to, void* address) {
    if (!from || !to || !address) {
        return HASH_ERROR;
    }

    LeaseTenant* from_tenant = lease_get_tenant(from, address);
    if (!from_tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    LeaseTenant* to_tenant = lease_get_tenant(to, address);
    if (to_tenant) {
        return HASH_KEY_EXISTS;
    }

    if (HASH_SUCCESS != hash_table_delete(from, address)) {
        return HASH_ERROR;
    }

    if (HASH_SUCCESS != hash_table_insert(to, address, from_tenant)) {
        return HASH_ERROR;
    }

    return HASH_SUCCESS;
}

LeaseState lease_terminate(LeaseOwner* owner, void* address) {
    LeaseTenant* tenant = lease_get_tenant(owner, address);
    if (!tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    HashTableState state = hash_table_delete(owner, address);
    if (HASH_SUCCESS == state) {
        lease_free_tenant(tenant);
    }

    return state; // return hash table state
}

/**
 * @section Debugging
 */

static inline const char* lease_debug_internal_access(const LeasePolicy* policy) {
    if (!policy) {
        return "LEASE_ACCESS_NULL";
    }

    switch (policy->access) {
        case LEASE_ACCESS_GLOBAL:
            return "LEASE_ACCESS_GLOBAL";
        case LEASE_ACCESS_LOCAL:
            return "LEASE_ACCESS_LOCAL";
        case LEASE_ACCESS_STATIC:
            return "LEASE_ACCESS_STATIC";
        default:
            return "LEASE_ACCESS_UNKNOWN";
    }
}

static inline const char* lease_debug_internal_contract(const LeasePolicy* policy) {
    if (!policy) {
        return "LEASE_CONTRACT_NULL";
    }

    switch (policy->contract) {
        case LEASE_CONTRACT_OWNED:
            return "LEASE_CONTRACT_OWNED";
        case LEASE_CONTRACT_BORROWED:
            return "LEASE_CONTRACT_BORROWED";
        case LEASE_CONTRACT_STATIC:
            return "LEASE_CONTRACT_STATIC";
        default:
            return "LEASE_CONTRACT_UNKNOWN";
    }
}

void lease_debug_tenant(LeaseTenant* tenant) {
    if (!tenant) {
        return;
    }

    LeasePolicy* policy = tenant->policy;
    LeaseObject* object = tenant->object;
    if (!policy || !object) {
        return;
    }

    LOG_INFO("--- Lease ---");
    LOG_INFO("[Tenant] policy=%p, object=%p, tenant=%p", policy, object, tenant);

    const char* access = lease_debug_internal_access(policy);
    const char* contract = lease_debug_internal_contract(policy);
    LOG_INFO("[Policy] access=%s, contract=%s", access, contract);

    const size_t alignment = object->alignment;
    const size_t size = object->size;
    const void* const address = object->address;
    LOG_INFO("[Object] alignment=%zu, size=%zu, address=%p", alignment, size, address);
}

void lease_debug_owner(LeaseOwner* owner) {
    if (owner) {
        LOG_INFO("[LeaseOwner] address=%p", owner);
        for (uint64_t i = 0; i < owner->size; i++) {
            HashTableEntry* entry = &owner->entries[i];
            if (!entry || !entry->key || !entry->value) {
                continue; // Skip empty or invalid entries
            }

            LeaseTenant* tenant = (LeaseTenant*) entry->value;
            if (tenant) {
                lease_debug_tenant(tenant);
            }
        }
    }
}
