# Setup implicit rule to build object files
CC  = gcc
CFLAGS = -std=c99 -Werror -Wall -Wextra


# Build and run all tests by default
TESTS = $(patsubst %.c,%,$(wildcard tests/*_test.c))
all: $(TESTS)
	$(foreach test,$(TESTS),$(shell ./$(test)))

# Individula test dependencies, tests are build by implicit rules
tests/slim_test_crashtest.c: slim_test.h
tests/math_3d_test: math_3d.h slim_test.h
tests/math_3d_test: LDLIBS += -lm
tests/slim_hash_test: slim_hash.h slim_test.h


# Clean all files in the .gitignore list, ensures that the ignore file
# is properly maintained.
clean:
	xargs --arg-file .gitignore --verbose rm -fr