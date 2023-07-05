#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hashmap.h"
#include "utils.h"

uint32_t hash_string(const char *string, const size_t string_len)
{
    uint32_t hashed = 0x12345678;

    for (size_t i = 0; i < string_len; ++i) {
        hashed ^= string[i];
        hashed *= 0x5bd1e995;
        hashed ^= hashed >> 15;
    }

    return hashed;
}

HashMap hashmap_new(HashFn hash_fn, EqualsFn equals_fn, const size_t key_size, const size_t value_size)
{
    HashMap hashmap = {
        .hash_fn = hash_fn,
        .equals_fn = equals_fn,
        .key_size = key_size,
        .value_size = value_size,
    };

    // clear the memory (init to NULL)
    hashmap.buckets = malloc(sizeof(*hashmap.buckets) * HASHMAP_SIZE);

    if (hashmap.buckets == NULL) {
        ALLOCATION_ERROR();
    }

    memset(hashmap.buckets, 0, sizeof(*hashmap.buckets) * HASHMAP_SIZE);

    return hashmap;
}

void hashmap_insert(HashMap *hashmap, const void *key, const void *value)
{
    // hash the key
    const uint32_t index = (*hashmap->hash_fn)(key) % HASHMAP_SIZE;

    // allocate an entry
    HashMapEntry *entry = malloc(sizeof(HashMapEntry));

    if (entry == NULL) {
        ALLOCATION_ERROR();
    }

    entry->key = malloc(hashmap->key_size);

    if (entry->key == NULL) {
        ALLOCATION_ERROR();
    }

    // copy the key
    memcpy(entry->key, key, hashmap->key_size);

    entry->value = malloc(hashmap->value_size);

    if (entry->value == NULL) {
        ALLOCATION_ERROR();
    }

    // copy the value
    memcpy(entry->value, value, hashmap->value_size);

    entry->next = NULL;

    HashMapEntry *iter = hashmap->buckets[index];

    if (iter == NULL) {
        hashmap->buckets[index] = entry;
        return;
    }

    // find the item in the hashmap (in case of collisions)
    while (iter->next != NULL && !(*hashmap->equals_fn)(iter->next->key, entry->key)) {
        iter = iter->next;
    }

    if (iter->next != NULL) {
        entry->next = iter->next->next;
        free(iter->next->key);
        free(iter->next->value);
        free(iter->next);
    }
    iter->next = entry;
}

void *hashmap_get(const HashMap *hashmap, const void *key)
{
    // hash the key
    const uint32_t index = (*hashmap->hash_fn)(key) % HASHMAP_SIZE;

    HashMapEntry *iter = hashmap->buckets[index];

    // find the item in the hashmap (in case of collisions)
    while (iter != NULL && !(*hashmap->equals_fn)(iter->key, key)) {
        iter = iter->next;
    }

    if (iter == NULL) {
        return NULL;
    }

    return iter->value;
}

void hashmap_remove(HashMap *hashmap, const void *key)
{
    const uint32_t index = (*hashmap->hash_fn)(key) % HASHMAP_SIZE;

    HashMapEntry *prev = NULL;
    HashMapEntry *iter = hashmap->buckets[index];

    // find the item in the hashmap
    while (iter != NULL && !(*hashmap->equals_fn)(iter->key, key)) {
        prev = iter;
        iter = iter->next;
    }

    if (iter == NULL) {
        return; // Key not found
    }

    if (prev != NULL) {
        prev->next = iter->next;
    } else {
        hashmap->buckets[index] = iter->next;
    }

    free(iter->key);
    free(iter->value);
    free(iter);
}

void hashmap_free(HashMap *hashmap)
{
    for (size_t i = 0; i < HASHMAP_SIZE; ++i) {
        HashMapEntry *iter = hashmap->buckets[i];

        // free every item
        while (iter != NULL) {
            HashMapEntry *next = iter->next;

            free(iter->key);
            free(iter->value);
            free(iter);

            iter = next;
        }
    }

    free(hashmap->buckets);
}
