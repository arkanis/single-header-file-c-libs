/**

Slim Hash v1.1
By Stephan Soller <stephan.soller@helionweb.de>
Licensed under the MIT license

Slim Hash is a simple "drop it in and it just works" hashmap implementation for C99. It's designed
to be simple to use and to avoid surprises. To keep it typesafe you have to generate code for each
new type of hashmap you want. 

It uses closed hashing, meaning that all data is stored in one contiguous memory block. In case of
collisions linear probing is used as it's more cache friendly and there is no need to calculate
prime numbers for the hashmap size.


EXAMPLE

    #define SLIM_HASH_IMPLEMENTATION
    #include "slim_hash.h"
    
    // Generates the types (struct dict and struct dict_slot) and function prototypes for a
    // char* -> int hashmap. All functions of the hashmap will start with "dict_".
    // This could also go into a separate header file.
    SH_GEN_DECL(dict, char*, int);
    
    // Generates the actual implementaion of all the dict_... functions. This could also go into a
    // separate C file.
    SH_GEN_DICT_IMPL(dict, char*, int);
    
    void main() {
        struct dict dict;
        dict_new(&dict);
        
        dict_put(&dict, "foo", 3);
        dict_put(&dict, "bar", 17);
        
        dict_get(&dict, "foo", -1);    // => 3
        dict_get(&dict, "bar", -1);    // => 17
        dict_get(&dict, "hurdl", -1);  // => -1
        
        dict_put(&dict, "foo", 5);
        dict_get(&dict, "foo", -1);  // => 5
        
        int* value_ptr = NULL;
        // dict_get_ptr() returns a pointer to the value or NULL if not in hash
        // Pointer only stays valid as long as the hash isn't manipulated!
        value_ptr = dict_get_ptr(&dict, "foo");    // *value_ptr => 5
        value_ptr = dict_get_ptr(&dict, "hurdl");  // value_ptr => NULL
        
        // dict_put_ptr() reserves new slot, returns ptr to value (useful for struct values)
        // Pointer only stays valid as long as items are not added or deleted from the hashmap!
        value_ptr = dict_put_ptr(&dict, "grumpf");
        *value_ptr = 21;
        dict_get(&dict, "grumpf", -1);  // => 21
        *value_ptr = 42;
        dict_get(&dict, "grumpf", -1);  // => 42
        
        dict_contains(&dict, "bar"); // => true
        dict_del(&dict, "bar");      // => true (true if value was deleted, false if not found)
        dict_contains(&dict, "bar"); // => false
        
        // This outputs all items (filled slots) of the hashmap in undefined order:
        // foo: 5
        // grumpf: 42
        for(struct dict_slot* it = dict_start(&dict); it != NULL; it = dict_next(&dict, it)) {
            printf("%s: %d\n", it->key, it->value);
            // You can remove a slot (clear the key and value inside it) during iteration with
            // dict_remove(&dict, it);
        }
        
        dict_destroy(&dict);
    }


USAGE

In *one* of your C files define SLIM_HASH_IMPLEMENTATION before you include this library to create
the common code that is used by all hashmaps (that are acutally the murmur3 and fnv1a hash
functions). All other C files that include this library will then just reference these functions.

Creating a hashmap with this library happens in 2 steps:

- First, declare the hashmap with the SH_GEN_DECL() macro.
- Second, generate the implementation of the hashmap with the SH_GEN_HASH_IMPL(), SH_GEN_DICT_IMPL()
  or SH_GEN_IMPL() macros.

You can put the declaration in a header file or simply declare the same hashmap in multiple places.
But the implementation should be generated in only one file (otherwiese you would have multiple
implementation for the same functions). After that the hashmap is ready and can be used via the
generated functions.

SH_GEN_DECL() will define a struct for the hashmap and for an iterator. struct dict and
struct dict_slot in the example above. By default it will also typedef these to dict_t, dict_p and
dict_it_p (it for iterator and _p because it's a pointer type). If you don't want that or if you're
concerned about possible conflicts with POSIX types you can define SLIM_HASH_NO_TYPEDEFS before
including the library. In that case no typedefs will be generated.

For most cases you can use the SH_GEN_HASH_IMPL() or SH_GEN_DICT_IMPL() macros to generate the
implementation. They take the same 3 paramters as SH_GEN_DECL() but generate the actual functions
instead of just the prototypes.

SH_GEN_DICT_IMPL() generates code for a hashmap that uses string keys. It'll use strlen() and
sh_murmur3() to hash the keys. strcmp() is used to compare keys and strdup() to duplicate the keys
when you insert a new key.

SH_GEN_HASH_IMPL() on the other hand generates code that simply hashes any value type with murmur3.
it uses the == operator to compare keys. This works for key types like int, float, etc.

SH_GEN_IMPL() is a all purpose most flexible way to generate a hash implementation. But it requires
some code snippets (expressions) to do so. These snippets specify how to calculate the hash, what to
do with keys, how to allocate and free memory, etc. So you can use it to generate all kinds of
hashmaps, for example one that uses structs as keys.


    struct grid_cell { ... };
    struct grid_id { int x, y; };
    
    SH_GEN_DECL(grid, struct grid_id, struct grid);
    SH_GEN_IMPL(grid, struct grid_id, struct grid,
        sh_murmur3(&key, sizeof(key), 0),  // key_hash_expr(key_t key)
        (memcmp(&a, &b, sizeof(a)) == 0),  // key_cmp_expr(key_t a, key_t b)
        key,                               // key_put_expr(key_t key)
        memset(&key, 0, sizeof(key)),      // key_del_expr(key_t key)
        calloc(capacity, slot_size),       // calloc_expr(size_t capacity, size_t slot_size)
        free(ptr)                          // free_expr(void* ptr)
    )

The difference to SH_GEN_HASH_IMPL() is that memcpy() instead of the == operator is used to compare
two keys. Also when a key is deleted it is zeroed out with memset(). This should work for all kinds
of structs.

See the SH_GEN_IMPL() documentation below for more details on the different expressions.
SH_GEN_HASH_IMPL() and SH_GEN_DICT_IMPL() are simple shorthands for that macro with code snippes for
the two most common kinds of hashmaps.


THE PUBLIC API

This is the API declared by SH_GEN_DECL(dict, char*, int). Hashmaps with a different name will of
course have different type names and function prefixes.

    struct dict {
        uint32_t length, capacity;
        // Some internal fields
    };
    struct dict_slot {
        // Some internal fields
        char* key;
        int   value;
    };
    
    // You can disable these two by defining SLIM_HASH_NO_TYPEDEFS
    typedef struct dict dict_t, *dict_p;
    typedef struct dict_slot *dict_it_p;
    
    void  dict_new(struct dict* hashmap);
    void  dict_destroy(struct dict* hashmap);
    
    int   dict_get(struct dict* hashmap, char* key, int default_value);
    void  dict_put(struct dict* hashmap, char* key, int value);
    bool  dict_del(struct dict* hashmap, char* key);
    bool  dict_contains(struct dict* hashmap, char* key);
    
    int*  dict_get_ptr(struct dict* hashmap, char* key);
    int*  dict_put_ptr(struct dict* hashmap, char* key);
    
    struct dict_slot*  dict_start(struct dict* hashmap);
    struct dict_slot*  dict_next(struct dict* hashmap, struct dict_slot* it);
    void               dict_remove(struct dict* hashmap, struct dict_slot* it);
    bool               dict_shrink_if_necessary(struct dict* hashmap);

See the function implementations in the SH_GEN_IMPL() macro for detailed documentation.


VERSION HISTORY

v1.0  2016-06-22  Initial release
v1.1  2017-07-12  ADD: Added the fnv1a 32 bit hash function (see sh_fnv1a() documentation).
                  ADD: Manually ported the murmur3 hash function from the original source. Also
                       added the seed parameter (again, see sh_murmur3() docs).
                  ADD: Added calloc and free expressions to SH_GEN_IMPL() so memory can be allocated
                       from a garbage collector.
                  CHANGE: Removed the ..._optimize() function because it didn't improve performance.
                  CHANGE: Cleaned up the code and added documentation.
                  CHANGE: The hashmap now uses power to two capacities.
                  CHANGE: Made typedefs optional. They can be removed by defining
                          SLIM_HASH_NO_TYPEDEFS before including the library.
                  FIX: Made sure that key_del_expr and free_expr of SH_GEN_IMPL() don't emit unused
                       variable warnings when they don't use the key argument.
                  FIX: key_del_expr is now also executed for each key when the hashmap is destroyed
                       (so each key that is duplicated is also freed).
                  FIX: key_put_expr is no longer executed when the hashmap is resized.
                  FIX: Renamed ..._DEF macros to ..._IMPL because it's easy to confuse declaration
                       and definition. IMPL (implementation) makes more clear what the macros
                       generate.
                  FIX: Added missing prototypes for functions shared by all implementations.

**/
#ifndef SLIM_HASH_HEADER
#define SLIM_HASH_HEADER

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


