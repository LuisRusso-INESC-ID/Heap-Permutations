/*
 * permgen.h -- generation of all permutations of an array by
 * Heap-type swap schedules, with a known final arrangement.
 *
 * Part of the PERMGEN package (ISO C99, no third-party code).
 *
 * PURPOSE
 *   Enumerate all n! arrangements of an n-element array so that every
 *   arrangement differs from its predecessor by exactly one swap of
 * two positions.  Three schedules ("variants") are provided:
 *
 *     PERMGEN_CLASSIC  B. R. Heap's original schedule (1963).
 *     PERMGEN_CYCLIC   same, but for even k the swaps of level k are
 * taken in the order (k-1 k), (2 k), (3 k), ..., (k-2 k), (1 k).  A
 * complete sweep over an even number n of elements then rotates the
 * array by one position. PERMGEN_LEXMAX   (even n only) a schedule
 * for which the array left after the sweep, started from 1,2,...,n,
 * is the lexicographically largest arrangement that any single-swap
 * enumeration can leave (see the paper).
 *
 *   The permutation ("net permutation") that a complete sweep applies
 * to the array is known in closed form for every variant; it is
 * available through permgen_net(), and permgen_restore() uses it to
 * put the array back into its initial order in O(n) time.
 *
 * CONVENTIONS
 *   Positions in this interface are 0-based: position p of the
 * interface is position p+1 of the accompanying paper.  A permutation
 * is stored in one-line notation: pi[p] is the position to which the
 * item that started at position p has been moved.
 *
 * THREAD SAFETY
 *   There are no global variables.  Distinct permgen objects may be
 * used concurrently from different threads.
 *
 * BASIC USE (iterator interface)
 *
 *     int a[4] = {0, 1, 2, 3}, i, j;
 *     permgen g;
 *     if (permgen_init(&g, 4, PERMGEN_CYCLIC) != PERMGEN_OK)
 * ...error... use(a);                              // first
 * arrangement while (permgen_next(&g, &i, &j)) {   // one swap per
 * call int t = a[i]; a[i] = a[j]; a[j] = t; use(a); // next
 * arrangement
 *     }
 *     // all 24 arrangements have been visited; a is now in the final
 * order permgen_restore(&g, a);              // a = {0,1,2,3} again
 *
 * The items may be of any type: permgen_next() only reports which two
 * positions to exchange.
 */
#ifndef PERMGEN_H
#define PERMGEN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Largest supported number of elements: n! < 2^64 for n <= 20. */
#define PERMGEN_NMAX 20

/* Return codes. */
/* Success. */
#define PERMGEN_OK 0
/* Null pointer, n outside 1..PERMGEN_NMAX, or unknown variant. */
#define PERMGEN_E_ARGUMENT 1
/* Variant not defined for this n (PERMGEN_LEXMAX needs n = 1 or
 * n even). */
#define PERMGEN_E_UNSUPPORTED 2
/* Internal consistency check failed: a bug, should never happen. */
#define PERMGEN_E_INTERNAL 3

typedef enum {
    PERMGEN_CLASSIC = 0,
    PERMGEN_CYCLIC = 1,
    PERMGEN_LEXMAX = 2
} permgen_variant;

/*
 * Generator state.  The members are private; they are exposed only so
 * that a permgen object can live on the stack or in static storage
 * (no dynamic allocation is performed anywhere in the package). n
 * number of elements variant schedule in use sched   sched[k][i], 2
 * <= k <= n, 1 <= i <= k-1 (both 1-based): the position that is
 * swapped with position k in round i of level k rounds  rounds[k]:
 * swaps already performed at level k since it last started net
 * net[p], 1 <= p <= n (1-based): final position of the item that
 *           started at position p after one complete sweep
 */
typedef struct {
    int n;
    permgen_variant variant;
    int sched[PERMGEN_NMAX + 1][PERMGEN_NMAX + 1];
    int rounds[PERMGEN_NMAX + 1];
    int net[PERMGEN_NMAX + 1];
} permgen;

/* Callback type for permgen_run(): receives the current arrangement.
 * Returning a nonzero value stops the enumeration. */
typedef int (*permgen_visit_fn)(const int *a, int n, void *user);

/*
 * permgen_init -- prepare a generator.
 *   g  (out) generator to initialise; must not be NULL.
 *   n  (in)  number of elements, 1 <= n <= PERMGEN_NMAX.
 *   v  (in)  variant.  PERMGEN_LEXMAX requires n = 1 or n even.
 * Returns PERMGEN_OK, PERMGEN_E_ARGUMENT, PERMGEN_E_UNSUPPORTED or
 * PERMGEN_E_INTERNAL.  Cost: O(n^3) time, independent of n!.
 */
int permgen_init(permgen *g, int n, permgen_variant v);

/*
 * permgen_next -- produce the next swap.
 *   g  (in/out) generator initialised by permgen_init().
 *   i, j (out) 0-based positions to be exchanged, i != j; j is the
 * largest position involved. Returns 1 if a swap is to be performed
 * (applying it yields the next unvisited arrangement), or 0 when all
 * n! arrangements have been visited; in that case *i and *j are not
 * modified and the generator has been reset, so that a further call
 * would start a new sweep from the present arrangement.  Each call
 * takes O(1) amortised time (O(n) worst case). The caller owns the
 * array and performs the swap itself.
 */
int permgen_next(permgen *g, int *i, int *j);

/*
 * permgen_net -- the net permutation of one complete sweep.
 *   g   (in)  initialised generator.
 *   pi  (out) array of g->n ints; pi[p] is the final position of the
 * item that started at position p (0-based, one-line notation).
 */
void permgen_net(const permgen *g, int *pi);

/*
 * permgen_restore -- undo a complete sweep.
 *   g  (in)     generator that has just completed a sweep
 *   a  (in/out) array of g->n ints holding the arrangement left by
 * the sweep; on return it holds the arrangement the sweep started
 * from. Cost: O(n).
 */
void permgen_restore(const permgen *g, int *a);

/*
 * permgen_run -- convenience driver around the iterator.
 *   a      (in/out) array of n ints (the data to be permuted)
 *   n      (in)     number of elements
 *   v      (in)     variant
 *   visit  (in)     called once for every arrangement, including the
 * first user   (in)     opaque pointer handed to visit count  (out)
 * if not NULL, receives the number of arrangements visited If visit
 * returns nonzero the enumeration stops early.  Otherwise a is left
 * in the final arrangement (use permgen_init + permgen_restore to
 * undo it). Returns PERMGEN_OK or an error code from permgen_init().
 */
int permgen_run(int *a, int n, permgen_variant v,
                permgen_visit_fn visit, void *user,
                unsigned long long *count);

/* permgen_strerror -- constant text describing a return code. */
const char *permgen_strerror(int code);

#ifdef __cplusplus
}
#endif

#endif /* PERMGEN_H */
