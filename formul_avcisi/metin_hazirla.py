#!/usr/bin/env python3
"""V8 veri hazirlama: Pride and Prejudice (Jane Austen, 1813; Project Gutenberg #1342, kamu malı). Ilk 50.000 token.
Bolme: 500 token'luk 100 BLOK; bloklar rastgele 80/10/10 egitim/dogrulama/test'e atanir (baglamlar blok disina tasmaz -> pencere ortusmesi/sizinti yok).
Sozluk, PPMI-SVD gomme ve n-gram sayimlari YALNIZ egitim bloklarindan. Cikti: /tmp/v8data/"""
import re, sys, json, numpy as np
from scipy.sparse.linalg import svds
out = "/tmp/v8data"; src = sys.argv[1] if len(sys.argv) > 1 else "/tmp/pg1342.txt"
txt = open(src, encoding="utf-8").read()
a = txt.index("*** START OF"); b = txt.index("*** END OF"); body = txt[a:b]
start = body.index("It is a truth universally acknowledged")
toks = re.findall(r"[a-z]+(?:'[a-z]+)?", body[start:].lower())[:50000]
assert len(toks) == 50000
BL = 500; nb = len(toks) // BL; rng = np.random.default_rng(0); perm = rng.permutation(nb)
split = {"train": sorted(perm[:80]), "val": sorted(perm[80:90]), "test": sorted(perm[90:])}
blocks = lambda s: [toks[i * BL:(i + 1) * BL] for i in split[s]]
from collections import Counter
cnt = Counter(w for blk in blocks("train") for w in blk)
V = 2001; vocab = ["<unk>"] + [w for w, _ in cnt.most_common(V - 1)]; wid = {w: i for i, w in enumerate(vocab)}
ids = lambda blk: np.array([wid.get(w, 0) for w in blk], dtype=np.int32)
B = {s: [ids(b_) for b_ in blocks(s)] for s in split}
# PPMI-SVD gomme (yalniz egitim bloklari, pencere +-2)
C = np.zeros((V, V))
for blk in B["train"]:
    for t in range(len(blk)):
        for d in (-2, -1, 1, 2):
            if 0 <= t + d < len(blk): C[blk[t], blk[t + d]] += 1
tot = C.sum(); pw = C.sum(1, keepdims=True) / tot; pc = (C.sum(0, keepdims=True) ** 0.75); pc = pc / pc.sum()
with np.errstate(divide="ignore", invalid="ignore"): P = np.maximum(np.log((C / tot) / (pw * pc)), 0); P[~np.isfinite(P)] = 0
U, S, _ = svds(P, k=64); E = U * np.sqrt(S); E /= np.maximum(np.linalg.norm(E, axis=1, keepdims=True), 1e-8); E = E.astype(np.float32)
meta = {"V": V, "d": 64, "tokens": 50000, "splits": {s: len(split[s]) * BL for s in split}, "kaynak": "Project Gutenberg #1342 Pride and Prejudice (kamu mali)"}
for c in (2, 4):
    for s in split:
        X = []; Y = []
        for blk in B[s]:
            for t in range(c, len(blk)):
                X.append(np.concatenate([E[blk[t - c + j]] for j in range(c)])); Y.append(blk[t])
        X = np.array(X, dtype=np.float32); Y = np.array(Y, dtype=np.int32)
        X.tofile(f"{out}/c{c}_{s}_X.f32"); Y.tofile(f"{out}/c{c}_{s}_y.i32"); meta[f"c{c}_{s}_n"] = int(len(Y))
for s in split: np.save(f"{out}/ids_{s}.npy", np.array([b_ for b_ in B[s]], dtype=np.int32))
json.dump(meta, open(f"{out}/meta.json", "w"), indent=1)
for c in (2, 4): open(f"{out}/meta_c{c}.txt", "w").write(f"{V} {c*64} {meta[f'c{c}_train_n']} {meta[f'c{c}_val_n']} {meta[f'c{c}_test_n']}\n")
unk = {s: float(np.mean([ (b_ == 0).mean() for b_ in B[s]])) for s in split}
print(json.dumps(meta), "unk orani:", unk, "egitim tur sayisi:", len(cnt))
