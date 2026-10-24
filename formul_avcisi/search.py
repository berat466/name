#!/usr/bin/env python3
"""Formul avcisi: matris carpimi icin AZ CARPMALI, KAYIPSIZ (tam esit) algoritma arar.

Fikir: n x n matris carpimi bir 3-boyutlu tensor T. T'yi R tane "tek carpma" terimine
ayirabilirsek (rank-R), algoritma R carpma ile calisir. Sayisal olarak (ALS) ararız,
degerleri {-1,0,1}'e iter, sonra TAM tamsayi aritmetigiyle dogrularız.
Dogrulama gecmeyen hicbir sey "bulgu" sayilmaz.

  python search.py --n 2 --rank 7 --restarts 400     # Strassen'i yeniden bulmayi dener
  python search.py --n 2 --rank 6                    # imkansiz: hicbir sey dogrulanmamali
"""
import argparse
import json
import sys
from multiprocessing import Pool

import numpy as np


def matmul_tensor(n):
    d = n * n
    T = np.zeros((d, d, d))
    for i in range(n):
        for j in range(n):
            for k in range(n):
                T[i * n + j, j * n + k, i * n + k] = 1.0  # a_ij * b_jk -> c_ik
    return T


def khatri_rao(X, Y):
    return np.einsum("ir,jr->ijr", X, Y).reshape(X.shape[0] * Y.shape[0], -1)


def _update(F, D, Y, lam, target):
    """Satir satir: min ||D f - y||^2 + lam ||f - target||^2"""
    R = F.shape[1]
    A = D.T @ D + lam * np.eye(R)
    B = D.T @ Y.T + lam * target.T
    return np.linalg.solve(A, B).T


def als(T, R, rng, iters=600):
    d = T.shape[0]
    U, V, W = (rng.standard_normal((d, R)) * 0.5 for _ in range(3))
    T1 = T.reshape(d, -1)
    T2 = T.transpose(1, 0, 2).reshape(d, -1)
    T3 = T.transpose(2, 0, 1).reshape(d, -1)
    for it in range(iters):
        # lam kucukten buyuge: once yaklastir, sonra {-1,0,1}'e it
        lam = 1e-4 if it < iters // 3 else 1e-4 * (1.0 + 400 * (it - iters // 3) / iters) ** 2
        rnd = it >= iters // 3
        U = _update(U, khatri_rao(V, W), T1, lam, np.clip(np.round(U), -1, 1) if rnd else U)
        V = _update(V, khatri_rao(U, W), T2, lam, np.clip(np.round(V), -1, 1) if rnd else V)
        W = _update(W, khatri_rao(U, V), T3, lam, np.clip(np.round(W), -1, 1) if rnd else W)
    return U, V, W


def to_int(U, V, W):
    Ui, Vi, Wi = (np.clip(np.round(M), -1, 1).astype(np.int64) for M in (U, V, W))
    return Ui, Vi, Wi


def exact_verify(n, U, V, W, trials=200, seed=0):
    """KAYIPSIZ kontrol. 1) Tam kanit: tamsayi tensor, matris carpim tensorune BIREBIR esit mi?
    (esitse algoritma tum girdiler icin dogrudur.) 2) Ek: rastgele tamsayi matrislerle deneme."""
    T = matmul_tensor(n).astype(np.int64)
    if not np.array_equal(np.einsum("ir,jr,kr->ijk", U, V, W), T):
        return False
    rng = np.random.default_rng(seed)
    for _ in range(trials):
        A = rng.integers(-9, 10, (n, n))
        B = rng.integers(-9, 10, (n, n))
        a, b = A.reshape(-1), B.reshape(-1)
        m = (U.T @ a) * (V.T @ b)
        c = W @ m
        if not np.array_equal(c.reshape(n, n), A @ B):
            return False
    return True


def one_try(args):
    n, R, seed = args
    rng = np.random.default_rng(seed)
    T = matmul_tensor(n)
    U, V, W = als(T, R, rng)
    Ui, Vi, Wi = to_int(U, V, W)
    if exact_verify(n, Ui, Vi, Wi):
        return Ui.tolist(), Vi.tolist(), Wi.tolist(), seed
    return None


def count_adds(U, V, W):
    """Toplama/cikarma sayisi: her carpim oncesi ve sonuc birlestirmede."""
    pre = sum(max(int((np.array(col) != 0).sum()) - 1, 0) for M in (U, V) for col in np.array(M).T)
    post = sum(max(int((np.array(row) != 0).sum()) - 1, 0) for row in np.array(W))
    return pre + post


def pretty(n, U, V, W):
    names_a = [f"a{i+1}{j+1}" for i in range(n) for j in range(n)]
    names_b = [f"b{i+1}{j+1}" for i in range(n) for j in range(n)]
    names_c = [f"c{i+1}{j+1}" for i in range(n) for j in range(n)]

    def lin(coefs, names):
        parts = []
        for c, nm in zip(coefs, names):
            if c == 0:
                continue
            parts.append(("+" if c > 0 else "-") + nm)
        s = "".join(parts)
        return s[1:] if s.startswith("+") else s

    U, V, W = np.array(U), np.array(V), np.array(W)
    lines = []
    for r in range(U.shape[1]):
        lines.append(f"m{r+1} = ({lin(U[:, r], names_a)}) * ({lin(V[:, r], names_b)})")
    for k, nm in enumerate(names_c):
        lines.append(f"{nm} = " + (lin(W[k], [f"m{r+1}" for r in range(U.shape[1])]) or "0"))
    return "\n".join(lines)


def main(argv=None):
    p = argparse.ArgumentParser()
    p.add_argument("--n", type=int, default=2)
    p.add_argument("--rank", type=int, default=7)
    p.add_argument("--restarts", type=int, default=300)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--out", default=None, help="bulunan algoritmalari JSON olarak yaz")
    a = p.parse_args(argv)

    jobs = [(a.n, a.rank, a.seed * 100000 + s) for s in range(a.restarts)]
    found = []
    with Pool() as pool:
        for res in pool.imap_unordered(one_try, jobs, chunksize=4):
            if res:
                found.append(res)
    # tekrarlari ele
    uniq = {}
    for U, V, W, seed in found:
        key = json.dumps([U, V, W])
        uniq.setdefault(key, (U, V, W, seed))
    print(f"n={a.n} rank={a.rank} (klasik: {a.n**3} carpma): "
          f"{len(found)}/{a.restarts} deneme DOGRULANDI, {len(uniq)} farkli cozum")
    best = None
    for U, V, W, seed in uniq.values():
        adds = count_adds(U, V, W)
        if best is None or adds < best[0]:
            best = (adds, U, V, W, seed)
    if best:
        adds, U, V, W, seed = best
        print(f"\nEn az toplamali cozum: {a.rank} carpma, {adds} toplama/cikarma (tohum {seed})\n")
        print(pretty(a.n, U, V, W))
        if a.out:
            json.dump({"n": a.n, "rank": a.rank, "adds": adds, "U": U, "V": V, "W": W},
                      open(a.out, "w"))
    return 0 if best else 1


if __name__ == "__main__":
    sys.exit(main())
