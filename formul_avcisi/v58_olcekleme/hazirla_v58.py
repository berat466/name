#!/usr/bin/env python3
"""V58: gercek veriyle (P&P pg1342 + Alice pg11, sunucuda mevcut tek metinler) V kelimelik V51b-formatli veri kumesi.
Bolme: 500-token bloklar, rng(0) permutasyon, %80 egitim / %10 val / %10 test. Sozluk YALNIZ egitimden (<unk> + en sik V-1). Gomme: PPMI-SVD(64) egitimden.
Egitimde V-1'den az farkli kelime varsa HATA (sahte kelime uretilmez)."""
import re, sys, json, os, numpy as np
from collections import Counter
from scipy.sparse.linalg import svds
V = int(sys.argv[1]); out = sys.argv[2]; os.makedirs(out, exist_ok=True)
BL = 500; toks = []
for f in ("/tmp/pg1342.txt", "/tmp/pg11.txt"):
    t = open(f, encoding="utf-8").read(); a = t.index("*** START OF"); b = t.index("*** END OF"); body = t[a:b]
    if "pg1342" in f: body = body[body.index("It is a truth universally acknowledged"):]
    toks += re.findall(r"[a-z]+(?:'[a-z]+)?", body.lower()); 
nb = len(toks) // BL; rng = np.random.default_rng(0); perm = rng.permutation(nb); nv = nb // 10
split = {"val": sorted(perm[:nv]), "test": sorted(perm[nv:2 * nv]), "train": sorted(perm[2 * nv:])}
blocks = lambda s: [toks[i * BL:(i + 1) * BL] for i in split[s]]
cnt = Counter(w for b in blocks("train") for w in b); ntype = len(cnt)
if ntype < V - 1: print(f"HATA: egitimde yalniz {ntype} farkli kelime var, V={V} icin yetersiz"); sys.exit(2)
vocab = ["<unk>"] + [w for w, _ in cnt.most_common(V - 1)]; wid = {w: i for i, w in enumerate(vocab)}
ids = lambda blk: np.array([wid.get(w, 0) for w in blk], dtype=np.int32)
B = {s: [ids(b) for b in blocks(s)] for s in split}
C = np.zeros((V, V), np.float32)
for blk in B["train"]:
    for t in range(len(blk)):
        for d in (-2, -1, 1, 2):
            if 0 <= t + d < len(blk): C[blk[t], blk[t + d]] += 1
tot = C.sum(); pw = C.sum(1, keepdims=True) / tot; pc = C.sum(0, keepdims=True) ** 0.75; pc = pc / pc.sum()
with np.errstate(divide="ignore", invalid="ignore"): P = np.maximum(np.log((C / tot) / (pw * pc)), 0); P[~np.isfinite(P)] = 0
U, S, _ = svds(P.astype(np.float64), k=64, v0=np.ones(V)); E = U * np.sqrt(S); E /= np.maximum(np.linalg.norm(E, axis=1, keepdims=True), 1e-8); E = E.astype(np.float32)
meta = {"V": V, "egitim_tur_sayisi": ntype, "toplam_token": len(toks), "bloklar": {s: len(B[s]) for s in B}}
c = 2
for s in B:
    X = []; Y = []; CT = []
    for blk in B[s]:
        for t in range(c, len(blk)):
            X.append(np.concatenate([E[blk[t - c + j]] for j in range(c)])); Y.append(blk[t]); CT.append(blk[t - c:t])
    X = np.array(X, np.float32); Y = np.array(Y, np.int32); CT = np.array(CT, np.int32)
    X.tofile(f"{out}/c2_{s}_X.f32"); Y.tofile(f"{out}/c2_{s}_y.i32"); CT.tofile(f"{out}/c2_{s}_ctx.i32"); meta[f"c2_{s}_n"] = int(len(Y))
for s in B: np.save(f"{out}/ids_{s}.npy", np.array(B[s], dtype=np.int32))
open(f"{out}/meta_c2.txt", "w").write(f"{V} 128 {meta['c2_train_n']} {meta['c2_val_n']} {meta['c2_test_n']}\n")
tr = np.concatenate(B["train"]); va = np.concatenate(B["val"])
((np.bincount(tr, minlength=V) + 1.0) / (len(tr) + V)).astype(np.float64).tofile(f"{out}/uni_tr.f64")
((np.bincount(np.concatenate([tr, va]), minlength=V) + 1.0) / (len(tr) + len(va) + V)).astype(np.float64).tofile(f"{out}/uni_trva.f64")
json.dump(meta, open(f"{out}/meta.json", "w")); print(json.dumps(meta))
