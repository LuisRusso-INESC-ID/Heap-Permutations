/*
 * permgen.c -- implementation of the PERMGEN package (see permgen.h).
 *
 * Terminology (see the accompanying paper):
 *   level k     the call Heap(k) acting on positions 1..k (1-based
 * here); round i     in Heap(k): swap positions s_k(i) and k, then
 * run Heap(k-1); net         permutation that a complete call applies
 * to the array; parking order  (e_1,...,e_{k-1}): the sequence of
 * items that sit at position k during rounds 0,1,...,k-1 of level k
 * (e_0 = k).
 *
 * INTERNAL STORAGE CONVENTION
 *   All permutations in this file are 1-based one-line arrays: p[x]
 * is the image of x, 1 <= x <= m.  The public interface converts to
 * 0-based.
 *
 * INTERNAL VARIABLES (alphabetical, used consistently in all
 * routines) c      net permutation of level k-1 (length k-1) cc     c
 * extended to length k by cc[k] = k e      parking order, e[1..k-1],
 * a permutation of 1..k-1 k      level m      round counter s
 * schedule of one level, s[1..k-1] theta  state at the start of the
 * current round (one-line, length k) u, v   scratch values x loop
 * index over positions
 *
 * No global variables, no dynamic allocation, no machine-dependent
 * constants other than PERMGEN_NMAX.
 */
#include <string.h>
#include "permgen.h"

/* ---------------------------------------------------------------- */
/* Schedules of the CLASSIC and CYCLIC variants */
/* ---------------------------------------------------------------- */

/* Position (1-based) swapped with position k in round i of level k.
 *   odd k          : always position 1
 *   even k, CLASSIC: position i
 *   even k, CYCLIC : positions k-1, 2, 3, ..., k-2, 1 in
 * rounds 1..k-1 */
static int partner(permgen_variant v, int k, int i)
{
    if (k % 2 == 1) return 1;
    if (v == PERMGEN_CLASSIC) return i;
    if (i == 1) return k - 1;
    if (i == k - 1) return 1;
    return i;
}

/* Fill the schedules of levels kfrom..kto with the rule of variant
 * v. */
static void fill_standard(permgen *g, permgen_variant v, int kfrom,
                          int kto)
{
    int k, i;
    for (k = kfrom; k <= kto; k++)
        for (i = 1; i < k; i++)
            g->sched[k][i] = partner(v, k, i);
}

/* ---------------------------------------------------------------- */
/* Net permutations from schedules */
/* ---------------------------------------------------------------- */

/* One step of the recursion: theta := tau_m o C o theta, where tau_m
 * is the transposition (s_m k) and C the net of level k-1 (cc, with
 * cc[k] = k). In one-line notation: map every entry through cc, then
 * exchange the values s_m and k. */
static void advance(int k, const int *cc, int sm, int *theta)
{
    int x, v;
    for (x = 1; x <= k; x++) {
        v = cc[theta[x]];
        if (v == sm)
            v = k;
        else if (v == k)
            v = sm;
        theta[x] = v;
    }
}

/* Net permutation of level k:  pi_k = C tau_{k-1} C ... tau_1 C
 *   c   (in)  net of level k-1, c[1..k-1]
 *   s   (in)  schedule of level k, s[1..k-1]
 *   out (out) net of level k, out[1..k]      (out must not alias c)
 */
static void level_net(int k, const int *c, const int *s, int *out)
{
    int cc[PERMGEN_NMAX + 1], theta[PERMGEN_NMAX + 1];
    int x, m;
    for (x = 1; x < k; x++)
        cc[x] = c[x];
    cc[k] = k;
    for (x = 1; x <= k; x++)
        theta[x] = x;
    for (m = 1; m < k; m++)
        advance(k, cc, s[m], theta);
    for (x = 1; x <= k; x++)
        out[x] = cc[theta[x]];
}