// Flags for hashtable slots
#define SH_SLOT_FREE     0x00000000  // Choosen so a calloc()ed hash is considered free
#define SH_SLOT_DELETED  0x00000001
#define SH_SLOT_FILLED   0x80000000  // If this bit is set hash_or_flags contains a hash

uint32_t sh_murmur3(const void* key, int size, uint32_t seed);
uint32_t sh_fnv1a(const char* key);

/**
 * This macro declares a hash. It doesn't generate the implementation, just the types and function
 * prototypes. You can use this macro at different places or use it once and include that whenever
 * you need it. Use one of the SH_GEN_HASH_IMPL(), SH_GEN_DICT_IMPL() or SH_GEN_IMPL() macros with
 * the same first three arguments to generate the implementation once.
 * 
 * If SLIM_HASH_NO_TYPEDEFS is defined no typedefs for shorthand types ending with ..._t and ..._p
 * are defined.
 */
#ifdef SLIM_HASH_NO_TYPEDEFS
    #define SH_GEN_DECL(name, key_t, value_t)  SH_GEN_TYPES_AND_PROTOTYPES(name, key_t, value_t)
#else
    #define SH_GEN_DECL(name, key_t, value_t)  SH_GEN_TYPES_AND_PROTOTYPES(name, key_t, value_t)   \
        typedef struct name name##_t, *name##_p;                                                   \
        typedef struct name##_slot *name##_it_p;                                                   
