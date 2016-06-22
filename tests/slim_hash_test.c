#define SLIM_HASH_IMPLEMENTATION
#include "../slim_hash.h"
#define SLIM_TEST_IMPLEMENTATION
#include "../slim_test.h"


SH_GEN_DECL(sh, int64_t, int);
SH_GEN_HASH_DEF(sh, int64_t, int);

SH_GEN_DECL(dict, const char*, int);
SH_GEN_DICT_DEF(dict, const char*, int);


void test_new_and_destroy() {
	sh_t hash;
	
	sh_new(&hash);
	st_check_int(hash.length, 0);
	
	sh_destroy(&hash);
	st_check_int(hash.length, 0);
	st_check_int(hash.capacity, 0);
	st_check_null(hash.slots);
}

void test_put_ptr() {
	sh_t hash;
	sh_new(&hash);
	
	int* ptr = sh_put_ptr(&hash, 174);
	st_check_not_null(ptr);
	st_check_not_null(hash.slots);
	st_check(ptr >= &hash.slots[0].value && ptr <= &hash.slots[hash.capacity-1].value);
	st_check_int(hash.length, 1);
	
	sh_destroy(&hash);
}

void test_get_ptr() {
	sh_t hash;
	sh_new(&hash);
	
	int* put_ptr = sh_put_ptr(&hash, 174);
	st_check_not_null(put_ptr);
	int* get_ptr = sh_get_ptr(&hash, 174);
	st_check_not_null(get_ptr);
	st_check(put_ptr == get_ptr);
	
	sh_destroy(&hash);
}

void test_get_ptr_not_found() {
	sh_t hash;
	sh_new(&hash);
	
	int* get_ptr = sh_get_ptr(&hash, 12345);
	st_check_null(get_ptr);
	
	int* put_ptr = sh_put_ptr(&hash, 174);
	st_check_not_null(put_ptr);
	
	get_ptr = sh_get_ptr(&hash, 12345);
	st_check_null(get_ptr);
	get_ptr = sh_get_ptr(&hash, 175);
	st_check_null(get_ptr);
	
	sh_destroy(&hash);
}

void test_del() {
	sh_t hash;
	sh_new(&hash);
	
	int* put_ptr = sh_put_ptr(&hash, 174);
	int* get_ptr = sh_get_ptr(&hash, 174);
	st_check(put_ptr == get_ptr);
	
	bool found = sh_del(&hash, 174);
	st_check_int(found, 1);
	get_ptr = sh_get_ptr(&hash, 174);
	st_check_null(get_ptr);
	
	found = sh_del(&hash, 174);
	st_check_int(found, 0);
	
	sh_destroy(&hash);
}

void test_get_and_put() {
	sh_t hash;
	sh_new(&hash);
	
	sh_put(&hash, 1, 10);
	int value = sh_get(&hash, 1, 0);
	st_check_int(value, 10);
	
	value = sh_get(&hash, 999, 7);
	st_check_int(value, 7);
	
	sh_destroy(&hash);
}

void test_contains() {
	sh_t hash;
	sh_new(&hash);
	
	sh_put(&hash, 1, 10);
	bool inside = sh_contains(&hash, 1);
	st_check_int(inside, 1);
	
	inside = sh_contains(&hash, 999);
	st_check_int(inside, 0);
	
	sh_destroy(&hash);
}

void test_iteration() {
	sh_t hash;
	sh_new(&hash);
	
	// empty hash
	sh_it_p it = sh_start(&hash);
	st_check_null(it);
	
	// Make sure st_next() doesn't break with NULL as input
	it = sh_next(&hash, it);
	st_check_null(it);
	
	// Add test data
	sh_put(&hash, 0, 10);
	sh_put(&hash, 1, 20);
	sh_put(&hash, 2, 30);
	
	// Do a full iteration
	bool visited[3] = {0};
	int loop_counter = 0;
	for(sh_it_p it = sh_start(&hash); it != NULL; it = sh_next(&hash, it)) {
		st_check(it->key >= 0 && it->key <= 2);
		st_check_int(it->value, (it->key + 1) * 10);
		visited[it->key] = true;
		loop_counter++;
	}
	st_check_int(loop_counter, 3);
	st_check_int(visited[0], 1);
	st_check_int(visited[1], 1);
	st_check_int(visited[2], 1);
	
	sh_destroy(&hash);
}

void test_remove_during_iteration() {
	sh_t hash;
	sh_new(&hash);
	
	sh_put(&hash, 0, 10);
	sh_put(&hash, 1, 20);
	sh_put(&hash, 2, 30);
	st_check_int(hash.length, 3);
	
	for(sh_it_p it = sh_start(&hash); it != NULL; it = sh_next(&hash, it)) {
		if (it->key == 1)
			sh_remove(&hash, it);
	}
	st_check_int(hash.length, 2);
	int* value_ptr = sh_get_ptr(&hash, 1);
	st_check_null(value_ptr);
	
	sh_destroy(&hash);
}

