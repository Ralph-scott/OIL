#ifndef HASHMAP_H_
#define HASHMAP_H_

#include <stdint.h>
#include <stdbool.h>

// prime number not close to 1024 or 2048
#define HASHMAP_SIZE 1669

// don't judge
typedef uint32_t (*HashFn)(const void *);
typedef bool (*EqualsFn)(const void *, const void *);

// An entry to the hashmap
typedef struct HashMapEntry
{
    void *key;
    void *value;
    struct HashMapEntry *next;
} HashMapEntry;

typedef struct HashMap
{
    HashFn hash_fn;
    EqualsFn equals_fn;

    size_t key_size;
    size_t value_size;

    HashMapEntry **buckets;
} HashMap;

uint32_t hash_string(const char *string, const size_t string_len);

HashMap hashmap_new(HashFn hash_fn, EqualsFn equals_fn, const size_t key_size, const size_t value_size);

void hashmap_insert(HashMap *hashmap, const void *key, const void *value);
void *hashmap_get(const HashMap *hashmap, const void *key);
void hashmap_remove(HashMap *hashmap, const void *key);

void hashmap_free(HashMap *hashmap);

#endif // HASHMAP_H_