#endif

/**
 * This macro declares a hash. It doesn't generate the implementation, just the types and function
 * prototypes. You can use this macro at different places or use it once and include that whenever
 * you need it. Use one of the SH_GEN_HASH_IMPL(), SH_GEN_DICT_IMPL() or SH_GEN_IMPL() macros with
 * the same first three arguments to generate the implementation once.
 */
#define SH_GEN_TYPES_AND_PROTOTYPES(name, key_t, value_t)                                          \
    struct name##_slot {                                                                           \
        uint32_t hash_or_flags;                                                                    \
        key_t key;                                                                                 \
        value_t value;                                                                             \
    };                                                                                             \
                                                                                                   \
    struct name {                                                                                  \
        uint32_t length, capacity, deleted;                                                        \
        struct name##_slot* slots;                                                                 \
    };                                                                                             \
                                                                                                   \
    void     name##_new(struct name* hashmap);                                                     \
    void     name##_destroy(struct name* hashmap);                                                 \
                                                                                                   \
    value_t  name##_get(struct name* hashmap, key_t key, value_t default_value);                   \
    void     name##_put(struct name* hashmap, key_t key, value_t value);                           \
    bool     name##_del(struct name* hashmap, key_t key);                                          \
    bool     name##_contains(struct name* hashmap, key_t key);                                     \
                                                                                                   \
    value_t* name##_get_ptr(struct name* hashmap, key_t key);                                      \
    value_t* name##_put_ptr(struct name* hashmap, key_t key);                                      \
                                                                                                   \
    struct name##_slot*  name##_start(struct name* hashmap);                                       \
    struct name##_slot*  name##_next(struct name* hashmap, struct name##_slot* it);                \
    void                 name##_remove(struct name* hashmap, struct name##_slot* it);              \
    bool                 name##_shrink_if_necessary(struct name* hashmap);                         \

/**
 * Shorthand macro to generate the implementation of an hash that uses value types as keys. That are
 * types like int, float or even pointers (but in that case just the pointer, that is the memory
 * address itself not the data it points to).
 * 
 * All types that can be compared with the == operator and work with sizeof() can be used as keys.
 * 
 * You need to generate the declarations first with SH_GEN_DECL() using the same arguments as for
 * the implementation.
 */
#define SH_GEN_HASH_IMPL(name, key_t, value_t)                                                     \
    SH_GEN_IMPL(name, key_t, value_t,                                                              \
        sh_murmur3(&key, sizeof(key), 0),  /* key_hash_expr(key_t key)                       */    \
        (a == b),                          /* key_cmp_expr(key_t a, key_t b)                 */    \
        key,                               /* key_put_expr(key_t key)                        */    \
        0,                                 /* key_del_expr(key_t key)                        */    \
        calloc(capacity, slot_size),       /* calloc_expr(size_t capacity, size_t slot_size) */    \
        free(ptr)                          /* free_expr(void* ptr)                           */    \
    )

/**
 * Shorthand macro to generate the implementation of an hash that uses zero terminated strings as
 * keys (a dictionary). The string keys are duplicated internally and are freed when an item is
 * deleted or the dictionary destroyed.
 * 
 * You need to generate the declarations first with SH_GEN_DECL() using the same arguments as for
 * the implementation.
 */
#define SH_GEN_DICT_IMPL(name, key_t, value_t)                                                     \
    SH_GEN_IMPL(name, key_t, value_t,                                                              \
        sh_murmur3(key, strlen(key), 0),  /* key_hash_expr(key_t key)                       */     \
        (strcmp(a, b) == 0),              /* key_cmp_expr(key_t a, key_t b)                 */     \
        sh_strdup(key),                   /* key_put_expr(key_t key)                        */     \
        (free((void*)key), NULL),         /* key_del_expr(key_t key)                        */     \
        calloc(capacity, slot_size),      /* calloc_expr(size_t capacity, size_t slot_size) */     \
        free(ptr)                         /* free_expr(void* ptr)                           */     \
    )

