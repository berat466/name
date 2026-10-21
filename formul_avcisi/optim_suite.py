#!/usr/bin/env python3
"""Cok gorevli optimizer formul simulatoru.

Her formul YUZLERCE farkli rastgele gorevde sinanir (siniflandirma + regresyon; farkli veri
sekilleri, gurultu, ag genisligi, adim sayisi; bazilarinda ardisik gorev = unutma testi).
Puan = baslangic kaybina oranla kalan kayip (dusuk iyi), gorevler uzerinde geometrik ortalama.
Her formul icin TEK ortak ogrenme orani secilir (gorev basina ayar yok: adil ve gercekci).
Arama sirasinda her nesil FARKLI gorevler kullanir (tek goreve asiri uyum olmasin);
final dogrulama arama sirasinda hic gorulmemis gorevlerde yapilir.

  python optim_suite.py --pop 40 --gens 14 --val-tasks 200
"""
import argparse
import math
import random
import sys
from multiprocessing import Pool

import numpy as np

from optim_search import BASELINES, crossover, evaluate, mutate, rand_tree, size, to_str

LRS = [0.0003, 0.001, 0.003, 0.01, 0.03, 0.1, 0.3]
CLIP = 5.0


# ------------------------------------------------------------------ gorev uretici
def _std(X):
    return (X - X.mean(0)) / (X.std(0) + 1e-9)


def gen_data(rng, kind, n, noise, shift=0.0):
    if kind == "spiral":
        k = int(rng.integers(2, 5)); per = n // k
        X, y = [], []
        for c in range(k):
            r = np.linspace(0.15, 1.0, per)
            t = np.linspace(0, 3.5, per) + c * 2 * np.pi / k + rng.normal(0, noise, per)
            X.append(np.c_[r * np.sin(t), r * np.cos(t)]); y += [c] * per
        return _std(np.vstack(X)), np.array(y), k
    if kind == "circles":
        per = n // 2; X, y = [], []
        for c, r in enumerate((0.45, 1.0)):
            t = rng.uniform(0, 2 * np.pi, per)
            X.append(np.c_[r * np.cos(t), r * np.sin(t)] + rng.normal(0, noise, (per, 2))); y += [c] * per
        return _std(np.vstack(X)), np.array(y), 2
    if kind == "moons":
        per = n // 2
        t = rng.uniform(0, np.pi, per)
        a = np.c_[np.cos(t), np.sin(t)]; b = np.c_[1 - np.cos(t), 0.4 - np.sin(t)]
        X = np.vstack([a, b]) + rng.normal(0, noise, (2 * per, 2))
        return _std(X), np.array([0] * per + [1] * per), 2
    if kind == "blobs":
        k = int(rng.integers(2, 5)); per = n // k
        cen = rng.normal(0, 1.6, (k, 2))
        X = np.vstack([cen[c] + rng.normal(0, 0.5 + noise * 2, (per, 2)) for c in range(k)])
        return _std(X), np.repeat(np.arange(k), per), k
    if kind == "xor":
        per = n // 4
        cen = np.array([[1, 1], [-1, -1], [1, -1], [-1, 1]], float)
        X = np.vstack([c + rng.normal(0, 0.35 + noise, (per, 2)) for c in cen])
        return _std(X), np.array([0] * (2 * per) + [1] * (2 * per)), 2
    X = rng.uniform(-2, 2, (n, 2))
    if kind == "reg_sin":
        y = np.sin(3 * X[:, 0] + shift) + 0.5 * X[:, 1] ** 2
    elif kind == "reg_prod":
        y = X[:, 0] * X[:, 1] + 0.3 * np.cos(2 * X[:, 0] + shift)
    else:  # reg_lin
        y = 1.5 * X[:, 0] - 0.7 * X[:, 1] + shift * 0.2
    y = y + rng.normal(0, noise * 0.5, n)
    return _std(X), (y - y.mean()) / (y.std() + 1e-9), 1