void test_growing() {
	sh_t hash;
	sh_new(&hash);
	st_check_int(hash.length, 0);
	st_check(hash.capacity < 100);
	
	for(size_t i = 0; i < 100; i++)
		sh_put(&hash, i, i*2);
	st_check_int(hash.length, 100);
	st_check(hash.capacity >= 100);
	
	for(size_t i = 0; i < 100; i++) {
		int value = sh_get(&hash, i, -1);
		st_check_int(value, (int)i*2);
	}
	
	sh_destroy(&hash);
}

void test_shrinking() {
	sh_t hash;
	sh_new(&hash);
	st_check(hash.capacity < 100);
	
	for(size_t i = 0; i < 100; i++)
		sh_put(&hash, i, i*2);
	st_check(hash.capacity >= 100);
	
	for(size_t i = 0; i < 100; i++)
		sh_del(&hash, i);
	st_check_int(hash.length, 0);
	st_check(hash.capacity < 100);
	
	sh_destroy(&hash);
}

void test_optimize() {
	sh_t hash;
	sh_new(&hash);
	
	for(size_t i = 0; i < 100; i++)
		sh_put(&hash, i, i*2);
	st_check(hash.capacity >= 100);
	
	for(size_t i = 0; i < 30; i++)
		sh_del(&hash, i);
	st_check_int(hash.length, 70);
	st_check(hash.deleted > 0);
	
	sh_optimize(&hash);
	st_check_int(hash.length, 70);
	st_check_int(hash.deleted, 0);
	
	sh_destroy(&hash);
}

void test_dict() {
	dict_t dict;
	dict_new(&dict);
	
	dict_put(&dict, "a", 1);
	st_check_int(dict_contains(&dict, "a"), 1);
	st_check_int(dict_contains(&dict, "b"), 0);
	st_check_int(dict_get(&dict, "a", 0), 1);
	
	dict_put(&dict, "b", 2);
	dict_put(&dict, "c", 3);
	st_check_int(dict_get(&dict, "b", 0), 2);
	st_check_int(dict_get(&dict, "c", 0), 3);
	
	dict_del(&dict, "b");
	st_check_int(dict_get(&dict, "a", 0), 1);
	st_check_int(dict_get(&dict, "b", 0), 0);
	st_check_int(dict_get(&dict, "c", 0), 3);
	
	dict_destroy(&dict);
}

void test_dict_update() {
	dict_t dict;
	dict_new(&dict);
	
	dict_put(&dict, "x", 7);
	st_check_int(dict_get(&dict, "x", 0), 7);
	dict_put(&dict, "x", 1);
	st_check_int(dict_get(&dict, "x", 0), 1);
	
	dict_del(&dict, "x");
	st_check_int(dict_get(&dict, "x", 0), 0);
	dict_put(&dict, "x", 3);
	st_check_int(dict_get(&dict, "x", 0), 3);
	
	dict_destroy(&dict);
}

SH_GEN_DECL(env, char*, int);
SH_GEN_DICT_DEF(env, char*, int);

void test_example() {
	env_t env;
	env_new(&env);
	
	env_put(&env, "foo", 3);
	env_put(&env, "bar", 17);
	
	env_get(&env, "foo", -1);  // => 3
	env_get(&env, "bar", -1);  // => 17
	env_get(&env, "hurdl", -1);  // => -1
	
	st_check_int( env_get(&env, "foo", -1), 3 );
	st_check_int( env_get(&env, "bar", -1), 17 );
	st_check_int( env_get(&env, "hurdl", -1), -1 );
	
	env_put(&env, "foo", 5);
	env_get(&env, "foo", -1);  // => 5
	st_check_int( env_get(&env, "foo", -1), 5 );
	
	int* value_ptr = NULL;
	value_ptr = env_get_ptr(&env, "foo");  // *value_ptr => 5
	st_check(*value_ptr == 5);
	value_ptr = env_get_ptr(&env, "hurdl");  // value_ptr => NULL
	st_check_null(value_ptr);
	
	value_ptr = env_put_ptr(&env, "grumpf");  // value_ptr pointer to entry value
	*value_ptr = 21;
	env_get(&env, "grumpf", -1);  // => 21
	st_check_int( env_get(&env, "grumpf", -1), 21 );
	*value_ptr = 42;
	env_get(&env, "grumpf", -1);  // => 42
	st_check_int( env_get(&env, "grumpf", -1), 42 );
	
	env_contains(&env, "bar"); // => true
	st_check_int( env_contains(&env, "bar"), true );
	env_del(&env, "bar");
	env_contains(&env, "bar"); // => false
	st_check_int( env_contains(&env, "bar"), false );
	
	for(env_it_p it = env_start(&env); it != NULL; it = env_next(&env, it)) {
		//printf("%s: %d\n", it->key, it->value);
	}
	
	env_destroy(&env);
}


int main() {
	st_run(test_new_and_destroy);
	st_run(test_put_ptr);
	st_run(test_get_ptr);
	st_run(test_get_ptr_not_found);
	st_run(test_del);
	st_run(test_get_and_put);
	st_run(test_contains);
	st_run(test_iteration);
	st_run(test_remove_during_iteration);
	st_run(test_growing);
	st_run(test_shrinking);
	st_run(test_optimize);
	st_run(test_dict);
	st_run(test_dict_update);
	st_run(test_example);
	return st_show_report();
}