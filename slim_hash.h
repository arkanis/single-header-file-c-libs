/**

Slim Hash v1.0
By Stephan Soller <stephan.soller@helionweb.de>
Licensed under the MIT license

Slim Hash is a simple hash table implementation for C99. It's designed to be
simple to use and avoid surprises. To keep it typesafe you have to generate code
for each new hash type you want.

It's an stb style single header file library. Define SLIM_HASH_IMPLEMENTATION
before you include this file in *one* C file to create the common code that is
used by all hash tables.

A hash type itself is declared using the SH_GEN_DECL() macro (usually in a
header file). Then in some C file the implementation is defined using the
SH_GEN_HASH_DEF(), SH_GEN_DICT_DEF() or SH_GEN_DEF() macro.


SIMPLE EXAMPLE USAGE

    #define SLIM_HASH_IMPLEMENTATION
    #include "slim_hash.h"
    
    SH_GEN_DECL(env, char*, int);
    SH_GEN_DICT_DEF(env, char*, int);
    
    void main() {
        env_t env;
        env_new(&env);
        
        env_put(&env, "foo", 3);
        env_put(&env, "bar", 17);
        
        env_get(&env, "foo", -1);  // => 3
        env_get(&env, "bar", -1);  // => 17
        env_get(&env, "hurdl", -1);  // => -1
        
        env_put(&env, "foo", 5);
        env_get(&env, "foo", -1);  // => 5
        
        int* value_ptr = NULL;
        // env_get_ptr() returns ptr to value or NULL if not in hash
        // Pointer only stays valid as long as the hash isn't manipulated!
        value_ptr = env_get_ptr(&env, "foo");  // *value_ptr => 5
        value_ptr = env_get_ptr(&env, "hurdl");  // value_ptr => NULL
        
        // env_put_ptr() reserves new slot, returns ptr to value (useful for struct values)
        // Pointer only stays valid as long as the hash isn't manipulated!
        value_ptr = env_put_ptr(&env, "grumpf");
        *value_ptr = 21;
        env_get(&env, "grumpf", -1);  // => 21
        *value_ptr = 42;
        env_get(&env, "grumpf", -1);  // => 42
        
        env_contains(&env, "bar"); // => true
        env_del(&env, "bar");  // => true (true if value was deleted, false if not found)
        env_contains(&env, "bar"); // => false
        
        // This output all slots in undefined order:
        // foo: 5
        // grumpf: 42
        for(env_it_p it = env_start(&env); it != NULL; it = env_next(&env, it)) {
            printf("%s: %d\n", it->key, it->value);
            // You can remove a slot during iteration with
            // env_remove(&env, it);
        }
        
        env_destroy(&env);
    }


THE PUBLIC API

SH_GEN_DECL(prefix, key_t, value_t) generates:

    typedef struct {
        uint32_t length, capacity;
        // Some internal fields ...
    } prefix_t, *prefix_p;
    
    typedef struct {
        // Some internal fields ...
        key_t key;
        value_t value;
    } *prefix_it_p;
    
    void     prefix_new(prefix_p hash);
    void     prefix_destroy(prefix_p hash);
    void     prefix_optimize(prefix_p hash);
    
    void     prefix_put(prefix_p hash, key_t key, value_t value);
    value_t  prefix_get(prefix_p hash, key_t key, value_t default_value);
    bool     prefix_del(prefix_p hash, key_t key);
    bool     prefix_contains(prefix_p hash, key_t key);
    
    value_t* prefix_put_ptr(prefix_p hash, key_t key);
    value_t* prefix_get_ptr(prefix_p hash, key_t key);
    
    prefix_it_p prefix_start(prefix_p hash);
    prefix_it_p prefix_next(prefix_p hash, prefix_it_p it);
    void        prefix_remove(prefix_p hash, prefix_it_p it);


HOW HASHES AND DICTS ARE GENERATED WITH SH_GEN_DEF()

You can use SH_GEN_DEF() to define hashes with more complex keys. As examples
here the way SH_GEN_HASH_DEF() uses SH_GEN_DEF() to define hashes for value
types and SH_GEN_DICT_DEF() for zero-terminated strings.

              SH_GEN_HASH_DEF                SH_GEN_DICT_DEF
prefix        prefix                         prefix
key_type      key_t                          const char* or char*
value_type    value_t                        value_t
hash_expr     sh_murmur3_32(&k, sizeof(k))   sh_murmur3_32(k, strlen(k))
key_cmp_expr  (a == b)                       strcmp(a, b) == 0
key_put_expr  k                              sh_strdup(k)
key_del_expr  0                              (free(k), NULL)


VERSION HISTORY

v1.0  2016-06-22  Initial release

**/
#ifndef SLIM_HASH_HEADER
#define SLIM_HASH_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


