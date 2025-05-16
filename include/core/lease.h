/**
 * Copyright © 2025 Austin Berrio
 *
 * @file include/core/lease.h
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
 * @brief Enum representing the accessibility scope of a memory lease.
 *
 * - LEASE_ACCESS_GLOBAL: Memory is accessible globally across the application.
 * - LEASE_ACCESS_LOCAL: Memory is accessible within a limited or local scope.
 * - LEASE_ACCESS_STATIC: Memory is statically allocated and not subject to deallocation.
 */
typedef enum LeaseAccess {
    LEASE_ACCESS_GLOBAL,
    LEASE_ACCESS_LOCAL,
    LEASE_ACCESS_STATIC
} LeaseAccess;

/**
 * @brief Enum representing the ownership contract of a memory lease.
 *
 * - LEASE_CONTRACT_OWNED: The LeaseTenant fully owns the memory and is responsible for freeing it.
 * - LEASE_CONTRACT_BORROWED: The LeaseTenant references memory owned elsewhere; it must not free
 * it.
 * - LEASE_CONTRACT_STATIC: The memory is static and has indefinite lifetime; no deallocation
 * needed.
 */
typedef enum LeaseContract {
    LEASE_CONTRACT_OWNED,
    LEASE_CONTRACT_BORROWED,
    LEASE_CONTRACT_STATIC
} LeaseContract;

/**
 * @brief Defines the access and ownership rules for a memory region.
 */
typedef struct LeasePolicy {
    LeaseAccess access; /**< Accessibility scope of the memory */
    LeaseContract contract; /**< Ownership contract of the memory */
} LeasePolicy;

/**
 * @brief Represents a memory allocation with size, alignment, and address.
 */
typedef struct LeaseObject {
    size_t alignment; /**< Alignment of the allocated memory block */
    size_t size; /**< Size of the allocated memory block in bytes */
    void* address; /**< Pointer to the allocated memory */
} LeaseObject;

/**
 * @brief Combines a LeasePolicy and LeaseObject into a complete allocation contract.
 *
 * This structure owns both the LeasePolicy and LeaseObject it references.
 */
typedef struct LeaseTenant {
    LeasePolicy* policy; /**< Pointer to the allocation policy */
    LeaseObject* object; /**< Pointer to the allocated object */
} LeaseTenant;

/** @typedef LeaseState
 *  @brief Represents the state or result of lease operations (e.g., success/failure).
 */
typedef HashTableState LeaseState;

/** @typedef LeaseOwner
 *  @brief Represents an owner tracking leases using an underlying hash table.
 */
typedef HashTable LeaseOwner;

/**
 * @name Creation Functions
 * @{
 */

/**
 * @brief Creates a new LeaseOwner with a given capacity.
 *
 * @param capacity Initial capacity for the internal tracking structures.
 * @return Pointer to a newly allocated LeaseOwner or NULL on failure.
 */
LeaseOwner* lease_create_owner(size_t capacity);

/**
 * @brief Creates a LeasePolicy with specified access and contract.
 *
 * @param access Accessibility scope for the lease.
 * @param contract Ownership contract for the lease.
 * @return Pointer to a newly allocated LeasePolicy.
 */
LeasePolicy* lease_create_policy(LeaseAccess access, LeaseContract contract);

/**
 * @brief Creates a LeaseObject describing an allocated memory block.
 *
 * @param address Pointer to the allocated memory.
 * @param size Size of the allocation in bytes.
 * @param alignment Alignment of the allocation.
 * @return Pointer to a newly allocated LeaseObject.
 */
LeaseObject* lease_create_object(void* address, size_t size, size_t alignment);

/**
 * @brief Creates a LeaseTenant combining a LeasePolicy and LeaseObject.
 *
 * @param policy Pointer to the LeasePolicy to associate.
 * @param object Pointer to the LeaseObject to associate.
 * @return Pointer to a newly allocated LeaseTenant.
 */
LeaseTenant* lease_create_tenant(LeasePolicy* policy, LeaseObject* object);

/** @} */

