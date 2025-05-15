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

#include "core/hash_table.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Access and ownership models for dynamic memory.
 */

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

/**
 * @brief Core memory lease structures.
 */

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
 * @section Creation
 */

LeaseOwner* lease_create_owner(size_t capacity);
LeasePolicy* lease_create_policy(LeaseAccess access, LeaseContract contract);
LeaseObject* lease_create_object(void* address, size_t size, size_t alignment);
LeaseTenant* lease_create_tenant(LeasePolicy* policy, LeaseObject* object);

/**
 * @section Destruction
 */

void lease_free_owner(LeaseOwner* owner);
void lease_free_policy(LeasePolicy* policy);
void lease_free_object(LeasePolicy* policy, LeaseObject* object);
void lease_free_tenant(LeaseTenant* tenant);

/**
 * @section Tenant Allocation
 */

LeaseTenant* lease_alloc_owned_tenant(size_t size, size_t alignment);
LeaseTenant* lease_alloc_borrowed_tenant(void* address, size_t size, size_t alignment);
LeaseTenant* lease_alloc_static_tenant(void* address, size_t size, size_t alignment);

/**
 * @section Address Allocation
 */

void* lease_alloc_owned_address(LeaseOwner* owner, size_t size, size_t alignment);
void* lease_alloc_borrowed_address(LeaseOwner* owner, void* address, size_t size, size_t alignment);
void* lease_alloc_static_address(LeaseOwner* owner, void* address, size_t size, size_t alignment);

/**
 * @section String Allocation
 */

char* lease_alloc_owned_string(LeaseOwner* owner, const char* address);
char* lease_alloc_borrowed_string(LeaseOwner* owner, const char* address);
char* lease_alloc_static_string(LeaseOwner* owner, const char* address);

/**
 * @section Metadata Access
 */

LeaseTenant* lease_get_tenant(LeaseOwner* owner, void* address);
LeaseObject* lease_get_object(LeaseOwner* owner, void* address);
LeasePolicy* lease_get_policy(LeaseOwner* owner, void* address);
LeaseAccess lease_get_access(LeaseOwner* owner, void* address);
LeaseContract lease_get_contract(LeaseOwner* owner, void* address);

/**
 * @section Mutation and Transfer
 */

LeaseState lease_realloc(LeaseOwner* owner, void* address, size_t size, size_t alignment);
LeaseState lease_transfer(LeaseOwner* from, LeaseOwner* to, void* address);
LeaseState lease_terminate(LeaseOwner* owner, void* address); // frees if owned

/**
 * @section Debugging
 */

void lease_debug_tenant(LeaseTenant* tenant);
void lease_debug_owner(LeaseOwner* owner);

#endif // LEASE_ALLOCATOR_H
