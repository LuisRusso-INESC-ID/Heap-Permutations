/*
 * test_permgen.c -- test driver for the PERMGEN package.
 *
 * Exercises every feature of permgen.h:
 *   - all three variants for all admissible n (default: n <= 10);
 *   - uniqueness of all n! arrangements (rank in the factorial number
 *     system), exactly one swap between consecutive arrangements;
 *   - the final arrangement and the net permutation against the
 * closed forms of the accompanying paper (Theorems/Corollaries),
 * which are computed here independently of the library;
 *   - the parity bound and the LEXMAX target array;
 *   - permgen_restore(), repeatability of sweeps, permgen_run(),
 * early stop;
 *   - error handling.
 *
 * Usage:  test_permgen          standard test (n <= 10), output is
 *                               deterministic and is compared with
 *                               tests/test.expected by "make check"
 *         test_permgen -long    additionally n = 11, 12 (count and
 * final arrangement only; takes several seconds)
 *
 * Exit status 0 if and only if every check passed.
 *
 * INTERNAL VARIABLES (alphabetical): a   current arrangement (0-based
 * items 1..n stored as values 1..n); fact  factorials; failures
 * failure count; g  generator; prev  previous arrangement; seen  rank
 * bitmap.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/permgen.h"

#define MAXN_EXHAUSTIVE 10

static unsigned long long fact[PERMGEN_NMAX + 1];
static int failures = 0;

static const char *vname(permgen_variant v)
{
    return v == PERMGEN_CLASSIC  ? "classic"
           : v == PERMGEN_CYCLIC ? "cyclic "
                                 : "lexmax ";
}

static void fail(const char *msg, permgen_variant v, int n)
{
    printf("FAIL %s n=%2d : %s\n", vname(v), n, msg);
    failures++;
}

/* rank of a permutation of 1..n in the factorial number system */
static unsigned long long rank_of(const int *a, int n)
{
    unsigned long long r = 0;
    int i, j, smaller;
    for (i = 0; i < n; i++) {
        smaller = 0;
        for (j = i + 1; j < n; j++)
            if (a[j] < a[i]) smaller++;
        r += (unsigned long long)smaller * fact[n - 1 - i];
    }
    return r;
}

/* sign of a permutation given as an array of the values 1..n (+1 or
 * -1) */
static int sign_of(const int *a, int n)
{
    int seen[PERMGEN_NMAX + 1], i, x, len, s = 1;
    for (i = 1; i <= n; i++)
        seen[i] = 0;
    for (i = 1; i <= n; i++) {
        if (seen[i]) continue;
        len = 0;
        x = i;
        while (!seen[x]) {
            seen[x] = 1;
            x = a[x - 1];
            len++;
        }
        if (len % 2 == 0) s = -s;
    }
    return s;
}

/* Predicted net permutation (1-based one-line: pi[p] = final position
 * of the item that started at p) from the closed forms of the paper.
 * Returns 0 and leaves pi untouched for (LEXMAX, k): the prediction
 * there is the final array. */
static void predicted_net(permgen_variant v, int k, int *pi)
{
    int p;
    for (p = 1; p <= k; p++)
        pi[p] = p;
    if (k == 1) return;
    if (k % 2 == 1) {
        pi[1] = k;
        pi[k] = 1;
        return;
    }                                    /* (1 k)           */
    if (v == PERMGEN_CYCLIC || k == 2) { /* (1 2 ... k)     */
        for (p = 1; p < k; p++)
            pi[p] = p + 1;
        pi[k] = 1;
        return;
    }
    pi[1] = k;
    pi[k] = k - 1;
    pi[k - 1] = 2; /* (1 k k-1 2 ... k-2) */
    for (p = 2; p <= k - 3; p++)
        pi[p] = p + 1;
    pi[k - 2] = 1;
}

/* Lexicographically largest odd permutation of 1..n, n >= 2 */
static void lexmax_target(int n, int *t)
{
    int p;
    for (p = 0; p < n; p++)
        t[p] = n - p;
    if (sign_of(t, n) == 1) {
        t[n - 2] = 1;
        t[n - 1] = 2;
    }
}

