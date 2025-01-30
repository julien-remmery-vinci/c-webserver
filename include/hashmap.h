#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    HM_OK,
    HM_ERR_INVALID_SIZE,
    HM_ERR_MEMORY_ALLOCATION,
} Hm_Error;

#define HM_DEFAULT_SIZE 10
#define HM_RESIZE_FACTOR 2
typedef struct HashMap HashMap;
typedef struct HashMapEntry HashMapEntry;

struct HashMapEntry {
    char* key;
    void* value;
    // Next entry in the linked list, 
    // NULL by default or if last entry in list
    HashMapEntry* next_entry;
};

struct HashMap {
    size_t size;
    size_t entries_count;
    HashMapEntry** entries;
};

HashMap
hm_create(size_t size);

/**
 * Get value for a specific key
 * Returns 
 *  - The stored value associated to the key
 *  - NULL if the key is not present, or the map is is NULL allocated
 */
void*
hm_get(HashMap* map, const char* key);

/**
 * Put the key, value pair in the hashmap
 * Returns
 *  - 0 if the key, value pair was successfully added
 *  - -1 if the map is NULL allocated
 */
Hm_Error
hm_put(HashMap* map, const char* key, void* value);

/**
 * Remove a key, value pair from the hashtable
 * Returns 
 *  - The removed value associated to the key
 *  - NULL if the key is not present, or the map is is NULL allocated
 */
void*
hm_remove(HashMap* map, const char* key);

/**
 * Free the memory allocated for the map
 */
void
hm_free(HashMap* map);

/**
 * Returns
 *  true if the map is empty
 *  false if the map contains at least one key
 */
bool
hm_isempty(HashMap map);

bool
hm_exists(HashMap* map, const  char* key);

#endif // HASHMAP_H