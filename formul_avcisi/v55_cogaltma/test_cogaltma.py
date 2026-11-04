# V55 fizibilite: tek ornegin k kez kullanimi / donusumlerinin Gram-RHS etkisi — kisa matematiksel yol. V54 DEGISTIRILMEDI; yalniz V54 dokumundeki gercek F satirlari kullanilir.
import numpy as np, time, json
rng = np.random.default_rng(0)
n, m, V = 113046, 513, 2001
F = np.fromfile('/tmp/f55/Fc.f32', dtype=np.float32).reshape(n, m); y = np.fromfile('/tmp/f55/ytr.i32', dtype=np.int32)
ctx = np.fromfile('/tmp/v53A_yeni/c2_train_ctx.i32', dtype=np.int32).reshape(-1, 2)
def tmed(fn, rep=5):
    ts = []
    for _ in range(rep): t = time.perf_counter(); r = fn(); ts.append(time.perf_counter() - t)
    return float(np.median(ts)) * 1e3, r
out = {}
# ---- A: bir ornek k=1000 kez
i0 = 1234; f32 = F[i0]; f = f32.astype(np.float64); y0 = int(y[i0]); k = 1000
R32 = np.tile(f32, (k, 1))                                           # 1000 fiziksel ornek (float32, sgemm yolu)
def ref_sgemm(): return R32.T @ R32                                   # V54 'cross': float32 sgemm
def ref_seq():                                                       # V54 sayim dongusunun mantigi: cift duyarlik, sirayla k kez ekle
    G = np.zeros((m, m)); RF = np.zeros((m, V)); acf = np.zeros(m); cnt = 0.0; rc = 0.0
    for _ in range(k): G += np.outer(f, f); RF[:, y0] += f; acf += f; cnt += 1.0; rc += 1.0
    return G, RF[:, y0].copy(), acf, cnt, rc
def trig():                                                          # tek tetikleyici: carpan k
    return k * np.outer(f, f), k * f, k * f, float(k), float(k)
t_s32, G32 = tmed(ref_sgemm); t_seq, (Gs, Rs, As, cs, rs) = tmed(ref_seq, 3); t_tr, (Gt, Rt, At, ct, rt) = tmed(trig)
Gex = np.array([[float(np.longdouble(k) * np.longdouble(a) * np.longdouble(b)) for b in f[:40]] for a in f[:40]])   # yuksek duyarlikli 40x40 alt blok
rel = lambda a, b: float(np.max(np.abs(a - b) / np.maximum(np.abs(b), 1e-300)))
out['A'] = dict(k=k, ms_1000_ornek_sgemm_float32=t_s32, ms_1000_ornek_sirali_cift_duyarlik=t_seq, ms_tek_tetikleyici=t_tr,
    G_trig_vs_seq_maks_bagil=rel(Gt, Gs), G_trig_vs_sgemm32_maks_bagil=rel(Gt, G32.astype(np.float64)), RHS_trig_vs_seq_maks_bagil=rel(Rt, Rs), ACF_bagil=rel(At, As), sayac_esit=(ct == cs and rt == rs),
    G_trig_vs_yuksek_duyarlik_40x40_bagil=rel(Gt[:40, :40], Gex), G_seq_vs_yuksek_duyarlik_40x40_bagil=rel(Gs[:40, :40], Gex))
# ---- A2: 2'nin kuvveti k (1024) icin sirali toplama tam mi?
k2 = 1024; Gs2 = np.zeros((m, m))
for _ in range(k2): Gs2 += np.outer(f, f)
out['A2_k1024_sirali_vs_carpan_bit_esit'] = bool(np.array_equal(Gs2, k2 * np.outer(f, f)))
# ---- B: gercek veride ayni F satiri tekrarlari (ayni (w2,w1,kat,y) -> ayni F): sikistirilmis Gram
rows = np.ascontiguousarray(F).view(np.dtype((np.void, F.dtype.itemsize * m))).ravel()
uq, first, inv, cnts = np.unique(rows, return_index=True, return_inverse=True, return_counts=True)
nu = len(uq); Fu = F[first]; mult = cnts.astype(np.float64)
Fd = F.astype(np.float64)
def full_gram(): return Fd.T @ Fd
def comp_gram(): Fud = Fu.astype(np.float64); return (Fud * mult[:, None]).T @ Fud
t_full, Gf = tmed(full_gram, 3); t_comp, Gc = tmed(comp_gram, 3)
Fs = np.ascontiguousarray(F); Fus = np.ascontiguousarray(Fu * np.sqrt(mult)[:, None].astype(np.float32))
t_full32, _ = tmed(lambda: Fs.T @ Fs, 3); t_comp32, _ = tmed(lambda: Fus.T @ Fus, 3)
# RHS: F^T Y
def rhs_full():
    R = np.zeros((m, V));
    for j in range(0, m, 1): pass
    return None
