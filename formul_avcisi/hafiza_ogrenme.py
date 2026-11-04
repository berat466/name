"""Hafiza gibi ogrenme vs gradyan inisi, TEST verisinde (ezber hilesi olmasin diye veri %70 egitim / %30 test).
 Adam         : gradyan inisi (klasik)
 ELM (tek hamle): rastgele ozellik + halka cozum (tek seferde agirlik uretimi, dongu yok)
 Hafiza (kNN) : ogrenmek = ornekleri saklamak, cevap = benzer saklilarin agirlikli ortalamasi
Metrik: test kaybi / ag baslangicindaki test kaybi (kalan kayip orani, dusuk iyi). Tek-fazli (ardisik olmayan) gorevler."""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s

gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))

def ce_from_probs(P, y):
    return float(-np.log(np.clip(P[np.arange(len(y)), y], 1e-9, 1)).mean())

def softmax(z):
    z = z - z.max(1, keepdims=True); p = np.exp(z); return p / p.sum(1, keepdims=True)

def split(task):
    X, y, _ = task["phases"][0]
    rng = np.random.default_rng(task["seed"] + 777)
    idx = rng.permutation(len(y)); ntr = int(0.7 * len(y))
    return X[idx[:ntr]], y[idx[:ntr]], X[idx[ntr:]], y[idx[ntr:]]

def loss_net(net, X, y, reg):
    return net.loss_grad(X, y, reg, False)

def adam_test(task, Xtr, ytr, Xte, yte, lr):
    reg = task["reg"]; net = s.Net(task["H"], task["out"], task["seed"] % 1000)
    l0 = loss_net(net, Xte, yte, reg)
    m = np.zeros_like(net.w); v = np.zeros_like(net.w)
    with np.errstate(all="ignore"):
        for t in range(1, task["steps"] + 1):
            _, g = net.loss_grad(Xtr, ytr, reg)
            m = 0.9 * m + 0.1 * g; v = 0.999 * v + 0.001 * g * g
            net.w = net.w - lr * (m / (1 - 0.9 ** t)) / (np.sqrt(v / (1 - 0.999 ** t)) + 1e-8)
            if not np.isfinite(net.w).all(): return s.CLIP
    lf = loss_net(net, Xte, yte, reg)
    return min(s.CLIP, lf / max(l0, 1e-6)) if np.isfinite(lf) else s.CLIP

def elm_test(task, Xtr, ytr, Xte, yte):
    reg, out = task["reg"], task["out"]
    net0 = s.Net(task["H"], task["out"], task["seed"] % 1000); l0 = loss_net(net0, Xte, yte, reg)
    rng = np.random.default_rng(task["seed"] + 5)
    Hf = min(512, task["H"] * 8)
    Win = rng.normal(0, 1.5, (2, Hf)); b = rng.normal(0, 1.0, Hf)
    F = lambda X: np.hstack([np.tanh(X @ Win + b), np.ones((len(X), 1))])
    T = ytr[:, None].astype(float) if reg else np.eye(out)[ytr]
    ntr = int(0.8 * len(ytr)); Ftr = F(Xtr)
    best = None
    for lam in (1e-4, 1e-2, 1.0):
        A = Ftr[:ntr].T @ Ftr[:ntr] + lam * np.eye(Hf + 1)
        beta = np.linalg.solve(A, Ftr[:ntr].T @ T[:ntr])
        S = Ftr[ntr:] @ beta
        if reg: sc = float(((S[:, 0] - ytr[ntr:]) ** 2).mean())
        else: sc = min(ce_from_probs(softmax(S * tau), ytr[ntr:]) for tau in (1, 2, 4, 8, 16))
        if best is None or sc < best[0]: best = (sc, lam)
    lam = best[1]
    beta = np.linalg.solve(Ftr.T @ Ftr + lam * np.eye(Hf + 1), Ftr.T @ T)       # TEK HAMLE: tum egitim verisiyle
    S = F(Xte) @ beta
    if reg: lf = float(((S[:, 0] - yte) ** 2).mean())
    else:
        Sv = Ftr[ntr:] @ np.linalg.solve(Ftr[:ntr].T @ Ftr[:ntr] + lam * np.eye(Hf + 1), Ftr[:ntr].T @ T[:ntr])
        tau = min((1, 2, 4, 8, 16), key=lambda tt: ce_from_probs(softmax(Sv * tt), ytr[ntr:]))
        lf = ce_from_probs(softmax(S * tau), yte)
    return min(s.CLIP, lf / max(l0, 1e-6)), Hf

