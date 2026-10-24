#!/usr/bin/env python3
"""Ekleme-yalniz (append-only / progressive) ag vs ince ayar, ardisik gorevlerde.

A gorevi ogrenilir, sonra DONDURULUR. B gorevi icin YENI bir blok EKLENIR: giris + A'nin gizli
ozelliklerinden yanal baglanti (eski hesap B'ye girdi olur). Eski parametrelere hic dokunulmaz.
Uc kosul:
  id     : hangi gorev oldugu BILINIYOR (dogru sutun secilir)
  router : kimlik YOK; girisin merkez uzakligina (A/B egitim girdisi ortalamasi) bakip sutun secilir
  toplam : kimlik YOK, yonlendirici YOK; cikis = A cikisi + B sutunu cikisi
Referanslar: Adam ince ayar, Adam + capa(0.1). Olcu: kalan kayip orani (son kayip / baslangic), dusuk iyi.
"""
import math
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool

import numpy as np

import optim_search as o
import optim_suite as s

LRS = [0.003, 0.01, 0.03, 0.1, 0.3]


def softmax_e(z, y):
    zz = z - z.max(1, keepdims=True); p = np.exp(zz); p /= p.sum(1, keepdims=True)
    e = p.copy(); e[np.arange(len(y)), y] -= 1
    return e


def loss_of(z, y, reg):
    if reg:
        return float(((z[:, 0] - y) ** 2).mean())
    zz = z - z.max(1, keepdims=True); p = np.exp(zz); p /= p.sum(1, keepdims=True)
    return float(-np.log(p[np.arange(len(y)), y] + 1e-12).mean())


class AdamV:
    def __init__(self, shapes, lr):
        self.m = [np.zeros(sh) for sh in shapes]; self.v = [np.zeros(sh) for sh in shapes]; self.t = 0; self.lr = lr

    def step(self, P, G):
        self.t += 1
        for i, g in enumerate(G):
            self.m[i] = 0.9 * self.m[i] + 0.1 * g; self.v[i] = 0.999 * self.v[i] + 0.001 * g * g
            P[i] -= self.lr * (self.m[i] / (1 - 0.9 ** self.t)) / (np.sqrt(self.v[i] / (1 - 0.999 ** self.t)) + 1e-8)


def col_forward(X, aA, P, zA, add_zA):
    """B sutunu: girdi + A'nin gizli ozelligi (yanal). add_zA: kimliksiz toplam kosulu."""
    W1b, b1b, L, W2b, b2b = P
    a = np.tanh(X @ W1b + aA @ L + b1b)
    z = a @ W2b + b2b
    return a, (z + zA if add_zA else z)


def train_B(X, y, reg, PA, P, steps, lr, add_zA):
    W1, b1, W2, b2 = PA
    aA = np.tanh(X @ W1 + b1); zA = aA @ W2 + b2          # donuk A sutunu (sabit)
    opt = AdamV([p.shape for p in P], lr); n = len(y)
    with np.errstate(all="ignore"):
        for _ in range(steps):
            a, z = col_forward(X, aA, P, zA, add_zA)
            e = (2 * (z[:, 0] - y))[:, None] if reg else softmax_e(z, y)
            e = e / n
            da = e @ P[3].T * (1 - a * a)
            G = [X.T @ da, da.sum(0), aA.T @ da, a.T @ e, e.sum(0)]
            opt.step(P, G)
            if not all(np.isfinite(p).all() for p in P):
                return False
    return True


def run_append(task, lr):
    """Doner: (id_A, id_B), (router_A, router_B), (toplam_A, toplam_B) kalan kayip oranlari + parametre buyumesi."""
    H, out, reg = task["H"], task["out"], task["reg"]
    (XA, yA, _), (XB, yB, _) = task["phases"]
    net = s.Net(H, out, task["seed"] % 1000)
    PA = [p.copy() for p in net.parts()]
    l0A = loss_of(np.tanh(XA @ PA[0] + PA[1]) @ PA[2] + PA[3], yA, reg)
    l0B = loss_of(np.tanh(XB @ PA[0] + PA[1]) @ PA[2] + PA[3], yB, reg)
    # --- faz A: Adam ile egit
    opt = AdamV([p.shape for p in PA], lr); n = len(yA)
    with np.errstate(all="ignore"):
        for _ in range(task["steps"]):
            a = np.tanh(XA @ PA[0] + PA[1]); z = a @ PA[2] + PA[3]
            e = ((2 * (z[:, 0] - yA))[:, None] if reg else softmax_e(z, yA)) / n
            da = e @ PA[2].T * (1 - a * a)
            opt.step(PA, [XA.T @ da, da.sum(0), a.T @ e, e.sum(0)])
            if not all(np.isfinite(p).all() for p in PA):
                return None
    rng = np.random.default_rng(task["seed"] + 11)

    def new_cols(zero_head):
        return [rng.normal(0, 0.7, (2, H)), np.zeros(H), rng.normal(0, 0.7 / math.sqrt(H), (H, H)),
                np.zeros((H, out)) if zero_head else rng.normal(0, 0.7 / math.sqrt(H), (H, out)), np.zeros(out)]

    def zA_of(X):
        aA = np.tanh(X @ PA[0] + PA[1]); return aA, aA @ PA[2] + PA[3]

    PB = new_cols(False)
    okB = train_B(XB, yB, reg, PA, PB, task["steps"], lr, False)
    PS = new_cols(True)
    okS = train_B(XB, yB, reg, PA, PS, task["steps"], lr, True)
    if not (okB and okS):
        return None
    cA, cB = XA.mean(0), XB.mean(0)

    def zB_of(X, P, add):
        aA, zA = zA_of(X); return col_forward(X, aA, P, zA, add)[1]

    res = {}
    # id: dogru sutun
    res["id"] = (loss_of(zA_of(XA)[1], yA, reg) / max(l0A, 1e-6), loss_of(zzB := zB_of(XB, PB, False), yB, reg) / max(l0B, 1e-6))
    # router: her ornek icin merkeze gore sutun
    def routed(X):
        zA_ = zA_of(X)[1]; zB_ = zB_of(X, PB, False)
        toA = (np.linalg.norm(X - cA, axis=1) <= np.linalg.norm(X - cB, axis=1))[:, None]
        return np.where(toA, zA_, zB_)
    res["router"] = (loss_of(routed(XA), yA, reg) / max(l0A, 1e-6), loss_of(routed(XB), yB, reg) / max(l0B, 1e-6))
    res["toplam"] = (loss_of(zB_of(XA, PS, True), yA, reg) / max(l0A, 1e-6), loss_of(zB_of(XB, PS, True), yB, reg) / max(l0B, 1e-6))
    for k, (a_, b_) in res.items():
        res[k] = (min(s.CLIP, a_) if np.isfinite(a_) else s.CLIP, min(s.CLIP, b_) if np.isfinite(b_) else s.CLIP)
    return res


