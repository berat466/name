#!/usr/bin/env python3
"""V69 ADAYI: CIFT-KOSULLU EGIM BIRIKIMI (tek kural). Ters/Cholesky/ridge/SGD/iterasyon yok; ornek bellegi yok.
Birikim (artimli, toplamsal): n, sum x, sum x x^T (d x d), sum y, sum x y^T  -> ikinci mertebe momentler (Gram'in KENDISI cozulmez; yalniz girisleri okunur)
V69-b: partner agirligi A[j,k]=rho_jk^(2q) (korelasyonu yuksek partner baskin; q=0 -> duz ortalama). Kural (her agirlik icin AYNI):   B_t[j,k] = ( c_j V_k - c_k C_jk ) / ( V_j V_k - C_jk^2 + e V_j V_k )      (j,k ozellik cifti, hedef t; 2x2 kosullu egim, KAPALI FORM)
                                  w_j = sum_k A[j,k] B_t[j,k] / sum_k A[j,k]            (her ozelligin, her partnere kosullandirilmis egiminin ortalamasi)
                                  yhat = ybar + s (x - xbar) w ,  s = (w.c)/(w^T C w) (tek skaler, momentlerden)
DURUST NOT: her (j,k) icin 2x2 sistemin kapali-form cozumu vardir (yerel/ikinci mertebe 'ters'); tam d x d ters/cozum YOKTUR. Bu bir YAKLASIMDIR (ucuncu mertebe iptali yakalamaz)."""
import numpy as np, time, tracemalloc, json
from sklearn.datasets import load_digits, load_diabetes
from sklearn.model_selection import train_test_split
class Moment:
    def __init__(s, d, C): s.n = 0; s.sx = np.zeros(d); s.sxx = np.zeros((d, d)); s.sy = np.zeros(C); s.sxy = np.zeros((d, C)); s.ops = 0
    def ekle(s, X, Y): s.n += len(X); s.sx += X.sum(0); s.sxx += X.T @ X; s.sy += Y.sum(0); s.sxy += X.T @ Y; s.ops += len(X) * X.shape[1] * (X.shape[1] + Y.shape[1])
    def agirlik(s, eps=1e-4, q=0):
        n = s.n; m = s.sx / n; my = s.sy / n; Cm = s.sxx / n - np.outer(m, m); c = s.sxy / n - np.outer(m, my); V = np.diag(Cm).copy(); d = len(V); C = c.shape[1]; W = np.zeros((d, C)); S = np.zeros(C)
        den = np.outer(V, V) - Cm ** 2 + eps * np.outer(V, V); np.fill_diagonal(den, 1.0)
        for t in range(C):
            ct = c[:, t]; B = (ct[:, None] * V[None, :] - ct[None, :] * Cm) / den; np.fill_diagonal(B, 0.0); rho2 = Cm ** 2 / np.outer(V, V); np.fill_diagonal(rho2, 0.0); A = rho2 ** q + 1e-12; np.fill_diagonal(A, 0.0); w = (A * B).sum(1) / A.sum(1); W[:, t] = w
            S[t] = (w @ ct) / (w @ Cm @ w + 1e-30); s.ops += 6 * d * d + d * d
        return W, S, m, my