/**
 * Macro to generate the implementation of an hash. This is the full features variant that needs a
 * few code snippets (expressions) to generate the finished hash. Depending on the hash you want
 * (value type keys, zero terminated string keys, structs as keys, etc.) you have to pass in
 * different expressions. This makes this macro very flexible but requires a bit more initial RTFM
 * than the SH_GEN_HASH_IMPL() or SH_GEN_DICT_IMPL(). See (TODO: insert ref to documentation header)
 * 
 * You need to generate the declarations first with SH_GEN_DECL().
 * 
 * The name, key_t, value_t arguments have to be the same as used with SH_GEN_DECL(). All following
 * arguments are expressions that are inserted into the generated functions. Each one defining a bit
 * of behaviour of the generated hash. Set an expression to NULL if you don't need it. Each
 * expression can use some variables from the generated code (here written like function
 * prototypes):
 * 
 * uint32_t key_hash_expr(key_t key)
 *     Executed whenever the hash of a key is needed.
 *     Examples: sh_murmur3(&key, sizeof(key), 0) or sh_murmur3(key, strlen(key), 0)
 * 
 * bool key_cmp_expr(key_t a, key_t b)
 *     Expression used as if condition to compare two keys.
 *     Examples: (a == b) or (strcmp(a, b) == 0) or (memcmp(&a, &b, sizeof(a)) == 0)
 * 
 * key_t key_put_expr(key_t key)
 *     This expression is executed for every new key that is inserted into the hashmap. The input is
 *     the key the user passed to the ..._put() function, the output is the key actually stored in
 *     the hashmap. Can be used to duplicate the keys for internal storage.
 *     Examples: key or sh_strdup(key)
 * 
 * key_t key_del_expr(key_t key)
 *     Executed when a key is deleted from the hashmap, e.g. to free a key previously duplicated for
 *     internal storage. The key field of the deleted hashmap slot is set to the returned value, so
 *     this expression should return a kind of 0 or NULL value for the key type.
 *     Examples: (free(key), NULL) or 0 or (struct foo){ 0 }
 * 
 * void* calloc_expr(size_t capacity, size_t slot_size)
 *     Expression used to allocate memory for the hashmap. Gets the same parameters as the calloc()
 *     function and is expected return a pointer to allocated and zeroed out memory just like
 *     calloc() does. This expression can be used to make the hashmap get its memory from somewhere
 *     else, e.g. a garbage collector or private memory pool.
 *     Examples: calloc(capacity, slot_size)
 * 
 * void free_expr(void* ptr)
 *     This expression is used to free memory previously allocated by calloc_expr.
 *     Examples: free(ptr) 
 */
