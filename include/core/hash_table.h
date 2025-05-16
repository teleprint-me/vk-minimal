/**
 * Copyright © 2024 Austin Berrio
 *
 * @file include/core/hash_table.h
 * @brief Minimalistic hash table implementation providing mapping between integers, strings, and
 * memory addresses.
 *
 * The Hash Interface provides a minimal dictionary-like API supporting insertion, search, deletion,
 * and clearing for keys of type integer, string, or memory address.
 *
 * @note Comparison functions must return 0 for equality, non-zero otherwise.
 * @note Thread Safety: Uses mutexes for thread-safe operations; calls must be within critical
 * sections.
 * @note Collision resolution: Uses linear probing.
 */

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdint.h>
#include <pthread.h>

/**
 * @brief Possible outcomes for hash table operations.
 */
typedef enum HashTableState {
    HASH_SUCCESS, /**< Operation completed successfully. */
    HASH_ERROR, /**< General error occurred during operation. */
    HASH_KEY_EXISTS, /**< Duplicate key insertion attempted. */
    HASH_KEY_NOT_FOUND, /**< Key not found in the table. */
    HASH_TABLE_FULL /**< Hash table has reached maximum capacity. */
} HashTableState;

/**
 * @brief Types of keys supported by the hash table.
 */
typedef enum HashTableType {
    HASH_TYPE_INTEGER, /**< Keys are integers (uint64_t). */
    HASH_TYPE_STRING, /**< Keys are null-terminated strings. */
    HASH_TYPE_ADDRESS /**< Keys are memory addresses (uintptr_t). */
} HashTableType;

/**
 * @brief Represents a key-value pair entry in the hash table.
 */
typedef struct HashTableEntry {
    void* key; /**< Pointer to the key (type depends on HashTableType). */
    void* value; /**< Pointer to the associated value. */
} HashTableEntry;

/**
 * @brief Core hash table structure.
 */
typedef struct HashTable {
    uint64_t count; /**< Current number of entries in the table. */
    uint64_t size; /**< Total capacity of the hash table. */
    HashTableType type; /**< Type of keys stored. */
    HashTableEntry* entries; /**< Array of hash entries. */
    uint64_t (*hash)(const void* key, uint64_t size, uint64_t i); /**< Hash function with probing. */
    int (*compare)(const void* key1, const void* key2); /**< Key comparison function. */
    pthread_mutex_t thread_lock; /**< Mutex for thread safety. */
} HashTable;

/**
 * @name Life-cycle Management
 * @{
 */

/**
 * @brief Creates a new hash table.
 *
 * @param initial_size Initial capacity of the table.
 * @param key_type Type of keys (integer, string, or address).
 * @return Pointer to the new hash table, or NULL on failure.
 */
HashTable* hash_table_create(uint64_t initial_size, HashTableType key_type);

/**
 * @brief Frees a hash table and all associated memory.
 *
 * @param table Pointer to the hash table to free.
 */
void hash_table_free(HashTable* table);

/** @} */

/**
 * @name Core Hash Operations
 * @{
 */

/**
 * @brief Inserts a key-value pair into the hash table.
 *
 * @param table Pointer to the hash table.
 * @param key Pointer to the key.
 * @param value Pointer to the value.
 * @return HASH_SUCCESS if insertion succeeded, or error code.
 */
HashTableState hash_table_insert(HashTable* table, const void* key, void* value);

/**
 * @brief Resizes the hash table to a new capacity.
 *
 * @param table Pointer to the hash table.
 * @param new_size Desired new capacity.
 * @return HASH_SUCCESS on success, HASH_ERROR on failure.
 */
HashTableState hash_table_resize(HashTable* table, uint64_t new_size);

/**
 * @brief Deletes a key and its associated value from the hash table.
 *
 * @param table Pointer to the hash table.
 * @param key Pointer to the key to delete.
 * @return HASH_SUCCESS if deletion succeeded, HASH_KEY_NOT_FOUND if not found.
 */
HashTableState hash_table_delete(HashTable* table, const void* key);

/**
 * @brief Removes all entries from the hash table.
 *
 * @param table Pointer to the hash table.
 * @return HASH_SUCCESS on success, HASH_ERROR on failure.
 */
HashTableState hash_table_clear(HashTable* table);

/**
 * @brief Searches for a key in the hash table.
 *
 * @param table Pointer to the hash table.
 * @param key Pointer to the key to search.
 * @return Pointer to the associated value, or NULL if not found.
 */
void* hash_table_search(HashTable* table, const void* key);

/** @} */

/**
 * @name Integer Key Support
 * @{
 */

/**
 * @brief Hash function for integer keys with linear probing.
 *
 * @param key Pointer to the integer key.
 * @param size Size of the hash table.
 * @param i Probe index for collision resolution.
 * @return Hash index for the given key.
 */
uint64_t hash_integer(const void* key, uint64_t size, uint64_t i);

/**
 * @brief Compares two integer keys.
 *
 * @param key1 Pointer to first integer key.
 * @param key2 Pointer to second integer key.
 * @return 0 if equal, non-zero otherwise.
 */
int hash_integer_compare(const void* key1, const void* key2);

/**
 * @brief Searches for an integer key in the hash table.
 *
 * @param table Pointer to the hash table.
 * @param key Pointer to the integer key to search.
 * @return Pointer to associated value, or NULL if not found.
 */
int32_t* hash_integer_search(HashTable* table, const void* key);

/** @} */

/**
 * @name String Key Support
 * @{
 */

/**
 * @brief Computes DJB2 hash for a string key.
 *
 * @param string Null-terminated string key.
 * @return Hash value of the string.
 */
uint64_t hash_djb2(const char* string);

/**
 * @brief Hash function for string keys with linear probing.
 *
 * @param key Pointer to the string key.
 * @param size Size of the hash table.
 * @param i Probe index.
 * @return Hash index for the given key.
 */
uint64_t hash_string(const void* key, uint64_t size, uint64_t i);

/**
 * @brief Compares two string keys.
 *
 * @param key1 Pointer to first string key.
 * @param key2 Pointer to second string key.
 * @return 0 if equal, non-zero otherwise.
 */
int hash_string_compare(const void* key1, const void* key2);

/**
 * @brief Searches for a string key in the hash table.
 *
 * @param table Pointer to the hash table.
 * @param key Pointer to the string key to search.
 * @return Pointer to associated value, or NULL if not found.
 */
char* hash_string_search(HashTable* table, const void* key);

/** @} */

/**
 * @name Address Key Support
 * @{
 */

/**
 * @brief Hash function for address keys with linear probing.
 *
 * @param key Pointer to the address key.
 * @param size Size of the hash table.
 * @param i Probe index.
 * @return Hash index for the given key.
 */
uint64_t hash_address(const void* key, uint64_t size, uint64_t i);

/**
 * @brief Compares two address keys.
 *
 * @param key1 Pointer to first address key.
 * @param key2 Pointer to second address key.
 * @return 0 if equal, non-zero otherwise.
 */
int hash_address_compare(const void* key1, const void* key2);

/**
 * @brief Searches for an address key in the hash table.
 *
 * @param table Pointer to the hash table.
 * @param key Pointer to the address key to search.
 * @return Pointer to associated value, or NULL if not found.
 */
void* hash_address_search(HashTable* table, const void* key);

/** @} */

#endif // HASH_TABLE_H
