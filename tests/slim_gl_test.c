#include "../../slim_gl/demos/gl_3_1_core.h"

#define SLIM_GL_IMPLEMENTATION
#include "../slim_gl.h"
#define SLIM_TEST_IMPLEMENTATION
#include "../slim_test.h"


void test_next_argument_basics() {
	const char* sample;
	const char* next;
	sgl_arg_t arg = { 0 };
	
	sample = "";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_null(arg.error_message);
	
	sample = "  \t  \n  \v  \f  \r  ";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_null(arg.error_message);
	
	sample = NULL;
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_null(arg.error_message);
	
	sample = ";";
	next = sgl__next_argument(sample, SGL__BUFFER_DIRECTIVES, &arg);
	st_check_not_null(next);
	st_check_int(arg.type, ';');
	next = sgl__next_argument(next, SGL__BUFFER_DIRECTIVES, &arg);
	st_check_null(next);
	st_check_null(arg.error_message);
	
	sample = "foo %4f";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS, &arg);
	st_check_not_null(next);
	st_check_str(arg.name, "foo");
	st_check_int(arg.type, 'f');
	st_check_str(arg.modifiers, "4");
	
	sample = "waaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaay_to_long %4f";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
	
	sample = "foo+bar %4f";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
	
	sample = "foo";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
	
	sample = "x";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
	
	sample = "%";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
	
	sample = "%xxxxxxxxxxxxxxxxf";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
}

void test_next_argument_for_program_new() {
	// sgl_program_new() only uses directives, named arguments or buffer directives result in errors
	
	const char* sample;
	const char* next;
	sgl_arg_t arg = { 0 };
	
	sample = "%G %fV %fF";
	next = sample;
	
	next = sgl__next_argument(next, 0, &arg);
	st_check_not_null(next);
	st_check_null(arg.error_message);
	st_check_int(arg.type, 'G');
	st_check_str(arg.modifiers, "");
	
	next = sgl__next_argument(next, 0, &arg);
	st_check_not_null(next);
	st_check_null(arg.error_message);
	st_check_int(arg.type, 'V');
	st_check_str(arg.modifiers, "f");
	
	next = sgl__next_argument(next, 0, &arg);
	st_check_not_null(next);
	st_check_null(arg.error_message);
	st_check_int(arg.type, 'F');
	st_check_str(arg.modifiers, "f");
	
	next = sgl__next_argument(next, 0, &arg);
	st_check_null(next);
	
	// Named arguments should give an error
	sample = "foo %4f";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
	
	// End of buffer directive should give an error
	sample = ";";
	next = sgl__next_argument(sample, 0, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
}

void test_next_argument_for_vao_new() {
	// sgl_vao_new() uses named arguments as well as the end of buffer directive
	
	const char* sample;
	const char* next;
	sgl_arg_t arg = { 0 };
	
	sample = "pos %3f ; color %4unb";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS | SGL__BUFFER_DIRECTIVES, &arg);
	st_check_not_null(next);
	st_check_str(arg.name, "pos");
	st_check_int(arg.type, 'f');
	st_check_str(arg.modifiers, "3");
	
	next = sgl__next_argument(next, SGL__NAMED_ARGS | SGL__BUFFER_DIRECTIVES, &arg);
	st_check_not_null(next);
	st_check_int(arg.type, ';');
	
	next = sgl__next_argument(next, SGL__NAMED_ARGS | SGL__BUFFER_DIRECTIVES, &arg);
	st_check_not_null(next);
	st_check_str(arg.name, "color");
	st_check_int(arg.type, 'b');
	st_check_str(arg.modifiers, "4un");
	
	next = sgl__next_argument(next, SGL__NAMED_ARGS | SGL__BUFFER_DIRECTIVES, &arg);
	st_check_null(next);
	st_check_null(arg.error_message);
}

void test_next_argument_for_draw() {
	// sgl_draw() only uses named directives for the uniforms, end of buffer directives result in errors
	
	const char* sample;
	const char* next;
	sgl_arg_t arg = { 0 };
	
	sample = "proj %4x4tm light_pos %3f";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS, &arg);
	st_check_not_null(next);
	st_check_str(arg.name, "proj");
	st_check_int(arg.type, 'm');
	st_check_str(arg.modifiers, "4x4t");
	
	next = sgl__next_argument(next, SGL__NAMED_ARGS, &arg);
	st_check_not_null(next);
	st_check_str(arg.name, "light_pos");
	st_check_int(arg.type, 'f');
	st_check_str(arg.modifiers, "3");
	
	next = sgl__next_argument(next, SGL__NAMED_ARGS, &arg);
	st_check_null(next);
	st_check_null(arg.error_message);
	
	// End of buffer directive should give an error
	sample = ";";
	next = sgl__next_argument(sample, SGL__NAMED_ARGS, &arg);
	st_check_null(next);
	st_check_not_null(arg.error_message);
	printf("error: %s\n%s\n%*s^\n", arg.error_message, sample, (int)(arg.error_at - sample), "");
}