static void check_sweep(permgen_variant v, int n, int full)
{
    permgen g;
    int a[PERMGEN_NMAX], prev[PERMGEN_NMAX], pi[PERMGEN_NMAX];
    int i, j, t, p, diff, rc, bad = 0;
    unsigned long long count = 1, r;
    char *seen = NULL;

    rc = permgen_init(&g, n, v);
    if (rc != PERMGEN_OK) {
        fail("permgen_init failed", v, n);
        return;
    }
    if (full) {
        seen = (char *)calloc((size_t)fact[n], 1);
        if (seen == NULL) {
            fail("out of memory", v, n);
            return;
        }
    }
    for (p = 0; p < n; p++)
        a[p] = p + 1;
    if (full) seen[rank_of(a, n)] = 1;

    while (permgen_next(&g, &i, &j)) {
        if (i == j || i < 0 || j < 0 || i >= n || j >= n || i > j)
            bad = 1;
        if (full) memcpy(prev, a, sizeof(int) * (size_t)n);
        t = a[i];
        a[i] = a[j];
        a[j] = t;
        count++;
        if (full) {
            diff = 0;
            for (p = 0; p < n; p++)
                diff += (a[p] != prev[p]);
            if (diff != 2) bad = 1;
            r = rank_of(a, n);
            if (seen[r]) bad = 1; /* repeated arrangement */
            seen[r] = 1;
        }
    }
    if (bad) fail("repeated arrangement or wrong swap", v, n);
    if (count != fact[n]) fail("wrong number of arrangements", v, n);

    /* net permutation reported by the library vs. closed form */
    permgen_net(&g, pi);
    if (v != PERMGEN_LEXMAX) {
        int want[PERMGEN_NMAX + 1];
        predicted_net(v, n, want);
        for (p = 0; p < n; p++)
            if (pi[p] != want[p + 1] - 1) {
                fail("net differs from closed form", v, n);
                break;
            }
    }
    /* final array vs. net: item that started at p sits at pi[p] */
    for (p = 0; p < n; p++)
        if (a[pi[p]] != p + 1) {
            fail("final array inconsistent with net", v, n);
            break;
        }

    /* final array properties */
    if (v == PERMGEN_CYCLIC &&
        n % 2 == 0) { /* right rotation by one */
        if (a[0] != n)
            fail("cyclic final array is not a right rotation", v, n);
        for (p = 1; p < n; p++)
            if (a[p] != p) {
                fail("cyclic final array is not a right rotation", v,
                     n);
                break;
            }
    }
    if (n >= 2) {
        if (sign_of(a, n) != -1)
            fail("final arrangement is not an odd permutation", v, n);
    }
    if (v == PERMGEN_LEXMAX && n >= 2) {
        int tgt[PERMGEN_NMAX];
        lexmax_target(n, tgt);
        for (p = 0; p < n; p++)
            if (a[p] != tgt[p]) {
                fail("final array is not the lexicographically "
                     "largest attainable",
                     v, n);
                break;
            }
    }

    /* restore */
    permgen_restore(&g, a);
    for (p = 0; p < n; p++)
        if (a[p] != p + 1) {
            fail("permgen_restore failed", v, n);
            break;
        }

    printf("%s n=%2d : %llu arrangements, final array", vname(v), n,
           count);
    { /* final array = inverse of the net permutation */
        int fin[PERMGEN_NMAX];
        for (p = 0; p < n; p++)
            fin[pi[p]] = p + 1;
        for (p = 0; p < n && p < 12; p++)
            printf(" %d", fin[p]);
        if (n > 12) printf(" ...");
    }
    printf("%s\n", full ? ", all distinct" : "");
    free(seen);
}

/* ---- smaller targeted tests ---- */

static int visit_count(const int *a, int n, void *user)
{
    (void)a;
    (void)n;
    ++*(unsigned long long *)user;
    return 0;
}
static int visit_stop_at_5(const int *a, int n, void *user)
{
    (void)a;
    (void)n;
    ++*(unsigned long long *)user;
    return *(unsigned long long *)user >= 5;
}