// Flags to mark slots
#define SH_SLOT_FREE     0x00000000  // Choosen so a calloc()ed hash is empty
#define SH_SLOT_DELETED  0x00000001
#define SH_SLOT_FILLED   0x80000000  // If this bit is set hash_and_flags contains a hash

/**
 * This macro declares a hash. It doesn't generate the implementation, just the
 * types and function prototypes. Use the SH_GEN_HASH_DEF(), SH_GEN_DICT_DEF()
 * or SH_GEN_DEF() macro to generate the implementation once.
 */
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


/**
 * Macro to generate the definitions (implementation) of an hash. You need to
 * generate the declarations first with SH_GEN_DECL().
 */
#define SH_GEN_DEF(prefix, key_t, value_t, hash_expr, key_cmp_expr, key_put_expr, key_del_expr)  \
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
    value_t* prefix##_put_ptr(prefix##_p hashmap, key_t key) {                                                                          \
        /* add the +1 to the capacity doubling to avoid beeing stuck on a capacity of 0 */                                              \
        if (hashmap->length + hashmap->deleted + 1 > hashmap->capacity * 0.5)                                                           \
            prefix##_resize(hashmap, (hashmap->capacity + 1) * 2);                                                                      \
                                                                                                                                        \
        uint32_t hash = (hash_expr) | SH_SLOT_FILLED;                                                                                   \
        size_t index = hash % hashmap->capacity;                                                                                        \
        while ( !(hashmap->slots[index].hash_and_flags == SH_SLOT_FREE || hashmap->slots[index].hash_and_flags == SH_SLOT_DELETED) ) {  \
            if (hashmap->slots[index].hash_and_flags == hash) {                                                                         \
                key_t a = hashmap->slots[index].key;                                                                                    \
                key_t b = key;                                                                                                          \
                if (key_cmp_expr)                                                                                                       \
                    break;                                                                                                              \
            }                                                                                                                           \
            index = (index + 1) % hashmap->capacity;                                                                                    \
        }                                                                                                                               \
                                                                                                                                        \
        if (hashmap->slots[index].hash_and_flags == SH_SLOT_DELETED)                                                                    \
            hashmap->deleted--;                                                                                                         \
        hashmap->length++;                                                                                                              \
        hashmap->slots[index].hash_and_flags = hash;                                                                                    \
        hashmap->slots[index].key = (key_put_expr);                                                                                     \
        return &hashmap->slots[index].value;                                                                                            \
    }                                                                                                                                   \
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


//
// Shorthand macros to define hashes (keys are byte blocks like ints, floats,
// structs, etc.) and dictionaries (keys are zero-terminated strings).
//

#define SH_GEN_HASH_DEF(prefix, key_t, value_t)  \
             SH_GEN_DEF(prefix, key_t, value_t, sh_murmur3_32(&key, sizeof(key)), (a == b), key, 0)
#define SH_GEN_DICT_DEF(prefix, key_t, value_t)  \
             SH_GEN_DEF(prefix, key_t, value_t, sh_murmur3_32(key, strlen(key)), (strcmp(a, b) == 0), sh_strdup(key), (free((void*)key), NULL))

#endif // SLIM_HASH_HEADER


#ifdef SLIM_HASH_IMPLEMENTATION

/**
 * Get 32-bit Murmur3 hash of a memory block. Taken from
 * https://github.com/wolkykim/qlibc/blob/master/src/utilities/qhash.c
 * 
 * MurmurHash3 was created by Austin Appleby  in 2008. The initial
 * implementation was published in C++ and placed in the public:
 * https://sites.google.com/site/murmurhash/
 * 
 * Seungyoung Kim has ported its implementation into C language in 2012 and
 * published it as a part of qLibc component.
 **/
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

/**
 * Portable version of strdup that doesn't require feature test macros. But if
 * got a POSIX strdup use it.
 */
#if _SVID_SOURCE || _BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED || _POSIX_C_SOURCE >= 200809L
#define sh_strdup strdup
#else
char *sh_strdup(const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (d == NULL)
        return NULL;
    strcpy(d, s);
    return d;
}
#endif

#endif // SLIM_HASH_IMPLEMENTATION