"""Anahtarla ust uste bindirme: 2, 5, 10 ARDISIK gorevde (genislik x8). Gurultu birikince nerede bozuluyor?"""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
import ekleme_agi as E

LRS = [0.0003, 0.001, 0.003, 0.01]   # alt uc genis: dusuk lr uzun dizilerde onemli
MULT = 8
NSEQ = 30
VARIANTS = [("anahtarsiz", False, 0.0), ("anahtarsiz+capa", False, 0.1), ("ANAHTARLI", True, 0.0), ("ANAHTARLI+capa", True, 0.1)]
gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))

def make_seq(seed, N):
    rng = np.random.default_rng(seed)
    kind = s.KINDS[int(rng.integers(len(s.KINDS)))]
    n = int(rng.choice([80, 160])); noise = float(rng.choice([0.05, 0.12])); H = int(rng.choice([8, 16, 32])) * MULT
    steps = int(rng.choice([60, 120]))
    ph = []
    for i in range(N):
        d = s.gen_data(rng, kind, n, noise, 1.3 * i)
        if not kind.startswith("reg"):
            d = (d[0] + rng.normal(0, 2.5, 2), d[1], d[2])
        ph.append(d)
    return {"kind": kind, "H": H, "steps": steps, "phases": ph, "out": max(p[2] for p in ph), "reg": kind.startswith("reg"), "seed": seed}

def fwd(X, P, K):
    W1, W2 = P[0] * K[0], P[2] * K[2]
    a = np.tanh(X @ W1 + P[1]); return a, a @ W2 + P[3], W2

def run(task, lr, use_key, capa):
    H, out, reg, N = task["H"], task["out"], task["reg"], len(task["phases"])
    rng = np.random.default_rng(task["seed"] + 21)
    net = s.Net(H, out, task["seed"] % 1000); P = [p.copy() for p in net.parts()]
    one = [np.ones_like(P[0]), 1.0, np.ones_like(P[2]), 1.0]
    keys = [[np.sign(rng.standard_normal(P[0].shape)), 1.0, np.sign(rng.standard_normal(P[2].shape)), 1.0] for _ in range(N)] if use_key else [one] * N
    l0 = [E.loss_of(fwd(X, P, keys[i])[1], y, reg) for i, (X, y, _) in enumerate(task["phases"])]
    opt = E.AdamV([p.shape for p in P], lr)
    with np.errstate(all="ignore"):
        for pi, (X, y, _) in enumerate(task["phases"]):
            K = keys[pi]; n = len(y); anchor = [p.copy() for p in P]
            for _ in range(task["steps"]):
                a, z, W2 = fwd(X, P, K)
                e = ((2 * (z[:, 0] - y))[:, None] if reg else E.softmax_e(z, y)) / n
                da = e @ W2.T * (1 - a * a)
                G = [(X.T @ da) * K[0], da.sum(0), (a.T @ e) * K[2], e.sum(0)]
                opt.t += 1
                for i, g in enumerate(G):
                    opt.m[i] = 0.9 * opt.m[i] + 0.1 * g; opt.v[i] = 0.999 * opt.v[i] + 0.001 * g * g
                    d = (opt.m[i] / (1 - 0.9 ** opt.t)) / (np.sqrt(opt.v[i] / (1 - 0.999 ** opt.t)) + 1e-8)
                    P[i] -= lr * (d + (capa * (P[i] - anchor[i]) if pi > 0 else 0.0))
                if not all(np.isfinite(p).all() for p in P):
                    return [s.CLIP] * N
    lf = [E.loss_of(fwd(X, P, keys[i])[1], y, reg) for i, (X, y, _) in enumerate(task["phases"])]
    return [min(s.CLIP, f / max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0)]

def job(a):
    name, use_key, capa, N = a
    tasks = [make_seq(10**7 + i, N) for i in range(NSEQ)]
    best = None
    for lr in LRS:
        arr = np.array([run(t, lr, use_key, capa) for t in tasks])      # (gorev, faz)
        sc = gm(arr.mean(1))
        if best is None or sc < best[0]: best = (sc, lr, gm(arr[:, 0]), gm(arr[:, -1]))
    return name, N, best

if __name__ == "__main__":
    jobs = [(n, k, c, N) for N in (2, 5, 10) for (n, k, c) in VARIANTS]
    with Pool() as p: res = p.map(job, jobs)
    print(f"Genislik x{MULT}, {NSEQ} dizi. Kalan kayip orani (dusuk iyi). EN ESKI = ilk gorev, EN YENI = son gorev.\n")
    print(f"{'gorev sayisi':>12} {'bicim':18} {'lr':>6} {'EN ESKI':>9} {'EN YENI':>9} {'ortalama':>9}")
    for name, N, (sc, lr, old, new) in res:
        print(f"{N:>12} {name:18} {lr:>6} {old:9.4f} {new:9.4f} {sc:9.4f}")
