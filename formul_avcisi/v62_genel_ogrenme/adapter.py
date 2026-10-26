#!/usr/bin/env python3
"""V62 veri adaptoru: tablosal sayisal veri -> V51b/V61 motorunun bekledigi dosya bicimi (c2_*_X.f32, _y.i32, _ctx.i32, meta_c2.txt, uni_*.f64).
Girdi: standardize edilmis gercek sayilar, ilk 64-lu blokta (kalan 0); ctx = sabit 0 (kelime bilgisi YOK -> motorun C blogu tek bos slota cokuyor);
hedef: sinif kimligi (V = sinif sayisi). Etiket karistirma kontrolu: shuffle=1."""
import sys, os, numpy as np
from sklearn.datasets import load_digits, load_breast_cancer, load_wine
from sklearn.model_selection import train_test_split
name, out, shuffle = sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 0
d = {"digits": load_digits, "bc": load_breast_cancer, "wine": load_wine}[name](); X, y = d.data.astype(np.float64), d.target.astype(np.int32); V = int(y.max() + 1)
Xtr, Xr, ytr, yr = train_test_split(X, y, test_size=0.4, random_state=0, stratify=y); Xva, Xte, yva, yte = train_test_split(Xr, yr, test_size=0.5, random_state=0, stratify=yr)
if shuffle: rng = np.random.default_rng(1); ytr = rng.permutation(ytr)
mu, sd = Xtr.mean(0), Xtr.std(0) + 1e-9; os.makedirs(out, exist_ok=True)
def pad(A): P = np.zeros((len(A), 128), np.float32); P[:, :A.shape[1]] = (A - mu) / sd; return P
for s, A, b in (("train", Xtr, ytr), ("val", Xva, yva), ("test", Xte, yte)):
    pad(A).tofile(f"{out}/c2_{s}_X.f32"); b.astype(np.int32).tofile(f"{out}/c2_{s}_y.i32"); np.zeros((len(b), 2), np.int32).tofile(f"{out}/c2_{s}_ctx.i32")
open(f"{out}/meta_c2.txt", "w").write(f"{V} 128 {len(ytr)} {len(yva)} {len(yte)}\n")
((np.bincount(ytr, minlength=V) + 1.0) / (len(ytr) + V)).astype(np.float64).tofile(f"{out}/uni_tr.f64")
((np.bincount(np.concatenate([ytr, yva]), minlength=V) + 1.0) / (len(ytr) + len(yva) + V)).astype(np.float64).tofile(f"{out}/uni_trva.f64")
print(name, "V", V, "n", len(ytr), len(yva), len(yte), "ozellik", X.shape[1])
