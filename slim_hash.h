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

prefix      sh_                        dict_
key_type      int64_t                      const char*
value_type  int                        void*
hash_expr    sh_murmur3_32(&k, sizeof(k))  sh_murmur3_32(k, strlen(k))
key_cmp_expr  (a == b)                    strcmp(a, b) == 0
key_put_expr  k                          strdup(k)
key_del_expr  0                          (free(k), NULL)

SH_GEN_DECL(dict, const char*, void*)
SH_GEN_DEF(dict, const char*, void*, sh_murmur3_32(k, strlen(k)), strcmp(a, b) == 0, strdup(k), (free(k), NULL))
SH_GEN_DICT_DEF(dict, void*)
SH_GEN_HASH_DEF(con_list, int32_t, void*)


*/

// example hash(int64_t → int)

//
// Data
//

#define SH_SLOT_FREE     0x00000000
#define SH_SLOT_DELETED  0x00000001
#define SH_SLOT_FILLED   0x80000000

#define SH_GEN_DECL(prefix, key_t, value_t)                                     \
    typedef struct {                                                            \
        uint32_t hash_and_flags;                                                \
        key_t key;                                                              \
        value_t value;                                                          \
    } prefix##_slot_t, *prefix##_slot_p, *prefix##_it_p;                        \
                                                                                \
    typedef struct {                                                            \
        uint32_t length, capacity, deleted;                                     \
        prefix##_slot_p slots;                                                  \
    } prefix##_t, *prefix##_p;                                                  \
                                                                                \
    void     prefix##_new(prefix##_p hash);                                     \
    void     prefix##_destroy(prefix##_p hash);                                 \
    void     prefix##_optimize(prefix##_p hash);                                \
                                                                                \
    void     prefix##_put(prefix##_p hash, key_t key, value_t value);           \
    value_t  prefix##_get(prefix##_p hash, key_t key, value_t default_value);   \
    bool     prefix##_del(prefix##_p hash, key_t key);                          \
    bool     prefix##_contains(prefix##_p hash, key_t key);                     \
                                                                                \
    value_t* prefix##_put_ptr(prefix##_p hash, key_t key);                      \
    value_t* prefix##_get_ptr(prefix##_p hash, key_t key);                      \
                                                                                \
    prefix##_it_p  prefix##_start(prefix##_p hash);                             \
    prefix##_it_p  prefix##_next(prefix##_p hash, prefix##_it_p it);            \
    void           prefix##_remove(prefix##_p hash, prefix##_it_p it);          \



#endif // SLIM_HASH_HEADER



#ifdef SLIM_HASH_IMPLEMENTATION

