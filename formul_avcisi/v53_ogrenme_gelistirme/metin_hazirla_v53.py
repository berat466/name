#!/usr/bin/env python3
"""V53 / Asama A: egitim verisini genislet (yalniz TRAIN'e).
metin_hazirla.py ilk 50.000 token'i kesiyordu ([:50000] + assert). Kalan ~73.6k token val/test'e AYRILMAMISTI, hic kullanilmiyordu.
Burada: ayni tokenizasyon, ayni rng(0) bolme (ilk 100 blok -> ayni 80/10/10), ayni 2001 kelimelik SOZLUK (val/test id'leri birebir ayni),
kalan TAM 500-token bloklari (100..) YALNIZ egitime eklenir (eski egitim satirlari basta, ayni sirada -> doc-fold atamasi eski satirlar icin ayni).
mod:  eskiemb -> gommeler ESKI egitim bloklarindan (yalniz veri etkisini izole eder)
      yeniemb -> PPMI-SVD gommeleri genisletilmis egitimden (sozluk sabit)
Cikti: <out>/ (V51b'nin bekledigi format: c2_*_X/y/ctx, meta_c2.txt, uni_tr.f64, uni_trva.f64, ids_*.npy)"""
import re, sys, json, numpy as np
from collections import Counter
from scipy.sparse.linalg import svds
mod = sys.argv[1]; out = sys.argv[2]; src = "/tmp/pg1342.txt"
import os; os.makedirs(out, exist_ok=True)
txt = open(src, encoding="utf-8").read()
a = txt.index("*** START OF"); b = txt.index("*** END OF"); body = txt[a:b]
start = body.index("It is a truth universally acknowledged")
alltoks = re.findall(r"[a-z]+(?:'[a-z]+)?", body[start:].lower())
toks = alltoks[:50000]
BL = 500; nb = len(toks) // BL; rng = np.random.default_rng(0); perm = rng.permutation(nb)
split = {"train": sorted(perm[:80]), "val": sorted(perm[80:90]), "test": sorted(perm[90:])}
blocks = lambda s: [toks[i * BL:(i + 1) * BL] for i in split[s]]
old_train = blocks("train")
extra = [alltoks[i * BL:(i + 1) * BL] for i in range(nb, len(alltoks) // BL)]          # kalan TAM bloklar
cnt = Counter(w for blk in old_train for w in blk)                                       # SOZLUK: eski egitimden (val/test id'leri sabit)
V = 2001; vocab = ["<unk>"] + [w for w, _ in cnt.most_common(V - 1)]; wid = {w: i for i, w in enumerate(vocab)}
ids = lambda blk: np.array([wid.get(w, 0) for w in blk], dtype=np.int32)
B = {s: [ids(b_) for b_ in blocks(s)] for s in ("val", "test")}
B["train"] = [ids(b_) for b_ in old_train] + [ids(b_) for b_ in extra]
n_old = len(old_train)
# --- sizinti denetimi: egitim (genisletilmis) ile val/test arasinda 8-gram ortusmesi
def grams(blks, n=8): return {tuple(b_[i:i + n]) for b_ in blks for i in range(len(b_) - n + 1)}
g_tr = grams(extra + old_train); ov = {s: len(g_tr & grams(blocks(s))) for s in ("val", "test")}
# --- gomme
emb_blocks = B["train"][:n_old] if mod == "eskiemb" else B["train"]
C = np.zeros((V, V))
for blk in emb_blocks:
    for t in range(len(blk)):
        for d in (-2, -1, 1, 2):
            if 0 <= t + d < len(blk): C[blk[t], blk[t + d]] += 1
tot = C.sum(); pw = C.sum(1, keepdims=True) / tot; pc = (C.sum(0, keepdims=True) ** 0.75); pc = pc / pc.sum()
with np.errstate(divide="ignore", invalid="ignore"): P = np.maximum(np.log((C / tot) / (pw * pc)), 0); P[~np.isfinite(P)] = 0
if mod == "eskiemb":   # gommeler V51b'nin kullandigi dosyalardan birebir geri alinir (svds'in ARPACK baslangici rastgele -> yeniden hesap isaret/donus farki verir)
    Xo = np.fromfile("/tmp/v8data/c2_train_X.f32", dtype=np.float32).reshape(-1, 128); Co = np.fromfile("/tmp/v8data/c2_train_ctx.i32", dtype=np.int32).reshape(-1, 2)
    E = np.zeros((V, 64), np.float32); seen = np.zeros(V, bool)
    for j in (0, 1): E[Co[:, j]] = Xo[:, 64 * j:64 * j + 64]; seen[Co[:, j]] = True
    assert seen.all()
else:
    U, S, _ = svds(P, k=64, v0=np.ones(V)); E = U * np.sqrt(S); E /= np.maximum(np.linalg.norm(E, axis=1, keepdims=True), 1e-8); E = E.astype(np.float32)
meta = {"V": V, "d": 64, "mod": mod, "train_blok": len(B["train"]), "eski_train_blok": n_old, "ek_blok": len(extra), "kullanilan_token": len(alltoks), "kitap_token": len(alltoks),
        "splits": {s: len(B[s]) * BL for s in B}, "8gram_ortusme": ov}
for c in (2,):
    for s in B:
        X = []; Y = []; CT = []
        for blk in B[s]:
            for t in range(c, len(blk)):
                X.append(np.concatenate([E[blk[t - c + j]] for j in range(c)])); Y.append(blk[t]); CT.append(blk[t - c:t])
        X = np.array(X, dtype=np.float32); Y = np.array(Y, dtype=np.int32); CT = np.array(CT, dtype=np.int32)
        X.tofile(f"{out}/c{c}_{s}_X.f32"); Y.tofile(f"{out}/c{c}_{s}_y.i32"); CT.tofile(f"{out}/c{c}_{s}_ctx.i32"); meta[f"c{c}_{s}_n"] = int(len(Y))
for s in B: np.save(f"{out}/ids_{s}.npy", np.array(B[s], dtype=np.int32))
json.dump(meta, open(f"{out}/meta.json", "w"), indent=1)
open(f"{out}/meta_c2.txt", "w").write(f"{V} {2*64} {meta['c2_train_n']} {meta['c2_val_n']} {meta['c2_test_n']}\n")
tr_ids = np.concatenate(B["train"]); va_ids = np.concatenate(B["val"])
(np.bincount(tr_ids, minlength=V) + 1.0).__truediv__(len(tr_ids) + V).astype(np.float64).tofile(f"{out}/uni_tr.f64")
(np.bincount(np.concatenate([tr_ids, va_ids]), minlength=V) + 1.0).__truediv__(len(tr_ids) + len(va_ids) + V).astype(np.float64).tofile(f"{out}/uni_trva.f64")
print(json.dumps(meta))
