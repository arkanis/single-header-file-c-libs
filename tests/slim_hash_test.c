#define SLIM_HASH_IMPLEMENTATION
#include "../slim_hash.h"
#define SLIM_TEST_IMPLEMENTATION
#include "../slim_test.h"





void test_new_and_destroy() {
	sh_hash_t hash;
	
	sh_new(&hash);
	st_check_int(hash.length, 0);
	
	sh_destroy(&hash);
	st_check_int(hash.length, 0);
	st_check_int(hash.capacity, 0);
	st_check_null(hash.slots);
}

void test_put_ptr() {
	sh_hash_t hash;
	sh_new(&hash);
	
	int* ptr = sh_put_ptr(&hash, 174);
	st_check_not_null(ptr);
	st_check_not_null(hash.slots);
	st_check(ptr >= &hash.slots[0].value && ptr <= &hash.slots[hash.capacity-1].value);
	st_check_int(hash.length, 1);
	
	sh_destroy(&hash);
}

void test_get_ptr() {
	sh_hash_t hash;
	sh_new(&hash);
	
	int* put_ptr = sh_put_ptr(&hash, 174);
	st_check_not_null(put_ptr);
	int* get_ptr = sh_get_ptr(&hash, 174);
	st_check_not_null(get_ptr);
	st_check(put_ptr == get_ptr);
	
	sh_destroy(&hash);
}

void test_get_ptr_not_found() {
	sh_hash_t hash;
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
	sh_hash_t hash;
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
	sh_hash_t hash;
	sh_new(&hash);
	
	sh_put(&hash, 1, 10);
	int value = sh_get(&hash, 1, 0);
	st_check_int(value, 10);
	
	value = sh_get(&hash, 999, 7);
	st_check_int(value, 7);
	
	sh_destroy(&hash);
}

void test_contains() {
	sh_hash_t hash;
	sh_new(&hash);
	
	sh_put(&hash, 1, 10);
	bool inside = sh_contains(&hash, 1);
	st_check_int(inside, 1);
	
	inside = sh_contains(&hash, 999);
	st_check_int(inside, 0);
	
	sh_destroy(&hash);
}

void test_iteration() {
	sh_hash_t hash;
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
	sh_hash_t hash;
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
	sh_hash_t hash;
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
	sh_hash_t hash;
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
	sh_hash_t hash;
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
	return st_show_report();
}