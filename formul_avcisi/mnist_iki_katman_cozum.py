"""GRADYANSIZ iki katman: once cikis katmanini coz, sonra gizli katmana HEDEF bul (pseudo-ters ile), birinci katmani en kucuk karelerle coz.
Alternatif cozumler (hedef yayilimi). Her tur: sadece dogrusal cozumler. MNIST 60k/10k, test dogrulugu, duvar saati."""
import sys, time
import numpy as np
import mnist_olcek as M

def run(H, rounds, beta, lam1=10.0, lam2=10.0, mu=1e-3, seed=0):
    rng = np.random.default_rng(seed); n, d = M.Xtr.shape
    Xa = np.hstack([M.Xtr, np.ones((n, 1), np.float32)]).astype(np.float64)
    Xt = np.hstack([M.Xte, np.ones((len(M.Xte), 1), np.float32)]).astype(np.float64)
    T = np.eye(10)[M.ytr]; t0 = time.time()
    Ginv = np.linalg.inv(Xa.T @ Xa + lam1 * np.eye(d + 1))                        # bir kez
    XaT = Xa.T
    W1 = rng.normal(0, 1 / np.sqrt(d), (d + 1, H)); W1[-1] = 0
    log = []
    for r in range(rounds + 1):
        Fh = np.tanh(Xa @ W1); F = np.hstack([Fh, np.ones((n, 1))])
        W2 = np.linalg.solve(F.T @ F + lam2 * np.eye(H + 1), F.T @ T)             # 1) cikis katmani: tek hamle
        acc = float((np.argmax(np.hstack([np.tanh(Xt @ W1), np.ones((len(Xt), 1))]) @ W2, 1) == M.yte).mean())
        log.append((r, acc, time.time() - t0))
        if r == rounds: break
        E = T - F @ W2                                                            # cikis artigi
        Wh = W2[:-1]                                                              # H x 10
        dH = E @ np.linalg.solve(Wh.T @ Wh + mu * np.eye(10), Wh.T)               # 2) gizli hedef: cikisi duzelten en kucuk degisim
        Hstar = np.clip(Fh + dH, -0.97, 0.97)
        W1new = Ginv @ (XaT @ np.arctanh(Hstar))                                  # 3) birinci katman: tek cozum
        W1 = (1 - beta) * W1 + beta * W1new
    return log

if __name__ == "__main__":
    print("MNIST, gradyansiz iki katman (alternatif tek cozumler). Referans: Adam 256 gizli 10 epok %97.46 @18sn; Adam 1024 %97.82 @41sn; salt ELM 1024 oz. %93.59 @1.1sn\n", flush=True)
    for H, beta in ((256, 0.5), (1024, 0.5), (1024, 1.0)):
        log = run(H, 8, beta)
        print(f"H={H} beta={beta}:", "  ".join(f"tur{r}: %{a*100:.2f} ({t:.0f}sn)" for r, a, t in log), flush=True)
