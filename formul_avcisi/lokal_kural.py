"""Yerel kural + eski degisim hafizasi, 200 gorunmemis gorevde.

Kural:  D_t = mean_n( giris * hata ) + mu * D_{t-1}     (delta kurali + eski degisim)   w <- w - lr * D_t
hata = sonuc - cevap (softmax: p - onehot). Gizli katman hatasi uc yolla tasinir:
  bp   : gercek geri yayilim (W2^T)         -> klasik SGD+momentum ile ayni
  fa   : SABIT rastgele geri baglanti B     -> geri yayilim YOK (feedback alignment)
  none : gizli katman hic ogrenmez (rastgele ozellik + sadece cikis)  -> alt sinir
"""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s

CONFIGS = [(fb, mu) for fb in ("bp", "fa", "none") for mu in (0.0, 0.9)]


def _loss(X, y, P, reg):
    W1, b1, W2, b2 = P
    z = np.tanh(X @ W1 + b1) @ W2 + b2
    if reg:
        return float(((z[:, 0] - y) ** 2).mean())
    z = z - z.max(1, keepdims=True); p = np.exp(z); p /= p.sum(1, keepdims=True)
    return float(-np.log(p[np.arange(len(y)), y] + 1e-12).mean())


def run_local(task, lr, fb, mu):
    net = s.Net(task["H"], task["out"], task["seed"] % 1000)
    P = [p.copy() for p in net.parts()]
    U = [np.zeros_like(p) for p in P]
    H, out, reg = task["H"], task["out"], task["reg"]
    rng = np.random.default_rng(task["seed"] + 5)
    B = rng.normal(0, 1 / math.sqrt(H), (out, H)) if False else rng.normal(0, 1.0, (out, H)) / np.sqrt(out)
    l0 = [_loss(X, y, P, reg) for X, y, _ in task["phases"]]
    with np.errstate(all="ignore"):
        for X, y, _ in task["phases"]:
            n = len(y)
            for _ in range(task["steps"]):
                W1, b1, W2, b2 = P
                a1 = np.tanh(X @ W1 + b1); z = a1 @ W2 + b2
                if reg:
                    e = z - y[:, None]
                else:
                    zz = z - z.max(1, keepdims=True); e = np.exp(zz); e /= e.sum(1, keepdims=True)
                    e[np.arange(n), y] -= 1
                if fb == "bp": eh = (e @ W2.T) * (1 - a1 * a1)
                elif fb == "fa": eh = (e @ B) * (1 - a1 * a1)
                else: eh = np.zeros((n, H))
                G = [X.T @ eh / n, eh.mean(0), a1.T @ e / n, e.mean(0)]
                for i in range(4):
                    U[i] = G[i] + mu * U[i]
                    P[i] = P[i] - lr * U[i]
                if not all(np.isfinite(p).all() for p in P):
                    return s.CLIP
    lf = [_loss(X, y, P, reg) for X, y, _ in task["phases"]]
    return float(np.mean([min(s.CLIP, f / max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0)]))


import math  # noqa: E402


def job(a):
    fb, mu, seeds = a
    tasks = [s.make_task(sd) for sd in seeds]
    best = None
    for lr in [0.003, 0.01, 0.03, 0.1, 0.3, 1.0]:
        rs = np.array([run_local(t, lr, fb, mu) for t in tasks])
        g = float(np.exp(np.mean(np.log(np.maximum(rs, 1e-4)))))
        if best is None or g < best[0]:
            best = (g, lr, rs)
    return best


if __name__ == "__main__":
    seeds = [10**7 + i for i in range(200)]
    cont = np.array([len(s.make_task(sd)["phases"]) > 1 for sd in seeds])
    gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(x, 1e-4)))))
    with Pool() as p:
        res = p.map(job, [(fb, mu, seeds) for fb, mu in CONFIGS])
    print("200 gorunmemis gorev. Referans: klasik Adam(bp) tum=0.0507, ardisik=0.5523 (kalan kayip, dusuk=iyi)\n")
    print(f"{'gizli hata yolu':16} {'eski degisim mu':15} {'lr':6} {'tum':>8} {'ardisik':>9}")
    for (fb, mu), (g, lr, rs) in zip(CONFIGS, res):
        print(f"{fb:16} {'0.9' if mu else '0 (yok)':15} {lr:<6} {g:8.4f} {gm(rs[cont]):9.4f}")
