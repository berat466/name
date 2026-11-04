#!/usr/bin/env python3
"""V15 (V9 ile ayni kalibrasyon, V15 skor dizini): P = (1-alpha) P_unigram + alpha softmax(z/T) olasilik kalibrasyonu. Agirliklar (W1, W2) DEGISMEZ: V8 ile ayni uretilmis,
dogrulama skorlari egitim-yalniz modelden, test skorlari egitim+dogrulama modelinden (V8 protokolu). alpha,T yalniz DOGRULAMA CE ile secilir.
Kullanim: python3 -I metin_kalibrasyon.py MOD c H TOHUM  -> OZET_V9 satirlari (sartname izgarasi ve genisletilmis izgara ayri)"""
import sys, time, resource, json, numpy as np
mode, c, H, seed = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
d = "/tmp/v8data"; z = "/tmp/v15data"; V = json.load(open(f"{d}/meta.json"))["V"]
t0 = time.process_time(); w0 = time.time()
ids = {s: np.load(f"{d}/ids_{s}.npy") for s in ("train", "val", "test")}
cnt = lambda blks: np.bincount(np.concatenate([b.ravel() for b in blks]), minlength=V).astype(np.float64)
ctr, cva = cnt([ids["train"]]), cnt([ids["val"]])
uni_val = (ctr + 1) / (ctr.sum() + V)                                   # dogrulama icin: yalniz egitim sayimlari
uni_test = (ctr + cva + 1) / (ctr.sum() + cva.sum() + V)                # test icin: egitim+dogrulama (V8/n-gram protokolu)
yv = np.fromfile(f"{d}/c{c}_val_y.i32", dtype=np.int32); yt = np.fromfile(f"{d}/c{c}_test_y.i32", dtype=np.int32)
tag = f"{mode}_c{c}_H{H}_s{seed}"
Zv = np.fromfile(f"{z}/{tag}_val.f32", dtype=np.float32).reshape(len(yv), V); Zt = np.fromfile(f"{z}/{tag}_test.f32", dtype=np.float32).reshape(len(yt), V)
def smax(Z, T): 
    A = (Z - Z.max(1, keepdims=True)) / T; E = np.exp(A); return E / E.sum(1, keepdims=True)
def py_of(P, y): return P[np.arange(len(y)), y]
def full_metrics(p, y, label):
    n = len(y); py = p[np.arange(n), y]; ce = float(-np.log(py).mean())
    rank = (p > py[:, None]).sum(1)
    return dict(ce=ce, ppl=float(np.exp(ce)), top1=float((rank == 0).mean()), top5=float((rank < 5).mean()), mse=float(((p ** 2).sum(1) - 2 * py + 1).mean() / V), toplam=float(p.sum(1).mean()))
def run(alphas, Ts, name):
    best = (1e30, None, None); pv = {}
    for T in Ts: pv[T] = py_of(smax(Zv, T), yv)
    for T in Ts:
        for a in alphas:
            ce = float(-np.log((1 - a) * uni_val[yv] + a * pv[T]).mean())
            if ce < best[0] - 1e-12: best = (ce, a, T)
    ce_v, a, T = best
    p = (1 - a) * uni_test[None, :] + a * smax(Zt, T); m = full_metrics(p, yt, name)
    print(f"OZET_V9|{name}|mod={mode}|c={c}|H={H}|tohum={seed}|alpha={a}|T={T}|val_CE={ce_v:.4f}|test_CE={m['ce']:.4f}|PPL={m['ppl']:.2f}|top1={m['top1']:.4f}|top5={m['top5']:.4f}|mse_olasilik={m['mse']:.6f}|olasilik_toplami={m['toplam']:.6f}", flush=True)
# referanslar: unigram (alpha=0) ve V8 (alpha=1, V8 sicaklik izgarasi)
p = np.tile(uni_test, (len(yt), 1)); m = full_metrics(p, yt, 'unigram'); print(f"OZET_V9|unigram|mod={mode}|c={c}|H={H}|tohum={seed}|test_CE={m['ce']:.4f}|PPL={m['ppl']:.2f}|top1={m['top1']:.4f}|top5={m['top5']:.4f}|mse_olasilik={m['mse']:.6f}", flush=True)
run([1.0], [0.003, 0.005, 0.01, 0.02, 0.03, 0.05, 0.07, 0.1, 0.2], "V8_alpha1_V8_T_izgarasi")
run([0, .25, .5, .75, 1], [.25, .5, 1, 2, 4], "V9_sartname_izgarasi")
run([0, .05, .1, .15, .2, .25, .35, .5, .75, 1], [.005, .01, .02, .03, .05, .1, .25, .5, 1, 2, 4], "V9_genisletilmis_izgara")
print(f"CPU_SURE|{tag}|kalibrasyon_cpu_sn={time.process_time()-t0:.2f}|duvar_sn={time.time()-w0:.2f}|RAM_MB={resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024:.0f}", flush=True)