/* Schedule of level k that parks the items e_1,...,e_{k-1} (in this
 * order) at position k during rounds 1..k-1.  By the net-permutation
 * theorem of the paper there is exactly one such schedule for every
 * ordering e: s_m = C(theta_{m-1}(e_m)). c (in) net of level k-1;  e
 * (in) permutation of 1..k-1;  s (out) */
static void schedule_from_order(int k, const int *c, const int *e,
                                int *s)
{
    int cc[PERMGEN_NMAX + 1], theta[PERMGEN_NMAX + 1];
    int x, m;
    for (x = 1; x < k; x++)
        cc[x] = c[x];
    cc[k] = k;
    for (x = 1; x <= k; x++)
        theta[x] = x;
    for (m = 1; m < k; m++) {
        s[m] = cc[theta[e[m]]];
        advance(k, cc, s[m], theta);
    }
}

/* ---------------------------------------------------------------- */
/* The LEXMAX variant (even n) */
/* ---------------------------------------------------------------- */

/*
 * Construction (n even, n >= 4); levels are numbered as in the paper.
 *   levels 2..n-2   cyclic schedule
 *   level  n-1      parking order  1,2,...,n-2                  if n
 * = 0 mod 4 1,..,m, n-2, n-3, ..., m+1   if n = 2 mod 4, m = (n-2)/2
 *   level  n        the unique parking order for which the net is the
 *                   state T of the target array, i.e. the cycle
 *                   Z = C^{-n} T  (C = net of level n-1).
 * The target array is the lexicographically largest odd permutation:
 * n, n-1, ..., 1 if n = 2 mod 4, and n, n-1, ..., 3, 1, 2 if n = 0
 * mod 4. Returns PERMGEN_OK, or PERMGEN_E_INTERNAL if Z is not an
 * n-cycle (this has been verified not to happen for any even n <=
 * 800).
 */
static int build_lexmax(permgen *g)
{
    int n = g->n;
    int nets[PERMGEN_NMAX + 1][PERMGEN_NMAX + 1];
    int e[PERMGEN_NMAX + 1] = {0}, t[PERMGEN_NMAX + 1] = {0};
    int z[PERMGEN_NMAX + 1] = {0}, cinv[PERMGEN_NMAX + 1] = {0};
    int k, x, m, r, half;

    if (n == 1) return PERMGEN_OK;
    if (n == 2) {
        fill_standard(g, PERMGEN_CYCLIC, 2, 2);
        return PERMGEN_OK;
    }

    /* levels 2..n-2: cyclic */
    fill_standard(g, PERMGEN_CYCLIC, 2, n - 2);
    nets[1][1] = 1;
    for (k = 2; k <= n - 2; k++)
        level_net(k, nets[k - 1], g->sched[k], nets[k]);

    /* level n-1: prescribed parking order of the items 1..n-2 */
    if (n % 4 == 0) {
        for (x = 1; x <= n - 2; x++)
            e[x] = x;
    } else {
        half = (n - 2) / 2;
        for (x = 1; x <= half; x++)
            e[x] = x;
        for (x = half + 1; x <= n - 2; x++)
            e[x] = (n - 2) - (x - half - 1);
    }
    schedule_from_order(n - 1, nets[n - 2], e, g->sched[n - 1]);
    level_net(n - 1, nets[n - 2], g->sched[n - 1], nets[n - 1]);

    /* level n: T = state of the target array (inverse of the
     * array) */
    for (x = 3; x <= n; x++)
        t[x] = n + 1 - x;
    if (n % 4 == 0) {
        t[1] = n - 1;
        t[2] = n;
    } else {
        t[1] = n;
        t[2] = n - 1;
    }

    /* Z = C^{-n} T, with C = nets[n-1] extended by C(n) = n */
    for (x = 1; x < n; x++)
        cinv[nets[n - 1][x]] = x;
    cinv[n] = n;
    for (x = 1; x <= n; x++)
        z[x] = t[x];
    for (r = 0; r < n; r++)
        for (x = 1; x <= n; x++)
            z[x] = cinv[z[x]];

    /* Z must be an n-cycle (n e_1 e_2 ... e_{n-1}): read off the
     * order */
    x = n;
    for (m = 1; m <= n - 1; m++) {
        x = z[x];
        if (x == n) return PERMGEN_E_INTERNAL;
        e[m] = x;
    }
    if (z[x] != n) return PERMGEN_E_INTERNAL;

    schedule_from_order(n, nets[n - 1], e, g->sched[n]);
    return PERMGEN_OK;
}

