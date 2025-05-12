/**
 * Copyright © 2024 Austin Berrio
 *
 * @file include/lease.h
 * @brief A dynamic runtime allocator for tracking memory address states.
 */

#ifndef LEASE_ALLOCATOR_H
#define LEASE_ALLOCATOR_H

#include "hash_table.h"

#include <stdint.h>
#include <stddef.h>

typedef enum LeaseAccess {
    LEASE_ACCESS_GLOBAL,
    LEASE_ACCESS_LOCAL,
    LEASE_ACCESS_STATIC
} LeaseAccess;

typedef enum LeaseContract {
    LEASE_CONTRACT_OWNED,
    LEASE_CONTRACT_BORROWED,
    LEASE_CONTRACT_STATIC
} LeaseContract;

typedef struct LeasePolicy {
    LeaseContract type;
    LeaseAccess scope;
} LeasePolicy;

typedef struct LeaseTenant {
    void* address;
    size_t size;
    LeasePolicy policy;
} LeaseTenant;

typedef HashTableState LeaseState;
typedef HashTable LeaseOwner;

LeaseOwner* lease_create(size_t capacity);
void* lease_address(LeaseOwner* owner, LeasePolicy policy, size_t size);
char* lease_string(LeaseOwner* owner, LeasePolicy policy, const char* source);
LeaseState lease_register(LeaseOwner* owner, LeasePolicy policy, void* address, size_t size);
LeaseTenant* lease_find(LeaseOwner* owner, void* address);
LeaseState lease_policy(LeaseOwner* owner, LeasePolicy new_policy, void* address);
LeaseState lease_realloc(LeaseOwner* owner, void* key, size_t new_size);
LeaseState lease_transfer(LeaseOwner* from, LeaseOwner* to, void* address);
LeaseState lease_terminate(LeaseOwner* owner, void* address); // free one if owned
void lease_free(LeaseOwner* owner); // free everything owned

#endif // LEASE_ALLOCATOR_H
