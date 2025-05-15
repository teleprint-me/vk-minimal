# Lease Allocator

A dynamic memory tracking system designed for C projects where explicit ownership, lifetime control, and modular memory contracts are necessary.

## Overview

The lease allocator tracks and manages memory regions (objects) via ownership policies (contracts). This enables deterministic memory lifetime in C with no garbage collection, while still supporting flexible sharing, borrowing, and reuse patterns.

## Components

### `LeaseOwner`

* Represents a table of tracked memory addresses.
* Internally wraps a `HashTable`.
* Responsible for storing and managing `LeaseTenant` entries.

### `LeaseTenant`

* Combines a `LeasePolicy` and a `LeaseObject`.
* Represents a complete memory contract.

### `LeasePolicy`

* Defines ownership rules.
* Enum values:

  * `LEASE_CONTRACT_OWNED`: The allocator is responsible for freeing this memory.
  * `LEASE_CONTRACT_BORROWED`: Memory is owned externally; lease is for tracking only.
  * `LEASE_CONTRACT_STATIC`: Memory has static lifetime; not owned or freed.

### `LeaseObject`

* Metadata about an allocated region:

  * `void* address`
  * `size_t size`
  * `size_t alignment`

## Usage

### Create a Lease Owner

```c
LeaseOwner* owner = lease_create_owner(128);
```

### Allocate and Track Owned Memory

```c
void* memory = lease_alloc_owned_address(owner, 1024, alignof(int));
```

### Track Borrowed Memory

```c
lease_alloc_borrowed_address(owner, external_buffer, size, alignof(float));
```

### Free a Lease (owned only)

```c
lease_terminate(owner, memory);
```

## Ownership Contracts

| Contract   | Frees Memory? | Valid for stack memory? | Notes                           |
| ---------- | ------------- | ----------------------- | ------------------------------- |
| `OWNED`    | Yes           | No                      | Leased memory will be freed     |
| `BORROWED` | No            | Yes                     | Safe for temporary input/output |
| `STATIC`   | No            | Yes                     | Lifetime managed by compiler    |

## API Summary

* Creation: `lease_create_owner`, `lease_create_policy`, etc.
* Allocation: `lease_alloc_*`
* Access: `lease_get_*`
* Mutation: `lease_realloc`, `lease_transfer`, `lease_terminate`
* Debug: `lease_debug_owner`, `lease_debug_tenant`

## Debugging

Call `lease_debug_owner(owner)` to print all tracked leases and their policies/objects.

## Implementation Notes

* All tenants and policies are heap-allocated to avoid stack overflows on scale.
* `realloc` creates a new tenant and rebinds the memory, copying data manually.
* Thread-safety depends on the underlying hash table implementation.

## Future Work

* Thread-safe `LeaseOwner` using `atomic` or `pthread` mutexes.
* Lease cloning and migration between allocators.
* Scoped lifetime helpers or allocator pools.
