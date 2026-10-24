"""Anahtarla ust uste bindirme (superposition): ayni agirlik, her gorevde farkli rastgele +-1 anahtar.
W_etkin(gorev t) = W (eleman eleman) K_t.  Gorev kimligi (anahtar) BILINIYOR. Ardisik gorevler, 93 adet.
Dort bicim: anahtarsiz / anahtarli, her biri capasiz / capali(0.1)."""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
import ekleme_agi as E

LRS = [0.003, 0.01, 0.03, 0.1, 0.3]
VARIANTS = [("anahtarsiz (ince ayar)", False, 0.0), ("anahtarsiz + capa(0.1)", False, 0.1),
            ("ANAHTARLI", True, 0.0), ("ANAHTARLI + capa(0.1)", True, 0.1)]
gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))

def fwd(X, P, K):
    W1 = P[0] * K[0]; W2 = P[2] * K[2]
    a = np.tanh(X @ W1 + P[1]); return a, a @ W2 + P[3], W2

def run(task, lr, use_key, capa):
    H, out, reg = task["H"], task["out"], task["reg"]
    rng = np.random.default_rng(task["seed"] + 21)
    net = s.Net(H, out, task["seed"] % 1000); P = [p.copy() for p in net.parts()]
    ones = [np.ones_like(P[0]), 1.0, np.ones_like(P[2]), 1.0]
    keys = [ones, ones]
    if use_key:
        keys = [[np.sign(rng.standard_normal(P[0].shape)), 1.0, np.sign(rng.standard_normal(P[2].shape)), 1.0] for _ in range(2)]
    l0 = [E.loss_of(fwd(task["phases"][i][0], P, keys[i])[1], task["phases"][i][1], reg) for i in range(2)]
    opt = E.AdamV([p.shape for p in P], lr)
    with np.errstate(all="ignore"):
        for pi in range(2):
            X, y, _ = task["phases"][pi]; K = keys[pi]; n = len(y)
            anchor = [p.copy() for p in P]
            for _ in range(task["steps"]):
                a, z, W2 = fwd(X, P, K)
                e = ((2 * (z[:, 0] - y))[:, None] if reg else E.softmax_e(z, y)) / n
                da = e @ W2.T * (1 - a * a)
                G = [(X.T @ da) * K[0], da.sum(0), (a.T @ e) * K[2], e.sum(0)]
                if capa and pi == 1:
                    G = None
                if G is None:   # capali: Adam yonu + 0.1*(w - capa); AdamV.step'i elle yapiyoruz
                    G = [(X.T @ da) * K[0], da.sum(0), (a.T @ e) * K[2], e.sum(0)]
                    opt.t += 1
                    for i, g in enumerate(G):
                        opt.m[i] = 0.9 * opt.m[i] + 0.1 * g; opt.v[i] = 0.999 * opt.v[i] + 0.001 * g * g
                        d = (opt.m[i] / (1 - 0.9 ** opt.t)) / (np.sqrt(opt.v[i] / (1 - 0.999 ** opt.t)) + 1e-8)
                        P[i] -= lr * (d + capa * (P[i] - anchor[i]))
                else:
                    opt.step(P, G)
                if not all(np.isfinite(p).all() for p in P):
                    return (s.CLIP, s.CLIP)
    lf = [E.loss_of(fwd(task["phases"][i][0], P, keys[i])[1], task["phases"][i][1], reg) for i in range(2)]
    r = [min(s.CLIP, f / max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0)]
    return (r[0], r[1])

def job(a):
    name, use_key, capa = a
    tasks = [t for t in (s.make_task(10**7 + i) for i in range(200)) if len(t["phases"]) == 2]
    best = None
    for lr in LRS:
        arr = np.array([run(t, lr, use_key, capa) for t in tasks])
        sc = gm(arr.mean(1))
        if best is None or sc < best[0]: best = (sc, lr, gm(arr[:, 0]), gm(arr[:, 1]))
    return name, best, len(tasks)

if __name__ == "__main__":
    with Pool() as p: res = p.map(job, VARIANTS)
    print(f"{res[0][2]} ardisik gorev. Kalan kayip orani (dusuk iyi), her bicim icin TEK ortak lr, kimlik biliniyor.\n")
    print(f"{'bicim':26} {'lr':>5} {'ESKI':>8} {'YENI':>8} {'ortalama':>9}")
    for name, (sc, lr, A, B), _ in res:
        print(f"{name:26} {lr:>5} {A:8.4f} {B:8.4f} {sc:9.4f}")
    print("\nReferans (onceki testler): ekleme zinciri (kimlikli) 0.030 | Adam+capa 0.244 | Adam ince ayar 0.287")