def pred(X, W, S, m, my): return my + (X - m) @ (W * S)
def met(yt, p): e = yt - p; return dict(R2=float(1 - (e ** 2).sum() / ((yt - yt.mean()) ** 2).sum()), RMSE=float(np.sqrt((e ** 2).mean())), MAE=float(np.abs(e).mean()))
R = {}
# ---------------- 1) iptal testi (V68 ile ayni veri/bolme) ----------------
rr = np.random.default_rng(11); n = 1200; u = rr.standard_normal((n, 20)); e1 = 0.2 * rr.standard_normal((n, 20)); e2 = 0.2 * rr.standard_normal((n, 20)); Xc = np.hstack([u + e1, u + e2]); cj = rr.standard_normal(20); yc = (e1 - e2) @ cj
Xtr, Xva, Xte = Xc[:700], Xc[700:950], Xc[950:]; ytr, yva, yte = yc[:700], yc[700:950], yc[950:]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr, Xva, Xte = [(Z - mu) / sd for Z in (Xtr, Xva, Xte)]
best = None
for eps in (1e-6, 1e-4, 1e-2):
  for q in (0, 1, 2, 4, 8, 16):
    M = Moment(40, 1); M.ekle(Xtr, ytr[:, None]); W, S, m, my = M.agirlik(eps, q); v = met(yva, pred(Xva, W, S, m, my)[:, 0])["R2"]
    if best is None or v > best[0]: best = (v, eps, q)
eps, Q = best[1], best[2]; tracemalloc.start(); t0 = time.perf_counter(); M = Moment(40, 1); M.ekle(Xtr, ytr[:, None]); W, S, m, my = M.agirlik(eps, Q); tw = time.perf_counter() - t0; peak = tracemalloc.get_traced_memory()[1] / 1e6; tracemalloc.stop()
pt = pred(Xte, W, S, m, my)[:, 0]; Mi = Moment(40, 1)
for sl in np.array_split(np.arange(700), 3): Mi.ekle(Xtr[sl], ytr[sl, None])
Wi, Si, _, _ = Mi.agirlik(eps, Q)
# x1/x2 belirleyici fark: ilk cift icin agirlik oranlari
w1, w2 = (W[:, 0] * S[0])[0], (W[:, 0] * S[0])[20]
# gercek katsayi: y = (e1-e2).cj ; x = (u+e)/sd  -> egim beklenen: cj/sd*... oran w1/w2 ~ -1
R["iptal"] = dict(eps_val=eps, q_val=Q, val_R2=float(best[0]), test=met(yte, pt), egitim=met(ytr, pred(Xtr, W, S, m, my)[:, 0]), uretilen_agirlik=int(W.size), agirlik_suresi_s=tw, madd=float(M.ops), tepe_RAM_MB=peak, artimli_agirlik_farki=float(np.abs(W * S - Wi * Si).max()),
   ilk_cift_agirliklari=[float(w1), float(w2)], w1_w2_orani=float(w1 / w2), korelasyon_x1_x2=float(np.corrcoef(Xtr[:, 0], Xtr[:, 20])[0, 1]))
# egitim ornekleri silinince tahmin: yalniz (W,S,m,my) ile -- yukaridaki pred zaten sadece bunlari kullanir; aciklikla:
del M, Mi
R["iptal"]["egitimsiz_tahmin_ayni"] = bool(np.allclose(pred(Xte, W, S, m, my)[:, 0], pt))
# sinir testi: 3'lu iptal  y = x1 + x2 - 2 x3  (uc korele ozellik: ortak u + kucuk bagimsiz gurultu; sinyal ortak bilesende degil, 'kucuk' yonlerde)
rr = np.random.default_rng(5); n = 1200; u = rr.standard_normal((n, 20)); ea, eb, ec = [0.2 * rr.standard_normal((n, 20)) for _ in range(3)]; Xc = np.hstack([u + ea, u + eb, u + ec]); yc = ((ea + eb - 2 * ec) @ cj)
Xtr, Xte = Xc[:700], Xc[950:]; ytr, yte = yc[:700], yc[950:]; mu, sd = Xtr.mean(0), Xtr.std(0); Xtr, Xte = (Xtr - mu) / sd, (Xte - mu) / sd
M = Moment(60, 1); M.ekle(Xtr, ytr[:, None]); W, S, m, my = M.agirlik(eps, Q); R["iptal_3lu_sinir"] = dict(test=met(yte, pred(Xte, W, S, m, my)[:, 0]))
json.dump(R, open("sonuc.json", "w"), indent=1); print(json.dumps(R, indent=1))