/**
 * @name Destruction Functions
 * @{
 */

/**
 * @brief Frees all resources associated with a LeaseOwner.
 *
 * This invalidates all tracked leases managed by the owner.
 *
 * @param owner Pointer to the LeaseOwner to free.
 */
void lease_free_owner(LeaseOwner* owner);

/**
 * @brief Frees a LeasePolicy.
 *
 * @param policy Pointer to the LeasePolicy to free.
 */
void lease_free_policy(LeasePolicy* policy);

/**
 * @brief Frees a LeaseObject if the associated policy indicates ownership.
 *
 * If the policy contract is OWNED, the underlying memory address will be freed.
 *
 * @param policy Pointer to the LeasePolicy governing the object.
 * @param object Pointer to the LeaseObject to free.
 */
void lease_free_object(LeasePolicy* policy, LeaseObject* object);

/**
 * @brief Frees a LeaseTenant and all associated resources.
 *
 * Frees the LeasePolicy and LeaseObject according to ownership rules.
 *
 * @param tenant Pointer to the LeaseTenant to free.
 */
void lease_free_tenant(LeaseTenant* tenant);

/** @} */

/**
 * @name Tenant Allocation Functions
 * @{
 */

/**
 * @brief Allocates and returns a new LeaseTenant with owned memory.
 *
 * The memory is allocated and owned by the tenant.
 *
 * @param size Size of the allocation in bytes.
 * @param alignment Alignment for the allocation.
 * @return Pointer to a newly allocated LeaseTenant with owned memory.
 */
LeaseTenant* lease_alloc_owned_tenant(size_t size, size_t alignment);

/**
 * @brief Allocates a LeaseTenant borrowing existing memory.
 *
 * The tenant does not own the memory and must not free it.
 *
 * @param address Pointer to existing memory.
 * @param size Size of the borrowed memory in bytes.
 * @param alignment Alignment of the borrowed memory.
 * @return Pointer to a newly allocated LeaseTenant borrowing the memory.
 */
LeaseTenant* lease_alloc_borrowed_tenant(void* address, size_t size, size_t alignment);

/**
 * @brief Allocates a LeaseTenant for static memory.
 *
 * The memory is static and has indefinite lifetime.
 *
 * @param address Pointer to static memory.
 * @param size Size of the static memory in bytes.
 * @param alignment Alignment of the static memory.
 * @return Pointer to a newly allocated LeaseTenant for static memory.
 */
LeaseTenant* lease_alloc_static_tenant(void* address, size_t size, size_t alignment);

/** @} */

/**
 * @name Address Allocation Functions
 * @{
 */

/**
 * @brief Allocates owned memory and tracks it in the LeaseOwner.
 *
 * @param owner Pointer to the LeaseOwner managing this allocation.
 * @param size Size of the allocation in bytes.
 * @param alignment Alignment for the allocation.
 * @return Pointer to the allocated memory or NULL on failure.
 */
void* lease_alloc_owned_address(LeaseOwner* owner, size_t size, size_t alignment);

/**
 * @brief Registers borrowed memory with the LeaseOwner without allocating.
 *
 * @param owner Pointer to the LeaseOwner managing this lease.
 * @param address Pointer to existing borrowed memory.
 * @param size Size of the borrowed memory in bytes.
 * @param alignment Alignment of the borrowed memory.
 * @return Pointer to the borrowed memory or NULL on failure.
 */
void* lease_alloc_borrowed_address(LeaseOwner* owner, void* address, size_t size, size_t alignment);

/**
 * @brief Registers static memory with the LeaseOwner without allocating.
 *
 * @param owner Pointer to the LeaseOwner managing this lease.
 * @param address Pointer to static memory.
 * @param size Size of the static memory in bytes.
 * @param alignment Alignment of the static memory.
 * @return Pointer to the static memory or NULL on failure.
 */
void* lease_alloc_static_address(LeaseOwner* owner, void* address, size_t size, size_t alignment);

/** @} */

/**
 * @name String Allocation Functions
 * @{
 */

