#ifndef SLIM_HASH_HEADER
#define SLIM_HASH_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

/**

bool sh_del(hash, key);  // ture if value was found and deleted, false if not found

for(sh_it_p it = sh_start(hash); it != NULL; it = sh_next(it)) {
	int64_t key = sh_key(it);
	int value = sh_value(it);
	sh_remove(it);
}

int* sh_put_ptr(sh_hash_p hash, int64_t key);  // reserve new slot, returns ptr to value
int* sh_get_ptr(sh_hash_p hash, int64_t key);  // returns ptr to value or NULL if not in hash

sh_it_p sh_start(sh_hash_p hash);  // iterator
sh_it_p sh_next(sh_hash_p hash, sh_it_p it);  // it of next set elem
void    sh_remove(sh_hash_p hash, sh_it_p it);  // removes current elem from hash (during iteration!)


**/

/*

prefix        sh_                           dict_
key_type      int64_t                       const char*
value_type    int                           void*
hash_expr     sh_murmur3_32(&k, sizeof(k))  sh_murmur3_32(k, strlen(k))
key_cmp_expr  (a == b)                      strcmp(a, b) == 0
key_add_expr  k                             strdup(k)
key_del_expr  0                             (free(k), NULL)

SH_GEN_DECL("dict_", const char*, void*)
SH_GEN_DEF("dict_", const char*, void*, sh_murmur3_32(k, strlen(k)), strcmp(a, b) == 0, strdup(k), (free(k), NULL))
SH_GEN_DICT_DEF("dict_", void*)
SH_GEN_HASH_DEF("con_list_", int32_t, void*)

*/

// example hash(int64_t → int)

//
// Data
//

typedef struct {
	uint32_t hash_and_flags;
	int64_t key;
	int value;
} sh_slot_t, *sh_slot_p, *sh_it_p;

typedef struct {
	uint32_t length, capacity, deleted;
	sh_slot_p slots;
} sh_hash_t, *sh_hash_p;

#define SH_SLOT_FREE     0x00000000
#define SH_SLOT_DELETED  0x00000001


//
// API
//

void sh_new(sh_hash_p hash);
void sh_destroy(sh_hash_p hash);
void sh_optimize(sh_hash_p hash);

void sh_put(sh_hash_p hash, int64_t key, int value);
int  sh_get(sh_hash_p hash, int64_t key, int default_value);
bool sh_del(sh_hash_p hash, int64_t key);
bool sh_contains(sh_hash_p hash, int64_t key);

int* sh_put_ptr(sh_hash_p hash, int64_t key);
int* sh_get_ptr(sh_hash_p hash, int64_t key);

sh_it_p sh_start(sh_hash_p hash);
sh_it_p sh_next(sh_hash_p hash, sh_it_p it);
void    sh_remove(sh_hash_p hash, sh_it_p it);

#endif // SLIM_HASH_HEADER



#ifdef SLIM_HASH_IMPLEMENTATION


/**
 * Get 32-bit Murmur3 hash of a memory block. Taken from
 * https://github.com/wolkykim/qlibc/blob/master/src/utilities/qhash.c
 *
 * MurmurHash3 was created by Austin Appleby  in 2008. The initial implementation
 * was published in C++ and placed in the public: https://sites.google.com/site/murmurhash/
 * 
 * Seungyoung Kim has ported its implementation into C language in 2012 and
 * published it as a part of qLibc component.
 */
uint32_t sh_murmur3_32(const void *data, size_t size) {
    if (data == NULL || size == 0)
        return 0;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const int nblocks = size / 4;
    const uint32_t *blocks = (const uint32_t *) (data);
    const uint8_t *tail = (const uint8_t *) (data + (nblocks * 4));

    uint32_t h = 0;

    int i;
    uint32_t k;
    for (i = 0; i < nblocks; i++) {
        k = blocks[i];

        k *= c1;
        k = (k << 15) | (k >> (32 - 15));
        k *= c2;

        h ^= k;
        h = (h << 13) | (h >> (32 - 13));
        h = (h * 5) + 0xe6546b64;
    }

    k = 0;
    switch (size & 3) {
        case 3:
            k ^= tail[2] << 16;
        case 2:
            k ^= tail[1] << 8;
        case 1:
            k ^= tail[0];
            k *= c1;
            k = (k << 15) | (k >> (32 - 15));
            k *= c2;
            h ^= k;
    };

    h ^= size;

    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}


bool sh_resize(sh_hash_p hashmap, size_t new_capacity) {
	// on filling empty slot: if exceeding load factor, double capacity
	// on deleting slot: if to empty, half capacity
	
	// Can't make hashmap smaller than it needs to be
	if (new_capacity < hashmap->length)
		return false;
	
	sh_hash_t new_hashmap;
	new_hashmap.length = 0;
	new_hashmap.capacity = new_capacity;
	new_hashmap.deleted = 0;
	new_hashmap.slots = calloc(new_hashmap.capacity, sizeof(new_hashmap.slots[0]));
	
	// Failed to allocate memory for new hash map, leave the original untouched
	if (new_hashmap.slots == NULL)
		return false;
	
	for(sh_it_p it = sh_start(hashmap); it != NULL; it = sh_next(hashmap, it)) {
		sh_put(&new_hashmap, it->key, it->value);
	}
	
	free(hashmap->slots);
	*hashmap = new_hashmap;
	return true;
}

