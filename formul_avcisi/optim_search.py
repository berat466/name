#!/usr/bin/env python3
"""Optimizer formul simulatoru: semboller rastgele birlestirilir, gercek mini sinir agi egitiminde puanlanir.

Degiskenler (hepsi eleman eleman, tum agirlik vektoru uzerinde):
  g = gradyan      m = momentum (gradyan ort., baslangic duzeltmeli)
  v = gradyan karelerinin ort. (duzeltmeli)   w = agirlik   d = g - onceki g (egrilik ipucu)
Operatorler: + - * / (korumali)  sign tanh sqrt abs sq neg
Guncelleme: w <- w - lr * formul(g, m, v, w, d)   (lr, 7 degerli izgarada taranir:
adil karsilastirma, "sadece buyuk lr" hilesi olmasin)

Puan (dusuk iyi) = L_A1 (A gorevini ogrenme hizi) + L_B2 (B gorevi) + L_A2 (B'den sonra A: unutma).

  python optim_search.py --pop 48 --gens 12                 # baseline tohumlu
  python optim_search.py --pop 48 --gens 12 --pure-random   # tamamen amacsiz baslangic
"""
import argparse
import copy
import math
import random
import sys
from multiprocessing import Pool

import numpy as np

LR_GRID = [0.0003, 0.001, 0.003, 0.01, 0.03, 0.1, 0.3]  # alt uc onemli: uzun egitimde Adam 0.001 ister
VARS = ["g", "m", "v", "w", "d", "a"]
UN = ["neg", "sign", "tanh", "sqrt", "abs", "sq"]
BIN = ["+", "-", "*", "/"]
CONSTS = [0.01, 0.05, 0.1, 0.5, 0.9, 1.0, 2.0, 1e-8]
MAX_NODES = 25


# ----------------------------------------------------------------- ifade agaci
def V(n): return ("v", n)
def C(c): return ("c", c)
def U(op, a): return ("u", op, a)
def B(op, a, b): return ("b", op, a, b)


def size(e):
    if e[0] in ("v", "c"): return 1
    if e[0] == "u": return 1 + size(e[2])
    return 1 + size(e[2]) + size(e[3])


def evaluate(e, env):
    k = e[0]
    if k == "v": return env[e[1]]
    if k == "c": return np.full_like(env["g"], e[1])
    if k == "u":
        x = evaluate(e[2], env); op = e[1]
        if op == "neg": r = -x
        elif op == "sign": r = np.sign(x)
        elif op == "tanh": r = np.tanh(x)
        elif op == "sqrt": r = np.sqrt(np.abs(x))
        elif op == "abs": r = np.abs(x)
        else: r = x * x
    else:
        a = evaluate(e[2], env); b = evaluate(e[3], env); op = e[1]
        if op == "+": r = a + b
        elif op == "-": r = a - b
        elif op == "*": r = a * b
        else:
            den = np.where(np.abs(b) < 1e-8, 1e-8, b)
            r = a / den
    return np.clip(np.nan_to_num(r, nan=0.0, posinf=1e6, neginf=-1e6), -1e6, 1e6)


def to_str(e):
    k = e[0]
    if k == "v": return {"d": "Δg", "a": "çapa"}.get(e[1], e[1])
    if k == "c": return f"{e[1]:g}"
    if k == "u":
        s = to_str(e[2])
        return f"-({s})" if e[1] == "neg" else f"{e[1]}({s})"
    return f"({to_str(e[2])} {e[1]} {to_str(e[3])})"


def rand_tree(rng, depth):
    if depth <= 0 or rng.random() < 0.25:
        return V(rng.choice(VARS)) if rng.random() < 0.7 else C(rng.choice(CONSTS))
    if rng.random() < 0.35:
        return U(rng.choice(UN), rand_tree(rng, depth - 1))
    return B(rng.choice(BIN), rand_tree(rng, depth - 1), rand_tree(rng, depth - 1))


def subtrees(e, path=()):
    yield path, e
    if e[0] == "u": yield from subtrees(e[2], path + (2,))
    elif e[0] == "b":
        yield from subtrees(e[2], path + (2,))
        yield from subtrees(e[3], path + (3,))


def replace(e, path, new):
    if not path: return new
    lst = list(e)
    lst[path[0]] = replace(e[path[0]], path[1:], new)
    return tuple(lst)


def mutate(e, rng):
    nodes = list(subtrees(e))
    path, node = rng.choice(nodes)
    if node[0] == "c" and rng.random() < 0.6:
        new = C(node[1] * math.exp(rng.gauss(0, 0.4)))
    else:
        new = rand_tree(rng, 2)
    out = replace(e, path, new)
    return out if size(out) <= MAX_NODES else e


def crossover(a, b, rng):
    pa, _ = rng.choice(list(subtrees(a)))
    _, sb = rng.choice(list(subtrees(b)))
    out = replace(a, pa, sb)
    return out if size(out) <= MAX_NODES else a


