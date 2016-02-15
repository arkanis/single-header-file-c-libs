/**

Slim Test v1.0
By Stephan Soller <stephan.soller@helionweb.de>
Licensed under the MIT license

This is an stb style single header file library to write test cases.
Define SLIM_TEST_IMPLEMENTATION before you include this file in *one* C file to
create the implementation.


USAGE

	#define SLIM_TEST_IMPLEMENTATION
	#include "slim_test.h"
	
	void test_case_a() {
		int x = 7;
		st_check_int(x, 7);
	}
	
	void test_case_b() {
		const char* text = "Hello Test!";
		st_check_str(text, "Hello World!");
	}
	
	int main() {
		st_run(test_case_a);
		st_run(test_case_b);
		return st_show_report();
	}

This will output:

	.F
	- test_case_b failed in tests/slim_test_example.c:11
	  text == "Hello World!" failed, got "Hello Test!", expected "Hello World!"
	1 tests failed, 1 tests passed, 1 checks passed


QUICK NOTES

- Each test case is a void function. If a check fails that test case is aborted
  but the others will still be run.
- Each test case has to be called using the `st_run()` function.
- `st_show_report()` shows some statistics and returns the number of failed test
  cases.
- The checks are macros that return from the test case when the check failed.
  You can add your own check via the `st_check_msg()` macro like this:
  
  	#define st_check_even(actual) st_check_msg( (actual) % 2 == 0, #actual " is odd, expected it to be even" )
  
  The message understands `printf()` format specifiers like %s and %f. Just like
  with `printf()` the values to inserted there are added as extra arguments after
  the message.
- Check messages can only be up to 1024 bytes long. If you need anything longer
  you can define ST_MAX_MESSAGE_SIZE with the size you need before you include
  this header.


VERSION HISTORY

v1.0  2016-02-14  Initial release

**/

#ifndef SLIM_TEST_INCLUDED
#define SLIM_TEST_INCLUDED

#include <string.h>
#include <math.h>

typedef void (*st_test_func_t)();
int  st_run(st_test_func_t test_case);
int  st_show_report();
void st_failed(const char* func, const char* file, const int line, const char *message, ...);

size_t st_tests_run = 0, st_tests_failed = 0, st_checks_passed = 0;

#define st_check(expr)                             if( expr                                       ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #expr); }
#define st_check_msg(expr, message, ...)           if( expr                                       ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, message, ##__VA_ARGS__); }
#define st_check_str(actual, expected)             if( strcmp((actual), (expected)) == 0          ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #actual " == " #expected " failed, got \"%s\", expected \"%s\"", (actual), (expected)); }
#define st_check_strn(actual, expected, size)      if( strncmp((actual), (expected), (size)) == 0 ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #actual " == " #expected " failed, got \"%.*s\", expected \"%.*s\"", (int)(size), (actual), (int)(size), (expected)); }
#define st_check_int(actual, expected)             if( (actual) == (expected)                     ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #actual " == " #expected " failed, got %d, expected %d", (actual), (expected)); }
#define st_check_float(actual, expected, epsilon)  if( fabs((actual) - (expected)) < (epsilon)    ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #actual " == " #expected " failed, got %f, expected %f (epsilon %f)", (actual), (expected), (epsilon)); }
#define st_check_not_null(actual)                  if( (actual) != NULL                           ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #actual " is NULL but should not be NULL"); }
#define st_check_null(actual)                      if( (actual) == NULL                           ){ st_checks_passed++; }else{ return st_failed(__func__, __FILE__, __LINE__, #actual " should be NULL, got %p", (actual)); }

#ifndef ST_MAX_MESSAGE_SIZE
#define ST_MAX_MESSAGE_SIZE 1024
#endif

#endif // SLIM_TEST_INCLUDED


#ifdef SLIM_TEST_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/**
 * A single linked list of error messages. For each failed check `st_failed()`
 * appends a message at the end of the list. It's only used internally so it's
 * not part of the public header.
 */
typedef struct st_report_item st_report_item_t, *st_report_item_p;
struct st_report_item {
    char *msg;
    st_report_item_p next;
};
st_report_item_p st__report_items = NULL;


/**
 * Records a message for a failed test case. You can use printf() format
 * specifiers in the `message` parameter.
 * 
 * The entire message can only be up to ST_MAX_MESSAGE_SIZE bytes long.
 * 
 * Implementation notes:
 * 
 * Would be easier with vasprintf() and asprintf() but this requires _GNU_SOURCE.
 * Not a good idea in a single header file library to pull in that feature test
 * macro.
 */
void st_failed(const char* func, const char* file, const int line, const char *message, ...) {
	st_tests_failed++;
	
	// Allocate a new report item for the message and insert it at the end of the linked list
	st_report_item_p item = malloc(sizeof(st_report_item_t));
	memset(item, 0, sizeof(st_report_item_t));
	if (st__report_items == NULL) {
		st__report_items = item;
	} else {
		st_report_item_p curr = st__report_items;
		while(curr->next != NULL)
			curr = curr->next;
		curr->next = item;
	}
	
	// Allocate message buffer in item
	size_t msg_buffer_size = ST_MAX_MESSAGE_SIZE, msg_buffer_filled = 0;
	item->msg = malloc(msg_buffer_size);
	
	// Put error message into buffer
	msg_buffer_filled += snprintf(item->msg, msg_buffer_size, "- %s failed in %s:%d\n  ", func, file, line);
	
	va_list args;
	va_start(args, message);
	msg_buffer_filled += vsnprintf(item->msg + msg_buffer_filled, msg_buffer_size - msg_buffer_filled, message, args);
	va_end(args);
	
	msg_buffer_filled += snprintf(item->msg + msg_buffer_filled, msg_buffer_size - msg_buffer_filled, "\n");
	
	// Write zero terminator to make sure that printing this message doesn't
	// read beyond the allocated buffer.
	item->msg[msg_buffer_size - 1] = '\0';
}


/**
 * Runs one test case function. Prints a "." and returns 1 (true) if the test
 * passed or prints an "F" and returns 0 (false) if it failed.
 */
int st_run(st_test_func_t test_case) {
	st_tests_run++;
	size_t test_failed_before_run = st_tests_failed;
	test_case();
	
	if (st_tests_failed > test_failed_before_run) {
		fprintf(stderr, "F");
		fflush(stderr);
		return 0;
	}
	
	fprintf(stderr, ".");
	fflush(stderr);
	return 1;
}

/**
 * Shows the messages of all failed test cases as well as a small summary.
 * Returns the number of failed test cases.
 */
int st_show_report() {
	fprintf(stderr, "\n");
	
	for(st_report_item_p item = st__report_items; item != NULL; item = item->next)
		fputs(item->msg, stderr);
	
	fprintf(stderr, "\x1b[%sm", (st_tests_failed > 0) ? "31" : "32");
	fprintf(stderr, "%zu tests failed, %zu tests passed, %zu checks passed\n", st_tests_failed, st_tests_run - st_tests_failed, st_checks_passed);
	fprintf(stderr, "\x1b[0m");
	return st_tests_failed;
}

#endif // SLIM_TEST_IMPLEMENTATION