static void test_misc(void)
{
    permgen g;
    int a[PERMGEN_NMAX], p, i, j, rc;
    unsigned long long cnt;
    static const char *want3[6] = {"123", "213", "312",
                                   "132", "231", "321"};
    char buf[8];
    int k;

    /* known sequence for n = 3 (Table 2 of the paper), all
     * variants */
    for (k = 0; k < 2; k++) { /* LEXMAX is not defined for odd n */
        permgen_variant v = (permgen_variant)k;
        permgen_init(&g, 3, v);
        for (p = 0; p < 3; p++)
            a[p] = p + 1;
        for (cnt = 0; cnt < 6; cnt++) {
            for (p = 0; p < 3; p++)
                buf[p] = (char)('0' + a[p]);
            buf[3] = '\0';
            if (strcmp(buf, want3[cnt]) != 0) {
                fail("n=3 sequence differs", v, 3);
                break;
            }
            if (cnt < 5) {
                permgen_next(&g, &i, &j);
                {
                    int t = a[i];
                    a[i] = a[j];
                    a[j] = t;
                }
            }
        }
    }
    printf("known n=3 sequence 123 213 312 132 231 321: checked\n");

    /* permgen_run: count and early stop */
    for (p = 0; p < 6; p++)
        a[p] = p + 1;
    cnt = 0;
    rc = permgen_run(a, 6, PERMGEN_CLASSIC, visit_count, &cnt, NULL);
    if (rc != PERMGEN_OK || cnt != 720)
        fail("permgen_run count", PERMGEN_CLASSIC, 6);
    for (p = 0; p < 6; p++)
        a[p] = p + 1;
    cnt = 0;
    rc = permgen_run(a, 6, PERMGEN_CYCLIC, visit_stop_at_5, &cnt,
                     NULL);
    if (rc != PERMGEN_OK || cnt != 5)
        fail("permgen_run early stop", PERMGEN_CYCLIC, 6);
    printf("permgen_run: full count and early stop: checked\n");

    /* two consecutive sweeps after restore give identical
     * sequences */
    {
        int first[120][5], q, same = 1;
        permgen_init(&g, 5, PERMGEN_CYCLIC);
        for (p = 0; p < 5; p++)
            a[p] = p + 1;
        cnt = 0;
        memcpy(first[cnt++], a, sizeof first[0]);
        while (permgen_next(&g, &i, &j)) {
            int t = a[i];
            a[i] = a[j];
            a[j] = t;
            memcpy(first[cnt++], a, sizeof first[0]);
        }
        permgen_restore(&g, a);
        cnt = 0;
        while (permgen_next(&g, &i, &j)) {
            int t = a[i];
            a[i] = a[j];
            a[j] = t;
            cnt++;
            for (q = 0; q < 5; q++)
                if (first[cnt][q] != a[q]) same = 0;
        }
        if (!same || cnt != 119)
            fail("second sweep differs from first", PERMGEN_CYCLIC,
                 5);
    }
    printf("repeatable sweeps after permgen_restore: checked\n");

    /* error handling */
    if (permgen_init(NULL, 4, PERMGEN_CLASSIC) != PERMGEN_E_ARGUMENT)
        fail("NULL generator", PERMGEN_CLASSIC, 4);
    if (permgen_init(&g, 0, PERMGEN_CLASSIC) != PERMGEN_E_ARGUMENT)
        fail("n = 0", PERMGEN_CLASSIC, 0);
    if (permgen_init(&g, PERMGEN_NMAX + 1, PERMGEN_CLASSIC) !=
        PERMGEN_E_ARGUMENT)
        fail("n too large", PERMGEN_CLASSIC, PERMGEN_NMAX + 1);
    if (permgen_init(&g, 4, (permgen_variant)7) != PERMGEN_E_ARGUMENT)
        fail("bad variant", PERMGEN_CLASSIC, 4);
    if (permgen_init(&g, 5, PERMGEN_LEXMAX) != PERMGEN_E_UNSUPPORTED)
        fail("LEXMAX odd n", PERMGEN_LEXMAX, 5);
    if (permgen_run(NULL, 4, PERMGEN_CLASSIC, visit_count, &cnt,
                    NULL) != PERMGEN_E_ARGUMENT)
        fail("NULL array", PERMGEN_CLASSIC, 4);
    printf("error codes: checked\n");

    /* n = PERMGEN_NMAX: initialisation only (cannot enumerate 20!
     * arrangements) */
    for (k = 0; k < 3; k++) {
        permgen_variant v = (permgen_variant)k;
        rc = permgen_init(&g, PERMGEN_NMAX, v);
        if (rc != PERMGEN_OK)
            fail("init at n = PERMGEN_NMAX", v, PERMGEN_NMAX);
        else { /* first 1000 swaps are legal and the net is a
                  permutation with odd sign */
            int pi[PERMGEN_NMAX], one[PERMGEN_NMAX],
                used[PERMGEN_NMAX + 1], ok = 1;
            for (p = 0; p <= PERMGEN_NMAX; p++)
                used[p] = 0;
            permgen_net(&g, pi);
            for (p = 0; p < PERMGEN_NMAX; p++) {
                if (pi[p] < 0 || pi[p] >= PERMGEN_NMAX || used[pi[p]])
                    ok = 0;
                else
                    used[pi[p]] = 1;
                one[p] = pi[p] + 1;
            }
            if (!ok || sign_of(one, PERMGEN_NMAX) != -1)
                fail("net at n = PERMGEN_NMAX", v, PERMGEN_NMAX);
            for (cnt = 0; cnt < 1000; cnt++)
                if (!permgen_next(&g, &i, &j) || i >= j ||
                    j >= PERMGEN_NMAX) {
                    fail("swap range at NMAX", v, PERMGEN_NMAX);
                    break;
                }
        }
    }
    printf("n = %d (initialisation, net, first swaps): checked\n",
           PERMGEN_NMAX);
}

int main(int argc, char **argv)
{
    int n, k, long_run = (argc > 1 && strcmp(argv[1], "-long") == 0);
    int maxn = long_run ? 12 : MAXN_EXHAUSTIVE;

    fact[0] = 1;
    for (n = 1; n <= PERMGEN_NMAX; n++)
        fact[n] = fact[n - 1] * (unsigned long long)n;

    for (k = 0; k < 3; k++) {
        permgen_variant v = (permgen_variant)k;
        for (n = 1; n <= maxn; n++) {
            if (v == PERMGEN_LEXMAX && n > 1 && n % 2 == 1)
                continue; /* even n only */
            check_sweep(v, n, n <= MAXN_EXHAUSTIVE);
        }
    }
    test_misc();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d FAILURE(S)\n", failures);
    return 1;
}
