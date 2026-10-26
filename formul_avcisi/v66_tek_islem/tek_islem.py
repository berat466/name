#!/usr/bin/env python3
"""V66: TEK ISLEM = korelasyon-birikimi (capraz-kovaryans / varyans). Cozucu yok: Gram yok, Cholesky yok, Schur yok, ridge yok, SGD yok.
Tanim:  istatistikler  S = (n, sum(phi), sum(phi^2), sum(y), phi^T y)  [tek gecis, artimli: yeni orneklerle S += ...]
        W = ( phi^T y / n - mean(phi) mean(y)^T ) / ( var(phi) + eps )         (oge-bazli bolme; matris tersi YOK)
        tahmin: yhat = mean(y) + s * (phi - mean(phi)) W    (siniflamada s onemsiz: argmax)  
        s: tek skaler (egitim istatistiklerinden: <yhat0-ybar, y-ybar>/<yhat0-ybar,yhat0-ybar>) -- yalniz regresyonda.
Ozellik haritasi phi: motordakiyle ayni sabit rastgele tanh(gamma(xW1+b)) (egitilmez)."""
import numpy as np, time, tracemalloc, json, sys
from sklearn.datasets import load_digits, load_diabetes
from sklearn.model_selection import train_test_split
class Birikim:
    def __init__(s, H, C): s.n = 0; s.sp = np.zeros(H); s.sp2 = np.zeros(H); s.sy = np.zeros(C); s.pty = np.zeros((H, C)); s.ops = 0
    def ekle(s, P, Y):   # tek islem: birikim
        s.n += len(P); s.sp += P.sum(0); s.sp2 += (P * P).sum(0); s.sy += Y.sum(0); s.pty += P.T @ Y; s.ops += P.shape[0] * P.shape[1] * (Y.shape[1] + 2) 
    def agirlik(s, eps=1e-8):
        mp = s.sp / s.n; my = s.sy / s.n; var = s.sp2 / s.n - mp ** 2; return (s.pty / s.n - np.outer(mp, my)) / (var[:, None] + eps), mp, my
def feat(X, W1, b, g): return np.tanh(g * (X @ W1 + b))
R = {}
# ---------------- A: islem belirleme (aday ilkel islem kutuphanesi + ayni birikim) ----------------
rng = np.random.default_rng(0); lib = {"+": lambda a, b: a + b, "-": lambda a, b: a - b, "*": lambda a, b: a * b, "/": lambda a, b: a / b}
A = {}
for op, fn in lib.items():
    a = rng.uniform(1, 20, 400); b = rng.uniform(1, 20, 400); y = fn(a, b)
    P = np.stack([f(a, b) for f in lib.values()], 1); acc = Birikim(4, 1); acc.ekle(P, y[:, None]); W, mp, my = acc.agirlik()
    sd = np.sqrt(acc.sp2 / acc.n - mp ** 2); sy = np.sqrt(acc.sy2 / acc.n - my ** 2) if hasattr(acc, "sy2") else y.std()
    corr = np.abs(W[:, 0] * sd / sy); k = int(np.argmax(corr)); sec = list(lib)[k]
    a2 = rng.uniform(50, 500, 200); b2 = rng.uniform(50, 500, 200); y2 = fn(a2, b2); pred = lib[sec](a2, b2) * W[k, 0] + (my[0] - W[k, 0] * mp[k])   # egitimde gorulmeyen ARALIK (50-500)
    A[op] = dict(secilen=sec, dogru_secim=sec == op, korelasyonlar=[round(float(c), 4) for c in corr], egim=float(W[k, 0]), goreli_hata_gorulmeyen=float(np.max(np.abs(pred - y2) / np.abs(y2))))
R["A"] = A
# ---------------- B: siniflandirma (digits) ----------------
d = load_digits(); X, y = d.data.astype(float), d.target; C = 10
Xtr, Xr, ytr, yr = train_test_split(X, y, test_size=0.4, random_state=0, stratify=y); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0, stratify=yr)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]
H = 512; r = np.random.default_rng(1); W1 = r.standard_normal((64, H)) / 8.0; b = r.standard_normal(H) * 0.1; Ytr = np.eye(C)[ytr]
best = None
for g in (0.05, 0.1, 0.2, 0.5, 1.0, 2.0, 4.0):
    P = feat(Xtr, W1, b, g); acc = Birikim(H, C); acc.ekle(P, Ytr); W, mp, my = acc.agirlik(); va = np.mean(np.argmax((feat(Xva, W1, b, g) - mp) @ W, 1) == yva)
    if best is None or va > best[0]: best = (va, g)