KINDS = ["spiral", "circles", "moons", "blobs", "xor", "reg_sin", "reg_prod", "reg_lin"]


def make_task(seed):
    rng = np.random.default_rng(seed)
    kind = KINDS[int(rng.integers(len(KINDS)))]
    n = int(rng.choice([80, 160, 300]))
    noise = float(rng.choice([0.05, 0.12, 0.25]))
    H = int(rng.choice([8, 16, 32, 64]))
    steps = int(rng.choice([60, 120, 250]))
    continual = rng.random() < 0.5
    d1 = gen_data(rng, kind, n, noise, 0.0)
    phases = [d1]
    if continual:
        d2 = gen_data(rng, kind, n, noise, 1.3)
        if kind in ("spiral", "circles", "moons", "blobs", "xor"):
            d2 = (d2[0] + rng.choice([-2.5, 2.5]), d2[1], d2[2])  # baska bolgede ayni tur gorev
        phases.append(d2)
    out = max(p[2] for p in phases)
    return {"kind": kind, "H": H, "steps": steps, "phases": phases, "out": out,
            "reg": kind.startswith("reg"), "seed": seed}


# ------------------------------------------------------------------ ag
class Net:
    def __init__(self, H, out, seed):
        rng = np.random.default_rng(seed)
        self.H, self.out = H, out
        self.sizes = [2 * H, H, H * out, out]
        self.w = np.concatenate([rng.normal(0, 0.7, 2 * H), np.zeros(H),
                                 rng.normal(0, 0.7 / math.sqrt(H), H * out), np.zeros(out)])

    def parts(self):
        i = np.cumsum([0] + self.sizes); w = self.w
        return (w[i[0]:i[1]].reshape(2, self.H), w[i[1]:i[2]],
                w[i[2]:i[3]].reshape(self.H, self.out), w[i[3]:i[4]])

    def loss_grad(self, X, y, reg, grad=True):
        W1, b1, W2, b2 = self.parts()
        a = np.tanh(X @ W1 + b1); z = a @ W2 + b2; n = len(y)
        if reg:
            err = z[:, 0] - y; loss = float((err ** 2).mean())
            dz = (2 * err / n)[:, None]
        else:
            zz = z - z.max(1, keepdims=True); p = np.exp(zz); p /= p.sum(1, keepdims=True)
            loss = float(-np.log(p[np.arange(n), y] + 1e-12).mean())
            dz = p.copy(); dz[np.arange(n), y] -= 1; dz /= n
        if not grad:
            return loss
        da = dz @ W2.T * (1 - a * a)
        return loss, np.concatenate([(X.T @ da).ravel(), da.sum(0), (a.T @ dz).ravel(), dz.sum(0)])


def run_task(expr, lr, task):
    """Kalan kayit orani: son kayip / baslangic kaybi, fazlar uzerinde ortalama."""
    net = Net(task["H"], task["out"], task["seed"] % 1000)
    reg = task["reg"]
    l0 = [net.loss_grad(X, y, reg, False) for X, y, _ in task["phases"]]
    m = np.zeros_like(net.w); v = np.zeros_like(net.w); gp = np.zeros_like(net.w); t = 0
    with np.errstate(all="ignore"):
        for X, y, _ in task["phases"]:
            for _ in range(task["steps"]):
                t += 1
                _, g = net.loss_grad(X, y, reg)
                m = 0.9 * m + 0.1 * g; v = 0.999 * v + 0.001 * g * g
                env = {"g": g, "m": m / (1 - 0.9 ** t), "v": v / (1 - 0.999 ** t), "w": net.w, "d": g - gp}
                gp = g
                net.w = net.w - lr * evaluate(expr, env)
                if not np.isfinite(net.w).all():
                    return CLIP
    lf = [net.loss_grad(X, y, reg, False) for X, y, _ in task["phases"]]
    r = np.mean([min(CLIP, f / max(a, 1e-6)) if np.isfinite(f) else CLIP for f, a in zip(lf, l0)])
    return float(r)