BASELINES = {
    "SGD": V("g"),
    "Momentum": V("m"),
    "RMSprop": B("/", V("g"), B("+", U("sqrt", V("v")), C(1e-8))),
    "Adam": B("/", V("m"), B("+", U("sqrt", V("v")), C(1e-8))),
    "AdamW": B("+", B("/", V("m"), B("+", U("sqrt", V("v")), C(1e-8))), B("*", C(0.01), V("w"))),
    "Lion-benzeri": B("+", U("sign", V("m")), B("*", C(0.01), V("w"))),
    # insan yapimi capali referanslar (EWC benzeri): bulunan formuller bunlari da gecmek zorunda
    "Adam+Capa(0.1)": B("+", B("/", V("m"), B("+", U("sqrt", V("v")), C(1e-8))),
                        B("*", C(0.1), B("-", V("w"), V("a")))),
    "Adam+Capa(sqrt v)": B("+", B("/", V("m"), B("+", U("sqrt", V("v")), C(1e-8))),
                           B("*", B("*", C(2.0), U("sqrt", V("v"))), B("-", V("w"), V("a")))),
}


# ----------------------------------------------------------------- gorev + egitim
def spiral(center, seed, classes=3, n=50, noise=0.12):
    rng = np.random.default_rng(seed)
    X, y = [], []
    for k in range(classes):
        r = np.linspace(0.15, 1.0, n)
        t = np.linspace(0, 3.5, n) + k * 2 * np.pi / classes + rng.normal(0, noise, n)
        X.append(np.c_[r * np.sin(t), r * np.cos(t)] + center)
        y += [k] * n
    return np.vstack(X), np.array(y)


def circles(center, seed, n=80, noise=0.08):
    rng = np.random.default_rng(seed)
    X, y = [], []
    for k, r in enumerate((0.45, 1.0)):
        t = rng.uniform(0, 2 * np.pi, n)
        X.append(np.c_[r * np.cos(t), r * np.sin(t)] + rng.normal(0, noise, (n, 2)) + center)
        y += [k] * n
    return np.vstack(X), np.array(y)


def make_tasks(kind, seed):
    f = spiral if kind == "spiral" else circles
    return f(np.array([-1.6, 0.0]), seed), f(np.array([1.6, 0.0]), seed + 7)


class Net:
    def __init__(self, H, C, seed):
        rng = np.random.default_rng(seed)
        self.H, self.C = H, C
        self.sizes = [2 * H, H, H * C, C]
        self.w = np.concatenate([rng.normal(0, 0.7, 2 * H), np.zeros(H),
                                 rng.normal(0, 0.7 / math.sqrt(H), H * C), np.zeros(C)])

    def parts(self):
        i = np.cumsum([0] + self.sizes)
        w = self.w
        return (w[i[0]:i[1]].reshape(2, self.H), w[i[1]:i[2]],
                w[i[2]:i[3]].reshape(self.H, self.C), w[i[3]:i[4]])

    def loss_grad(self, X, y, want_grad=True):
        W1, b1, W2, b2 = self.parts()
        a = np.tanh(X @ W1 + b1)
        z = a @ W2 + b2
        z = z - z.max(1, keepdims=True)
        p = np.exp(z); p /= p.sum(1, keepdims=True)
        n = len(y)
        loss = -np.log(p[np.arange(n), y] + 1e-12).mean()
        if not want_grad:
            return loss
        dz = p.copy(); dz[np.arange(n), y] -= 1; dz /= n
        dW2 = a.T @ dz; db2 = dz.sum(0)
        da = dz @ W2.T * (1 - a * a)
        dW1 = X.T @ da; db1 = da.sum(0)
        return loss, np.concatenate([dW1.ravel(), db1, dW2.ravel(), db2])


def run_once(expr, lr, kind, seed, H=24, steps=100):
    (XA, yA), (XB, yB) = make_tasks(kind, seed)
    C = int(max(yA.max(), yB.max()) + 1)
    net = Net(H, C, seed)
    m = np.zeros_like(net.w); v = np.zeros_like(net.w); gp = np.zeros_like(net.w)
    t = 0
    out = {}
    with np.errstate(all="ignore"):
        for phase, (X, y) in (("A", (XA, yA)), ("B", (XB, yB))):
            anchor = net.w.copy()
            for _ in range(steps):
                t += 1
                loss, g = net.loss_grad(X, y)
                m = 0.9 * m + 0.1 * g
                v = 0.999 * v + 0.001 * g * g
                env = {"g": g, "m": m / (1 - 0.9 ** t), "v": v / (1 - 0.999 ** t), "w": net.w, "d": g - gp, "a": anchor}
                gp = g
                net.w = net.w - lr * evaluate(expr, env)
                if not np.isfinite(net.w).all():
                    return {"A1": 10.0, "B2": 10.0, "A2": 10.0}
            if phase == "A":
                out["A1"] = net.loss_grad(XA, yA, False)
            else:
                out["B2"] = net.loss_grad(XB, yB, False)
                out["A2"] = net.loss_grad(XA, yA, False)
    return {k: (float(min(x, 10.0)) if np.isfinite(x) else 10.0) for k, x in out.items()}


