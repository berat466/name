"""Klasik agirlik uretiminin TEK adimi, tek fonksiyonda: ileri gecis -> kayip -> geri gecis -> Adam guncellemesi.
Calistir: python formul_adim.py   (el yazisi rakamlar, 64-32-10 ag, tam parti, test dogrulugunu yazar)"""
import numpy as np
from sklearn.datasets import load_digits

def adim(P, S, X, Y, t, lr=0.01, b1=0.9, b2=0.999, eps=1e-8):
    W1, c1, W2, c2 = P                                            # agirliklar ve kaymalar
    a = np.tanh(X @ W1 + c1)                                      # a = tanh(W1 x + b1)         (gizli katman)
    z = a @ W2 + c2; p = np.exp(z - z.max(1, keepdims=True)); p /= p.sum(1, keepdims=True)   # p = softmax(W2 a + b2)
    kayip = -np.log(p[np.arange(len(X)), Y.argmax(1)] + 1e-12).mean()                         # L = -log p[y]
    e = (p - Y) / len(X)                                          # e = p - onehot(y)           (cikis hatasi)
    da = (e @ W2.T) * (1 - a * a)                                 # (W2^T e) (.) (1 - a^2)      (gizli katman hatasi)
    g = [X.T @ da, da.sum(0), a.T @ e, e.sum(0)]                  # gradyanlar: dW1, db1, dW2, db2
    for i in range(4):                                            # Adam: P <- P - lr * m^ / (sqrt(v^) + eps)
        S["m"][i] = b1 * S["m"][i] + (1 - b1) * g[i]
        S["v"][i] = b2 * S["v"][i] + (1 - b2) * g[i] ** 2
        P[i] -= lr * (S["m"][i] / (1 - b1 ** t)) / (np.sqrt(S["v"][i] / (1 - b2 ** t)) + eps)
    return kayip

if __name__ == "__main__":
    d = load_digits(); X = d.data / 16.0; y = d.target
    idx = np.random.default_rng(0).permutation(len(y)); te, tr = idx[:540], idx[540:]
    X = (X - X[tr].mean(0)) / (X[tr].std(0) + 1e-6); Y = np.eye(10)[y]
    rng = np.random.default_rng(1); H = 32
    P = [rng.normal(0, 1 / 8, (64, H)), np.zeros(H), rng.normal(0, 1 / np.sqrt(H), (H, 10)), np.zeros(10)]
    S = {"m": [np.zeros_like(p) for p in P], "v": [np.zeros_like(p) for p in P]}
    for t in range(1, 301):
        L = adim(P, S, X[tr], Y[tr], t)
        if t in (1, 10, 50, 100, 200, 300):
            acc = (np.argmax(np.tanh(X[te] @ P[0] + P[1]) @ P[2] + P[3], 1) == y[te]).mean()
            print(f"adim {t:3d}: egitim kaybi {L:.4f}   test dogrulugu {acc*100:5.1f}%")