yu = y[first]; key = inv
# ayni (F satiri, y) cifti birlestirme: RHS carpani
pair = inv.astype(np.int64) * V + y; up, pinv, pc = np.unique(pair, return_inverse=True, return_counts=True)
out['B'] = dict(n=n, benzersiz_F_satiri=int(nu), benzersiz_oran=nu / n, benzersiz_F_ve_y_cifti=int(len(up)), ms_tam_gram_f64=t_full, ms_sikistirilmis_gram_f64=t_comp, ms_tam_gram_f32=t_full32, ms_sikistirilmis_gram_f32=t_comp32,
    gram_maks_bagil_fark=rel(Gc, Gf), gram_maks_mutlak_fark=float(np.max(np.abs(Gc - Gf))), en_cok_tekrar=int(cnts.max()))
# ---- C: donusum aileleri (ayni ornekten)
T = 1000
# C1: olcek (gecerli, dogrusal) donusum: s_t f
s = rng.uniform(0.5, 1.5, T); G1 = sum(np.outer(si * f, si * f) for si in s); G1t = np.sum(s ** 2) * np.outer(f, f); R1 = np.sum(s) * f
out['C1_olcek'] = dict(G_bagil=rel(G1t, G1), aciklama='tam: moment toplamlari (sum s, sum s^2) yeter')
# C2: ozellik uzayinda toplamsal gurultu: beklenen deger T(ff^T + sigma^2 I)
sig = 0.05; E = rng.normal(0, sig, (T, m)); Gn = (f + E).T @ (f + E); Gn_beklenen = T * (np.outer(f, f) + sig ** 2 * np.eye(m))
out['C2_gurultu'] = dict(sigma=sig, G_Frobenius_bagil_hata=float(np.linalg.norm(Gn - Gn_beklenen) / np.linalg.norm(Gn)), ort_kose_yerine_ort_fark=float(np.linalg.norm(Gn - T * np.outer(f, f)) / np.linalg.norm(Gn)),
    aciklama='beklenen etki = T*ff^T + T*sigma^2*I  (diyagonal ekleme = ridge); rastgele ornekle esit DEGIL, yalniz beklenen deger')
# C3: girdi uzayinda kucuk bozulma, gercek tanh haritasindan (V54 F1 yapisi: tanh(g(xW+b)))
Xr = np.fromfile('/tmp/v53A_yeni/c2_train_X.f32', dtype=np.float32).reshape(-1, 128).astype(np.float64); Xr = np.hstack([Xr, Xr[:, 64:] - Xr[:, :64]])
yl = y; P = 512; A = rng.integers(0, len(Xr), P); B = rng.integers(0, len(Xr), P); W = (Xr[A] - Xr[B]).T; W /= np.linalg.norm(W, axis=0, keepdims=True) + 1e-8; bb = -0.5 * np.einsum('ij,ij->j', (Xr[A] + Xr[B]).T, W)
x0 = Xr[i0]; g = 1.0; f0 = np.tanh(g * (x0 @ W + bb))
for sx in (0.01, 0.05):
    D = rng.normal(0, sx, (T, Xr.shape[1])); Ft = np.tanh(g * ((x0 + D) @ W + bb)); Gt_true = Ft.T @ Ft
    J = (1 - f0 ** 2)[:, None] * g * W.T                                   # 512 x 192
    Gfo = T * (np.outer(f0, f0) + sx ** 2 * (J @ J.T)); Gzero = T * np.outer(f0, f0)
    out[f'C3_girdi_bozulma_sigma{sx}'] = dict(sifirinci_mertebe_bagil_Frobenius_hata=float(np.linalg.norm(Gt_true - Gzero) / np.linalg.norm(Gt_true)), birinci_mertebe_formul_bagil_Frobenius_hata=float(np.linalg.norm(Gt_true - Gfo) / np.linalg.norm(Gt_true)))
print(json.dumps(out, indent=1, ensure_ascii=False)); json.dump(out, open('sonuc.json', 'w'), indent=1, ensure_ascii=False)