void sh_new(sh_hash_p hashmap) {
	hashmap->length = 0;
	hashmap->capacity = 0;
	hashmap->deleted = 0;
	hashmap->slots = NULL;
	sh_resize(hashmap, 8);
}

void sh_destroy(sh_hash_p hashmap) {
	hashmap->length = 0;
	hashmap->capacity = 0;
	hashmap->deleted = 0;
	free(hashmap->slots);
	hashmap->slots = NULL;
}


int* sh_put_ptr(sh_hash_p hashmap, int64_t key) {
	// add the +1 to the capacity doubling to avoid beeing stuck on a capacity of 0
	if (hashmap->length + hashmap->deleted + 1 > hashmap->capacity * 0.5)
		sh_resize(hashmap, (hashmap->capacity + 1) * 2);
	
	uint32_t hash = sh_murmur3_32(&key, sizeof(key)) | 0x80000000;
	size_t index = hash % hashmap->capacity;
	while ( !(hashmap->slots[index].hash_and_flags == SH_SLOT_FREE || hashmap->slots[index].hash_and_flags == SH_SLOT_DELETED) ) {
		index = (index + 1) % hashmap->capacity;
	}
	
	if (hashmap->slots[index].hash_and_flags == SH_SLOT_DELETED)
		hashmap->deleted--;
	hashmap->length++;
	hashmap->slots[index].hash_and_flags = hash;
	hashmap->slots[index].key = key;
	return &hashmap->slots[index].value;
}

int* sh_get_ptr(sh_hash_p hashmap, int64_t key) {
	uint32_t hash = sh_murmur3_32(&key, sizeof(key)) | 0x80000000;
	size_t index = hash % hashmap->capacity;
	while ( !(hashmap->slots[index].hash_and_flags == SH_SLOT_FREE) ) {
		if (hashmap->slots[index].hash_and_flags == hash && hashmap->slots[index].key == key)
			return &hashmap->slots[index].value;
		
		index = (index + 1) % hashmap->capacity;
	}
	
	return NULL;
}

bool sh_del(sh_hash_p hashmap, int64_t key) {
	uint32_t hash = sh_murmur3_32(&key, sizeof(key)) | 0x80000000;
	size_t index = hash % hashmap->capacity;
	while ( !(hashmap->slots[index].hash_and_flags == SH_SLOT_FREE) ) {
		if (hashmap->slots[index].hash_and_flags == hash && hashmap->slots[index].key == key) {
			hashmap->slots[index].hash_and_flags = SH_SLOT_DELETED;
			hashmap->length--;
			hashmap->deleted++;
			
			if (hashmap->length < hashmap->capacity * 0.2)
				sh_resize(hashmap, hashmap->capacity / 2);
			
			return true;
		}
		
		index = (index + 1) % hashmap->capacity;
	}
	
	return false;
}

void sh_put(sh_hash_p hashmap, int64_t key, int value) {
	*sh_put_ptr(hashmap, key) = value;
}

int sh_get(sh_hash_p hashmap, int64_t key, int default_value) {
	int* value_ptr = sh_get_ptr(hashmap, key);
	return (value_ptr) ? *value_ptr : default_value;
}

bool sh_contains(sh_hash_p hashmap, int64_t key) {
	return (sh_get_ptr(hashmap, key) != NULL);
}

// Search for the first not-empty slot
sh_it_p sh_start(sh_hash_p hashmap) {
	// We need to start at an invalid slot address since sh_next() increments it
	// before it looks at it (so it's safe).
	return sh_next(hashmap, hashmap->slots - 1);
}

sh_it_p sh_next(sh_hash_p hashmap, sh_it_p it) {
	if (it == NULL)
		return NULL;
	
	do {
		it++;
		// Check if we're past the last slot
		if (it - hashmap->slots >= hashmap->capacity)
			return NULL;
	} while( it->hash_and_flags == SH_SLOT_FREE || it->hash_and_flags == SH_SLOT_DELETED );
	
	return it;
}

void sh_remove(sh_hash_p hashmap, sh_it_p it) {
	if (it != NULL && it >= hashmap->slots && it - hashmap->slots < hashmap->capacity) {
		it->hash_and_flags = SH_SLOT_DELETED;
		
		hashmap->length--;
		hashmap->deleted++;
	}
}

void sh_optimize(sh_hash_p hashmap) {
	sh_resize(hashmap, hashmap->capacity);
}

#endif // SLIM_HASH_IMPLEMENTATION