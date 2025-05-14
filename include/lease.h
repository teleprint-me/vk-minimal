/**
 * Copyright © 2025 Austin Berrio
 *
 * @file include/lease.h
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
    LeaseAccess access;
    LeaseContract contract;
} LeasePolicy;

typedef struct LeaseObject {
    size_t alignment;
    size_t size;
    void* address;
} LeaseObject;

typedef struct LeaseTenant {
    LeasePolicy* policy;
    LeaseObject* object;
} LeaseTenant;

typedef HashTableState LeaseState;
typedef HashTable LeaseOwner;

/**
 * Create lease objects
 */

LeasePolicy* lease_create_policy(LeaseAccess access, LeaseContract contract);
LeaseObject* lease_create_object(void* address, size_t size, size_t alignment);
LeaseTenant* lease_create_tenant(LeasePolicy* policy, LeaseObject* object);
LeaseTenant* lease_create_tenant_owned(size_t size, size_t alignment);
LeaseTenant* lease_create_tenant_borrowed(void* address, size_t size, size_t alignment);
LeaseTenant* lease_create_tenant_static(void* address, size_t size, size_t alignment);
LeaseOwner* lease_create_owner(size_t capacity);

/**
 * Free lease objects
 */

void lease_free_policy(LeasePolicy* policy);
void lease_free_object(LeasePolicy* policy, LeaseObject* object);
void lease_free_tenant(LeaseTenant* tenant);
void lease_free_owner(LeaseOwner* owner);

/**
 * Lease operations
 */

LeaseTenant* lease_get_tenant(LeaseOwner* owner, void* address);
LeaseObject* lease_get_object(LeaseOwner* owner, void* address);
LeasePolicy* lease_get_policy(LeaseOwner* owner, void* address);
LeaseAccess lease_get_access(LeaseOwner* owner, void* address);
LeaseContract lease_get_contract(LeaseOwner* owner, void* address);

void* lease_alloc_owned(LeaseOwner* owner, size_t size, size_t alignment);
void* lease_alloc_borrowed(LeaseOwner* owner, void* address, size_t size, size_t alignment);
void* lease_alloc_static(LeaseOwner* owner, void* address, size_t size, size_t alignment);

char* lease_string_owned(LeaseOwner* owner, const char* address);
char* lease_string_borrowed(LeaseOwner* owner, const char* address);
char* lease_string_static(LeaseOwner* owner, const char* address);

LeaseState lease_realloc(LeaseOwner* owner, void* address, size_t size, size_t alignment);
LeaseState lease_transfer(LeaseOwner* from, LeaseOwner* to, void* address);
LeaseState lease_terminate(LeaseOwner* owner, void* address); // free one if owned

#endif // LEASE_ALLOCATOR_H
