"""GERCEK veri (el yazisi rakamlar, 64 boyut, 10 sinif): gradyan inisi vs tek-hamle (ELM) vs hafiza (kNN).
Hiperparametreler EGITIM icinden ayrilan dogrulama kismiyla secilir; sonuc TEST verisinde (%30), 5 farkli bolme."""
import time
import numpy as np
from sklearn.datasets import load_digits

def softmax(z):
    z = z - z.max(1, keepdims=True); p = np.exp(z); return p / p.sum(1, keepdims=True)
def ce(P, y): return float(-np.log(np.clip(P[np.arange(len(y)), y], 1e-9, 1)).mean())
def acc(P, y): return float((P.argmax(1) == y).mean())

def mlp_train(Xtr, ytr, Xva, yva, H, steps, lr, seed):
    rng = np.random.default_rng(seed); d = Xtr.shape[1]; C = 10
    W1 = rng.normal(0, 1 / np.sqrt(d), (d, H)); b1 = np.zeros(H); W2 = rng.normal(0, 1 / np.sqrt(H), (H, C)); b2 = np.zeros(C)
    P = [W1, b1, W2, b2]; m = [np.zeros_like(p) for p in P]; v = [np.zeros_like(p) for p in P]; n = len(ytr); Y = np.eye(C)[ytr]
    for t in range(1, steps + 1):
        a = np.tanh(Xtr @ P[0] + P[1]); p = softmax(a @ P[2] + P[3]); e = (p - Y) / n
        da = e @ P[2].T * (1 - a * a)
        G = [Xtr.T @ da, da.sum(0), a.T @ e, e.sum(0)]
        for i, g in enumerate(G):
            m[i] = 0.9 * m[i] + 0.1 * g; v[i] = 0.999 * v[i] + 0.001 * g * g
            P[i] -= lr * (m[i] / (1 - 0.9 ** t)) / (np.sqrt(v[i] / (1 - 0.999 ** t)) + 1e-8)
    return lambda X: softmax(np.tanh(X @ P[0] + P[1]) @ P[2] + P[3])

def elm_fit(Xtr, ytr, Hf, lam, seed):
    rng = np.random.default_rng(seed); d = Xtr.shape[1]
    Win = rng.normal(0, 1 / np.sqrt(d), (d, Hf)); b = rng.normal(0, 0.5, Hf)
    F = lambda X: np.hstack([np.tanh(X @ Win + b), np.ones((len(X), 1))])
    Ft = F(Xtr); beta = np.linalg.solve(Ft.T @ Ft + lam * np.eye(Hf + 1), Ft.T @ np.eye(10)[ytr])
    return lambda X, tau: softmax((F(X) @ beta) * tau)

def knn_pred(Xa, ya, Xq, h):
    D = (Xq ** 2).sum(1)[:, None] + (Xa ** 2).sum(1)[None, :] - 2 * Xq @ Xa.T
    W = np.exp(-(D - D.min(1, keepdims=True)) / (2 * h * h)) + 1e-30
    return 0.98 * (W @ np.eye(10)[ya]) / W.sum(1, keepdims=True) + 0.002

def run(seed):
    d = load_digits(); X = d.data / 16.0; y = d.target
    rng = np.random.default_rng(seed); idx = rng.permutation(len(y)); nte = int(0.3 * len(y))
    te, tr = idx[:nte], idx[nte:]; nva = int(0.2 * len(tr)); va, fit = tr[:nva], tr[nva:]
    mu, sd = X[tr].mean(0), X[tr].std(0) + 1e-6; Z = (X - mu) / sd
    out = {}
    # --- Adam (MLP 64-64-10), 300 adim; lr dogrulamayla secilir
    t0 = time.time(); best = None
    for lr in (0.001, 0.003, 0.01, 0.03):
        f = mlp_train(Z[fit], y[fit], Z[va], y[va], 64, 300, lr, seed)
        sc = ce(f(Z[va]), y[va])
        if best is None or sc < best[0]: best = (sc, lr)
    f = mlp_train(Z[tr], y[tr], None, None, 64, 300, best[1], seed)      # secilen lr ile tum egitim verisi
    P = f(Z[te]); out["Adam MLP"] = (ce(P, y[te]), acc(P, y[te]), time.time() - t0, 300 * len(tr) * 6 * (64 * 64 + 64 + 640 + 10))
    # --- ELM tek hamle: Hf 256 / 1024
    for Hf in (256, 1024):
        t0 = time.time(); best = None
        for lam in (1e-2, 1e-1, 1.0, 10.0):
            g = elm_fit(Z[fit], y[fit], Hf, lam, seed)
            for tau in (1, 2, 4, 8, 16):
                sc = ce(g(Z[va], tau), y[va])
                if best is None or sc < best[0]: best = (sc, lam, tau)
        g = elm_fit(Z[tr], y[tr], Hf, best[1], seed); P = g(Z[te], best[2])
        out[f"ELM tek hamle Hf={Hf}"] = (ce(P, y[te]), acc(P, y[te]), time.time() - t0, len(tr) * Hf * Hf + Hf ** 3 // 3 + len(tr) * Hf * 128)
    # --- Hafiza kNN
    t0 = time.time(); best = None
    for h in (2, 3, 4, 6, 8):
        sc = ce(knn_pred(Z[fit], y[fit], Z[va], h), y[va])
        if best is None or sc < best[0]: best = (sc, h)
    P = knn_pred(Z[tr], y[tr], Z[te], best[1])
    out["Hafiza (kNN)"] = (ce(P, y[te]), acc(P, y[te]), time.time() - t0, len(tr))        # ogrenme ~ saklama; sorgu basina n*64*2
    return out

if __name__ == "__main__":
    runs = [run(sd) for sd in range(5)]
    print("El yazisi rakamlar, TEST verisi (%30), 5 farkli bolme. Dogrulamayla secilen hiperparametre.\n")
    print(f"{'yontem':24} {'test dogruluk':>14} {'test CE':>9} {'sure(sn)':>9} {'ogrenme islemi (tahmini)':>26}")
    for k in runs[0]:
        a = [r[k][1] for r in runs]; c = [r[k][0] for r in runs]; tm = np.mean([r[k][2] for r in runs]); op = np.mean([r[k][3] for r in runs])
        print(f"{k:24} {np.mean(a)*100:11.1f}%  {np.mean(c):9.3f} {tm:9.1f} {op:26.2e}")