/* ---------------------------------------------------------------- */
/* Public interface */
/* ---------------------------------------------------------------- */

int permgen_init(permgen *g, int n, permgen_variant v)
{
    int nets[PERMGEN_NMAX + 1][PERMGEN_NMAX + 1];
    int k, x, rc;

    if (g == NULL || n < 1 || n > PERMGEN_NMAX)
        return PERMGEN_E_ARGUMENT;
    if (v != PERMGEN_CLASSIC && v != PERMGEN_CYCLIC &&
        v != PERMGEN_LEXMAX)
        return PERMGEN_E_ARGUMENT;
    if (v == PERMGEN_LEXMAX && n > 1 && n % 2 == 1)
        return PERMGEN_E_UNSUPPORTED;

    memset(g, 0, sizeof *g);
    g->n = n;
    g->variant = v;

    if (v == PERMGEN_LEXMAX) {
        rc = build_lexmax(g);
        if (rc != PERMGEN_OK) return rc;
    } else {
        fill_standard(g, v, 2, n);
    }

    nets[1][1] = 1;
    for (k = 2; k <= n; k++)
        level_net(k, nets[k - 1], g->sched[k], nets[k]);
    for (x = 1; x <= n; x++)
        g->net[x] = nets[n][x];
    return PERMGEN_OK;
}

int permgen_next(permgen *g, int *i, int *j)
{
    int k;
    for (k = 2; k <= g->n; k++) {
        if (g->rounds[k] < k - 1) {
            g->rounds[k]++;
            *i = g->sched[k][g->rounds[k]] - 1;
            *j = k - 1;
            return 1;
        }
        g->rounds[k] = 0;
    }
    return 0;
}

void permgen_net(const permgen *g, int *pi)
{
    int p;
    for (p = 0; p < g->n; p++)
        pi[p] = g->net[p + 1] - 1;
}

void permgen_restore(const permgen *g, int *a)
{
    int tmp[PERMGEN_NMAX], p;
    for (p = 0; p < g->n; p++)
        tmp[p] = a[g->net[p + 1] - 1];
    for (p = 0; p < g->n; p++)
        a[p] = tmp[p];
}

int permgen_run(int *a, int n, permgen_variant v,
                permgen_visit_fn visit, void *user,
                unsigned long long *count)
{
    permgen g;
    int rc, i, j, t, stop;
    unsigned long long c;

    if (a == NULL || visit == NULL) return PERMGEN_E_ARGUMENT;
    rc = permgen_init(&g, n, v);
    if (rc != PERMGEN_OK) return rc;

    c = 1;
    stop = visit(a, n, user);
    while (!stop && permgen_next(&g, &i, &j)) {
        t = a[i];
        a[i] = a[j];
        a[j] = t;
        c++;
        stop = visit(a, n, user);
    }
    if (count != NULL) *count = c;
    return PERMGEN_OK;
}

const char *permgen_strerror(int code)
{
    switch (code) {
    case PERMGEN_OK:
        return "success";
    case PERMGEN_E_ARGUMENT:
        return "invalid argument";
    case PERMGEN_E_UNSUPPORTED:
        return "variant not defined for this n";
    case PERMGEN_E_INTERNAL:
        return "internal consistency check failed";
    default:
        return "unknown error code";
    }
}
