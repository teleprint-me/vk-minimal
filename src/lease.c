/**
 * Copyright © 2025 Austin Berrio
 *
 * @file src/lease.c
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

#include "memory.h"
#include "lease.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Create lease objects
 */

LeasePolicy* lease_create_policy(LeaseAccess access, LeaseContract contract) {
    LeasePolicy* policy = memory_aligned_size(sizeof(LeasePolicy), alignof(LeasePolicy));
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

    LeaseObject* object = memory_aligned_size(sizeof(LeaseObject), alignof(LeaseObject));
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

    LeaseTenant* tenant = memory_aligned_size(sizeof(LeaseTenant), alignof(LeaseTenant));
    if (!tenant) {
        return NULL;
    }

    *tenant = (LeaseTenant) {
        .policy = policy,
        .object = object,
    };

    return tenant;
}

LeaseTenant* lease_create_tenant_owned(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) {
        return NULL;
    }

    LeasePolicy* policy = lease_create_policy(LEASE_ACCESS_LOCAL, LEASE_CONTRACT_OWNED);
    if (!policy) {
        return NULL;
    }

    void* address = memory_aligned_alloc(size, alignment);
    if (!address) {
        lease_free_policy(policy);
        return NULL;
    }

    LeaseObject* object = lease_create_object(address, size, alignment);
    if (!object) {
        free(address);
        lease_free_policy(policy);
        return NULL;
    }

    LeaseTenant* tenant = lease_create_tenant(policy, object);
    if (!tenant) {
        free(address);
        lease_free_object(policy, object);
        lease_free_policy(policy);
        return NULL;
    }

    return tenant;
}

LeaseTenant* lease_create_tenant_borrowed(void* address, size_t size, size_t alignment) {
    if (!address || size == 0) {
        return NULL;
    }

    LeasePolicy* policy = lease_create_policy(LEASE_ACCESS_LOCAL, LEASE_CONTRACT_BORROWED);
    if (!policy) {
        return NULL;
    }

    LeaseObject* object = lease_create_object(address, size, alignment);
    if (!object) {
        lease_free_policy(policy);
        return NULL;
    }

    LeaseTenant* tenant = lease_create_tenant(policy, object);
    if (!tenant) {
        // User claimed ownership of address
        lease_free_object(policy, object);
        lease_free_policy(policy);
        return NULL;
    }

    return tenant;
}

LeaseTenant* lease_create_tenant_static(void* address, size_t size, size_t alignment) {
    if (!address || size == 0 || alignment == 0) {
        return NULL;
    }

    LeasePolicy* policy = lease_create_policy(LEASE_ACCESS_STATIC, LEASE_CONTRACT_STATIC);
    if (!policy) {
        return NULL;
    }

    LeaseObject* object = lease_create_object(address, size, alignment);
    if (!object) {
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

LeaseOwner* lease_create_owner(size_t capacity) {
    return hash_table_create(capacity, HASH_TYPE_ADDRESS);
}

/**
 * Free lease objects
 */

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

/**
 * Lease operations
 */

void* lease_address(LeaseOwner* owner, LeasePolicy* policy, size_t size, size_t alignment) {
    if (!owner || size == 0) {
        goto default_error;
    }

    LeaseTenant* tenant = lease_tenant_create(owner, policy, size, alignment);
    if (!tenant) {
        goto default_error;
    }

    const void* address = tenant->address;
    if (!address) {
        goto tenant_error;
    }

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        goto tenant_error;
    }

    return address;

tenant_error:
    lease_tenant_free(tenant);
default_error:
    return NULL;
}

char* lease_string(LeaseOwner* owner, LeasePolicy* policy, const char* source) {
    size_t size = strlen(source);
    char* lease = lease_address(owner, policy, size + 1, alignof(char)); // alloc null char
    if (!lease) {
        return NULL; // Alloc failed
    }

    memcpy(lease, source, size); // copy up to size bytes
    lease[size] = '\0'; // null terminate string
    return lease;
}

LeaseState
lease_owned(LeaseOwner* owner, LeasePolicy* policy, void* address, size_t size, size_t alignment) {
    if (!owner || !policy || !address) {
        return HASH_ERROR;
    }

    if (hash_table_search(owner, address)) {
        return HASH_KEY_EXISTS;
    }

    LeaseTenant* tenant = memory_aligned_alloc(sizeof(LeaseTenant), alignof(LeaseTenant));
    if (!tenant) {
        return HASH_ERROR;
    }

    if (!policy) {
        // Set the default policy access and contract
        policy = lease_policy_create(LEASE_ACCESS_LOCAL, LEASE_CONTRACT_OWNED);
        if (!policy) {
            lease_tenant_free(tenant);
        }
    }

    *tenant = (LeaseTenant) {
        .alignment = alignment,
        .size = size,
        .policy = policy,
        .address = address,
    };

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        lease_tenant_free(tenant);
        return HASH_ERROR;
    }

    return HASH_SUCCESS;
}

LeaseTenant* lease_find(LeaseOwner* owner, void* address) {
    return (LeaseTenant*) hash_table_search(owner, address);
}

LeaseState lease_policy(LeaseOwner* owner, LeasePolicy policy, void* address) {
    LeaseTenant* tenant = (LeaseTenant*) hash_table_search(owner, address);
    if (!tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    tenant->policy = policy;
    return HASH_SUCCESS;
}

LeaseState lease_realloc(LeaseOwner* owner, void* address, size_t size, size_t alignment) {
    if (!owner || !address || size == 0 || alignment == 0) {
        return HASH_ERROR;
    }

    // Get the current tenant
    LeaseTenant* tenant = lease_find(owner, address);
    if (!tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    // Expand if new_size is smaller or equal
    if (size <= tenant->size) {
        size = tenant->size * 2;
    }

    // Create a new tenant
    void* address = lease_address(owner, )

        // Tenant may be assigned a new address
        void* new_address
        = realloc(tenant->address, new_size);
    if (!new_address) {
        return HASH_ERROR;
    }

    // Remove the current tenant from the table
    if (HASH_SUCCESS != hash_table_delete(owner, address)) {
        free(new_address);
        return HASH_ERROR;
    }

    // Update tenant
    tenant->address = new_address;
    tenant->size = new_size;

    // Insert under new key (rehash)
    if (HASH_SUCCESS != hash_table_insert(owner, new_address, tenant)) {
        free(new_address); // if you want to be defensive
        return HASH_ERROR;
    }

    return HASH_SUCCESS;
}

LeaseState lease_transfer(LeaseOwner* from, LeaseOwner* to, void* address) {
    if (!from || !to || !address) {
        return HASH_ERROR;
    }

    LeaseTenant* from_tenant = lease_find(from, address);
    if (!from_tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    LeaseTenant* to_tenant = lease_find(to, address);
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
    LeaseTenant* tenant = (LeaseTenant*) hash_table_search(owner, address);
    if (!tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    if (tenant->address && tenant->policy.type == LEASE_CONTRACT_OWNED) {
        free(tenant->address);
    }

    // remove entry from table
    free(tenant); // hash table will not free tenants
    return hash_table_delete(owner, address); // return hash table state
}
