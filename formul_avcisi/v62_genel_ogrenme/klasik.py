#!/usr/bin/env python3
"""V62: klasik iteratif referans (YALNIZ karsilastirma; Tek Hamle'ye bagli degil). Ayni bolme (adapter.py ile ayni random_state), ayni standardizasyon.
Siniflandirma: MLP(512 gizli, Adam) + lojistik regresyon; alpha/lr val'e gore secilir (gucu dusurulmez). Regresyon (diabetes): MLP(512,Adam), Ridge; ayrica
'Tek Hamle denklemi' = ayni cozum denklemi (rastgele sabit tanh ozellik + tek Cholesky ridge cozumu), MOTOR DISI numpy gerceklemesi."""
import time, json, sys, numpy as np, warnings; warnings.filterwarnings("ignore")
from sklearn.datasets import load_digits, load_breast_cancer, load_wine, load_diabetes
from sklearn.model_selection import train_test_split
from sklearn.neural_network import MLPClassifier, MLPRegressor
from sklearn.linear_model import LogisticRegression, Ridge
R = {}
def split(X, y, strat):
    a = dict(stratify=y) if strat else {}
    Xtr, Xr, ytr, yr = train_test_split(X, y, test_size=0.4, random_state=0, **a)
    b = dict(stratify=yr) if strat else {}
    Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0, **b); mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9
    return [(A - mu) / sd for A in (Xtr, Xva, Xte)], ytr, yva, yte
for name, ld in (("digits", load_digits), ("bc", load_breast_cancer), ("wine", load_wine)):
    d = ld(); (Xtr, Xva, Xte), ytr, yva, yte = split(d.data.astype(float), d.target, True); V = int(d.target.max() + 1); dd = Xtr.shape[1]; best = None; tt = 0
    for alpha in (1e-4, 1e-2, 1.0):
        t = time.time(); m = MLPClassifier((512,), solver="adam", alpha=alpha, learning_rate_init=1e-3, max_iter=500, random_state=0).fit(Xtr, ytr); dt = time.time() - t; tt += dt
        va = m.score(Xva, yva)
        if best is None or va > best[0]: best = (va, alpha, m, dt)
    va, alpha, m, dt = best; ys = np.random.default_rng(1).permutation(ytr); ms = MLPClassifier((512,), solver="adam", alpha=alpha, max_iter=500, random_state=0).fit(Xtr, ys)
    t = time.time(); lr = LogisticRegression(max_iter=2000).fit(Xtr, ytr); dlr = time.time() - t
    R[name] = dict(mlp=dict(val=va, test=m.score(Xte, yte), train=m.score(Xtr, ytr), alpha=alpha, epochs=int(m.n_iter_), sure_secilen=dt, sure_grid_toplam=tt, parametre=(dd + 1) * 512 + 513 * V, shuf_test=ms.score(Xte, yte)),
                   lojistik=dict(val=lr.score(Xva, yva), test=lr.score(Xte, yte), sure=dlr, parametre=(dd + 1) * V))
    print(name, json.dumps(R[name]))
# ---- regresyon: diabetes (442 x 10, surekli hedef)
d = load_diabetes(); (Xtr, Xva, Xte), ytr, yva, yte = split(d.data, d.target, False); ym, ys = ytr.mean(), ytr.std()
def met(y, p): e = y - p; return dict(MAE=float(np.abs(e).mean()), RMSE=float(np.sqrt((e ** 2).mean())), R2=float(1 - (e ** 2).sum() / ((y - y.mean()) ** 2).sum()))
best = None; tt = 0
for alpha in (1e-4, 1e-2, 1.0, 10.0):
    t = time.time(); m = MLPRegressor(hidden_layer_sizes=(512,), solver="adam", alpha=alpha, learning_rate_init=1e-3, max_iter=1000, random_state=0).fit(Xtr, (ytr - ym) / ys); dt = time.time() - t; tt += dt
    va = met(yva, m.predict(Xva) * ys + ym)["RMSE"]
    if best is None or va < best[0]: best = (va, alpha, m, dt)
va, alpha, m, dt = best; rg = {}
rg["mlp"] = dict(val=met(yva, m.predict(Xva) * ys + ym), test=met(yte, m.predict(Xte) * ys + ym), alpha=alpha, epochs=int(m.n_iter_), sure_secilen=dt, sure_grid_toplam=tt, parametre=(10 + 1) * 512 + 513)
t = time.time(); rd = Ridge(alpha=10.0).fit(Xtr, ytr); rg["ridge_dogrusal"] = dict(test=met(yte, rd.predict(Xte)), sure=time.time() - t, parametre=11)
def th_fit(Xa, ya, g, lam, seed=1, H=512):   # Tek Hamle denklemi (motor disi): F=[1,tanh(g(XW+b))], W sabit rastgele; w=(F'F+lam I)^-1 F'y tek Cholesky cozumu
    r = np.random.default_rng(seed); W = r.standard_normal((Xa.shape[1], H)) / np.sqrt(Xa.shape[1]); b = r.standard_normal(H) * 0.1
    f = lambda X: np.hstack([np.ones((len(X), 1)), np.tanh(g * (X @ W + b))]); F = f(Xa); A = F.T @ F + lam * np.eye(F.shape[1]); L = np.linalg.cholesky(A); w = np.linalg.solve(L.T, np.linalg.solve(L, F.T @ ya)); return lambda X: f(X) @ w
t0 = time.time(); bb = None
for g in (0.1, 0.3, 1.0):
    for lam in (0.1, 1, 10, 100, 1000):
        p = th_fit(Xtr, (ytr - ym) / ys, g, lam); v = met(yva, p(Xva) * ys + ym)["RMSE"]
        if bb is None or v < bb[0]: bb = (v, g, lam)
tg = time.time() - t0; t = time.time(); p = th_fit(Xtr, (ytr - ym) / ys, bb[1], bb[2]); ts = time.time() - t
rg["tekhamle_denklemi_motor_disi"] = dict(val=met(yva, p(Xva) * ys + ym), test=met(yte, p(Xte) * ys + ym), gamma=bb[1], lam=bb[2], sure_tek_cozum=ts, sure_grid_toplam=tg, parametre_cozulen=513)
pr = th_fit(Xtr, (np.random.default_rng(2).permutation(ytr) - ym) / ys, bb[1], bb[2]); rg["tekhamle_denklemi_etiket_karisik_kontrol"] = met(yte, pr(Xte) * ys + ym)
rg["ortalama_tahmini_referans"] = met(yte, np.full(len(yte), ytr.mean()))
R["diabetes"] = rg; print("diabetes", json.dumps(rg)); json.dump(R, open("klasik_sonuc.json", "w"), indent=1)
