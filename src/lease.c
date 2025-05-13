/**
 * Copyright © 2024 Austin Berrio
 *
 * @file src/lease.c
 * @brief A dynamic runtime allocator for tracking memory address states.
 */

#include "memory.h"
#include "lease.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

LeaseOwner* lease_create(size_t capacity) {
    /// @note Hash Table does not take ownership of allocated objects
    return hash_table_create(capacity, HASH_TYPE_ADDRESS);
}

void* lease_address(LeaseOwner* owner, LeasePolicy policy, size_t size) {
    if (!owner || size == 0) {
        return NULL;
    }

    void* address = memory_aligned_alloc(size, alignof(max_align_t));
    if (!address) {
        return NULL;
    }

    LeaseTenant* tenant = memory_aligned_alloc(sizeof(LeaseTenant), alignof(LeaseTenant));
    if (!tenant) {
        free(address);
        return NULL;
    }

    *tenant = (LeaseTenant) {
        .address = address,
        .size = size,
        .policy = policy,
    };

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        free(address);
        free(tenant);
        return NULL;
    }

    return address;
}

char* lease_string(LeaseOwner* owner, LeasePolicy policy, const char* source) {
    int64_t size = strlen(source);
    if (-1 == size) {
        return NULL; // Invalid UTF-8 byte sequence
    }

    char* lease = lease_address(owner, policy, size + 1); // alloc null char
    if (!lease) {
        return NULL; // Alloc failed
    }

    memcpy(lease, source, size); // copy up to size bytes
    lease[size] = '\0'; // null terminate string
    return lease;
}

LeaseState lease_register(LeaseOwner* owner, LeasePolicy policy, void* address, size_t size) {
    if (hash_table_search(owner, address)) {
        return HASH_KEY_EXISTS;
    }

    LeaseTenant* tenant = memory_aligned_alloc(sizeof(LeaseTenant), alignof(LeaseTenant));
    if (!tenant) {
        return HASH_ERROR;
    }

    *tenant = (LeaseTenant) {
        .address = address,
        .size = size,
        .policy = policy,
    };

    if (HASH_SUCCESS != hash_table_insert(owner, address, tenant)) {
        free(tenant);
        return HASH_ERROR;
    }

    return HASH_SUCCESS;
}

LeaseTenant* lease_find(LeaseOwner* owner, void* address) {
    return (LeaseTenant*) hash_table_search(owner, address);
}

LeaseState lease_policy(LeaseOwner* owner, LeasePolicy new_policy, void* address) {
    LeaseTenant* tenant = (LeaseTenant*) hash_table_search(owner, address);
    if (!tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    tenant->policy = new_policy;
    return HASH_SUCCESS;
}

LeaseState lease_realloc(LeaseOwner* owner, void* address, size_t new_size) {
    if (!owner || !address || new_size == 0) {
        return HASH_ERROR;
    }

    LeaseTenant* tenant = lease_find(owner, address);
    if (!tenant) {
        return HASH_KEY_NOT_FOUND;
    }

    // Expand if new_size is smaller or equal
    if (new_size <= tenant->size) {
        new_size = tenant->size * 2;
    }

    // Tenant may be assigned a new address
    void* new_address = realloc(tenant->address, new_size);
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

void lease_free(LeaseOwner* owner) {
    if (owner) {
        for (uint64_t i = 0; i < owner->size; i++) {
            HashTableEntry* entry = &owner->entries[i];
            if (!entry || !entry->key || !entry->value) {
                continue;
            }

            LeaseTenant* tenant = (LeaseTenant*) entry->value;
            if (tenant && tenant->policy.type == LEASE_CONTRACT_OWNED && tenant->address) {
                free(tenant->address);
                free(tenant);
            }
        }

        hash_table_free(owner);
    }
}
