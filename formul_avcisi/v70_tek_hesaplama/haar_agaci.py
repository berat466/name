#!/usr/bin/env python3
"""V70 ADAYI: HIYERARSIK TOPLAM/FARK (Haar-benzeri) DEKORELASYON + Hebb kurali. Ters/Cholesky/ridge/SGD/iterasyon/2x2-cozucu/ornek bellegi YOK.
Tek hesaplama turu = 'dik cift donusumu':  iki standartlastirilmis ozellik a,b (isaret s=sgn(rho)) -> toplam (a+s b)/sqrt2 , fark (a-s b)/sqrt2.  (yalniz +,-, /sqrt2; determinant/ters/bolme-ile-cozum YOK)
 Esit varyansli iki ozellik icin toplam ve fark TAM dekorele olur (Cov=(Va-Vb)/2=0). Farklar son ozellik olur (dondurulur), toplamlar ayni kuralla tekrar eslestirilir (|rho|>tau, acgozlu); eslesmeyenler son ozellik olur.
 Son ozellikler (z_f = T_f x) uzerinde Hebb kurali  W_f = Cov(z_f,y)/Var(z_f)  ; nihai agirlik w = T^T W (d x C, girdi uzayinda).  Tek skaler s = (w.c)/(w^T C w) (momentlerden).
Birikim artimli (n, sum x, sum x x^T, sum y, sum x y^T). BILINEN YONTEMLER: Haar/Walsh-Hadamard donusumu, hiyerarsik kumeleme ozellik birlestirme; yeni kesif degil.
DURUST NOT: eslesme korelasyon matrisinden (siralama) yapilir; dekorelasyon yalniz esit-varyansli (esit yuklemeli) yapida tam, aksi halde YAKLASIK."""
import numpy as np, time, tracemalloc, json
class Moment:
    def __init__(s, d, C): s.n = 0; s.sx = np.zeros(d); s.sxx = np.zeros((d, d)); s.sy = np.zeros(C); s.sxy = np.zeros((d, C)); s.ops = 0
    def ekle(s, X, Y): s.n += len(X); s.sx += X.sum(0); s.sxx += X.T @ X; s.sy += Y.sum(0); s.sxy += X.T @ Y; s.ops += len(X) * X.shape[1] * (X.shape[1] + Y.shape[1])
    def agirlik(s, tau=0.5, max_level=20, full=False):
        n = s.n; m = s.sx / n; my = s.sy / n; Cm = s.sxx / n - np.outer(m, m); c = s.sxy / n - np.outer(m, my); d = len(m); Cn = c.shape[1]; M = (d * (max_level + 2) + 2) if full else (3 * d + 2)
        T = np.zeros((M, d)); Cz = np.zeros((M, M)); k = d; T[:d] = np.eye(d); Cz[:d, :d] = Cm; ops = 0
        def add(al, a, be, b):
            nonlocal k
            r = k; k += 1; T[r] = al * T[a] + be * T[b]; row = al * Cz[a, :r] + be * Cz[b, :r]; Cz[r, :r] = row; Cz[:r, r] = row
            Cz[r, r] = al * al * Cz[a, a] + 2 * al * be * Cz[a, b] + be * be * Cz[b, b]; return r
        active = list(range(d)); finals = []
        for lev in range(max_level):
            if len(active) < 2: break
            sig = np.sqrt(np.maximum(np.array([Cz[i, i] for i in active]), 1e-30)); sub = Cz[np.ix_(active, active)] / np.outer(sig, sig); np.fill_diagonal(sub, 0.0)
            iu = np.triu_indices(len(active), 1); rho = sub[iu]; order = np.argsort(-np.abs(rho)); used = set(); pairs = []
            for o in order:
                if abs(rho[o]) < tau: break
                i, j = iu[0][o], iu[1][o]
                if i in used or j in used: continue
                used.add(i); used.add(j); pairs.append((i, j, 1.0 if rho[o] >= 0 else -1.0))
            ops += len(active) ** 2
            if not pairs: break
            new_active = []
            for i, j, sg in pairs:
                a, b = active[i], active[j]; ia, ib = 1 / (np.sqrt(2) * np.sqrt(Cz[a, a])), sg / (np.sqrt(2) * np.sqrt(Cz[b, b]))
                sm = add(ia, a, ib, b); df = add(ia, a, -ib, b); new_active.append(sm); (new_active if full else finals).append(df)
            for i in range(len(active)):
                if i not in used: finals.append(active[i])
            active = new_active
        finals += active; F = np.array(finals); Tf = T[F]; var = np.array([Cz[f, f] for f in F]); cz = Tf @ c   # cov(z_f, y)
        Wz = cz / (var[:, None] + 1e-12); w = Tf.T @ Wz; S = np.array([(w[:, t] @ c[:, t]) / (w[:, t] @ Cm @ w[:, t] + 1e-30) for t in range(Cn)]); s.ops += ops + len(F) * d * Cn + len(F) * d
        return w, S, m, my, len(F)
