"""Calistirma (inference): 16-bit agirlik -> kucuk tam sayi + blok olcek. GPU kucuk tam sayilarla carpar, 'donusturucu' olcegi geri carpar.
Gercek veri (el yazisi rakamlar), egitilmis agi SONRADAN sikistirir; TEST dogrulugu."""
import numpy as np
import hafiza_digits as H
from sklearn.datasets import load_digits

def quantize(W, bits, block=32):
    """W'yi (satir icinde) block'lara bol; her blok icin olcek = max|w| / (2^(bits-1)-1); w_int = round(w/olcek)."""
    q = 2 ** (bits - 1) - 1
    flat = W.reshape(-1); pad = (-len(flat)) % block
    f = np.concatenate([flat, np.zeros(pad)]).reshape(-1, block)
    scale = np.abs(f).max(1, keepdims=True) / q + 1e-12
    w_int = np.clip(np.round(f / scale), -q, q)                   # kucuk tam sayilar (GPU bunlarla carpar)
    deq = (w_int * scale).reshape(-1)[:len(flat)].reshape(W.shape)   # donusturucu: olcegi geri carp
    nbytes = (w_int.size * bits + scale.size * 16) / 8
    return deq, nbytes

def train(seed=0):
    d = load_digits(); X = d.data / 16.0; y = d.target
    rng = np.random.default_rng(seed); idx = rng.permutation(len(y)); nte = int(0.3 * len(y)); te, tr = idx[:nte], idx[nte:]
    mu, sd = X[tr].mean(0), X[tr].std(0) + 1e-6; Z = (X - mu) / sd
    # H.mlp_train donuk bir fonksiyon dondurdugu icin agirliklari kendimiz egitiyoruz (ayni Adam, H=128 daha buyuk ag)
    Hh, C = 128, 10; dd = 64
    P = [rng.normal(0, 1/np.sqrt(dd), (dd, Hh)), np.zeros(Hh), rng.normal(0, 1/np.sqrt(Hh), (Hh, C)), np.zeros(C)]
    m = [np.zeros_like(p) for p in P]; v = [np.zeros_like(p) for p in P]; Y = np.eye(C)[y[tr]]; n = len(tr)
    for t in range(1, 401):
        a = np.tanh(Z[tr] @ P[0] + P[1]); p = H.softmax(a @ P[2] + P[3]); e = (p - Y) / n; da = e @ P[2].T * (1 - a * a)
        G = [Z[tr].T @ da, da.sum(0), a.T @ e, e.sum(0)]
        for i, g in enumerate(G):
            m[i] = 0.9*m[i] + 0.1*g; v[i] = 0.999*v[i] + 0.001*g*g
            P[i] -= 0.01 * (m[i]/(1-0.9**t)) / (np.sqrt(v[i]/(1-0.999**t)) + 1e-8)
    return P, Z[te], y[te]

def evaluate(P, X, y):
    p = H.softmax(np.tanh(X @ P[0] + P[1]) @ P[2] + P[3])
    return float((p.argmax(1) == y).mean()), H.ce(p, y)

if __name__ == "__main__":
    accs = {}
    res = {}
    for seed in range(5):
        P, Xte, yte = train(seed)
        base = evaluate(P, Xte, yte); nb16 = (P[0].size + P[2].size) * 2
        res.setdefault("16 bit (referans)", []).append((base[0], base[1], nb16))
        for bits in (8, 6, 4, 3, 2):
            for block in (32,):
                W1q, b1 = quantize(P[0], bits, block); W2q, b2 = quantize(P[2], bits, block)
                a, c = evaluate([W1q, P[1], W2q, P[3]], Xte, yte)
                res.setdefault(f"{bits} bit + blok olcek", []).append((a, c, b1 + b2))
    print("El yazisi rakamlar, egitilmis agirliklar SONRADAN kucuk tam sayiya cevriliyor, 5 farkli egitim/bolme.\n")
    print(f"{'agirlik bicimi':22} {'test dogruluk':>14} {'test CE':>9} {'boyut':>10} {'kucultme':>9}")
    ref = np.mean([r[2] for r in res['16 bit (referans)']])
    for k, v in res.items():
        print(f"{k:22} {np.mean([x[0] for x in v])*100:13.1f}% {np.mean([x[1] for x in v]):9.3f} {np.mean([x[2] for x in v])/1024:8.1f} KB {ref/np.mean([x[2] for x in v]):8.1f}x")
