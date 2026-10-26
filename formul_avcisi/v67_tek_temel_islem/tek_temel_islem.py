#!/usr/bin/env python3
"""V67: TEK TEMEL ISLEM = softmax-agirlikli dis-carpim OKUMA/YAZMA (yogun birlesmeli bellek / Nadaraya-Watson cekirdek duzgunlestirici).
BILINEN YONTEM (yeni kesif degil): Nadaraya-Watson (1964), modern Hopfield / dense associative memory (Krotov-Hopfield 2016, Ramsauer 2020), dikkat (attention).
YAZMA (agirlik uretimi): her ornek icin  k_i = (phi(x_i)-m)/||phi(x_i)-m||,  v_i = y_i   ->  bellek (K,V) = satirlara EKLE (yeni ornek = tek ekleme; eski hesap yok)
OKUMA (tahmin):         yhat(x) = sum_i softmax_i( beta * k(x).k_i ) v_i      (tek islem: benzerlik -> softmax -> agirlikli hedef toplami)
Gram/Cholesky/Schur/ridge/SGD/kovaryans-varyans YOK; ozellikler arasi iliski k.k_i ic carpiminda dogrudan kullanilir.
'Agirlik' = (K,V) bellegi: boyutu O(n*(H+C)) -- sabit boyutlu parametre degil (bu bir kisit; rapora yazildi)."""
import numpy as np, time, tracemalloc, json
from sklearn.datasets import load_digits, load_diabetes
from sklearn.model_selection import train_test_split
def feat(X, W1, b, g): return np.tanh(g * (X @ W1 + b))
class Bellek:
    def __init__(s): s.K = None; s.V = None; s.m = None; s.ops = 0
    def yaz(s, P, Y, m):   # tek islem: normalize et + ekle
        s.m = m; Kn = P - m; Kn = Kn / (np.linalg.norm(Kn, axis=1, keepdims=True) + 1e-12); s.ops += P.size
        s.K = Kn if s.K is None else np.vstack([s.K, Kn]); s.V = Y if s.V is None else np.vstack([s.V, Y])
    def oku(s, P, beta):
        Q = P - s.m; Q = Q / (np.linalg.norm(Q, axis=1, keepdims=True) + 1e-12); S = beta * (Q @ s.K.T); S -= S.max(1, keepdims=True); A = np.exp(S); A /= A.sum(1, keepdims=True); s.ops += Q.shape[0] * s.K.shape[0] * (Q.shape[1] + s.V.shape[1]); return A @ s.V
def met(yt, p): e = yt - p; return dict(MAE=float(np.abs(e).mean()), RMSE=float(np.sqrt((e ** 2).mean())), R2=float(1 - (e ** 2).sum() / ((yt - yt.mean()) ** 2).sum()))
BETAS = (1, 2, 5, 10, 20, 50, 100, 200); H = 512; R = {}
# ---------- A: Digits ----------
d = load_digits(); X, y = d.data.astype(float), d.target; C = 10
Xtr, Xr, ytr, yr = train_test_split(X, y, test_size=0.4, random_state=0, stratify=y); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0, stratify=yr)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; r = np.random.default_rng(1); W1 = r.standard_normal((64, H)) / 8.0; b = r.standard_normal(H) * 0.1; Ytr = np.eye(C)[ytr]
best = None
for g in (0.05, 0.1, 0.2, 0.5, 1.0):
    P = feat(Xtr, W1, b, g); M = Bellek(); M.yaz(P, Ytr, P.mean(0)); Pv = feat(Xva, W1, b, g)
    for be in BETAS:
        va = np.mean(np.argmax(M.oku(Pv, be), 1) == yva)
        if best is None or va > best[0]: best = (va, g, be)
