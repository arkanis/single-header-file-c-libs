# Setup implicit rule to build object files
CC  = gcc
CFLAGS = -std=c99 -Werror -Wall -Wextra


# Tests are created by implicit rules
TESTS = $(patsubst %.c,%,$(wildcard tests/*test.c))
all: $(TESTS)

# Individula test dependencies, tests are build by implicit rules
tests/slim_test_crashtest.c: slim_test.h


# Clean all files in the .gitignore list, ensures that the ignore file
# is properly maintained.
clean:
	xargs --arg-file .gitignore --verbose rm -fr