#define SH_GEN_DEF(prefix, key_t, value_t, hash_expr, key_cmp_expr, key_put_expr, key_del_expr)  \
    /**                                                                          \
     * Get 32-bit Murmur3 hash of a memory block. Taken from                     \
     * https://github.com/wolkykim/qlibc/blob/master/src/utilities/qhash.c       \
     *                                                                           \
     * MurmurHash3 was created by Austin Appleby  in 2008. The initial           \
     * implementation was published in C++ and placed in the public:             \
     * https://sites.google.com/site/murmurhash/                                 \
     *                                                                           \
     * Seungyoung Kim has ported its implementation into C language in 2012 and  \
     * published it as a part of qLibc component.                                \
     **/                                                                         \
    uint32_t prefix##_murmur3_32(const void *data, size_t size) {                \
        if (data == NULL || size == 0)                                           \
            return 0;                                                            \
                                                                                 \
        const uint32_t c1 = 0xcc9e2d51;                                          \
        const uint32_t c2 = 0x1b873593;                                          \
                                                                                 \
        const int nblocks = size / 4;                                            \
        const uint32_t *blocks = (const uint32_t *) (data);                      \
        const uint8_t *tail = (const uint8_t *) (data + (nblocks * 4));          \
                                                                                 \
        uint32_t h = 0;                                                          \
                                                                                 \
        int i;                                                                   \
        uint32_t k;                                                              \
        for (i = 0; i < nblocks; i++) {                                          \
            k = blocks[i];                                                       \
                                                                                 \
            k *= c1;                                                             \
            k = (k << 15) | (k >> (32 - 15));                                    \
            k *= c2;                                                             \
                                                                                 \
            h ^= k;                                                              \
            h = (h << 13) | (h >> (32 - 13));                                    \
            h = (h * 5) + 0xe6546b64;                                            \
        }                                                                        \
                                                                                 \
        k = 0;                                                                   \
        switch (size & 3) {                                                      \
            case 3:                                                              \
                k ^= tail[2] << 16;                                              \
            case 2:                                                              \
                k ^= tail[1] << 8;                                               \
            case 1:                                                              \
                k ^= tail[0];                                                    \
                k *= c1;                                                         \
                k = (k << 15) | (k >> (32 - 15));                                \
                k *= c2;                                                         \
                h ^= k;                                                          \
        };                                                                       \
                                                                                 \
        h ^= size;                                                               \
                                                                                 \
        h ^= h >> 16;                                                            \
        h *= 0x85ebca6b;                                                         \
        h ^= h >> 13;                                                            \
        h *= 0xc2b2ae35;                                                         \
        h ^= h >> 16;                                                            \
                                                                                 \
        return h;                                                                \
    }                                                                            \
                                                                                 \
    bool prefix##_resize(prefix##_p hashmap, uint32_t new_capacity) {                                   \
        /* on filling empty slot: if exceeding load factor, double capacity                             \
        // on deleting slot: if to empty, half capacity                                                 \
                                                                                                        \
        // Can't make hashmap smaller than it needs to be  */                                           \
        if (new_capacity < hashmap->length)                                                             \
            return false;                                                                               \
                                                                                                        \
        prefix##_t new_hashmap;                                                                         \
        new_hashmap.length = 0;                                                                         \
        new_hashmap.capacity = new_capacity;                                                            \
        new_hashmap.deleted = 0;                                                                        \
        new_hashmap.slots = calloc(new_hashmap.capacity, sizeof(new_hashmap.slots[0]));                 \
                                                                                                        \
        /* Failed to allocate memory for new hash map, leave the original untouched */                  \
        if (new_hashmap.slots == NULL)                                                                  \
            return false;                                                                               \
                                                                                                        \
        for(prefix##_it_p it = prefix##_start(hashmap); it != NULL; it = prefix##_next(hashmap, it)) {  \
            prefix##_put(&new_hashmap, it->key, it->value);                                             \
        }                                                                                               \
                                                                                                        \
        free(hashmap->slots);                                                                           \
        *hashmap = new_hashmap;                                                                         \
        return true;                                                                                    \
    }                                                                                                   \
                                                                                                        \
    void prefix##_new(prefix##_p hashmap) {      \
        hashmap->length = 0;                     \
        hashmap->capacity = 0;                   \
        hashmap->deleted = 0;                    \
        hashmap->slots = NULL;                   \
        prefix##_resize(hashmap, 8);             \
    }                                            \
                                                 \
    void prefix##_destroy(prefix##_p hashmap) {  \
        hashmap->length = 0;                     \
        hashmap->capacity = 0;                   \
        hashmap->deleted = 0;                    \
        free(hashmap->slots);                    \
        hashmap->slots = NULL;                   \
    }                                            \
                                                 \
    value_t* prefix##_put_ptr(prefix##_p hashmap, key_t key) {                                                                                                                          \
        /* add the +1 to the capacity doubling to avoid beeing stuck on a capacity of 0 */                                                                                              \
        if (hashmap->length + hashmap->deleted + 1 > hashmap->capacity * 0.5)                                                                                                           \
            prefix##_resize(hashmap, (hashmap->capacity + 1) * 2);                                                                                                                      \
                                                                                                                                                                                        \
        uint32_t hash = (hash_expr) | SH_SLOT_FILLED;                                                                                                                                   \
        size_t index = hash % hashmap->capacity;                                                                                                                                        \
        while ( !(hashmap->slots[index].hash_and_flags == hash || hashmap->slots[index].hash_and_flags == SH_SLOT_FREE || hashmap->slots[index].hash_and_flags == SH_SLOT_DELETED) ) {  \
            index = (index + 1) % hashmap->capacity;                                                                                                                                    \
        }                                                                                                                                                                               \
                                                                                                                                                                                        \
        if (hashmap->slots[index].hash_and_flags == SH_SLOT_DELETED)                                                                                                                    \
            hashmap->deleted--;                                                                                                                                                         \
        hashmap->length++;                                                                                                                                                              \
        hashmap->slots[index].hash_and_flags = hash;                                                                                                                                    \
        hashmap->slots[index].key = (key_put_expr);                                                                                                                                     \
        return &hashmap->slots[index].value;                                                                                                                                            \
    }                                                                                                                                                                                   \
                                                                                                                                                                                        \
    value_t* prefix##_get_ptr(prefix##_p hashmap, key_t key) {               \
        uint32_t hash = (hash_expr) | SH_SLOT_FILLED;                        \
        size_t index = hash % hashmap->capacity;                             \
        while ( !(hashmap->slots[index].hash_and_flags == SH_SLOT_FREE) ) {  \
            if (hashmap->slots[index].hash_and_flags == hash) {              \
                key_t a = hashmap->slots[index].key;                         \
                key_t b = key;                                               \
                if (key_cmp_expr)                                            \
                    return &hashmap->slots[index].value;                     \
            }                                                                \
                                                                             \
            index = (index + 1) % hashmap->capacity;                         \
        }                                                                    \
                                                                             \
        return NULL;                                                         \
    }                                                                        \
                                                                             \
    bool prefix##_del(prefix##_p hashmap, key_t key) {                       \
        uint32_t hash = (hash_expr) | SH_SLOT_FILLED;                        \
        size_t index = hash % hashmap->capacity;                             \
        while ( !(hashmap->slots[index].hash_and_flags == SH_SLOT_FREE) ) {  \
            if (hashmap->slots[index].hash_and_flags == hash) {              \
                key_t a = hashmap->slots[index].key;                         \
                key_t b = key;                                               \
                if (key_cmp_expr) {                                          \
                    key_t key = hashmap->slots[index].key;                   \
                    key = key; /* avoid unused variable warning */           \
                    hashmap->slots[index].key = (key_del_expr);              \
                    hashmap->slots[index].hash_and_flags = SH_SLOT_DELETED;  \
                    hashmap->length--;                                       \
                    hashmap->deleted++;                                      \
                                                                             \
                    if (hashmap->length < hashmap->capacity * 0.2)           \
                        prefix##_resize(hashmap, hashmap->capacity / 2);     \
                                                                             \
                    return true;                                             \
                }                                                            \
            }                                                                \
                                                                             \
            index = (index + 1) % hashmap->capacity;                         \
        }                                                                    \
                                                                             \
        return false;                                                        \
    }                                                                        \
                                                                             \
    void prefix##_put(prefix##_p hashmap, key_t key, value_t value) {             \
        *prefix##_put_ptr(hashmap, key) = value;                                  \
    }                                                                             \
                                                                                  \
    value_t prefix##_get(prefix##_p hashmap, key_t key, value_t default_value) {  \
        value_t* value_ptr = prefix##_get_ptr(hashmap, key);                      \
        return (value_ptr) ? *value_ptr : default_value;                          \
    }                                                                             \
                                                                                  \
    bool prefix##_contains(prefix##_p hashmap, key_t key) {                       \
        return (prefix##_get_ptr(hashmap, key) != NULL);                          \
    }                                                                             \
                                                                                  \
    /* Search for the first not-empty slot */                                                    \
    prefix##_it_p prefix##_start(prefix##_p hashmap) {                                           \
        /* We need to start at an invalid slot address since sh_next() increments it             \
        // before it looks at it (so it's safe). */                                              \
        return prefix##_next(hashmap, hashmap->slots - 1);                                       \
    }                                                                                            \
                                                                                                 \
    prefix##_it_p prefix##_next(prefix##_p hashmap, prefix##_it_p it) {                          \
        if (it == NULL)                                                                          \
            return NULL;                                                                         \
                                                                                                 \
        do {                                                                                     \
            it++;                                                                                \
            /* Check if we're past the last slot */                                              \
            if (it - hashmap->slots >= hashmap->capacity)                                        \
                return NULL;                                                                     \
        } while( it->hash_and_flags == SH_SLOT_FREE || it->hash_and_flags == SH_SLOT_DELETED );  \
                                                                                                 \
        return it;                                                                               \
    }                                                                                            \
                                                                                                 \
    void prefix##_remove(prefix##_p hashmap, prefix##_it_p it) {                                 \
        if (it != NULL && it >= hashmap->slots && it - hashmap->slots < hashmap->capacity) {     \
            key_t key = it->key;                                                                 \
            key = key; /* avoid unused variable warning */                                       \
            key = key; /* avoid unused variable warning */                                       \
            it->key = (key_del_expr);                                                            \
            it->hash_and_flags = SH_SLOT_DELETED;                                                \
                                                                                                 \
            hashmap->length--;                                                                   \
            hashmap->deleted++;                                                                  \
        }                                                                                        \
    }                                                                                            \
                                                                                                 \
    void prefix##_optimize(prefix##_p hashmap) {      \
        prefix##_resize(hashmap, hashmap->capacity);  \
    }                                                 \


#define SH_GEN_HASH_DEF(prefix, key_t, value_t)  \
             SH_GEN_DEF(prefix, key_t, value_t, prefix##_murmur3_32(&key, sizeof(key)), (a == b), key, 0)
#define SH_GEN_DICT_DEF(prefix, key_t, value_t)  \
             SH_GEN_DEF(prefix, key_t, value_t, prefix##_murmur3_32(key, strlen(key)), (strcmp(a, b) == 0), strdup(key), (free((void*)key), NULL))


#endif // SLIM_HASH_IMPLEMENTATION