/*
void test_parse_name() {
	const char* sample = "foo batz bar";
	sgl_option_t option;
	
	const char* next = sgl__parse_name(sample, &option);
	st_check_str(option.name, "foo");
	st_check_int(option.name[3], '\0');
	st_check_str(next, " batz bar");
	
	next = sgl__parse_name(next, &option);
	st_check_str(option.name, "batz");
	st_check_int(option.name[4], '\0');
	st_check_str(next, " bar");
	
	next = sgl__parse_name(next, &option);
	st_check_str(option.name, "bar");
	st_check_int(option.name[3], '\0');
	st_check_str(next, "");
	
	next = sgl__parse_name(next, &option);
	st_check_null(next);
}

void test_parse_directive() {
	const char* sample = "%4f %unb %4x4tm %*rt";
	sgl_option_t option;
	
	const char* next = sgl__parse_directive(sample, &option);
	st_check_str(option.modifiers, "4");
	st_check_int(option.modifiers[1], '\0');
	st_check_int(option.type, 'f');
	st_check_str(next, " %unb %4x4tm %*rt");
	
	next = sgl__parse_directive(next, &option);
	st_check_str(option.modifiers, "un");
	st_check_int(option.modifiers[2], '\0');
	st_check_int(option.type, 'b');
	st_check_str(next, " %4x4tm %*rt");
	
	next = sgl__parse_directive(next, &option);
	st_check_str(option.modifiers, "4x4t");
	st_check_int(option.modifiers[4], '\0');
	st_check_int(option.type, 'm');
	st_check_str(next, " %*rt");
	
	next = sgl__parse_directive(next, &option);
	st_check_str(option.modifiers, "*r");
	st_check_int(option.modifiers[2], '\0');
	st_check_int(option.type, 't');
	st_check_str(next, "");
	
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
}

void test_parse_name_and_directive() {
	const char* sample = "pos %4f color %unb -index %s";
	const char* next = sample;
	sgl_option_t option;
	
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_str(option.name, "pos");
	st_check_int(option.type, 'f');
	st_check_str(option.modifiers, "4");
	st_check_str(next, " color %unb -index %s");
	
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_str(option.name, "color");
	st_check_int(option.type, 'b');
	st_check_str(option.modifiers, "un");
	st_check_str(next, " -index %s");
	
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_str(option.name, "-index");
	st_check_int(option.type, 's');
	st_check_str(option.modifiers, "");
	st_check_str(next, "");
	
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
}

void test_parse_name_and_directive_errors() {
	sgl_option_t option = { 0 };
	
	const char* sample = "";
	const char* next = sample;
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
	
	sample = "waaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaay_to_long %4f";
	next = sample;
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
	st_check_not_null(option.error_message);
	printf("error: %s\n%s\n%*s^\n", option.error_message, sample, (int)(option.error_at - sample), "");
	
	sample = "foo+bar %4f";
	next = sample;
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
	st_check_not_null(option.error_message);
	printf("error: %s\n%s\n%*s^\n", option.error_message, sample, (int)(option.error_at - sample), "");
	
	sample = "foo %";
	next = sample;
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
	st_check_not_null(option.error_message);
	printf("error: %s\n%s\n%*s^\n", option.error_message, sample, (int)(option.error_at - sample), "");
	
	sample = "foo %to_looooooooooooooooongf";
	next = sample;
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
	st_check_not_null(option.error_message);
	printf("error: %s\n%s\n%*s^\n", option.error_message, sample, (int)(option.error_at - sample), "");
	
	sample = "foo bar";
	next = sample;
	next = sgl__parse_name(next, &option);
	next = sgl__parse_directive(next, &option);
	st_check_null(next);
	st_check_not_null(option.error_message);
	printf("error: %s\n%s\n%*s^\n", option.error_message, sample, (int)(option.error_at - sample), "");
}
*/


int main() {
	/*
	st_run(test_parse_name);
	st_run(test_parse_directive);
	st_run(test_parse_name_and_directive);
	st_run(test_parse_name_and_directive_errors);
	*/
	
	st_run(test_next_argument_basics);
	st_run(test_next_argument_for_program_new);
	st_run(test_next_argument_for_vao_new);
	st_run(test_next_argument_for_draw);
	
	return st_show_report();
}