va, g, be = best; tracemalloc.start(); t0 = time.perf_counter(); P = feat(Xtr, W1, b, g); M = Bellek(); M.yaz(P, Ytr, P.mean(0)); tw = time.perf_counter() - t0; peak = tracemalloc.get_traced_memory()[1] / 1e6; tracemalloc.stop(); wr_ops = M.ops
Pte = feat(Xte, W1, b, g); t0 = time.perf_counter(); pte = M.oku(Pte, be); tr_ = time.perf_counter() - t0; te = float(np.mean(np.argmax(pte, 1) == yte)); read_ops = M.ops - wr_ops
# kendi kendine okuma (egitim ornegi kendini bulur -> ezber gostergesi) yerine: 1 ornek disarida
trn = float(np.mean(np.argmax(M.oku(P, be), 1) == ytr)); Ys = np.eye(C)[np.random.default_rng(2).permutation(ytr)]; Ms = Bellek(); Ms.yaz(P, Ys, P.mean(0)); te_shuf = float(np.mean(np.argmax(Ms.oku(Pte, be), 1) == yte))
# artimli: 3 parca
Mi = Bellek()
for sl in np.array_split(np.arange(len(Xtr)), 3): Mi.yaz(P[sl], Ytr[sl], P.mean(0))
inc_diff = float(np.abs(Mi.oku(Pte, be) - pte).max())
R["A_digits"] = dict(gamma=g, beta=be, val=float(va), egitim_kendi_bellegi=trn, test_gorulmeyen=te, etiket_karisik_test=te_shuf, yazma_suresi_s=tw, okuma_suresi_360_ornek_s=tr_, yazma_madd=float(wr_ops), okuma_madd=float(read_ops), tepe_RAM_MB=peak, bellek_boyutu_MB=(M.K.nbytes + M.V.nbytes) / 1e6, artimli_tahmin_farki=inc_diff)
# ---------- B: Diabetes ----------
d = load_diabetes(); Xd, yd = d.data, d.target; Xtr, Xr, ytr, yr = train_test_split(Xd, yd, test_size=0.4, random_state=0); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; r = np.random.default_rng(1); W1 = r.standard_normal((10, H)) / np.sqrt(10); b = r.standard_normal(H) * 0.1
best = None
for g in (0.1, 0.3, 1.0):
    P = feat(Xtr, W1, b, g); M = Bellek(); M.yaz(P, ytr[:, None], P.mean(0)); Pv = feat(Xva, W1, b, g)
    for be in BETAS + (500, 1000):
        v = met(yva, M.oku(Pv, be)[:, 0])["RMSE"]
        if best is None or v < best[0]: best = (v, g, be)
_, g, be = best; P = feat(Xtr, W1, b, g); t0 = time.perf_counter(); M = Bellek(); M.yaz(P, ytr[:, None], P.mean(0)); tw = time.perf_counter() - t0
pt = M.oku(feat(Xte, W1, b, g), be)[:, 0]; Ms = Bellek(); Ms.yaz(P, np.random.default_rng(2).permutation(ytr)[:, None], P.mean(0)); psh = Ms.oku(feat(Xte, W1, b, g), be)[:, 0]
R["B_diabetes"] = dict(gamma=g, beta=be, test_gorulmeyen=met(yte, pt), etiket_karisik=met(yte, psh), yazma_suresi_s=tw)
# ---------- C: korele ozellik testi (sentetik: 5 gizli degisken -> 300 asiri korele ozellik, dogrusal hedef) ----------
def v66_fit(P, y):   # V66 islemi (karsilastirma icin yeniden gerceklestirildi)
    mp = P.mean(0); my = y.mean(); var = P.var(0); W = ((P - mp).T @ (y - my) / len(y)) / (var + 1e-8); h0 = (P - mp) @ W; s = float(h0 @ (y - my) / (h0 @ h0)); return lambda Q: my + s * (Q - mp) @ W
rr = np.random.default_rng(7); n = 1000; z = rr.standard_normal((n, 5)); A = rr.standard_normal((5, 300)); Xc = z @ A + 0.1 * rr.standard_normal((n, 300)); wt = rr.standard_normal(5); yc = z @ wt + 0.1 * rr.standard_normal(n)
Xtr, Xva, Xte = Xc[:600], Xc[600:800], Xc[800:]; ytr, yva, yte = yc[:600], yc[600:800], yc[800:]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]
cc = np.corrcoef(Xtr.T); ortalama_korelasyon = float(np.abs(cc[np.triu_indices(300, 1)]).mean()); r = np.random.default_rng(1); W1 = r.standard_normal((300, H)) / np.sqrt(300); b = r.standard_normal(H) * 0.1
bestc = {}
for g in (0.05, 0.1, 0.3, 1.0):
    P = feat(Xtr, W1, b, g); Pv = feat(Xva, W1, b, g); M = Bellek(); M.yaz(P, ytr[:, None], P.mean(0))
    for be in BETAS + (500, 1000):
        v = met(yva, M.oku(Pv, be)[:, 0])["RMSE"]
        if "v67" not in bestc or v < bestc["v67"][0]: bestc["v67"] = (v, g, be)
    v = met(yva, v66_fit(P, ytr)(Pv))["RMSE"]
    if "v66" not in bestc or v < bestc["v66"][0]: bestc["v66"] = (v, g)
    Pc = np.hstack([np.ones((len(P), 1)), P])
    for lam in (0.01, 0.1, 1, 10, 100):
        Wr = np.linalg.solve(Pc.T @ Pc + lam * np.eye(H + 1), Pc.T @ ytr); v = met(yva, np.hstack([np.ones((len(Pv), 1)), Pv]) @ Wr)["RMSE"]
        if "ridge" not in bestc or v < bestc["ridge"][0]: bestc["ridge"] = (v, g, lam)