def run_expr(task, lr, expr):
    """Referans: ince ayar (Adam) veya capali Adam; faz basina ayri kalan kayip orani."""
    net = s.Net(task["H"], task["out"], task["seed"] % 1000); reg = task["reg"]
    l0 = [net.loss_grad(X, y, reg, False) for X, y, _ in task["phases"]]
    m = np.zeros_like(net.w); v = np.zeros_like(net.w); gp = np.zeros_like(net.w); t = 0
    with np.errstate(all="ignore"):
        for pi, (X, y, _) in enumerate(task["phases"]):
            anchor = net.w.copy()
            for _ in range(task["steps"]):
                t += 1
                if pi == 0:
                    anchor = net.w
                _, g = net.loss_grad(X, y, reg)
                m = 0.9 * m + 0.1 * g; v = 0.999 * v + 0.001 * g * g
                env = {"g": g, "m": m / (1 - 0.9 ** t), "v": v / (1 - 0.999 ** t), "w": net.w, "d": g - gp, "a": anchor}
                gp = g
                net.w = net.w - lr * o.evaluate(expr, env)
                if not np.isfinite(net.w).all():
                    return (s.CLIP, s.CLIP)
    lf = [net.loss_grad(X, y, reg, False) for X, y, _ in task["phases"]]
    r = [min(s.CLIP, f / max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0)]
    return (r[0], r[1])


def job(a):
    method, seeds = a
    tasks = [t for t in (s.make_task(sd) for sd in seeds) if len(t["phases"]) == 2]
    best = None
    for lr in LRS:
        if method in ("Adam ince ayar", "Adam + capa(0.1)"):
            expr = o.BASELINES["Adam"] if method == "Adam ince ayar" else o.BASELINES["Adam+Capa(0.1)"]
            rs = [run_expr(t, lr, expr) for t in tasks]
            parts = {method: rs}
        else:
            outs = [run_append(t, lr) for t in tasks]
            parts = {f"EKLEME ({k})": [(o_[k] if o_ else (s.CLIP, s.CLIP)) for o_ in outs] for k in ("id", "router", "toplam")}
        score = {k: np.exp(np.mean(np.log(np.maximum(np.array(v).mean(1), 1e-4)))) for k, v in parts.items()}
        if best is None:
            best = {k: (score[k], lr, parts[k]) for k in parts}
        else:
            for k in parts:
                if score[k] < best[k][0]:
                    best[k] = (score[k], lr, parts[k])
    return best, len(tasks)


if __name__ == "__main__":
    seeds = [10 ** 7 + i for i in range(200)]
    gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))
    with Pool() as p:
        r = p.map(job, [(m, seeds) for m in ("Adam ince ayar", "Adam + capa(0.1)", "ekleme")])
    n = r[0][1]
    print(f"{n} ardisik gorev (200 gorunmemisten). Kalan kayip orani, dusuk=iyi. Her yontem icin TEK ortak lr.\n")
    print(f"{'yontem':26} {'lr':>5} {'ESKI gorev':>11} {'YENI gorev':>11} {'ortalama':>9}")
    for best, _ in r:
        for k, (sc, lr, parts) in best.items():
            arr = np.array(parts)
            print(f"{k:26} {lr:>5} {gm(arr[:, 0]):11.4f} {gm(arr[:, 1]):11.4f} {sc:9.4f}")
    print("\nParametre buyumesi (H hidden, out cikis): temel = 3H+H*out+out ; B sutunu eklenince ORTALAMA ~11x (yanal baglanti H*H) -> bkz ekleme_kontrol.py")
