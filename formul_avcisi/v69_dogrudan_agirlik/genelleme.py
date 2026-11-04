import numpy as np, json, time, tracemalloc
src = open("cift_kosullu.py").read(); exec(src.split("R = {}")[0])   # sinif/fonksiyon tanimlari (Moment, pred, met) -- ayni mekanizma
from sklearn.datasets import load_digits, load_diabetes
R = {}
# (1) iptal testi: 8 bagimsiz tohum (sağlamlik); eps,q her tohumda YALNIZ val ile
r2s = []
for sd_ in range(8):
    rr = np.random.default_rng(100 + sd_); n = 1200; u = rr.standard_normal((n, 20)); e1 = 0.2 * rr.standard_normal((n, 20)); e2 = 0.2 * rr.standard_normal((n, 20)); Xc = np.hstack([u + e1, u + e2]); cj = rr.standard_normal(20); yc = (e1 - e2) @ cj
    Xtr, Xva, Xte = Xc[:700], Xc[700:950], Xc[950:]; ytr, yva, yte = yc[:700], yc[700:950], yc[950:]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; best = None
    for eps in (1e-6, 1e-4, 1e-2):
        for q in (0, 1, 2, 4, 8, 16):
            M = Moment(40, 1); M.ekle(Xtr, ytr[:, None]); W, S, m, my = M.agirlik(eps, q); v = met(yva, pred(Xva, W, S, m, my)[:, 0])["R2"]
            if best is None or v > best[0]: best = (v, eps, q)
    M = Moment(40, 1); M.ekle(Xtr, ytr[:, None]); W, S, m, my = M.agirlik(best[1], best[2]); r2s.append(met(yte, pred(Xte, W, S, m, my)[:, 0])["R2"])
R["iptal_8_tohum_test_R2"] = [round(x, 4) for x in r2s]; R["iptal_8_tohum_ort_min"] = [float(np.mean(r2s)), float(np.min(r2s))]
def feat(X, W1, b, g): return np.tanh(g * (X @ W1 + b))
H = 512
# (2) Digits
d = load_digits(); X, y = d.data.astype(float), d.target; C = 10
Xtr, Xr, ytr, yr = train_test_split(X, y, test_size=0.4, random_state=0, stratify=y); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0, stratify=yr)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; r = np.random.default_rng(1); W1 = r.standard_normal((64, H)) / 8.0; b = r.standard_normal(H) * 0.1; Ytr = np.eye(C)[ytr]; best = None; t_all = time.perf_counter()
for g in (0.1, 0.5, 1.0):
    P = feat(Xtr, W1, b, g); Pv = feat(Xva, W1, b, g)
    for eps in (1e-3, 1e-1):
        for q in (0, 2, 8):
            M = Moment(H, C); M.ekle(P, Ytr); W, S, m, my = M.agirlik(eps, q); va = np.mean(np.argmax(pred(Pv, W, S, m, my), 1) == yva)
            if best is None or va > best[0]: best = (va, g, eps, q)
_, g, eps, q = best; P = feat(Xtr, W1, b, g); tracemalloc.start(); t0 = time.perf_counter(); M = Moment(H, C); M.ekle(P, Ytr); W, S, m, my = M.agirlik(eps, q); tw = time.perf_counter() - t0; peak = tracemalloc.get_traced_memory()[1] / 1e6; tracemalloc.stop()
te = float(np.mean(np.argmax(pred(feat(Xte, W1, b, g), W, S, m, my), 1) == yte)); ys = np.random.default_rng(2).permutation(ytr); Ms = Moment(H, C); Ms.ekle(P, np.eye(C)[ys]); Ws, Ss, ms, mys = Ms.agirlik(eps, q)
R["digits"] = dict(secilen=dict(gamma=g, eps=eps, q=q), val=float(best[0]), test=te, egitim=float(np.mean(np.argmax(pred(P, W, S, m, my), 1) == ytr)), etiket_karisik_test=float(np.mean(np.argmax(pred(feat(Xte, W1, b, g), Ws, Ss, ms, mys), 1) == yte)), agirlik_sayisi=int(W.size), agirlik_suresi_s=tw, madd=float(M.ops), tepe_RAM_MB=peak)
# (3) Diabetes
d = load_diabetes(); Xd, yd = d.data, d.target; Xtr, Xr, ytr, yr = train_test_split(Xd, yd, test_size=0.4, random_state=0); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]; r = np.random.default_rng(1); W1 = r.standard_normal((10, H)) / np.sqrt(10); b = r.standard_normal(H) * 0.1; best = None
for g in (0.1, 0.3, 1.0):
    P = feat(Xtr, W1, b, g); Pv = feat(Xva, W1, b, g)
    for eps in (1e-3, 1e-1):
        for q in (0, 2, 8):
            M = Moment(H, 1); M.ekle(P, ytr[:, None]); W, S, m, my = M.agirlik(eps, q); v = met(yva, pred(Pv, W, S, m, my)[:, 0])["RMSE"]
            if best is None or v < best[0]: best = (v, g, eps, q)
_, g, eps, q = best; P = feat(Xtr, W1, b, g); t0 = time.perf_counter(); M = Moment(H, 1); M.ekle(P, ytr[:, None]); W, S, m, my = M.agirlik(eps, q); tw = time.perf_counter() - t0
ysh = np.random.default_rng(2).permutation(ytr); Ms = Moment(H, 1); Ms.ekle(P, ysh[:, None]); Ws, Ss, ms, mys = Ms.agirlik(eps, q)
R["diabetes"] = dict(secilen=dict(gamma=g, eps=eps, q=q), test=met(yte, pred(feat(Xte, W1, b, g), W, S, m, my)[:, 0]), etiket_karisik=met(yte, pred(feat(Xte, W1, b, g), Ws, Ss, ms, mys)[:, 0]), agirlik_sayisi=int(W.size), agirlik_suresi_s=tw, madd=float(M.ops))
json.dump(R, open("sonuc_genelleme.json", "w"), indent=1); print(json.dumps(R, indent=1))