g = bestc["v67"][1]; P = feat(Xtr, W1, b, g); M = Bellek(); M.yaz(P, ytr[:, None], P.mean(0)); p67 = M.oku(feat(Xte, W1, b, g), bestc["v67"][2])[:, 0]
g6 = bestc["v66"][1]; p66 = v66_fit(feat(Xtr, W1, b, g6), ytr)(feat(Xte, W1, b, g6)); gr, lr = bestc["ridge"][1], bestc["ridge"][2]; Pr = feat(Xtr, W1, b, gr); Pc = np.hstack([np.ones((len(Pr), 1)), Pr]); Wr = np.linalg.solve(Pc.T @ Pc + lr * np.eye(H + 1), Pc.T @ ytr); pr_ = np.hstack([np.ones((len(Xte), 1)), feat(Xte, W1, b, gr)]) @ Wr
R["C_korele"] = dict(ortalama_mutlak_ozellik_korelasyonu=ortalama_korelasyon, V66_test=met(yte, p66), V67_test=met(yte, p67), ridge_zinciri_test=met(yte, pr_), ortalama_tahmin=met(yte, np.full(len(yte), ytr.mean())), beta_v67=bestc["v67"][2])
json.dump(R, open("sonuc.json", "w"), indent=1); print(json.dumps(R, indent=1))
# ---------- D: iptal (cancellation) testi: sinyal kucuk temel-bilesende (y = x1 - x2, x1,x2 cok korele) ----------
rr = np.random.default_rng(11); n = 1200; u = rr.standard_normal((n, 20)); e1 = 0.2 * rr.standard_normal((n, 20)); e2 = 0.2 * rr.standard_normal((n, 20)); Xc = np.hstack([u + e1, u + e2]); cj = rr.standard_normal(20); yc = (e1 - e2) @ cj
Xtr, Xva, Xte = Xc[:700], Xc[700:950], Xc[950:]; ytr, yva, yte = yc[:700], yc[700:950], yc[950:]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; r = np.random.default_rng(1); W1 = r.standard_normal((40, H)) / np.sqrt(40); b = r.standard_normal(H) * 0.1
bd = {}
for g in (0.05, 0.1, 0.3, 1.0):
    P = feat(Xtr, W1, b, g); Pv = feat(Xva, W1, b, g); M = Bellek(); M.yaz(P, ytr[:, None], P.mean(0))
    for be in BETAS + (500, 1000):
        v = met(yva, M.oku(Pv, be)[:, 0])["RMSE"]
        if "v67" not in bd or v < bd["v67"][0]: bd["v67"] = (v, g, be)
    v = met(yva, v66_fit(P, ytr)(Pv))["RMSE"]
    if "v66" not in bd or v < bd["v66"][0]: bd["v66"] = (v, g)
    Pc = np.hstack([np.ones((len(P), 1)), P])
    for lam in (0.001, 0.01, 0.1, 1, 10, 100):
        Wr = np.linalg.solve(Pc.T @ Pc + lam * np.eye(H + 1), Pc.T @ ytr); v = met(yva, np.hstack([np.ones((len(Pv), 1)), Pv]) @ Wr)["RMSE"]
        if "ridge" not in bd or v < bd["ridge"][0]: bd["ridge"] = (v, g, lam)
g = bd["v67"][1]; P = feat(Xtr, W1, b, g); M = Bellek(); M.yaz(P, ytr[:, None], P.mean(0)); p67 = M.oku(feat(Xte, W1, b, g), bd["v67"][2])[:, 0]; g6 = bd["v66"][1]; p66 = v66_fit(feat(Xtr, W1, b, g6), ytr)(feat(Xte, W1, b, g6))
gr, lr = bd["ridge"][1], bd["ridge"][2]; Pr = feat(Xtr, W1, b, gr); Pc = np.hstack([np.ones((len(Pr), 1)), Pr]); Wr = np.linalg.solve(Pc.T @ Pc + lr * np.eye(H + 1), Pc.T @ ytr); pr_ = np.hstack([np.ones((len(Xte), 1)), feat(Xte, W1, b, gr)]) @ Wr
R["D_iptal"] = dict(V66_test=met(yte, p66), V67_test=met(yte, p67), ridge_zinciri_test=met(yte, pr_), ortalama_tahmin=met(yte, np.full(len(yte), ytr.mean())), ozellik_cifti_korelasyonu=float(np.corrcoef(Xtr[:, 0], Xtr[:, 20])[0, 1]))
json.dump(R, open("sonuc.json", "w"), indent=1); print(json.dumps(R["D_iptal"], indent=1))