g = best[1]; tracemalloc.start(); t0 = time.perf_counter(); P = feat(Xtr, W1, b, g); acc = Birikim(H, C); acc.ekle(P, Ytr); W, mp, my = acc.agirlik(); tw = time.perf_counter() - t0; peak = tracemalloc.get_traced_memory()[1] / 1e6; tracemalloc.stop()
te = np.mean(np.argmax((feat(Xte, W1, b, g) - mp) @ W, 1) == yte); tr = np.mean(np.argmax((P - mp) @ W, 1) == ytr)
ys = np.random.default_rng(2).permutation(ytr); acs = Birikim(H, C); acs.ekle(P, np.eye(C)[ys]); Ws, mps, _ = acs.agirlik(); te_shuf = np.mean(np.argmax((feat(Xte, W1, b, g) - mps) @ Ws, 1) == yte)
# artimli: veri 3 parcada gelir; ayni agirlik (onceki hesap tekrarlanmaz)
ai = Birikim(H, C)
for sl in np.array_split(np.arange(len(Xtr)), 3): ai.ekle(P[sl], Ytr[sl])
Wi, _, _ = ai.agirlik()
# referans: AYNI ozellikler uzerinde klasik ridge zinciri (Gram+Cholesky) -- yalniz karsilastirma
t0 = time.perf_counter(); Pc = np.hstack([np.ones((len(P), 1)), P]); best_r = None
for lam in (0.1, 1, 10, 100):
    Wr = np.linalg.solve(Pc.T @ Pc + lam * np.eye(H + 1), Pc.T @ Ytr); vr = np.mean(np.argmax(np.hstack([np.ones((len(Xva), 1)), feat(Xva, W1, b, g)]) @ Wr, 1) == yva)
    if best_r is None or vr > best_r[0]: best_r = (vr, lam, Wr)
tr_ridge = time.perf_counter() - t0; Wr = best_r[2]; te_ridge = np.mean(np.argmax(np.hstack([np.ones((len(Xte), 1)), feat(Xte, W1, b, g)]) @ Wr, 1) == yte)
n = len(Xtr); R["B_C"] = dict(gamma=g, val_dogruluk=float(best[0]), egitim_dogr=float(tr), test_dogr_gorulmeyen=float(te), etiket_karisik_test=float(te_shuf), cogunluk=float(np.mean(yte == np.bincount(ytr).argmax())),
   artimli_agirlik_farki_maks=float(np.abs(W - Wi).max()), agirlik_uretim_suresi_s=tw, tepe_RAM_MB=peak, islem_madd_tek_islem=float(n * H * (C + 2)), uretilen_agirlik_sayisi=int(W.size),
   ridge_ayni_ozellik=dict(test_dogr=float(te_ridge), lam=best_r[1], sure_s_4lam_taramasi=tr_ridge, islem_madd_tek_lam=float(n * (H + 1) ** 2 + (H + 1) ** 3 / 3 + n * (H + 1) * C), cozucu="Gram+Cholesky/solve"))
# ---------------- C: regresyon (diabetes) ----------------
d = load_diabetes(); Xd, yd = d.data, d.target; Xtr, Xr, ytr, yr = train_test_split(Xd, yd, test_size=0.4, random_state=0); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; r = np.random.default_rng(1); W1 = r.standard_normal((10, H)) / np.sqrt(10); b = r.standard_normal(H) * 0.1
def met(yt, p): e = yt - p; return dict(MAE=float(np.abs(e).mean()), RMSE=float(np.sqrt((e ** 2).mean())), R2=float(1 - (e ** 2).sum() / ((yt - yt.mean()) ** 2).sum()))
best = None
for g in (0.1, 0.3, 1.0):
    P = feat(Xtr, W1, b, g); acc = Birikim(H, 1); acc.ekle(P, ytr[:, None]); W, mp, my = acc.agirlik(); h0 = (P - mp) @ W; s = float(((h0[:, 0]) * (ytr - my[0])).sum() / (h0[:, 0] ** 2).sum())
    pv = my[0] + s * ((feat(Xva, W1, b, g) - mp) @ W)[:, 0]; v = met(yva, pv)["RMSE"]
    if best is None or v < best[0]: best = (v, g, s)
g = best[1]; P = feat(Xtr, W1, b, g); t0 = time.perf_counter(); acc = Birikim(H, 1); acc.ekle(P, ytr[:, None]); W, mp, my = acc.agirlik(); h0 = (P - mp) @ W; s = float(((h0[:, 0]) * (ytr - my[0])).sum() / (h0[:, 0] ** 2).sum()); tw = time.perf_counter() - t0
pt = my[0] + s * ((feat(Xte, W1, b, g) - mp) @ W)[:, 0]
ysh = np.random.default_rng(2).permutation(ytr); a2 = Birikim(H, 1); a2.ekle(P, ysh[:, None]); W2, mp2, my2 = a2.agirlik(); h2 = (P - mp2) @ W2; s2 = float(((h2[:, 0]) * (ysh - my2[0])).sum() / (h2[:, 0] ** 2).sum()); pshuf = my2[0] + s2 * ((feat(Xte, W1, b, g) - mp2) @ W2)[:, 0]
Pc = np.hstack([np.ones((len(P), 1)), P]); br = None
for lam in (0.1, 1, 10, 100, 1000):
    Wr = np.linalg.solve(Pc.T @ Pc + lam * np.eye(H + 1), Pc.T @ ytr); v = met(yva, np.hstack([np.ones((len(Xva), 1)), feat(Xva, W1, b, g)]) @ Wr)["RMSE"]
    if br is None or v < br[0]: br = (v, lam, Wr)
R["C"] = dict(gamma=g, olcek_skaleri_s=s, test_gorulmeyen=met(yte, pt), etiket_karisik_test=met(yte, pshuf), ortalama_tahmin=met(yte, np.full(len(yte), ytr.mean())), agirlik_suresi_s=tw,
   ridge_ayni_ozellik=dict(test=met(yte, np.hstack([np.ones((len(Xte), 1)), feat(Xte, W1, b, g)]) @ br[2]), lam=br[1]), n_egitim=len(ytr), uretilen_agirlik_sayisi=int(W.size))
json.dump(R, open("sonuc.json", "w"), indent=1); print(json.dumps(R, indent=1))
