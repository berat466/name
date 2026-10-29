#!/usr/bin/env python3
"""Geri yayilimsiz YEREL kural arama (gizli katmana hata sabit rastgele geri baglantiyla tasinir).

Her parametre tensoru icin formul su degiskenleri gorur (hepsi agirlik boyutunda):
  g  = ort(giris * hata)      yerel delta carpimi (gradyan YOK; gizli katmanda hata rastgele geri baglantidan)
  xs = giris gucu (rms)       es = hata gucu (rms)
  u  = bir onceki degisim     w  = deger      q = g^2'nin hareketli ort. (buyukluk hafizasi)
  a  = capa (gorev baslamadan onceki agirlik; ilk gorevde cekmez)
  m  = g'nin hareketli ort.   (YALNIZ --mode full; bagimsiz modda yok)
Guncelleme: w <- w - lr * formul(...)

  python lokal_arama.py --mode indep --pure-random
  python lokal_arama.py --mode full
"""
import argparse
import math
import random
import sys
from multiprocessing import Pool

import numpy as np

import optim_search as o
import optim_suite as s
from optim_search import B, C, U, V, crossover, evaluate, mutate, rand_tree, size, to_str

LRS = [0.003, 0.01, 0.03, 0.1, 0.3, 1.0]
EPS = C(1e-8)
VARS_INDEP = ["g", "xs", "es", "u", "w", "q", "a"]
VARS_FULL = VARS_INDEP + ["m"]


def _loss(X, y, P, reg):
    W1, b1, W2, b2 = P
    z = np.tanh(X @ W1 + b1) @ W2 + b2
    if reg:
        return float(((z[:, 0] - y) ** 2).mean())
    z = z - z.max(1, keepdims=True); p = np.exp(z); p /= p.sum(1, keepdims=True)
    return float(-np.log(p[np.arange(len(y)), y] + 1e-12).mean())


def run_local(expr, lr, task):
    net = s.Net(task["H"], task["out"], task["seed"] % 1000)
    P = [p.copy() for p in net.parts()]
    Uu = [np.zeros_like(p) for p in P]; Q = [np.zeros_like(p) for p in P]; M = [np.zeros_like(p) for p in P]
    H, out, reg = task["H"], task["out"], task["reg"]
    rng = np.random.default_rng(task["seed"] + 5)
    Bf = rng.normal(0, 1.0, (out, H)) / np.sqrt(out)
    l0 = [_loss(X, y, P, reg) for X, y, _ in task["phases"]]
    t = 0
    with np.errstate(all="ignore"):
        for pi, (X, y, _) in enumerate(task["phases"]):
            n = len(y)
            anchors = [p.copy() for p in P]
            for _ in range(task["steps"]):
                t += 1
                if pi == 0:                         # ilk gorevde capa cekmez
                    anchors = P
                W1, b1, W2, b2 = P
                a1 = np.tanh(X @ W1 + b1); z = a1 @ W2 + b2
                if reg:
                    e = z - y[:, None]
                else:
                    zz = z - z.max(1, keepdims=True); e = np.exp(zz); e /= e.sum(1, keepdims=True)
                    e[np.arange(n), y] -= 1
                eh = (e @ Bf) * (1 - a1 * a1)                # sabit rastgele geri baglanti
                ones = np.ones((n, 1))
                specs = [(X, eh), (ones, eh), (a1, e), (ones, e)]
                for i, (xin, err) in enumerate(specs):
                    g = xin.T @ err / n
                    if P[i].ndim == 1:
                        g = g.ravel()
                    xs = np.sqrt((xin ** 2).mean(0)); es = np.sqrt((err ** 2).mean(0))
                    if P[i].ndim == 2:
                        xs = np.broadcast_to(xs[:, None], P[i].shape); es = np.broadcast_to(es[None, :], P[i].shape)
                    else:
                        xs = np.ones_like(P[i]); es = np.broadcast_to(es, P[i].shape)
                    M[i] = 0.9 * M[i] + 0.1 * g; Q[i] = 0.999 * Q[i] + 0.001 * g * g
                    env = {"g": g, "xs": np.array(xs), "es": np.array(es), "u": Uu[i], "w": P[i],
                           "q": Q[i] / (1 - 0.999 ** t), "m": M[i] / (1 - 0.9 ** t), "a": anchors[i]}
                    D = evaluate(expr, env)
                    Uu[i] = D
                    P[i] = P[i] - lr * D
                if not all(np.isfinite(p).all() for p in P):
                    return s.CLIP
                if pi == 0:
                    anchors = P
    lf = [_loss(X, y, P, reg) for X, y, _ in task["phases"]]
    return float(np.mean([min(s.CLIP, f / max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0)]))


def score_local(args):
    expr, seeds = args
    tasks = [s.make_task(sd) for sd in seeds]
    best = None
    for lr in LRS:
        rs = np.array([run_local(expr, lr, t) for t in tasks])
        g = float(np.exp(np.mean(np.log(np.maximum(rs, 1e-4)))))
        if best is None or g < best[0]:
            best = (g, lr, rs)
    return best


