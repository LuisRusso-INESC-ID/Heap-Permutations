# PERMGEN: Heap-type permutation generation (ISO C99).
#
#   make             build the library, the example and the tests
#   make check       run them and compare with the expected output
#   make check-long  also test n = 11 and 12 (several seconds)
#   make verify      check the computational claims of the paper
#   make clean       remove generated files
#
# Compiler and flags are parameters, e.g.
#   make CC=clang CFLAGS="-std=c99 -O2" check

CC      = cc
CFLAGS  = -std=c99 -pedantic -Wall -Wextra -O2
AR      = ar
ARFLAGS = rcs
PYTHON  = python3

LIB = libpermgen.a
OBJ = src/permgen.o
BIN = examples/example tests/test_permgen

all: $(LIB) $(BIN)

$(OBJ): src/permgen.c src/permgen.h
	$(CC) $(CFLAGS) -c src/permgen.c -o $(OBJ)

$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $(OBJ)

examples/example: examples/example.c $(LIB)
	$(CC) $(CFLAGS) examples/example.c $(LIB) -o $@

tests/test_permgen: tests/test_permgen.c $(LIB)
	$(CC) $(CFLAGS) tests/test_permgen.c $(LIB) -o $@

check: all
	./examples/example > examples/example.out
	diff examples/example.expected examples/example.out
	./tests/test_permgen > tests/test.out
	diff tests/test.expected tests/test.out
	@echo "check: all output agrees with the expected output"

check-long: all
	./tests/test_permgen -long

verify:
	$(PYTHON) verify_claims.py

clean:
	rm -f $(OBJ) $(LIB) $(BIN) examples/example.out tests/test.out

.PHONY: all check check-long verify clean