/**
 * @brief Allocates owned memory and copies the provided string.
 *
 * @param owner Pointer to the LeaseOwner managing this allocation.
 * @param address Pointer to the null-terminated source string.
 * @return Pointer to the newly allocated string or NULL on failure.
 */
char* lease_alloc_owned_string(LeaseOwner* owner, const char* address);

/**
 * @brief Registers a borrowed string with the LeaseOwner without copying.
 *
 * @param owner Pointer to the LeaseOwner managing this lease.
 * @param address Pointer to the existing null-terminated string.
 * @return Pointer to the borrowed string or NULL on failure.
 */
char* lease_alloc_borrowed_string(LeaseOwner* owner, const char* address);

/**
 * @brief Registers a static string with the LeaseOwner without copying.
 *
 * @param owner Pointer to the LeaseOwner managing this lease.
 * @param address Pointer to the static null-terminated string.
 * @return Pointer to the static string or NULL on failure.
 */
char* lease_alloc_static_string(LeaseOwner* owner, const char* address);

/** @} */

/**
 * @name Metadata Access Functions
 * @{
 */

/**
 * @brief Retrieves the LeaseTenant associated with a given address.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address to query.
 * @return Pointer to the associated LeaseTenant, or NULL if not found.
 */
LeaseTenant* lease_get_tenant(LeaseOwner* owner, void* address);

/**
 * @brief Retrieves the LeaseObject associated with a given address.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address to query.
 * @return Pointer to the associated LeaseObject, or NULL if not found.
 */
LeaseObject* lease_get_object(LeaseOwner* owner, void* address);

/**
 * @brief Retrieves the LeasePolicy associated with a given address.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address to query.
 * @return Pointer to the associated LeasePolicy, or NULL if not found.
 */
LeasePolicy* lease_get_policy(LeaseOwner* owner, void* address);

/**
 * @brief Retrieves the LeaseAccess for a given address.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address to query.
 * @return LeaseAccess enum value; defaults to LEASE_ACCESS_LOCAL if not found.
 */
LeaseAccess lease_get_access(LeaseOwner* owner, void* address);

/**
 * @brief Retrieves the LeaseContract for a given address.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address to query.
 * @return LeaseContract enum value; defaults to LEASE_CONTRACT_BORROWED if not found.
 */
LeaseContract lease_get_contract(LeaseOwner* owner, void* address);

/** @} */

/**
 * @name Mutation and Transfer Functions
 * @{
 */

/**
 * @brief Attempts to reallocate a lease to a new size and alignment.
 *
 * If the lease is owned, the underlying memory will be reallocated.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address of the memory to reallocate.
 * @param size New size in bytes.
 * @param alignment New alignment.
 * @return LeaseState indicating success or failure.
 */
LeaseState lease_realloc(LeaseOwner* owner, void* address, size_t size, size_t alignment);

/**
 * @brief Transfers a lease from one LeaseOwner to another.
 *
 * @param from Pointer to the source LeaseOwner.
 * @param to Pointer to the destination LeaseOwner.
 * @param address Address of the lease to transfer.
 * @return LeaseState indicating success or failure.
 */
LeaseState lease_transfer(LeaseOwner* from, LeaseOwner* to, void* address);

/**
 * @brief Terminates a lease and frees owned memory if applicable.
 *
 * @param owner Pointer to the LeaseOwner managing leases.
 * @param address Address of the lease to terminate.
 * @return LeaseState indicating success or failure.
 */
LeaseState lease_terminate(LeaseOwner* owner, void* address);

/** @} */

/**
 * @name Debugging Functions
 * @{
 */

/**
 * @brief Prints debug information about a LeaseTenant.
 *
 * @param tenant Pointer to the LeaseTenant to debug.
 */
void lease_debug_tenant(LeaseTenant* tenant);

/**
 * @brief Prints debug information about a LeaseOwner and all tracked leases.
 *
 * @param owner Pointer to the LeaseOwner to debug.
 */
void lease_debug_owner(LeaseOwner* owner);

/** @} */

#endif // LEASE_ALLOCATOR_H