#define SH_GEN_IMPL(name, key_t, value_t, key_hash_expr, key_cmp_expr, key_put_expr, key_del_expr, calloc_expr, free_expr)  \
                                                                                                   \
    /* Some forward declarations for internal functions.                                        */ \
    value_t* name##_put_ptr_internal(struct name* hashmap, key_t key);                             \
    bool     name##_resize(struct name* hashmap, uint32_t new_capacity);                           \
                                                                                                   \
    /**                                                                                         */ \
    /* Initializes a new empty hashmap. A small capacity is allocated so the hashmap doesn't    */ \
    /* need to be resized as soon as the first item is inserted.                                */ \
    void name##_new(struct name* hashmap) {                                                        \
        hashmap->length = 0;                                                                       \
        hashmap->capacity = 0;                                                                     \
        hashmap->deleted = 0;                                                                      \
        hashmap->slots = NULL;                                                                     \
        name##_resize(hashmap, 8);                                                                 \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Destroys the hashmap. Cleans up all the keys be executing the key_del_expr for each one  */ \
    /* and frees all associated memory.                                                         */ \
    void name##_destroy(struct name* hashmap) {                                                    \
        /* Execute key_del_expr for all keys still in the hashmap.                              */ \
        for(struct name##_slot* it = name##_start(hashmap); it; it = name##_next(hashmap, it)) {   \
            key_t key = it->key;                                                                   \
            key = key;  /* avoid unused variable warning                                        */ \
            it->key = (key_del_expr);                                                              \
            it->hash_or_flags = SH_SLOT_DELETED;                                                   \
        }                                                                                          \
                                                                                                   \
        hashmap->length = 0;                                                                       \
        hashmap->capacity = 0;                                                                     \
        hashmap->deleted = 0;                                                                      \
                                                                                                   \
        /* ptr is used as an argument to the  free_expr which should free the  memory of ptr    */ \
        /* like free() does.                                                                    */ \
        void* ptr = hashmap->slots;                                                                \
        ptr = ptr;  /* avoid unused variable warning if free_expr doesn't use ptr               */ \
        free_expr;                                                                                 \
                                                                                                   \
        hashmap->slots = NULL;                                                                     \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Resizes the hashmap to the specified new capacity. All existing itmes are rehashed into  */ \
    /* the new hashmap. For zero terminated string keys this is slightly faster than using the  */ \
    /* existing hashes, for integer keys this is slightly slower.                               */ \
    /*                                                                                          */ \
    /* Returns `true` if the resizing was successfull, `false` if the memory allocation for the */ \
    /* new capacity failed. In that case the hashmap remains unchanged and can still be used.   */ \
    /*                                                                                          */ \
    /* This function is not part of the public API! Don't use it in your code unless you know   */ \
    /* _exactly_ what you're doing!                                                             */ \
    bool name##_resize(struct name* hashmap, uint32_t new_capacity) {                              \
        /* Can't make hashmap smaller than it needs to be.                                      */ \
        if (new_capacity < hashmap->length)                                                        \
            return false;                                                                          \
                                                                                                   \
        struct name new_hashmap;                                                                   \
        new_hashmap.length = 0;                                                                    \
        new_hashmap.capacity = new_capacity;                                                       \
        new_hashmap.deleted = 0;                                                                   \
                                                                                                   \
        /* capacity and slot_size variables are used to pass arguments to calloc_expr which in  */ \
        /* turn should allocate memory like calloc() does.                                      */ \
        size_t capacity = new_capacity;                                                            \
        size_t slot_size = sizeof(new_hashmap.slots[0]);                                           \
        new_hashmap.slots = calloc_expr;                                                           \
                                                                                                   \
        /* Failed to allocate memory for new hashmap, leave the original untouched.             */ \
        if (new_hashmap.slots == NULL)                                                             \
            return false;                                                                          \
                                                                                                   \
        for(struct name##_slot* it = name##_start(hashmap); it; it = name##_next(hashmap, it)) {   \
            *name##_put_ptr_internal(&new_hashmap, it->key) = it->value;                           \
        }                                                                                          \
                                                                                                   \
        /* ptr is used as an argument to the free_expr which should free the memory of ptr like */ \
        /* free() does.                                                                         */ \
        void* ptr = hashmap->slots;                                                                \
        ptr = ptr;  /* avoid unused variable warning if free_expr doesn't use ptr               */ \
        free_expr;                                                                                 \
        *hashmap = new_hashmap;                                                                    \
        return true;                                                                               \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Inserts a new key-value pair into the hashmap, growing the hashmap if necessary. Returns */ \
    /* a pointer to the storage for the value. That pointer can then be used to store the value */ \
    /* itself in the hashmap.                                                                   */ \
    /*                                                                                          */ \
    /* The returned pointer stays valid as long as the hashmap isn't chaned (not adding or      */ \
    /* deleting items or resizing the hashmap). These operations potentiall force a resizing of */ \
    /* the hashmap and thus probably invalidate the memory address. So make sure you don't      */ \
    /* store the pointer somewhere or use it after a function that changes the hashmap.         */ \
    /*                                                                                          */ \
    /* Don't use this function while iterating over the hashmap. This function might resize the */ \
    /* hashmap and thus invalidates the pointer used as iterator, possibly leading to segfaults */ \
    /*                                                                                          */ \
    /* Returns `NULL` if the hashmap needs to grow but failed to allocate more memory for that. */ \
    value_t* name##_put_ptr(struct name* hashmap, key_t key) {                                     \
        if (hashmap->length + hashmap->deleted + 1 > hashmap->capacity * 0.5) {                    \
            uint32_t new_capacity = (hashmap->capacity == 0) ? 8 : hashmap->capacity * 2;          \
            if ( name##_resize(hashmap, new_capacity) == false )                                   \
                return NULL;                                                                       \
        }                                                                                          \
                                                                                                   \
        return name##_put_ptr_internal(hashmap, (key_put_expr));                                   \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* An internal function that reserves a new slot for the specified key and returns a        */ \
    /* pointer to the value of that slot. It DOESN'T check if the hashmap as a free slot nore   */ \
    /* executes the key_put_expr. It's only used by name##_put_ptr() and name##_resize()        */ \
    /* that already take care of that.                                                          */ \
    /*                                                                                          */ \
    /* This function is not part of the public API! Don't use it in your code unless you know   */ \
    /* _exactly_ what you're doing!                                                             */ \
    /*                                                                                          */ \
    /* Implementation detail: This function uses linear probing in case of hash collisions.     */ \
    value_t* name##_put_ptr_internal(struct name* hashmap, key_t key) {                            \
        uint32_t hash = (key_hash_expr) | SH_SLOT_FILLED;                                          \
        size_t index = hash % hashmap->capacity;                                                   \
                                                                                                   \
        while ( !(                                                                                 \
            hashmap->slots[index].hash_or_flags == SH_SLOT_FREE ||                                 \
            hashmap->slots[index].hash_or_flags == SH_SLOT_DELETED                                 \
        ) ) {                                                                                      \
            if (hashmap->slots[index].hash_or_flags == hash) {                                     \
                key_t a = hashmap->slots[index].key;                                               \
                key_t b = key;                                                                     \
                if (key_cmp_expr)                                                                  \
                    break;                                                                         \
            }                                                                                      \
            index = (index + 1) % hashmap->capacity;                                               \
        }                                                                                          \
                                                                                                   \
        if (hashmap->slots[index].hash_or_flags == SH_SLOT_DELETED)                                \
            hashmap->deleted--;                                                                    \
        hashmap->length++;                                                                         \
        hashmap->slots[index].hash_or_flags = hash;                                                \
        hashmap->slots[index].key = key;                                                           \
                                                                                                   \
        return &hashmap->slots[index].value;                                                       \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Looks up the specified key in the hashmap and returns a pointer to the value stored for  */ \
    /* that key. Returns `NULL` if the key could not be found.                                  */ \
    /*                                                                                          */ \
    /* The returned pointer stays valid as long as the hashmap isn't chaned (not adding or      */ \
    /* deleting items or resizing the hashmap). These operations potentiall force a resizing of */ \
    /* the hashmap and thus probably invalidate the memory address. So make sure you don't      */ \
    /* store the pointer somewhere or use it after a function that changes the hashmap.         */ \
    value_t* name##_get_ptr(struct name* hashmap, key_t key) {                                     \
        uint32_t hash = (key_hash_expr) | SH_SLOT_FILLED;                                          \
        size_t index = hash % hashmap->capacity;                                                   \
        while ( !(hashmap->slots[index].hash_or_flags == SH_SLOT_FREE) ) {                         \
            if (hashmap->slots[index].hash_or_flags == hash) {                                     \
                key_t a = hashmap->slots[index].key;                                               \
                key_t b = key;                                                                     \
                if (key_cmp_expr)                                                                  \
                    return &hashmap->slots[index].value;                                           \
            }                                                                                      \
                                                                                                   \
            index = (index + 1) % hashmap->capacity;                                               \
        }                                                                                          \
                                                                                                   \
        return NULL;                                                                               \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Deletes the key-value pair for the specified key from the hashmap. The hashmap is shrunk */ \
    /* if it is to sparse. Even if no key was deleted. This way you can use this function to    */ \
    /* with an arbitary (non-existing) key to shrink the hashmap if necessary after deleting    */ \
    /* some items while iterating over the hashmap.                                             */ \
    /*                                                                                          */ \
    /* Returns `true` if the key-value pair was deleted, `false` if the key wasn't found in the */ \
    /* hashmap.                                                                                 */ \
    /*                                                                                          */ \
    /* Don't use this function while iterating over the hashmap. This function might resize the */ \
    /* hashmap and thus invalidates the pointer used as iterator, possibly leading to segfaults */ \
    bool name##_del(struct name* hashmap, key_t key) {                                             \
        uint32_t hash = (key_hash_expr) | SH_SLOT_FILLED;                                          \
        size_t index = hash % hashmap->capacity;                                                   \
        while ( !(hashmap->slots[index].hash_or_flags == SH_SLOT_FREE) ) {                         \
            if (hashmap->slots[index].hash_or_flags == hash) {                                     \
                key_t a = hashmap->slots[index].key;                                               \
                key_t b = key;                                                                     \
                if (key_cmp_expr) {                                                                \
                    /* Define a local variable named "key" that is used to pass the key of the  */ \
                    /* deleted item to key_del_expr. We can't use the function argument named   */ \
                    /* "key" because the key_put_expr might have duplicated a string key. In    */ \
                    /* that case the key_del_expr expects to get exactly the value returned by  */ \
                    /* key_put_expr... and that is the key stored in the deleted slot.          */ \
                    key_t key = hashmap->slots[index].key;                                         \
                    key = key; /* avoid unused variable warning                                 */ \
                    hashmap->slots[index].key = (key_del_expr);                                    \
                    hashmap->slots[index].hash_or_flags = SH_SLOT_DELETED;                         \
                    hashmap->length--;                                                             \
                    hashmap->deleted++;                                                            \
                                                                                                   \
                    name##_shrink_if_necessary(hashmap);                                           \
                    return true;                                                                   \
                }                                                                                  \
            }                                                                                      \
                                                                                                   \
            index = (index + 1) % hashmap->capacity;                                               \
        }                                                                                          \
                                                                                                   \
        return false;                                                                              \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Inserts a value at the specified key into the hashmap. An existing value for the same    */ \
    /* key is overwritten. The hashmap is grown if necessary.                                   */ \
    /*                                                                                          */ \
    /* Causes a segmentation fault in case the hashmap needs to grow but couldn't allocate the  */ \
    /* necessary memory. Use name##_put_ptr() if you want to handle that case.                  */ \
    /*                                                                                          */ \
    /* Don't use this function while iterating over the hashmap. This function might resize the */ \
    /* hashmap and thus invalidates the pointer used as iterator, possibly leading to segfaults */ \
    void name##_put(struct name* hashmap, key_t key, value_t value) {                              \
        *name##_put_ptr(hashmap, key) = value;                                                     \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Fetches the value for the specified key from the hashmap. In case the key isn't found    */ \
    /* the `default_value` is returned.                                                         */ \
    value_t name##_get(struct name* hashmap, key_t key, value_t default_value) {                   \
        value_t* value_ptr = name##_get_ptr(hashmap, key);                                         \
        return (value_ptr) ? *value_ptr : default_value;                                           \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Checks if the specified key exists in the hashmap. Is just as fast as name##_get() or    */ \
    /* name##_get_ptr() so use those if you want to actually get a value. This function is      */ \
    /* useful if you only need to know that the key is there.                                   */ \
    /*                                                                                          */ \
    /* Returns `true` if the key was found, `false` if not.                                     */ \
    bool name##_contains(struct name* hashmap, key_t key) {                                        \
        return (name##_get_ptr(hashmap, key) != NULL);                                             \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Returns an iterator pointing to the first item of the hashmap. Returns `NULL` if the     */ \
    /* hashmap is empty. Meant to be used with name##_next() in a for loop to iterate over      */ \
    /* all items in the hashmap:                                                                */ \
    /*                                                                                          */ \
    /*    for(hash_it_p it = hash_start(&hash); it != NULL; it = hash_next(&hash, it)) {        */ \
    /*        it->key;    // access the key                                                     */ \
    /*        it->value;  // access the value                                                   */ \
    /*        hash_remove(&hash, it);  // remove the current item                               */ \
    /*    }                                                                                     */ \
    /*                                                                                          */ \
    /*    // If you removed almost all items this will shrink the hashmap after the loop.       */ \
    /*    // This is only needed in extreme cases.                                              */ \
    /*    hash_shrink_if_necessary(&hash);                                                      */ \
    /*                                                                                          */ \
    /* You can't add new items to the hashmap during iteration (name##_put() or                 */ \
    /* name##_put_ptr()) since that invalidates the pointer of the iterator (and might lead to  */ \
    /* seg faults). name##_del() also can't be used during iteration (same problem) but you can */ \
    /* use name##_remove() to mark the current item as deleted and call                         */ \
    /* name##_shrink_if_necessary() if chances are that you deleted most items.                 */ \
    /*                                                                                          */ \
    struct name##_slot* name##_start(struct name* hashmap) {                                       \
        /* We need to start at an invalid slot address since sh_next() increments it before it  */ \
        /* looks at it (so it's safe).                                                          */ \
        return name##_next(hashmap, hashmap->slots - 1);                                           \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Advances the iterator to the next item in the hashmap or returns `NULL` if there is no   */ \
    /* next element.                                                                            */ \
    /*                                                                                          */ \
    /* See name##_start() for how to use this function.                                         */ \
    struct name##_slot* name##_next(struct name* hashmap, struct name##_slot* it) {                            \
        if (it == NULL)                                                                            \
            return NULL;                                                                           \
                                                                                                   \
        do {                                                                                       \
            it++;                                                                                  \
            /* Check if we're past the last slot.                                               */ \
            if (it - hashmap->slots >= hashmap->capacity)                                          \
                return NULL;                                                                       \
        } while( it->hash_or_flags == SH_SLOT_FREE || it->hash_or_flags == SH_SLOT_DELETED );      \
                                                                                                   \
        return it;                                                                                 \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Removes the item the iterator points to from the hashmap. This function is meant to be   */ \
    /* used while iterating over all items of the hashmap. The hashmap is not resized, even if  */ \
    /* all items are removed. Instead it is shrunk at the next call to name##_del() or          */ \
    /* name##_shrink_if_necessary().                                                            */ \
    /*                                                                                          */ \
    /* See name##_start() for how to use this function.                                         */ \
    void name##_remove(struct name* hashmap, struct name##_slot* it) {                             \
        if (it != NULL && it >= hashmap->slots && it - hashmap->slots < hashmap->capacity) {       \
            key_t key = it->key;                                                                   \
            key = key;  /* avoid unused variable warning                                        */ \
            it->key = (key_del_expr);                                                              \
            it->hash_or_flags = SH_SLOT_DELETED;                                                   \
                                                                                                   \
            hashmap->length--;                                                                     \
            hashmap->deleted++;                                                                    \
        }                                                                                          \
    }                                                                                              \
                                                                                                   \
    /**                                                                                         */ \
    /* Shrinks the hashmap down if it became to sparse. Returns `true` if it was shrunk,        */ \
    /* `false` if not. This function is meant to be called after you removed many items while   */ \
    /* iterating over the hashmap and want to make sure you don't wast to much memory.          */ \
    /*                                                                                          */ \
    /* Implementation detail: Other functions assume that the capacity is always > 0 or they'll */ \
    /* crash. Therefore this function only shrinks the capacity down to a minimal value but not */ \
    /* 0.                                                                                       */ \
    bool name##_shrink_if_necessary(struct name* hashmap) {                                        \
        uint32_t new_capacity = hashmap->capacity;                                                 \
        while ( hashmap->length < new_capacity * 0.25 && new_capacity > 8 )                        \
            new_capacity /= 2;                                                                     \
                                                                                                   \
        if (new_capacity < hashmap->capacity) {                                                    \
            name##_resize(hashmap, new_capacity);                                                  \
            return true;                                                                           \
        }                                                                                          \
                                                                                                   \
        return false;                                                                              \
    }                                                                                              \

