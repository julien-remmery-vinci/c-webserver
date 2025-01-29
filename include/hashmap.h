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

#ifndef HASHMAP_DEFINITIONS
#ifdef HASHMAP_IMPLEMENTATION
#define HASHMAP_DEFINITIONS 

#include <stdio.h>
#include <string.h>

/**
 * djb2 algorithm
 * source: https://www.cse.yorku.ca/~oz/hash.html
 */
unsigned long
hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * Create a new entry with the key, value pair
 */
HashMapEntry*
create_entry(const char* key, void* value)
{
    HashMapEntry* entry = calloc(1, sizeof(HashMapEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->key = strdup(key);
    if (entry->key == NULL) {
        free(entry);
        return NULL;
    }
    entry->value = value;
    entry->next_entry = NULL;
    return entry;
}

HashMap
hm_create(size_t init_size)
{
    if(init_size == 0) {
        perror("HashMap Error: illegal initialization size provided");
        exit(EXIT_FAILURE);
    }
    HashMap map = {
        .size = init_size,
        .entries_count = 0,
        .entries = calloc(init_size, sizeof(HashMapEntry*))
    };
    if(map.entries == NULL) {
        perror("HashMap Error: init map allocation error");
        exit(EXIT_FAILURE);
    }
    return map;
}

Hm_Error
hm_resize(HashMap* map)
{
    HashMap tmp = {0};
    tmp.size = map->size * HM_RESIZE_FACTOR;
    tmp.entries = calloc(tmp.size, sizeof(HashMapEntry));
    if(tmp.entries == NULL) {
        return -1;
    }

    for(size_t i = 0; i < map->size; i++) {
        if(map->entries[i] != NULL) {
            hm_put(&tmp, map->entries[i]->key, map->entries[i]->value);
            HashMapEntry* entry = map->entries[i]->next_entry;
            while(entry != NULL) {
                hm_put(&tmp, entry->key, entry->value);
                entry = entry->next_entry;
            }
        }
    }
    hm_free(map);
    *map = tmp;
    return 0;
}

void*
hm_get(HashMap* map, const char* key)
{
    if(map == NULL) {
        return NULL;
    }

    char* lower_key = Ju_toLower(key);

    unsigned long hashcode = hash((unsigned char*)lower_key);
    size_t index = hashcode % map->size;

    HashMapEntry* current = map->entries[index];
    while (current != NULL) {
        if (strcmp(current->key, lower_key) == 0) {
            return current->value;
        }
        current = current->next_entry;
    }

    return NULL;
}

Hm_Error
hm_put(HashMap* map, const char* key, void* value)
{
    if(map == NULL) {
        return -1;
    }
    if(key == NULL) {
        return -1;
    }

    int ret;
    // Resize if map is full
    // Subject to change to partially full (75%)
    if(map->size == map->entries_count) {
        ret = hm_resize(map);
        if (ret != HM_OK) return ret;
    }
    char* lower_key = Ju_toLower(key);

    // Hashcode and bucket index
    unsigned long hashcode = hash((unsigned char*)lower_key);
    size_t index = hashcode % map->size;

    HashMapEntry* new_entry = create_entry(lower_key, value);
    if (new_entry == NULL) {
        return HM_ERR_MEMORY_ALLOCATION;
    }

    if(map->entries[index] == NULL) { // Empty bucket
        map->entries[index] = new_entry;
    }
    else { // Full bucket
        HashMapEntry* current = map->entries[index];
        while (current != NULL) {
            if (strcmp(current->key, lower_key) == 0) {
                // Replace value if same key
                current->value = value;
                free(new_entry->key);
                free(new_entry);
                return 0;
            }
            current = current->next_entry;
        }
        new_entry->next_entry = map->entries[index];
        map->entries[index] = new_entry;
    }
    map->entries_count++;
    return 0;
}

void*
hm_remove(HashMap* map, const char* key){
    if(map == NULL) {
        return NULL;
    }

    unsigned long hashcode = hash((unsigned char*)key);
    size_t index = hashcode % map->size;

    HashMapEntry* current = map->entries[index];
    HashMapEntry* prev = NULL;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            void* value = current->value;
            if (prev == NULL) {
                map->entries[index] = current->next_entry;
            } else {
                prev->next_entry = current->next_entry;
            }
            free(current->key);
            free(current);
            map->entries_count--;
            return value;
        }
        prev = current;
        current = current->next_entry;
    }

    return NULL;
}

void 
hm_free(HashMap* map) 
{
    for (size_t i = 0; i < map->size; i++) {
        HashMapEntry* entry = map->entries[i];
        while (entry != NULL) {
            HashMapEntry* next = entry->next_entry;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    free(map->entries);
}

bool
hm_isempty(HashMap map)
{
    return map.entries_count == 0;
}

bool
hm_exists(HashMap* map, const char* key)
{
    char* lower_key = Ju_toLower(key);

    unsigned long hashcode = hash((unsigned char*)lower_key);
    size_t index = hashcode % map->size;

    HashMapEntry* current = map->entries[index];
    while (current != NULL) {
        if (strcmp(current->key, lower_key) == 0) {
            return true;
        }
        current = current->next_entry;
    }

    return false;
}

#endif // HASHMAP_IMPLEMENTATION
#endif // HASHMAP_DEFINITIONS