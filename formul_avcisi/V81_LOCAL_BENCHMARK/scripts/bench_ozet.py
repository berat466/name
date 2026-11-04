#!/usr/bin/env python3
import re, sys, statistics as st, json
out, isa, n = sys.argv[1], sys.argv[2], int(sys.argv[3])
def rd(v, r):
    t = open(f"{out}/bench_v{v}_{isa}_{r}.log", errors="ignore").read()
    f = lambda p: float(re.search(p, t).group(1)) if re.search(p, t) else float("nan")
    return dict(wall=f(r"TH_WALL_SECONDS=([0-9.]+)"), model=f(r"MODEL_HAZIR\|([0-9.]+)"), rss=f(r"tepe RSS=([0-9.]+)"), ppl=(re.search(r"TEST CE=[0-9.]+ PPL=([0-9.]+)", t) or [0, "?"])[1])
R = {v: [rd(v, r) for r in range(1, n + 1)] for v in (80, 81)}
S = {}
for v in (80, 81):
    S[v] = {k: st.median(x[k] for x in R[v]) for k in ("wall", "model")}; S[v]["wall_aralik"] = (min(x["wall"] for x in R[v]), max(x["wall"] for x in R[v])); S[v]["model_aralik"] = (min(x["model"] for x in R[v]), max(x["model"] for x in R[v])); S[v]["ppl"] = sorted(set(x["ppl"] for x in R[v]))
dif = [(a["wall"] - b["wall"], a["model"] - b["model"]) for a, b in zip(R[80], R[81])]
md = f"""# Yerel hiz olcumu ({isa}, {n} donusumlu cift)
| | V80 | V81 |
|---|---|---|
| tam program (medyan s) | {S[80]['wall']:.3f} [{S[80]['wall_aralik'][0]:.2f}-{S[80]['wall_aralik'][1]:.2f}] | {S[81]['wall']:.3f} [{S[81]['wall_aralik'][0]:.2f}-{S[81]['wall_aralik'][1]:.2f}] |
| model hazirlama (medyan s) | {S[80]['model']:.3f} [{S[80]['model_aralik'][0]:.2f}-{S[80]['model_aralik'][1]:.2f}] | {S[81]['model']:.3f} [{S[81]['model_aralik'][0]:.2f}-{S[81]['model_aralik'][1]:.2f}] |
| test PPL | {S[80]['ppl']} | {S[81]['ppl']} |
Cift farklari (V80-V81) tam program: {[round(d[0], 2) for d in dif]} ; model: {[round(d[1], 2) for d in dif]}
"""
open(f"{out}/bench_{isa}_ozet.md", "w").write(md); json.dump({"V80": S[80], "V81": S[81], "cift_farklari": dif}, open(f"{out}/bench_{isa}_ozet.json", "w"), indent=1); print(md)
