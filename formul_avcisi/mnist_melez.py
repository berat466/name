"""MELEZ: ozellik (gizli) katmanini gradyanla KISA egit, cikis katmanini TEK HAMLEDE (halka cozum) yeniden uret.
Karsilastirma: ayni sayida epokla yalniz Adam. MNIST, 60k/10k, test dogrulugu, duvar saati."""
import time
import numpy as np
import mnist_olcek as M

def run(H, epochs, seed=0):
    rng = np.random.default_rng(seed); d = M.Xtr.shape[1]
    P = [rng.normal(0, 1 / np.sqrt(d), (d, H)).astype(np.float32), np.zeros(H, np.float32),
         rng.normal(0, 1 / np.sqrt(H), (H, 10)).astype(np.float32), np.zeros(10, np.float32)]
    m = [np.zeros_like(p) for p in P]; v = [np.zeros_like(p) for p in P]; t = 0; bs = 256
    t0 = time.time()
    for ep in range(epochs):
        idx = rng.permutation(60000)
        for s in range(0, 60000, bs):
            b = idx[s:s + bs]; xb = M.Xtr[b]; Y = np.eye(10, dtype=np.float32)[M.ytr[b]]; t += 1
            a = np.tanh(xb @ P[0] + P[1]); e = (M.softmax(a @ P[2] + P[3]) - Y) / len(b); da = e @ P[2].T * (1 - a * a)
            G = [xb.T @ da, da.sum(0), a.T @ e, e.sum(0)]
            for i, g in enumerate(G):
                m[i] = 0.9 * m[i] + 0.1 * g; v[i] = 0.999 * v[i] + 0.001 * g * g
                P[i] -= 0.001 * (m[i] / (1 - 0.9 ** t)) / (np.sqrt(v[i] / (1 - 0.999 ** t)) + 1e-8)
    t_adam = time.time() - t0
    acc_adam = float((np.argmax(np.tanh(M.Xte @ P[0] + P[1]) @ P[2] + P[3], 1) == M.yte).mean())
    # --- tek hamle: cikis katmanini halka cozumle yeniden uret (ozellikler sabit, egitilmis gizli katman)
    t1 = time.time(); F = np.hstack([np.tanh(M.Xtr @ P[0] + P[1]), np.ones((60000, 1), np.float32)]).astype(np.float64)
    S = F.T @ F; R = F.T @ np.eye(10)[M.ytr]; best = None; nva = 5000
    Fv = F[-nva:]; Sf = S - Fv.T @ Fv; Rf = R - Fv.T @ np.eye(10)[M.ytr[-nva:]]
    for lam in (0.1, 1.0, 10.0, 100.0):
        beta = np.linalg.solve(Sf + lam * np.eye(H + 1), Rf); a_v = float((np.argmax(Fv @ beta, 1) == M.ytr[-nva:]).mean())
        if best is None or a_v > best[0]: best = (a_v, lam)
    beta = np.linalg.solve(S + best[1] * np.eye(H + 1), R)
    t_solve = time.time() - t1
    Ft = np.hstack([np.tanh(M.Xte @ P[0] + P[1]), np.ones((10000, 1), np.float32)]).astype(np.float64)
    acc_mel = float((np.argmax(Ft @ beta, 1) == M.yte).mean())
    return acc_adam, t_adam, acc_mel, t_adam + t_solve

if __name__ == "__main__":
    print("MNIST, test dogrulugu. 'yalniz Adam' = ayni epokla gradyan; 'melez' = ayni epok + cikis katmani tek hamlede.\n")
    print(f"{'ag':>10} {'epok':>5} {'yalniz Adam':>12} {'sure':>7} {'MELEZ':>9} {'toplam sure':>12}")
    for H in (256, 1024):
        for ep in (1, 2, 3, 5):
            a, ta, b, tb = run(H, ep)
            print(f"{'784-%d-10' % H:>10} {ep:>5} {a*100:11.2f}% {ta:6.1f}s {b*100:8.2f}% {tb:11.1f}s", flush=True)
