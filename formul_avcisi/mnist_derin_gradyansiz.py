"""Derinlik testi: 2 katmanda calisan 'once ustu coz, alttakini ona gore' kurali L gizli katmanda ne yapiyor?
A) alt katmanlar rastgele kalir, sadece cikis kapali formda (digerleri 'islenmemis')
B) hedef yayilimi: her katman ust katmandan inen hedefe ridge ile cozulur (gradyansiz), tur tekrari
C) Adam (geri yayilim) ayni derinlik. MNIST 60k/10k test dogrulugu."""
import time, sys
import numpy as np
import mnist_olcek as M

n, d = M.Xtr.shape
T = np.eye(10)[M.ytr]
def aug(a): return np.hstack([a, np.ones((len(a), 1), a.dtype)])

def init(L, H, rng):
    dims = [d] + [H] * L
    return [rng.normal(0, 1 / np.sqrt(dims[i] + 1), (dims[i] + 1, H)).astype(np.float64) for i in range(L)]

def fwd(Ws, X):
    hs = [X.astype(np.float64)]
    for W in Ws: hs.append(np.tanh(aug(hs[-1]) @ W))
    return hs

def readout(hs, lam=10.0):
    F = aug(hs[-1]); return np.linalg.solve(F.T @ F + lam * np.eye(F.shape[1]), F.T @ T)

def acc(Ws, Wo):
    return float((np.argmax(aug(fwd(Ws, M.Xte)[-1]) @ Wo, 1) == M.yte).mean())

def run_random(L, H, seed=0):
    t0 = time.time(); Ws = init(L, H, np.random.default_rng(seed)); Wo = readout(fwd(Ws, M.Xtr))
    return acc(Ws, Wo), time.time() - t0

def run_tp(L, H, rounds, seed=0, lam=10.0, mu=1e-3, beta=0.5):
    t0 = time.time(); Ws = init(L, H, np.random.default_rng(seed)); out = []
    Gs = [None] * L
    for r in range(rounds + 1):
        hs = fwd(Ws, M.Xtr); Wo = readout(hs)
        out.append((r, acc(Ws, Wo), time.time() - t0))
        if r == rounds: break
        E = T - aug(hs[-1]) @ Wo; A = Wo[:-1]
        tgt = np.clip(hs[-1] + E @ np.linalg.solve(A.T @ A + mu * np.eye(10), A.T), -.97, .97)
        for l in range(L - 1, -1, -1):                       # ustten alta
            Xa = aug(hs[l])
            if Gs[l] is None: Gs[l] = np.linalg.inv(Xa.T @ Xa + lam * np.eye(Xa.shape[1]))
            Wnew = Gs[l] @ (Xa.T @ np.arctanh(tgt))          # bu katman: tek kapali cozum
            Wold = Ws[l]; Ws[l] = (1 - beta) * Wold + beta * Wnew
            if l > 0:
                pre_t = np.arctanh(tgt); pre_now = Xa @ Wold
                Wl = Wold[:-1]                               # girdi-uzayina hedef: en kucuk degisim
                dh = (pre_t - pre_now) @ np.linalg.solve(Wl.T @ Wl + mu * np.eye(Wl.shape[1]), Wl.T)
                tgt = np.clip(hs[l] + dh, -.97, .97)
    return out

def run_adam(L, H, epochs, lr=1e-3, bs=256, seed=0):
    rng = np.random.default_rng(seed); Ws = [w.astype(np.float32) for w in init(L, H, rng)]
    Ws.append(rng.normal(0, 1 / np.sqrt(H + 1), (H + 1, 10)).astype(np.float32))
    m = [np.zeros_like(w) for w in Ws]; v = [np.zeros_like(w) for w in Ws]; t = 0; t0 = time.time()
    X = M.Xtr.astype(np.float32)
    for ep in range(epochs):
        idx = rng.permutation(n)
        for i in range(0, n, bs):
            b = idx[i:i + bs]; hs = [X[b]]
            for W in Ws[:-1]: hs.append(np.tanh(aug(hs[-1]) @ W))
            z = aug(hs[-1]) @ Ws[-1]; z -= z.max(1, keepdims=True); p = np.exp(z); p /= p.sum(1, keepdims=True)
            e = (p - T[b].astype(np.float32)) / len(b); t += 1
            for l in range(L, -1, -1):
                g = aug(hs[l]).T @ e
                if l > 0: e = (e @ Ws[l][:-1].T) * (1 - hs[l] ** 2)
                m[l] = .9 * m[l] + .1 * g; v[l] = .999 * v[l] + .001 * g * g
                Ws[l] -= lr * (m[l] / (1 - .9 ** t)) / (np.sqrt(v[l] / (1 - .999 ** t)) + 1e-8)
    h = M.Xte.astype(np.float32)
    for W in Ws[:-1]: h = np.tanh(aug(h) @ W)
    return float((np.argmax(aug(h) @ Ws[-1], 1) == M.yte).mean()), time.time() - t0

if __name__ == "__main__":
    H = 256
    for L in (1, 2, 3, 4):
        a, t = run_random(L, H)
        print(f"L={L} A) alt katmanlar rastgele, sadece cikis kapali: %{a*100:.2f} ({t:.1f}sn)", flush=True)
        o = run_tp(L, H, 6)
        print(f"L={L} B) hedef yayilimi: " + "  ".join(f"t{r}: %{a*100:.2f}({s:.0f}sn)" for r, a, s in o), flush=True)
        a, t = run_adam(L, H, 5)
        print(f"L={L} C) Adam 5 epok: %{a*100:.2f} ({t:.0f}sn)\n", flush=True)
