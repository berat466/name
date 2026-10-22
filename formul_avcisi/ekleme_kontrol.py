"""Ekleme agi icin kontroller: (1) kapasite esitleme, (2) yonlendirici hangi gorev turunde calisiyor."""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_search as o
import optim_suite as s
import ekleme_agi as E

gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))
SEEDS = [10**7 + i for i in range(200)]
def tasks():
    return [t for t in (s.make_task(sd) for sd in SEEDS) if len(t["phases"]) == 2]

def params_base(t): H, o_ = t["H"], t["out"]; return 3 * H + H * o_ + o_
def params_append(t): H, o_ = t["H"], t["out"]; return params_base(t) + 3 * H + H * H + H * o_ + o_

def wide(args):
    mult, lr, expr_name = args
    out = []
    for t in tasks():
        t2 = dict(t); t2["H"] = t["H"] * mult
        out.append(E.run_expr(t2, lr, o.BASELINES[expr_name]))
    return mult, lr, expr_name, out

def by_kind(lr):
    res = []
    for t in tasks():
        r = E.run_append(t, lr)
        res.append((t["reg"], r))
    return res

if __name__ == "__main__":
    T = tasks()
    pb = np.mean([params_base(t) for t in T]); pa = np.mean([params_append(t) for t in T])
    print(f"ortalama parametre: temel ag {pb:.0f},  ekleme agi (A+B sutunu) {pa:.0f}  -> {pa/pb:.1f}x\n")
    print("(1) KAPASITE ESITLEME: ince ayar yapan GENIS ag (H x2, x3), Adam ve Adam+capa, lr taramali")
    jobs = [(m, lr, n) for m in (2, 3) for n in ("Adam", "Adam+Capa(0.1)") for lr in E.LRS]
    with Pool() as p:
        res = p.map(wide, jobs)
        best = {}
        for m, lr, n, out in res:
            a = np.array(out); sc = gm(a.mean(1))
            if (m, n) not in best or sc < best[(m, n)][0]:
                best[(m, n)] = (sc, lr, gm(a[:, 0]), gm(a[:, 1]))
        for (m, n), (sc, lr, A, Bn) in sorted(best.items()):
            ptot = np.mean([params_base(dict(t, H=t["H"] * m)) for t in T])
            print(f"  H x{m} {n:16} (~{ptot:.0f} param) lr={lr:<5} ESKI={A:.4f} YENI={Bn:.4f} ortalama={sc:.4f}")
        print("\n(2) YONLENDIRICI: gorev turune gore (ekleme agi, lr=0.1)")
        kr = by_kind(0.1)
    for name, flag in (("siniflandirma (B girdileri baska bolgede)", False), ("regresyon (A ve B ayni girdi bolgesi)", True)):
        rows = [r for reg, r in kr if reg == flag and r]
        for k in ("id", "router"):
            A = gm([r[k][0] for r in rows]); Bn = gm([r[k][1] for r in rows])
            print(f"  {name:44} {k:7} ESKI={A:.4f} YENI={Bn:.4f}  ({len(rows)} gorev)")
