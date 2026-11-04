#!/usr/bin/env python3
"""V59: ORTAK genisletilmis veri kumesi (Project Gutenberg, kamu malı): pg1342 Pride&Prejudice, pg11 Alice, pg2701 Moby Dick.
500-token bloklar, rng(0) permutasyon, %80/%10/%10. Kelime kimlikleri EGITIMDEN frekansa gore SABIT (id 1.. = en sik); V boyutu yalniz esik.
Gomme: egitimden PPMI-SVD(64), tam sozluk (VMAX) uzerinde BIR KEZ; tum V boyutlari ayni X'i kullanir (girdi ozellikleri V'den bagimsiz).
Hedef y ve baglam ctx kimlikleri: id>=V -> 0 (<unk>). Sahte kelime yok; val/test egitime sizdirilmaz (sozluk ve gomme yalniz egitimden)."""
import re, sys, json, os, numpy as np
from collections import Counter
from scipy.sparse.linalg import svds
VMAX = int(sys.argv[1]); sizes = [int(x) for x in sys.argv[2].split(",")]; BL = 500
def body(f, start=None):
    t = open(f, encoding="utf-8", errors="ignore").read(); a = t.index("*** START OF"); b = t.index("*** END OF"); s = t[a:b]
    return s[s.index(start):] if start else s
toks = []
import glob
for f, st in [("/tmp/pg1342.txt", "It is a truth universally acknowledged"), ("/tmp/pg11.txt", None)] + [(g, None) for g in sorted(glob.glob("/tmp/v59_kitap/pg*.txt"))]:
    toks += re.findall(r"[a-z]+(?:'[a-z]+)?", body(f, st).lower())
nb = len(toks) // BL; perm = np.random.default_rng(0).permutation(nb); nv = nb // 10
split = {"val": sorted(perm[:nv]), "test": sorted(perm[nv:2 * nv]), "train": sorted(perm[2 * nv:])}
blocks = lambda s: [toks[i * BL:(i + 1) * BL] for i in split[s]]
cnt = Counter(w for b in blocks("train") for w in b); ntype = len(cnt)
assert ntype >= VMAX - 1, f"egitimde {ntype} tur < {VMAX-1}"
vocab = ["<unk>"] + [w for w, _ in cnt.most_common(VMAX - 1)]; wid = {w: i for i, w in enumerate(vocab)}
ids = lambda blk: np.array([wid.get(w, 0) for w in blk], dtype=np.int32)
B = {s: [ids(b) for b in blocks(s)] for s in split}
C = np.zeros((VMAX, VMAX), np.float32)
for blk in B["train"]:
    for d in (-2, -1, 1, 2):
        a, b = (0, len(blk) - d) if d > 0 else (-d, len(blk)); np.add.at(C, (blk[a:b], blk[a + d:b + d]), 1)
tot = C.sum(); pw = C.sum(1, keepdims=True) / tot; pc = C.sum(0, keepdims=True) ** 0.75; pc = pc / pc.sum()
with np.errstate(divide="ignore", invalid="ignore"): P = np.maximum(np.log((C / tot) / (pw * pc)), 0); P[~np.isfinite(P)] = 0
U, S, _ = svds(P.astype(np.float64), k=64, v0=np.ones(VMAX)); E = U * np.sqrt(S); E /= np.maximum(np.linalg.norm(E, axis=1, keepdims=True), 1e-8); E = E.astype(np.float32)
meta = {"VMAX": VMAX, "egitim_tur": ntype, "toplam_token": len(toks), "egitim_token": 500 * len(B["train"]), "bloklar": {s: len(B[s]) for s in B}}
raw = {}
for s in B:
    X = []; Y = []; CT = []
    for blk in B[s]:
        X.append(np.concatenate([E[blk[0:-2, None]][:, 0], E[blk[1:-1, None]][:, 0]], 1)); Y.append(blk[2:]); CT.append(np.stack([blk[:-2], blk[1:-1]], 1))
    raw[s] = (np.concatenate(X).astype(np.float32), np.concatenate(Y).astype(np.int32), np.concatenate(CT).astype(np.int32))
for V in sizes:
    out = f"/tmp/v76_{V}"; os.makedirs(out, exist_ok=True); n = {}
    for s in B:
        X, Y, CT = raw[s]; m = lambda a: np.where(a >= V, 0, a).astype(np.int32)
        X.tofile(f"{out}/c2_{s}_X.f32"); m(Y).tofile(f"{out}/c2_{s}_y.i32"); m(CT).tofile(f"{out}/c2_{s}_ctx.i32"); n[s] = len(Y)
    for s in B: np.save(f"{out}/ids_{s}.npy", np.array([np.where(b >= V, 0, b) for b in B[s]], dtype=np.int32))
    open(f"{out}/meta_c2.txt", "w").write(f"{V} 128 {n['train']} {n['val']} {n['test']}\n")
    tr = np.where(np.concatenate(B["train"]) >= V, 0, np.concatenate(B["train"])); va = np.where(np.concatenate(B["val"]) >= V, 0, np.concatenate(B["val"]))
    ((np.bincount(tr, minlength=V) + 1.0) / (len(tr) + V)).astype(np.float64).tofile(f"{out}/uni_tr.f64")
    ((np.bincount(np.concatenate([tr, va]), minlength=V) + 1.0) / (len(tr) + len(va) + V)).astype(np.float64).tofile(f"{out}/uni_trva.f64")
    meta[f"n_{V}"] = n; meta[f"unk_orani_egitim_{V}"] = float((tr == 0).mean())
np.save("/tmp/v76_ids_train_full.npy", np.concatenate(B["train"]))
json.dump(meta, open("/tmp/v76_meta.json", "w")); print(json.dumps(meta))