def pred(X, w, S, m, my): return my + (X - m) @ (w * S)
def met(yt, p): e = yt - p; return dict(R2=float(1 - (e ** 2).sum() / ((yt - yt.mean()) ** 2).sum()), RMSE=float(np.sqrt((e ** 2).mean())), MAE=float(np.abs(e).mean()))
FULL = False
def sec(Xtr, ytr, Xva, yva, C=1, taus=(0.1, 0.3, 0.5)):   # tau yalniz val ile
    best = None
    for tau in taus:
        M = Moment(Xtr.shape[1], C); M.ekle(Xtr, ytr.reshape(len(ytr), -1)); w, S, m, my, nf = M.agirlik(tau, 20, FULL); p = pred(Xva, w, S, m, my); v = met(yva, p[:, 0])["R2"] if C == 1 else np.mean(np.argmax(p, 1) == yva)
        if best is None or v > best[0]: best = (v, tau)
    return best
if __name__ == "__main__":
    import sys; FULL = len(sys.argv) > 1 and sys.argv[1] == "full"
    R = {}; cj = np.random.default_rng(11).standard_normal(20)
    def split(Xc, yc, a=700, b=950):
        Xtr, Xva, Xte = Xc[:a], Xc[a:b], Xc[b:]; mu, sd = Xtr.mean(0), Xtr.std(0); return [(Z - mu) / sd for Z in (Xtr, Xva, Xte)], yc[:a], yc[a:b], yc[b:]
    def run(name, Xc, yc):
        (Xtr, Xva, Xte), ytr, yva, yte = split(Xc, yc); v, tau = sec(Xtr, ytr, Xva, yva); tracemalloc.start(); t0 = time.perf_counter(); M = Moment(Xtr.shape[1], 1); M.ekle(Xtr, ytr[:, None]); w, S, m, my, nf = M.agirlik(tau, 20, FULL); tw = time.perf_counter() - t0; peak = tracemalloc.get_traced_memory()[1] / 1e6; tracemalloc.stop()
        R[name] = dict(tau_val=tau, val_R2=float(v), test=met(yte, pred(Xte, w, S, m, my)[:, 0]), son_ozellik=int(nf), agirlik_sayisi=int(w.size), sure_s=tw, madd=float(M.ops), RAM_MB=peak); return R[name]["test"]["R2"]
    # A: ikili (V69 ile ayni veri, tohum 11)
    rr = np.random.default_rng(11); n = 1200; u = rr.standard_normal((n, 20)); e1 = 0.2 * rr.standard_normal((n, 20)); e2 = 0.2 * rr.standard_normal((n, 20)); a = run("A_ikili", np.hstack([u + e1, u + e2]), (e1 - e2) @ cj)
    # B: uclu (V69 ile ayni veri, tohum 5)
    rr = np.random.default_rng(5); u = rr.standard_normal((n, 20)); ea, eb, ec = [0.2 * rr.standard_normal((n, 20)) for _ in range(3)]; b3 = run("B_uclu", np.hstack([u + ea, u + eb, u + ec]), (ea + eb - 2 * ec) @ cj)
    # C: coklu: 12 korele ozellik (r~0.96), hedef ortak-disi bilesenlerin bilinmeyen katsayili toplami; (C1) esit yuklemeli, (C2) heterojen yukleme
    rr = np.random.default_rng(21); u = rr.standard_normal((n, 1)); E = 0.25 * rr.standard_normal((n, 12)); aj = rr.standard_normal(12)
    c1 = run("C1_coklu_esit_yukleme", u + E, E @ aj)
    lam = rr.uniform(0.5, 1.5, 12); c2 = run("C2_coklu_heterojen_yukleme", u * lam + E, E @ aj)
    # C3: 2 gizli degiskenli, 16 ozellik
    u2 = rr.standard_normal((n, 2)); L = rr.uniform(0.3, 1.5, (2, 16)); E2 = 0.25 * rr.standard_normal((n, 16)); a2 = rr.standard_normal(16); c3 = run("C3_iki_faktor_16_ozellik", u2 @ L + E2, E2 @ a2)
    R["gecti_A_B_C"] = bool(a >= 0.95 and b3 >= 0.95 and c1 >= 0.95 and c2 >= 0.95 and c3 >= 0.95)
    json.dump(R, open("sonuc_full.json" if FULL else "sonuc.json", "w"), indent=1); print(json.dumps({k: (v if not isinstance(v, dict) else dict(test_R2=v["test"]["R2"], tau=v["tau_val"], val=v["val_R2"], sure_ms=v["sure_s"] * 1e3, madd=v["madd"])) for k, v in R.items()}, indent=1))
