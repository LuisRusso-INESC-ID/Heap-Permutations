#!/usr/bin/env python3
"""Check the computational claims of the paper.

Permutations are tuples p with p[0] unused and p[x] the image of x.
Products compose right to left: mul(a, b)(x) = a(b(x)).
"""
import sys


def ident(n):
    return tuple(range(n + 1))


def mul(a, b):
    return tuple(a[b[x]] for x in range(len(b)))


def inv(a):
    r = [0] * len(a)
    for x in range(len(a)):
        r[a[x]] = x
    return tuple(r)


def power(a, e):
    if e < 0:
        return power(inv(a), -e)
    r = ident(len(a) - 1)
    while e:
        if e & 1:
            r = mul(a, r)
        a = mul(a, a)
        e >>= 1
    return r


def cycle(n, pts):
    r = list(range(n + 1))
    for i, x in enumerate(pts):
        r[x] = pts[(i + 1) % len(pts)]
    return tuple(r)


def transp(n, i, j):
    return cycle(n, [i, j])


def is_full_cycle(a):
    n = len(a) - 1
    x, length = n, 0
    while True:
        x = a[x]
        length += 1
        if x == n:
            return length == n


def cycles(a):
    seen, out = set(), []
    for x in range(1, len(a)):
        if x in seen or a[x] == x:
            seen.add(x)
            continue
        c = [x]
        seen.add(x)
        while a[c[-1]] != x:
            c.append(a[c[-1]])
            seen.add(c[-1])
        out.append(tuple(c))
    return out


def best_array(n):
    rev = list(range(n, 0, -1))
    if n >= 2 and n % 4 in (0, 1):
        rev[-2], rev[-1] = 1, 2
    return tuple([0] + rev)


def target(n):
    return inv(best_array(n))


def cyclic_net(k):
    if k == 1:
        return ident(1)
    if k % 2 == 1:
        return transp(k, 1, k)
    return cycle(k, list(range(1, k + 1)))


def extend(a, n):
    return tuple(list(a) + list(range(len(a), n + 1)))


def net_from_order(c, order):
    k = len(order) + 1
    c = extend(c, k)
    return mul(power(c, k), cycle(k, [k] + list(order)))


def schedule_from_order(c, order):
    k = len(order) + 1
    c = extend(c, k)
    theta, s = ident(k), []
    for e in order:
        s.append(c[theta[e]])
        theta = mul(transp(k, s[-1], k), mul(c, theta))
    return s


def lexmax_order_low(n):
    if n % 4 == 0:
        return list(range(1, n - 1))
    m = (n - 2) // 2
    return list(range(1, m + 1)) + list(range(n - 2, m, -1))


def lexmax_top(n):
    a = cyclic_net(n - 2)
    low = lexmax_order_low(n)
    u = net_from_order(a, low)
    z = mul(power(extend(u, n), -n), target(n))
    return a, low, u, z


def order_of_cycle(z):
    n = len(z) - 1
    out, x = [], z[n]
    while x != n:
        out.append(x)
        x = z[x]
    return out


def check(name, ok):
    print(("ok   " if ok else "FAIL ") + name)
    return ok


def claim_thm_lexmax_c(nmax):
    bad = [n for n in range(6, nmax + 1, 2)
           if not is_full_cycle(lexmax_top(n)[3])]
    return check("Z_n is an n-cycle, even 6<=n<=%d" % nmax, not bad)


def lemma_u(n):
    odd = list(range(1, n - 2, 2))
    even = list(range(2, n - 1, 2)) + [n - 1]
    return mul(cycle(n - 1, odd), cycle(n - 1, even))


def claim_lemma_u(nmax):
    ok = all(lexmax_top(n)[2] == lemma_u(n)
             for n in range(4, nmax + 1, 4))
    return check("Lemma U, n = 0 mod 4, n<=%d" % nmax, ok)


def u_mod2(n):
    m = (n - 2) // 2
    pts = list(range(1, m, 2)) + [m + 1, n - 1]
    return cycle(n - 1, pts + list(range(2, m + 1, 2)))


def claim_u_mod2(nmax):
    ok = all(lexmax_top(n)[2] == u_mod2(n)
             for n in range(6, nmax + 1, 4))
    return check("pi_{n-1}, n = 2 mod 4, n<=%d" % nmax, ok)


def claim_per_level():
    def ok_at(k):
        z = mul(power(extend(target(k - 1), k), -k), target(k))
        return is_full_cycle(z)
    fails = all(not ok_at(k) for k in range(8, 41, 2))
    odd = all(ok_at(k) for k in range(3, 40, 2))
    small = all(ok_at(k) for k in range(2, 8))
    return check("per-level optimality remark", fails and odd
                 and small)


def claim_odd_n():
    def ok_at(n):
        z = mul(power(extend(cyclic_net(n - 1), n), -n),
                target(n))
        return is_full_cycle(z)
    good = ok_at(3) and ok_at(5)
    bad = all(not ok_at(n) for n in range(7, 32, 2))
    return check("odd n remark", good and bad)