#if _SVID_SOURCE || _BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED || _POSIX_C_SOURCE >= 200809L
#define sh_strdup strdup
#else
#define SH_NEED_STRDUP_IMPL
#endif

#endif // SLIM_HASH_HEADER


#ifdef SLIM_HASH_IMPLEMENTATION

/**
 * Calculate the 32 bit Murmur3 hash of a memory block.
 * 
 * You can set the seed parameter to 0. You can change it if you want to defend agains denail of
 * service attacks that try to send you strings that all hash to the same slot in the hashmap, thus
 * destroying the performance of the hashmap. It would be optimal to use a random seed for each
 * different hashmap but that would require a random number generator. If you don't need or want
 * that complexity just use an arbitary static seed for your hashmap (e.g. 0 or 1337).
 * 
 * See https://stackoverflow.com/questions/9241230/what-is-murmurhash3-seed-parameter
 * 
 * MurmurHash3 was created by Austin Appleby  in 2008. The initial implementation was published in
 * C++ and placed in the public: https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
 * 
 * The C++ code has been manually ported to C. The body loop has been changed to count from 0 to
 * nblocks-1 instead of -nblocks to 0 so that gcc -O3 doesn't get confused about starting the loop
 * with an invalid pointer.
 */
uint32_t sh_murmur3(const void* key, int size, uint32_t seed) {
    if (key == NULL || size == 0)
        return 0;
    
    const uint32_t* data = (const uint32_t*)key;
    const int nblocks = size / 4;
    
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    
    // Body
    for(int i = 0; i < nblocks; i++) {
        uint32_t k1 = data[i];
        
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;
        
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> (32 - 13));
        h1 = h1 * 5 + 0xe6546b64;
    }
    
    // Tail
    const uint8_t* tail = (const uint8_t*)(data + nblocks);
    uint32_t k1 = 0;
    
    switch(size & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> (32 - 15));;
            k1 *= c2;
            h1 ^= k1;
    };
    
    // Finalization
    h1 ^= size;
    
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;
    
    return h1;
}

