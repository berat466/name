import numpy as np, json
exec(open("haar_agaci.py").read().split('if __name__')[0])
cj = np.random.default_rng(11).standard_normal(20); n = 1200; rr = np.random.default_rng(5); u = rr.standard_normal((n, 20)); ea, eb, ec = [0.2 * rr.standard_normal((n, 20)) for _ in range(3)]
X = np.hstack([u + ea, u + eb, u + ec]); y = (ea + eb - 2 * ec) @ cj; Xtr = X[:700]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr = (Xtr - mu) / sd; ytr = y[:700]
# Gram'dan (yalniz TESHIS) gercek ozdeger yayilimi ve sinyalin dagilimi
C = np.cov(Xtr.T); ev, V = np.linalg.eigh(C); c = np.cov(Xtr.T, ytr)[:-1, -1]; sig = (V.T @ c) ** 2 / np.maximum(ev, 1e-12) ; frac = np.cumsum(sig[::-1]) / sig.sum()
print(json.dumps(dict(kosul_sayisi=float(ev[-1] / ev[0]), en_kucuk_ozdegerler=[float(x) for x in ev[:6]], gerekli_katki_orani_en_kucuk_20_yon=float(sig[:20].sum() / sig.sum()), ornek_sayisi=700, ozdeger_20_alti_sayisi=int((ev < 0.2).sum()))))
# Haar agaci son ozellik korelasyonu
M = Moment(60, 1); M.ekle(Xtr, ytr[:, None]); mm = M.sx / M.n; w, S, m, my, nf = M.agirlik(0.3); print("son ozellik sayisi", nf)
