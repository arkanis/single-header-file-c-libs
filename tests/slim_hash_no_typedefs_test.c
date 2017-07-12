#define SLIM_HASH_NO_TYPEDEFS
#define SLIM_HASH_IMPLEMENTATION
#include "../slim_hash.h"
#define SLIM_TEST_IMPLEMENTATION
#include "../slim_test.h"


SH_GEN_DECL(dict, const char*, int);
SH_GEN_DICT_IMPL(dict, const char*, int);

void test_struct_types() {
	struct dict dict;
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
	
	struct dict_slot* it = dict_start(&dict);
	st_check(it->value == 1 || it->value == 3);
	it = dict_next(&dict, it);
	st_check(it->value == 1 || it->value == 3);
	it = dict_next(&dict, it);
	st_check_null(it);
	
	dict_destroy(&dict);
}

int main() {
	st_run(test_struct_types);
	return st_show_report();
}