/**
 * Calculate the 32 bit fnv1a hash of a zero terminated string. This hash function can be used for
 * identifiers, common field names or other strings. From the fnv1a page: "The high dispersion of
 * the FNV hashes makes them well suited for hashing nearly identical strings such as URLs,
 * hostnames, filenames, text, IP addresses, etc.". See http://www.isthe.com/chongo/tech/comp/fnv/
 * 
 * In performance tests on an old 64 bit x86 CPU the murmur3 hash function was faster than this
 * function.
 * 
 * The function is directly implemented from pseudocode on the website.
 */
uint32_t sh_fnv1a(const char* key) {
    uint32_t hash = 2166136261;
    for(uint8_t c = *key; c != '\0'; c++) {
        hash ^= c;
        hash *= 16777619;
    }
    return hash;
}

/**
 * Portable version of strdup() that doesn't require feature test macros to be set. But if we got a
 * POSIX strdup() use it.
 * 
 * The feature test macro expression is taken from the strdup() man page.
 */
#ifdef SH_NEED_STRDUP_IMPL
char* sh_strdup(const char* src) {
    char* dest = malloc(strlen(src) + 1);
    if (dest == NULL)
        return NULL;
    strcpy(dest, src);
    return dest;
}
#endif


#endif // SLIM_HASH_IMPLEMENTATION