def baselines(mode):
    d = {
        "HB-FA (g+0.9u)": B("+", V("g"), B("*", C(0.9), V("u"))),
        "RMS-FA g/(sqrt q)": B("/", V("g"), B("+", U("sqrt", V("q")), EPS)),
        "RMS-FA+Capa": B("+", B("/", V("g"), B("+", U("sqrt", V("q")), EPS)), B("*", C(0.1), B("-", V("w"), V("a")))),
    }
    if mode == "full":
        adam = B("/", V("m"), B("+", U("sqrt", V("q")), EPS))
        d["Adam-FA"] = adam
        d["Adam-FA+Capa"] = B("+", adam, B("*", C(0.1), B("-", V("w"), V("a"))))
    return d


def evolve(mode, pop_n, gens, tpg, pure_random, rng, pool):
    seeds_pop = [] if pure_random else list(baselines(mode).values())
    pop = list(seeds_pop)
    while len(pop) < pop_n:
        base = rng.choice(seeds_pop) if (seeds_pop and rng.random() < 0.5) else None
        pop.append(mutate(base, rng) if base is not None else rand_tree(rng, 3))
    hall = {}
    for gen in range(gens):
        seeds = [rng.randrange(10 ** 6) for _ in range(tpg)]
        res = pool.map(score_local, [(e, seeds) for e in pop])
        scored = sorted(zip(pop, res), key=lambda er: er[1][0] + 0.003 * size(er[0]))
        for e, r in scored:
            k = to_str(e)
            if k not in hall or r[0] < hall[k][1]:
                hall[k] = (e, r[0])
        print(f"nesil {gen+1:2}/{gens}: en iyi={scored[0][1][0]:.3f} (lr={scored[0][1][1]})  {to_str(scored[0][0])[:100]}", flush=True)
        elite = [e for e, _ in scored[:max(4, pop_n // 8)]]
        nxt = list(elite)
        while len(nxt) < pop_n:
            a, b = rng.sample(scored[:max(8, pop_n // 2)], 2)
            child = crossover(a[0], b[0], rng) if rng.random() < 0.4 else mutate(a[0], rng)
            if rng.random() < 0.3:
                child = mutate(child, rng)
            nxt.append(child)
        pop = nxt
    return hall


def main(argv=None):
    p = argparse.ArgumentParser()
    p.add_argument("--mode", choices=["indep", "full"], default="indep")
    p.add_argument("--pop", type=int, default=36)
    p.add_argument("--gens", type=int, default=12)
    p.add_argument("--tasks-per-gen", type=int, default=8)
    p.add_argument("--val-tasks", type=int, default=200)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--pure-random", action="store_true")
    p.add_argument("--out", default=None)
    a = p.parse_args(argv)
    o.VARS = VARS_INDEP if a.mode == "indep" else VARS_FULL
    rng = random.Random(a.seed)
    with Pool() as pool:
        hall = evolve(a.mode, a.pop, a.gens, a.tasks_per_gen, a.pure_random, rng, pool)
        base = baselines(a.mode)
        bn = {to_str(e) for e in base.values()}
        top = sorted((v for k, v in hall.items() if k not in bn), key=lambda ev: ev[1])[:6]
        cands = list(base.items()) + [(f"KESIF#{i+1}", e) for i, (e, _) in enumerate(top)]
        vs = [10 ** 7 + i for i in range(a.val_tasks)]
        res = pool.map(score_local, [(e, vs) for _, e in cands])
        ref = s.suite_scores((o.BASELINES["Adam"], vs))              # klasik Adam (geri yayilimli) referans
    cont = np.array([len(s.make_task(sd)["phases"]) > 1 for sd in vs])
    gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(x, 1e-4)))))
    rows = ["| Ad | Formul | lr | TUM | klasik Adam(bp)'e gore | ARDISIK | klasik Adam(bp)'e gore |", "|---|---|---|---|---|---|---|",
            f"| KLASIK Adam (geri yayilimli) | `m/(sqrt(v)+eps)` | {ref[1]} | {ref[0]:.4f} | 0% | {gm(ref[2][cont]):.4f} | 0% |"]
    for (name, e), (g, lr, rs) in zip(cands, res):
        gc = gm(rs[cont])
        rows.append(f"| {name} | `{to_str(e)[:62]}` | {lr} | {g:.4f} | {(1 - g / ref[0]) * 100:+.0f}% | {gc:.4f} | {(1 - gc / gm(ref[2][cont])) * 100:+.0f}% |")
    text = "\n".join(rows)
    print(f"\nDOGRULAMA: {a.val_tasks} gorunmemis gorev ({int(cont.sum())} ardisik), mod={a.mode}, tohum={'yok' if a.pure_random else 'var'}\n")
    print(text)
    if a.out:
        open(a.out, "w", encoding="utf-8").write(text + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
