# Setup implicit rule to build object files
CC  = gcc
CFLAGS = -std=c99 -Werror -Wall -Wextra -g -Og


# Build and run all tests by default
TESTS = $(patsubst %.c,%,$(wildcard tests/*_test.c))
all: $(TESTS)
	$(foreach test,$(TESTS),$(shell ./$(test)))

# Individual test dependencies, tests are build by implicit rules
tests/slim_test_crashtest.c: slim_test.h
tests/math_3d_test: math_3d.h slim_test.h
tests/math_3d_test: LDLIBS += -lm
tests/slim_hash_test: slim_hash.h slim_test.h
tests/slim_hash_no_typedefs_test: slim_hash.h slim_test.h

tests/slim_gl_test.o: slim_gl.h slim_test.h
tests/slim_gl_test: LDLIBS += -lGL


# Clean all files in the .gitignore list, ensures that the ignore file is
# properly maintained. Use bash to execute rm -rf so that wildcards get expanded.
clean:
	xargs --arg-file .gitignore --verbose -I % bash -c "rm -fr %"
