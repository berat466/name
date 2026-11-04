#!/usr/bin/env python3
"""V68: iptal testinde (y=x1-x2, r=0.965) kural-uyumlu tek-islem adaylari + gereken kazanc teshisi. Cozucu/ters/SGD ADAY OLARAK kullanilmaz; referans yalniz karsilastirma/teshis."""
import numpy as np, json, itertools
def met(yt, p): e = yt - p; return dict(R2=float(1 - (e ** 2).sum() / ((yt - yt.mean()) ** 2).sum()), RMSE=float(np.sqrt((e ** 2).mean())))
rr = np.random.default_rng(11); n = 1200; u = rr.standard_normal((n, 20)); e1 = 0.2 * rr.standard_normal((n, 20)); e2 = 0.2 * rr.standard_normal((n, 20)); Xc = np.hstack([u + e1, u + e2]); cj = rr.standard_normal(20); yc = (e1 - e2) @ cj   # V67 ile AYNI veri
Xtr, Xva, Xte = Xc[:700], Xc[700:950], Xc[950:]; ytr, yva, yte = yc[:700], yc[700:950], yc[950:]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]
R = {}
# Aday 1 (V66 kurali, referans): W = cov/var
def diag_rule(P, y): m = P.mean(0); my = y.mean(); W = ((P - m).T @ (y - my) / len(y)) / (P.var(0) + 1e-8); h = (P - m) @ W; s = float(h @ (y - my) / (h @ h)); return lambda Q: my + s * (Q - m) @ W
R["V66_kurali"] = met(yte, diag_rule(Xtr, ytr)(Xte))
# Aday 2: sabit, veriden BAGIMSIZ ikili-kontrast haritasi + AYNI birikim kurali (O(d^2) ozellik) -- sonucu elle kodlamaz ama cevabi (fark) iceren bir aday kume uretir
def kontrast(X): d = X.shape[1]; I, J = np.triu_indices(d, 1); return np.hstack([X, X[:, I] - X[:, J]])
Ktr, Kte = kontrast(Xtr), kontrast(Xte); R["ikili_kontrast+ayni_kural"] = dict(**met(yte, diag_rule(Ktr, ytr)(Kte)), ozellik_sayisi=int(Ktr.shape[1]))
# Aday 3: kontrast + ikinci mertebe zayiflama: sadece Hebbian'in KENDI cikisindaki kalintiyi bir kez daha biriktir (tek-gecis, 2 kez ayni kural = 'artik birikimi'); iteratif degil sabit 2 uygulama
def residual2(P, y):
    f1 = diag_rule(P, y); r = y - f1(P); f2 = diag_rule(P, r); return lambda Q: f1(Q) + f2(Q)
R["V66_kurali_2_uygulama(artik)"] = met(yte, residual2(Xtr, ytr)(Xte))
# TESHIS (aday degil): gereken kazanc. Ozdeger yayilimi ve tam-batch gradyan adimi sayisi (Landweber) -- yalniz kanit, yontem olarak KULLANILMAZ
Xc0 = Xtr - Xtr.mean(0); G = Xc0.T @ Xc0 / len(Xtr); ev = np.linalg.eigvalsh(G); c = Xc0.T @ (ytr - ytr.mean()) / len(Xtr); evec = np.linalg.eigh(G)[1]
R["teshis"] = dict(en_buyuk_ozdeger=float(ev[-1]), en_kucuk_ozdeger=float(ev[0]), kosul_sayisi=float(ev[-1] / ev[0]), sinyal_enerjisinin_en_kucuk_5_ozdegerdeki_payi=float(((evec.T @ c)[:20] ** 2).sum() / ((evec.T @ c) ** 2).sum()), ozdeger_20_alti_sayisi=int((ev < 0.2).sum()))
w = np.zeros(40); step = 1.0 / ev[-1]; need = None
for k in range(1, 200001):
    w += step * (c - G @ w)
    if k in (1, 10, 100, 1000, 10000, 100000) or need is None:
        r2 = met(yte, ytr.mean() + (Xte - Xtr.mean(0)) @ w)["R2"]
        if need is None and r2 >= 0.95: need = k
        if k in (1, 10, 100, 1000, 10000, 100000): R.setdefault("teshis_adim_R2", {})[k] = r2
    if need is not None and k > 100000: break
R["teshis_R2_0.95_icin_tam_batch_adim"] = need
R["referans_ridge_zinciri(yalniz karsilastirma)"] = met(yte, ytr.mean() + (Xte - Xtr.mean(0)) @ np.linalg.solve(G + 1e-6 * np.eye(40), c))
json.dump(R, open("sonuc.json", "w"), indent=1); print(json.dumps(R, indent=1))
