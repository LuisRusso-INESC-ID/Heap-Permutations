# Heap-type permutation generation

Software for the paper *Heap-type permutation generation with a
prescribed final arrangement* by Luís M. S. Russo (submitted to
Discrete Mathematics & Theoretical Computer Science).

The repository contains:

- `verify_claims.py`: checks every computational claim of the
  paper (tables, exhaustive runs for small n, and the construction
  of the extremal variant for all even n up to 10000).
- `src/`: PERMGEN, a small ISO C99 library that enumerates all n!
  arrangements of an array with one swap between consecutive
  arrangements, using one of three Heap-type schedules.
- `tests/`, `examples/`: a test driver and an example program, each
  with its expected output.

## Variants

All three variants list every arrangement exactly once. They differ
in the arrangement left by a complete sweep started from
`1, 2, ..., n`.

| Constant          | Defined for     | Final arrangement          |
|-------------------|-----------------|----------------------------|
| `PERMGEN_CLASSIC` | 1 ≤ n ≤ 20      | Heap's algorithm (1963)    |
| `PERMGEN_CYCLIC`  | 1 ≤ n ≤ 20      | even n: `n, 1, 2, ..., n-1` |
| `PERMGEN_LEXMAX`  | n = 1 or even n | lexicographically largest  |

For odd n the classic and cyclic variants agree and end in
`n, 2, 3, ..., n-1, 1`. The extremal variant ends in the reversal
`n, n-1, ..., 1` when n ≡ 2 (mod 4), and in `n, n-1, ..., 3, 1, 2`
when n ≡ 0 (mod 4). No single-swap enumeration can end in a
lexicographically larger arrangement.

## Building and testing

Requirements: a C99 compiler, POSIX `make`, and Python 3 for the
verification.

    make             # library, example and test driver
    make check       # run them and compare with expected output
    make check-long  # also n = 11 and 12 (several seconds)
    make verify      # check the claims of the paper (about 1 min)
    make clean

Compiler and flags are Makefile parameters, for example
`make CC=clang CFLAGS="-std=c99 -O2" check`. The library compiles
without warnings under `-std=c99 -pedantic-errors -Wall -Wextra
-Wconversion -Wshadow -Wcast-qual`.

## Using the library

Include `src/permgen.h` and link with `libpermgen.a`. Positions are
0-based. The library reports which two positions to swap; the caller
owns the data and performs the swap, so items of any type can be
permuted.

```c
#include "permgen.h"

double x[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
permgen g;
int i, j;

if (permgen_init(&g, 5, PERMGEN_CYCLIC) != PERMGEN_OK)
    return 1;
process(x);                        /* first arrangement */
while (permgen_next(&g, &i, &j)) {
    double t = x[i];
    x[i] = x[j];
    x[j] = t;
    process(x);                    /* every other arrangement */
}
```

### Functions

- `int permgen_init(permgen *g, int n, permgen_variant v)`
  prepares a generator for `n` items, 1 ≤ n ≤ `PERMGEN_NMAX` (20).
  Returns `PERMGEN_OK`, `PERMGEN_E_ARGUMENT`,
  `PERMGEN_E_UNSUPPORTED` (`PERMGEN_LEXMAX` with odd n ≥ 3) or
  `PERMGEN_E_INTERNAL` (a bug). Cost O(n³), independent of n!.
- `int permgen_next(permgen *g, int *i, int *j)` stores in `*i`
  and `*j` the next two positions to exchange and returns 1, or
  returns 0 after the last arrangement, leaving `*i` and `*j`
  unchanged and resetting the generator. O(1) amortised per call.
- `void permgen_net(const permgen *g, int *pi)` writes the net
  permutation of a complete sweep: `pi[p]` is the final position of
  the item that started at position `p`.
- `void permgen_restore(const permgen *g, int *a)` undoes a
  complete sweep of the `int` array `a` in O(n) time.
- `int permgen_run(int *a, int n, permgen_variant v,
  permgen_visit_fn visit, void *user, unsigned long long *count)`
  calls `visit(a, n, user)` for every arrangement of `a`, including
  the first; a nonzero return value stops the enumeration. If
  `count` is not `NULL` it receives the number of arrangements
  visited.
- `const char *permgen_strerror(int code)` describes a return code.

The generator has no global state and allocates no memory, so
distinct `permgen` objects can be used concurrently.
`examples/example.c` is a complete program.

## License

MIT; see `LICENSE`.