def heap_states(n, sched):
    state, out = list(range(n + 1)), []

    def run(k):
        if k == 1:
            out.append(tuple(state))
            return
        run(k - 1)
        for i in range(1, k):
            p = sched[k][i - 1]
            for x in range(1, n + 1):
                if state[x] == p:
                    state[x] = k
                elif state[x] == k:
                    state[x] = p
            run(k - 1)

    run(n)
    return out


def classic_sched(n):
    return {k: [1] * (k - 1) if k % 2 else list(range(1, k))
            for k in range(2, n + 1)}


def cyclic_sched(n):
    def row(k):
        if k % 2:
            return [1] * (k - 1)
        return [k - 1] + list(range(2, k - 1)) + [1] if k > 2 else [1]
    return {k: row(k) for k in range(2, n + 1)}


def lexmax_sched(n):
    sched = cyclic_sched(n)
    if n < 4:
        return sched
    a, low, u, z = lexmax_top(n)
    sched[n - 1] = schedule_from_order(a, low)
    sched[n] = schedule_from_order(u, order_of_cycle(z))
    return sched


def valid_run(n, sched, final_state):
    st = heap_states(n, sched)
    if len(set(st)) != len(st) or len(st) != factorial(n):
        return False
    for x, y in zip(st, st[1:]):
        if sum(1 for i in range(1, n + 1) if x[i] != y[i]) != 2:
            return False
    return st[-1] == final_state


def factorial(n):
    return 1 if n < 2 else n * factorial(n - 1)


def classic_net(k):
    if k == 1:
        return ident(1)
    if k % 2:
        return transp(k, 1, k)
    if k == 2:
        return transp(2, 1, 2)
    return cycle(k, [1, k, k - 1] + list(range(2, k - 1)))


def claim_table_nets(nmax):
    ok = all(valid_run(k, classic_sched(k), classic_net(k))
             and valid_run(k, cyclic_sched(k), cyclic_net(k))
             for k in range(2, nmax + 1))
    return check("Table nets, exhaustive runs k<=%d" % nmax, ok)


def claim_lexmax_runs(nmax):
    ok = all(valid_run(n, lexmax_sched(n), target(n))
             for n in range(2, nmax + 1, 2))
    return check("extremal variant, exhaustive runs n<=%d" % nmax,
                 ok)


def table_lex_row(n):
    a, low, u, z = lexmax_top(n)
    top = order_of_cycle(z)
    return (low, schedule_from_order(a, low), top,
            schedule_from_order(u, top), cycles(target(n)))


TABLE_LEX = {
    4: ([1, 2], [2, 2], [1, 3, 2], [1, 3, 3], [(1, 3, 2, 4)]),
    6: ([1, 2, 4, 3], [2, 4, 3, 3], [5, 3, 4, 2, 1],
        [2, 2, 4, 2, 3], [(1, 6), (2, 5), (3, 4)]),
    8: ([1, 2, 3, 4, 5, 6], [2, 4, 6, 2, 4, 6],
        [3, 6, 5, 4, 1, 7, 2], [5, 2, 5, 4, 5, 4, 7],
        [(1, 7, 2, 8), (3, 6), (4, 5)]),
    10: ([1, 2, 3, 4, 8, 7, 6, 5], [2, 4, 6, 8, 5, 5, 5, 5],
         [5, 6, 2, 4, 7, 3, 8, 9, 1], [9, 6, 3, 9, 7, 3, 8, 4, 9],
         [(1, 10), (2, 9), (3, 8), (4, 7), (5, 6)]),
}


def claim_table_lex():
    ok = all(tuple(table_lex_row(n)) == v
             for n, v in TABLE_LEX.items())
    net12 = [(1, 11, 2, 12), (3, 10), (4, 9), (5, 8), (6, 7)]
    ok = ok and cycles(target(12)) == net12
    return check("Table of extremal schedules", ok)


def claim_count(nmax):
    def count(n):
        r = 1
        for k in range(2, n + 1):
            r *= factorial(k - 1)
        return r
    ok = True
    for n in range(2, nmax + 1):
        lows = all_families(n)
        ok = ok and len(lows) == count(n)
    return check("Counting corollary, n<=%d" % nmax, ok)


def all_families(n):
    from itertools import product
    rows = [list(product(range(1, k), repeat=k - 1))
            for k in range(2, n + 1)]
    good = []
    for fam in product(*rows):
        sched = {k: list(fam[k - 2]) for k in range(2, n + 1)}
        st = heap_states(n, sched)
        if len(set(st)) == len(st):
            good.append(fam)
    return good


def main():
    nmax = int(sys.argv[1]) if len(sys.argv) > 1 else 10000
    results = [
        claim_table_nets(7),
        claim_table_lex(),
        claim_lexmax_runs(8),
        claim_lemma_u(nmax),
        claim_u_mod2(2000),
        claim_thm_lexmax_c(nmax),
        claim_per_level(),
        claim_odd_n(),
        claim_count(4),
    ]
    sys.exit(0 if all(results) else 1)


if __name__ == "__main__":
    main()
