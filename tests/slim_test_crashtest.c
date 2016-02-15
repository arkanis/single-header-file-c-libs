/**

This file contains code to see if Slim Test behaves as it should. All test cases
here are expected to fail at the last check. Not nice but adding an extra
check_failed() would complicate matters more than it's worth it.

**/

#define SLIM_TEST_IMPLEMENTATION
#include "../slim_test.h"


void test_check() {
	st_check(1 == 0);
}

void test_check_msg() {
	st_check_msg(1 == 1, "broken!");
	st_check_msg(1 == 0, "broken! expected %d", 7);
}

void test_check_str() {
	st_check_str("foo", "bar");
}

void test_check_strn() {
	st_check_strn("foo1", "foo2", 3);
	st_check_strn("fox1", "foo2", 3);
}

void test_check_int() {
	int value = 7;
	st_check_int(value, 7);
	st_check_int(value, 8);
}

void test_check_float() {
	float value = 3.141;
	st_check_float(value, 3.141, 0.001);
	st_check_float(value, 3.5, 0.001);
}

void test_check_not_null() {
	int value = 7;
	void* p = &value;
	st_check_not_null(p);
	
	p = NULL;
	st_check_not_null(p);
}

void test_check_null() {
	void* p = NULL;
	st_check_null(p);
	
	int value = 7;
	p = &value;
	st_check_null(p);
}

int main() {
	st_run(test_check);
	st_run(test_check_msg);
	st_run(test_check_str);
	st_run(test_check_strn);
	st_run(test_check_int);
	st_run(test_check_float);
	st_run(test_check_not_null);
	st_run(test_check_null);
	return st_show_report();
}