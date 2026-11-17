/*
 * example.c -- short example driver for the PERMGEN package.
 *
 * Shows (1) the iterator interface, (2) the callback interface,
 * (3) permgen_net() and permgen_restore(), (4) the final arrangement
 * left by each variant.  The program reads no input; its output is
 * compared with examples/example.expected by "make check".
 */
#include <stdio.h>
#include "../src/permgen.h"

static void print_array(const int *a, int n)
{
    int p;
    for (p = 0; p < n; p++)
        printf("%d%c", a[p], p + 1 < n ? ' ' : '\n');
}

/* callback used by permgen_run(): print every arrangement */
static int show(const int *a, int n, void *user)
{
    (void)user;
    print_array(a, n);
    return 0;
}

int main(void)
{
    static const char *names[3] = {"CLASSIC", "CYCLIC ", "LEXMAX "};
    int v, p, i, j, t, n = 4, a[4], pi[4];
    permgen g;
    unsigned long long count;

    /* (1) iterator interface: all 24 arrangements of 1 2 3 4, cyclic
     * variant */
    printf("== iterator interface, n = %d, CYCLIC ==\n", n);
    for (p = 0; p < n; p++)
        a[p] = p + 1;
    if (permgen_init(&g, n, PERMGEN_CYCLIC) != PERMGEN_OK) return 1;
    print_array(a, n);
    while (permgen_next(&g, &i, &j)) {
        t = a[i];
        a[i] = a[j];
        a[j] = t;
        print_array(a, n);
    }
    printf("-- last arrangement above; net permutation (0-based "
           "one-line):");
    permgen_net(&g, pi);
    for (p = 0; p < n; p++)
        printf(" %d", pi[p]);
    printf("\n");
    permgen_restore(&g, a);
    printf("-- after permgen_restore:");
    for (p = 0; p < n; p++)
        printf(" %d", a[p]);
    printf("\n\n");

    /* (2) callback interface for n = 3 */
    printf("== callback interface, n = 3, CLASSIC ==\n");
    {
        int b[3] = {1, 2, 3};
        if (permgen_run(b, 3, PERMGEN_CLASSIC, show, NULL, &count) !=
            PERMGEN_OK)
            return 1;
        printf("-- %llu arrangements visited\n\n", count);
    }

    /* (3) final arrangement of every variant for n = 6 */
    printf("== final arrangement after a complete sweep, n = 6, "
           "started from 1..6 ==\n");
    for (v = 0; v < 3; v++) {
        int b[6] = {1, 2, 3, 4, 5, 6};
        if (permgen_init(&g, 6, (permgen_variant)v) != PERMGEN_OK)
            return 1;
        while (permgen_next(&g, &i, &j)) {
            t = b[i];
            b[i] = b[j];
            b[j] = t;
        }
        printf("%s ", names[v]);
        print_array(b, 6);
    }
    return 0;
}