def knn_test(task, Xtr, ytr, Xte, yte):
    reg, out = task["reg"], task["out"]
    net0 = s.Net(task["H"], task["out"], task["seed"] % 1000); l0 = loss_net(net0, Xte, yte, reg)
    ntr = int(0.8 * len(ytr))
    def predict(Xa, ya, Xq, h):
        D = ((Xq[:, None, :] - Xa[None, :, :]) ** 2).sum(2)
        W = np.exp(-D / (2 * h * h)) + 1e-12
        if reg: return (W @ ya) / W.sum(1)
        Pm = W @ np.eye(out)[ya] / W.sum(1, keepdims=True)
        return 0.98 * Pm + 0.02 / out
    def lossf(pred, yy):
        return float(((pred - yy) ** 2).mean()) if reg else ce_from_probs(pred, yy)
    h = min((0.05, 0.1, 0.2, 0.4, 0.8), key=lambda hh: lossf(predict(Xtr[:ntr], ytr[:ntr], Xtr[ntr:], hh), ytr[ntr:]))
    lf = lossf(predict(Xtr, ytr, Xte, h), yte)
    return min(s.CLIP, lf / max(l0, 1e-6))

def one(seed):
    t = s.make_task(seed)
    if len(t["phases"]) != 1: return None
    Xtr, ytr, Xte, yte = split(t)
    adam = min(adam_test(t, Xtr, ytr, Xte, yte, lr) for lr in (0.001, 0.003, 0.01, 0.03, 0.1, 0.3))  # gorev basina EN IYI lr (Adam'a ayricalik)
    elm, Hf = elm_test(t, Xtr, ytr, Xte, yte)
    knn = knn_test(t, Xtr, ytr, Xte, yte)
    n, H, out = len(ytr), t["H"], t["out"]; P = 3 * H + H * out + out
    ops_adam = t["steps"] * n * 6 * P
    ops_elm = n * Hf * 4 + n * Hf * Hf + Hf ** 3 // 3
    ops_knn = n * 4          # ogrenme = saklama; sorgu basina ~n*5
    return adam, elm, knn, ops_adam, ops_elm, ops_knn

if __name__ == "__main__":
    seeds = [10**7 + i for i in range(200)]
    with Pool() as p: res = [r for r in p.map(one, seeds) if r]
    R = np.array(res)
    print(f"{len(R)} tek-fazli gorev, TEST verisinde kalan kayip orani (dusuk iyi). Adam: gorev basina en iyi lr (ayricalikli).\n")
    names = ["Adam (gradyan inisi)", "ELM (tek hamle)", "Hafiza (kNN, saklama)"]
    adam = R[:, 0]
    for i, nme in enumerate(names):
        col = R[:, i]
        print(f"{nme:24} geo.ort={gm(col):.4f}  Adam'a gore {(1-gm(col)/gm(adam))*100:+6.1f}%  kazanma %{np.mean(col < adam - 1e-9) * 100:3.0f}")
    print(f"\nOGRENME maliyeti (tahmini islem sayisi, medyan):  Adam {np.median(R[:,3]):.2e}   ELM {np.median(R[:,4]):.2e}   kNN {np.median(R[:,5]):.2e}")
    print(f"ELM, Adam'dan ~{np.median(R[:,3] / R[:,4]):.0f}x az ogrenme islemi;  kNN ~{np.median(R[:,3] / R[:,5]):.0f}x az (sorgu maliyeti ayri)")