def suite_scores(args):
    """Bir formul icin: her ogrenme orasinda gorev basina oranlar -> en iyi TEK lr."""
    expr, seeds = args
    tasks = [make_task(s) for s in seeds]
    best = None
    for lr in LRS:
        rs = np.array([run_task(expr, lr, t) for t in tasks])
        g = float(np.exp(np.mean(np.log(np.maximum(rs, 1e-4)))))
        if best is None or g < best[0]:
            best = (g, lr, rs)
    return best


# ------------------------------------------------------------------ evrim
def evolve(pop_n, gens, tasks_per_gen, pure_random, rng, pool, log=print):
    pop = [] if pure_random else list(BASELINES.values())
    while len(pop) < pop_n:
        base = rng.choice(list(BASELINES.values())) if (not pure_random and rng.random() < 0.5) else None
        pop.append(mutate(base, rng) if base is not None else rand_tree(rng, 3))
    hall = {}
    for gen in range(gens):
        seeds = [rng.randrange(10 ** 6) for _ in range(tasks_per_gen)]   # her nesil farkli gorevler
        res = pool.map(suite_scores, [(e, seeds) for e in pop])
        scored = sorted(zip(pop, res), key=lambda er: er[1][0] + 0.003 * size(er[0]))
        for e, r in scored:
            k = to_str(e)
            if k not in hall or r[0] < hall[k][1]:
                hall[k] = (e, r[0])
        base_best = min(r[0] for e, r in scored if to_str(e) in {to_str(b) for b in BASELINES.values()}) \
            if not pure_random else float("nan")
        log(f"nesil {gen+1:2}/{gens}: en iyi={scored[0][1][0]:.3f} (lr={scored[0][1][1]}) "
            f"en iyi baseline={base_best:.3f}  {to_str(scored[0][0])[:90]}")
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
    p.add_argument("--pop", type=int, default=40)
    p.add_argument("--gens", type=int, default=14)
    p.add_argument("--tasks-per-gen", type=int, default=10)
    p.add_argument("--val-tasks", type=int, default=200)
    p.add_argument("--seed", type=int, default=0)
    p.add_argument("--pure-random", action="store_true")
    p.add_argument("--out", default=None)
    a = p.parse_args(argv)
    rng = random.Random(a.seed)

    with Pool() as pool:
        hall = evolve(a.pop, a.gens, a.tasks_per_gen, a.pure_random, rng, pool)
        bnames = {to_str(e) for e in BASELINES.values()}
        top = sorted((v for k, v in hall.items() if k not in bnames), key=lambda ev: ev[1])[:6]
        cands = list(BASELINES.items()) + [(f"KESIF#{i+1}", e) for i, (e, _) in enumerate(top)]
        val_seeds = [10 ** 7 + i for i in range(a.val_tasks)]       # arama sirasinda HIC gorulmedi
        res = pool.map(suite_scores, [(e, val_seeds) for _, e in cands])

    adam = res[[n for n, _ in cands].index("Adam")]
    rows = ["| Ad | Formul | lr | geo.ort. kalan kayip | Adam'a gore | gorev kazanma orani vs Adam | en kotu gorev |",
            "|---|---|---|---|---|---|---|"]
    for (name, e), (g, lr, rs) in zip(cands, res):
        win = float(np.mean(rs < adam[2] - 1e-9)) * 100
        rows.append(f"| {name} | `{to_str(e)[:70]}` | {lr} | {g:.4f} | {(1 - g / adam[0]) * 100:+.1f}% | "
                    f"%{win:.0f} | {rs.max():.2f} |")
    text = "\n".join(rows)
    print(f"\nDOGRULAMA: {a.val_tasks} gorunmemis rastgele gorev\n")
    print(text)
    if a.out:
        open(a.out, "w", encoding="utf-8").write(text + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
