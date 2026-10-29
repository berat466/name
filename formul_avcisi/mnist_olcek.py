"""MNIST (60k egitim / 10k test): klasik mini-batch Adam MLP vs tek-hamle (ELM + halka cozum), ozellik sayisi buyuyerek.
Tek hamle: S = sum_parca F^T F (TOPLANABILIR, parca parca), R = F^T T ; lambda dogrulamayla (5k) secilir; sonra S_fit + S_val ile son cozum.
Zaman = duvar saati (numpy BLAS, 4 cekirdek)."""
import time
import numpy as np

X = np.load("/tmp/mnist_X.npy"); y = np.load("/tmp/mnist_y.npy")
mu = X[:60000].mean(0); sd = X[:60000].std()
X = ((X - mu) / sd).astype(np.float32)
Xtr, ytr, Xte, yte = X[:60000], y[:60000], X[60000:], y[60000:]
C = 10

def softmax(z):
    z = z - z.max(1, keepdims=True); p = np.exp(z); return p / p.sum(1, keepdims=True)

def mlp(H, epochs, lr, bs=256, seed=0):
    rng = np.random.default_rng(seed); d = Xtr.shape[1]
    P = [rng.normal(0, 1 / np.sqrt(d), (d, H)).astype(np.float32), np.zeros(H, np.float32),
         rng.normal(0, 1 / np.sqrt(H), (H, C)).astype(np.float32), np.zeros(C, np.float32)]
    m = [np.zeros_like(p) for p in P]; v = [np.zeros_like(p) for p in P]; t = 0
    t0 = time.time()
    for ep in range(epochs):
        idx = rng.permutation(len(ytr))
        for s in range(0, len(idx), bs):
            b = idx[s:s + bs]; xb = Xtr[b]; Y = np.eye(C, dtype=np.float32)[ytr[b]]; t += 1
            a = np.tanh(xb @ P[0] + P[1]); e = (softmax(a @ P[2] + P[3]) - Y) / len(b); da = e @ P[2].T * (1 - a * a)
            G = [xb.T @ da, da.sum(0), a.T @ e, e.sum(0)]
            for i, g in enumerate(G):
                m[i] = 0.9 * m[i] + 0.1 * g; v[i] = 0.999 * v[i] + 0.001 * g * g
                P[i] -= lr * (m[i] / (1 - 0.9 ** t)) / (np.sqrt(v[i] / (1 - 0.999 ** t)) + 1e-8)
    dt = time.time() - t0
    acc = float((np.argmax(np.tanh(Xte @ P[0] + P[1]) @ P[2] + P[3], 1) == yte).mean())
    return acc, dt, 6.0 * len(ytr) * epochs * (X.shape[1] * H + H * C)

def elm(Hf, seed=0, chunk=5000):
    rng = np.random.default_rng(seed); d = Xtr.shape[1]
    Win = (rng.normal(0, 1 / np.sqrt(d), (d, Hf))).astype(np.float32); b = rng.normal(0, 0.5, Hf).astype(np.float32)
    feat = lambda Xb: np.hstack([np.tanh(Xb @ Win + b), np.ones((len(Xb), 1), np.float32)])
    m = Hf + 1; nfit = 55000
    t0 = time.time()
    def stats(lo, hi):
        S = np.zeros((m, m)); R = np.zeros((m, C))
        for s in range(lo, hi, chunk):
            Fb = feat(Xtr[s:min(s + chunk, hi)]); S += Fb.T @ Fb; R += Fb.T @ np.eye(C, dtype=np.float32)[ytr[s:min(s + chunk, hi)]]
        return S, R
    Sf, Rf = stats(0, nfit); Sv, Rv = stats(nfit, 60000)           # toplanabilir istatistikler
    Fva = feat(Xtr[nfit:60000]); best = None
    for lam in (1.0, 10.0, 100.0, 1000.0):
        beta = np.linalg.solve(Sf + lam * np.eye(m), Rf); acc_v = float((np.argmax(Fva @ beta, 1) == ytr[nfit:60000]).mean())
        if best is None or acc_v > best[0]: best = (acc_v, lam)
    beta = np.linalg.solve(Sf + Sv + best[1] * np.eye(m), Rf + Rv)  # son cozum: tum 60k
    dt = time.time() - t0
    acc = float((np.argmax(feat(Xte) @ beta, 1) == yte).mean())
    return acc, dt, best[1], 60000 * m * m + m ** 3 / 3, (m * m * 8) / 1e6

if __name__ == "__main__":
    print("MNIST, 60k egitim / 10k test (standart bolme). Dogruluk TEST uzerinde.\n", flush=True)
    print(f"{'yontem':36} {'dogruluk':>9} {'sure (sn)':>10} {'~islem':>10} {'S bellegi':>11}", flush=True)
    for H, ep in ((256, 10), (1024, 10)):
        a, dt, fl = mlp(H, ep, 0.001)
        print(f"{'Adam MLP 784-%d-10 (%d epok)' % (H, ep):36} {a*100:8.2f}% {dt:10.1f} {fl:10.1e} {'-':>11}", flush=True)
    for Hf in (1024, 2048, 4096, 8192):
        a, dt, lam, fl, mb = elm(Hf)
        print(f"{'Tek hamle ELM Hf=%d (lam=%g)' % (Hf, lam):36} {a*100:8.2f}% {dt:10.1f} {fl:10.1e} {mb:9.0f}MB", flush=True)
