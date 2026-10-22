#!/usr/bin/env python3
"""V8 n-gram referansi: dogrusal interpolasyonlu n-gram (ML olasiliklari + unigram + uniform), karisim agirliklari dogrulamada EM ile.
Ayni bloklar, ayni sozluk (egitimden), ayni degerlendirme konumlari (blok icinde t>=c) kullanilir. Son model egitim+dogrulama sayimlariyla, agirliklar sabit.
Kullanim: python3 metin_ngram.py   -> OZET_NGRAM satirlari"""
import numpy as np, json, time, resource, sys
from collections import defaultdict
d = "/tmp/v8data"; meta = json.load(open(f"{d}/meta.json")); V = meta["V"]
ids = {s: np.load(f"{d}/ids_{s}.npy") for s in ("train", "val", "test")}
def positions(blocks, c):
    for b in blocks:
        for t in range(c, len(b)): yield b, t
def counts(blocks, order):
    cn = [defaultdict(lambda: defaultdict(int)) for _ in range(order)]   # cn[k][ctx(k tokens)][w]
    for b in blocks:
        for t in range(len(b)):
            for k in range(order):
                if t - k < 0: break
                cn[k][tuple(b[t - k:t])][int(b[t])] += 1
    return cn
def comp_probs(cn, b, t, w, order):
    ps = []
    for k in range(order):
        ctx = tuple(b[t - k:t]); d_ = cn[k].get(ctx)
        if not d_: ps.append(0.0); continue
        tot = sum(d_.values()); ps.append(d_.get(w, 0) / tot)
    return ps
def run(c, order):
    t0 = time.time(); cn = counts(ids["train"], order)
    uni = np.zeros(V); [uni.__setitem__(w, uni[w] + 1) for b in ids["train"] for w in b]; uni = (uni + 1) / (uni.sum() + V)
    # EM agirliklari (dogrulama)
    rows = []
    for b, t in positions(ids["val"], c):
        w = int(b[t]); ps = comp_probs(cn, b, t, w, order); ps[0] = uni[w]; ps.append(1.0 / V); rows.append(ps)
    P = np.array(rows); lam = np.ones(P.shape[1]) / P.shape[1]
    for _ in range(200):
        r = P * lam; r /= r.sum(1, keepdims=True); lam = r.mean(0)
    val_ce = float(-np.log((P * lam).sum(1)).mean())
    cn2 = counts(list(ids["train"]) + list(ids["val"]), order)                                  # son model: egitim+dogrulama
    uni2 = np.zeros(V); [uni2.__setitem__(w, uni2[w] + 1) for blk in (ids["train"], ids["val"]) for b in blk for w in b]; uni2 = (uni2 + 1) / (uni2.sum() + V)
    t_fit = time.time() - t0
    t1 = time.time(); n = 0; ce = 0.0; mse = 0.0; c1 = 0; c5 = 0
    for b, t in positions(ids["test"], c):
        y = int(b[t]); p = lam[0] * uni2 + lam[-1] / V
        for k in range(1, order):
            d_ = cn2[k].get(tuple(b[t - k:t]))
            if d_:
                tot = sum(d_.values()); 
                for w, cnt in d_.items(): p[w] += lam[k] * cnt / tot
        ce += -np.log(p[y]); mse += ((p ** 2).sum() - 2 * p[y] + 1) / V
        top = np.argpartition(-p, 5)[:5]; c5 += (y in top); c1 += (int(np.argmax(p)) == y); n += 1
    t_eval = time.time() - t1
    ce /= n
    print(f"OZET_NGRAM|c={c}|n={order}|val_CE={val_ce:.4f}|mse_olasilik={mse/n:.6f}|CE={ce:.4f}|PPL={np.exp(ce):.2f}|top1={c1/n:.4f}|top5={c5/n:.4f}|egitim_sn={t_fit:.2f}|degerlendirme_sn={t_eval:.2f}|lambda={np.round(lam,3).tolist()}|RAM_MB={resource.getrusage(resource.RUSAGE_SELF).ru_maxrss/1024:.0f}", flush=True)
for c in (2, 4):
    for order in (1, 2, c + 1): run(c, order)