def score(args):
    expr, kind, seeds, H = args
    best = None
    for lr in LR_GRID:
        rs = [run_once(expr, lr, kind, s, H) for s in seeds]
        r = {k: float(np.mean([x[k] for x in rs])) for k in ("A1", "B2", "A2")}
        r["total"] = r["A1"] + r["B2"] + r["A2"]
        r["lr"] = lr
        if best is None or r["total"] < best["total"]:
            best = r
    return best


# ----------------------------------------------------------------- evrim
def evolve(pop_n, gens, pure_random, rng, pool, log=print):
    pop = []
    if not pure_random:
        pop += list(BASELINES.values())
    while len(pop) < pop_n:
        base = rng.choice(list(BASELINES.values())) if (not pure_random and rng.random() < 0.5) else None
        pop.append(mutate(base, rng) if base is not None else rand_tree(rng, 3))
    cache = {}

    def evaluate_pop(exprs):
        todo = [e for e in exprs if to_str(e) not in cache]
        res = pool.map(score, [(e, "spiral", (1, 2), 24) for e in todo])
        for e, r in zip(todo, res):
            cache[to_str(e)] = (e, r)
        return [cache[to_str(e)] for e in exprs]

    for gen in range(gens):
        scored = evaluate_pop(pop)
        scored.sort(key=lambda er: er[1]["total"] + 0.003 * size(er[0]))
        best = scored[0]
        log(f"nesil {gen+1:2}/{gens}: en iyi toplam={best[1]['total']:.3f} lr={best[1]['lr']}  {to_str(best[0])}")
        elite = [e for e, _ in scored[:max(4, pop_n // 8)]]
        nxt = list(elite)
        while len(nxt) < pop_n:
            a, b = rng.sample(scored[:max(8, pop_n // 2)], 2)
            r = rng.random()
            child = crossover(a[0], b[0], rng) if r < 0.4 else mutate(a[0], rng)
            if rng.random() < 0.3:
                child = mutate(child, rng)
            nxt.append(child)
        pop = nxt
    return [er for er in sorted(cache.values(), key=lambda er: er[1]["total"] + 0.003 * size(er[0]))]


def main(argv=None):
    p = argparse.ArgumentParser()
    p.add_argument("--pop", type=int, default=48)
    p.add_argument("--gens", type=int, default=12)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--pure-random", action="store_true")
    p.add_argument("--out", default=None)
    a = p.parse_args(argv)
    rng = random.Random(a.seed)

    with Pool() as pool:
        ranked = evolve(a.pop, a.gens, a.pure_random, rng, pool)
        top = [er for er in ranked if to_str(er[0]) not in {to_str(b) for b in BASELINES.values()}][:5]
        names = list(BASELINES.items()) + [(f"KESIF#{i+1}", e) for i, (e, _) in enumerate(top)]
        # DOGRULAMA: arama sirasinda hic gorulmeyen tohumlar, hem spiral hem tamamen farkli gorev (circles), farkli genislik
        val_seeds = (101, 102, 103, 104, 105)
        jobs = []
        for name, e in names:
            jobs.append((e, "spiral", val_seeds, 48))
            jobs.append((e, "circles", val_seeds, 16))
        res = pool.map(score, jobs)

    rows = []
    for i, (name, e) in enumerate(names):
        rs, rc = res[2 * i], res[2 * i + 1]
        rows.append((name, to_str(e), rs, rc))
    lines = ["| Ad | Formul | spiral(yeni tohum) | lr | daire(farkli gorev) | lr | unutma (A2) spiral |",
             "|---|---|---|---|---|---|---|"]
    for name, f, rs, rc in rows:
        lines.append(f"| {name} | `{f}` | {rs['total']:.3f} | {rs['lr']} | {rc['total']:.3f} | {rc['lr']} | {rs['A2']:.3f} |")
    text = "\n".join(lines)
    print("\nDOGRULAMA (dusuk = iyi; arama sirasinda gorulmeyen tohum ve gorev):\n")
    print(text)
    base_best_s = min(r[2]["total"] for r in rows if not r[0].startswith("KESIF"))
    base_best_c = min(r[3]["total"] for r in rows if not r[0].startswith("KESIF"))
    for name, f, rs, rc in rows:
        if name.startswith("KESIF"):
            ds = (1 - rs["total"] / base_best_s) * 100
            dc = (1 - rc["total"] / base_best_c) * 100
            print(f"{name}: en iyi baseline'a gore spiral {ds:+.1f}%, daire {dc:+.1f}%")
    if a.out:
        open(a.out, "w", encoding="utf-8").write(text + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
