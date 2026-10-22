"""Kucuk uzayda egit, tam agirligi uret: theta = theta0 + P z   (z: d kucuk sayi, P: sabit donusturucu, theta: D tam agirlik)
GPU/ag HER ZAMAN tam D agirlikla calisir; yalniz d sayi egitilir. Gercek veri: el yazisi rakamlar, TEST dogrulugu."""
import numpy as np
from sklearn.datasets import load_digits

H, C, d_in = 64, 10, 64
D = d_in * H + H + H * C + C

def unpack(th):
    i = 0; W1 = th[i:i + d_in * H].reshape(d_in, H); i += d_in * H
    b1 = th[i:i + H]; i += H; W2 = th[i:i + H * C].reshape(H, C); i += H * C; return W1, b1, W2, th[i:i + C]

def softmax(z):
    z = z - z.max(1, keepdims=True); p = np.exp(z); return p / p.sum(1, keepdims=True)

def grad_theta(th, X, Y):
    W1, b1, W2, b2 = unpack(th); a = np.tanh(X @ W1 + b1); p = softmax(a @ W2 + b2); n = len(X); e = (p - Y) / n
    da = e @ W2.T * (1 - a * a)
    return np.concatenate([(X.T @ da).ravel(), da.sum(0), (a.T @ e).ravel(), e.sum(0)])

def predict(th, X):
    W1, b1, W2, b2 = unpack(th); return softmax(np.tanh(X @ W1 + b1) @ W2 + b2)

def train(theta0, P, X, y, lr, steps=400):
    Y = np.eye(C)[y]; z = np.zeros(P.shape[1] if P is not None else D)
    if P is None: z = theta0.copy()
    m = np.zeros_like(z); v = np.zeros_like(z)
    for t in range(1, steps + 1):
        th = z if P is None else theta0 + P @ z
        g = grad_theta(th, X, Y); g = g if P is None else P.T @ g                 # zincir kurali: dL/dz = P^T dL/dtheta
        m = 0.9 * m + 0.1 * g; v = 0.999 * v + 0.001 * g * g
        z = z - lr * (m / (1 - 0.9 ** t)) / (np.sqrt(v / (1 - 0.999 ** t)) + 1e-8)
    return z if P is None else theta0 + P @ z

def run(seed, d):
    dg = load_digits(); X = dg.data / 16.0; y = dg.target
    rng = np.random.default_rng(seed); idx = rng.permutation(len(y)); nte = int(0.3 * len(y))
    te, tr = idx[:nte], idx[nte:]; nva = int(0.2 * len(tr)); va, fit = tr[:nva], tr[nva:]
    Z = (X - X[tr].mean(0)) / (X[tr].std(0) + 1e-6)
    th0 = np.concatenate([rng.normal(0, 1 / np.sqrt(d_in), d_in * H), np.zeros(H), rng.normal(0, 1 / np.sqrt(H), H * C), np.zeros(C)])
    P = None if d >= D else rng.normal(0, 1, (D, d)) / np.sqrt(d)
    best = None
    for lr in (0.003, 0.01, 0.03, 0.1):
        th = train(th0, P, Z[fit], y[fit], lr)
        sc = -np.log(np.clip(predict(th, Z[va])[np.arange(len(va)), y[va]], 1e-9, 1)).mean()
        if best is None or sc < best[0]: best = (sc, lr)
    th = train(th0, P, Z[tr], y[tr], best[1])
    return float((predict(th, Z[te]).argmax(1) == y[te]).mean())

if __name__ == "__main__":
    print(f"Tam ag D = {D} agirlik (GPU hep bunlarla calisir). Yalniz d tane kucuk sayi egitiliyor. TEST dogrulugu, 3 bolme.\n")
    print(f"{'egitilen sayi d':>16} {'kucultme (D/d)':>15} {'test dogruluk':>14}")
    for d in (D, 2000, 1000, 500, 250, 100, 50, 20):
        acc = np.mean([run(s, d) for s in range(3)])
        print(f"{d:>16} {D/d:>14.1f}x {acc*100:13.1f}%")
