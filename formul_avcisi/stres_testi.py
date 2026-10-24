import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_search as o
from optim_search import V, C, U, B

adam = o.BASELINES["Adam"]
X = B('+', U('sign', B('-', B('-', C(1.10413), V('w')), V('w'))), B('/', V('m'), B('+', U('sqrt', V('v')), C(1.29199e-08))))
E3 = B('+', B('-', B('*', C(0.0130888), V('w')), B('-', U('neg', X), V('m'))), B('*', C(0.0116193), V('v')))
forms = {
  "SGD": o.BASELINES["SGD"], "RMSprop": o.BASELINES["RMSprop"], "Adam": adam, "AdamW": o.BASELINES["AdamW"],
  "KESIF#3 (tam)": E3,
  "abl: Adam+m": B('+', adam, V('m')),
  "abl: Adam+sign(1.1-2w)": B('+', adam, U('sign', B('-', B('-', C(1.10413), V('w')), V('w')))),
  "abl: Adam+m+sign": B('+', B('+', adam, V('m')), U('sign', B('-', B('-', C(1.10413), V('w')), V('w')))),
}
SEEDS = (201, 202, 203, 204, 205)
settings = [("spiral H=24 100adim (arama ortami)", "spiral", 24, 100),
            ("spiral H=64 500adim (uzun, genis)", "spiral", 64, 500),
            ("daire H=16 500adim (uzun, farkli gorev)", "circles", 16, 500)]

def job(a):
    name, e, kind, H, steps = a
    best = None
    for lr in o.LR_GRID:
        rs = [o.run_once(e, lr, kind, s, H, steps) for s in SEEDS]
        t = float(np.mean([r["A1"] + r["B2"] + r["A2"] for r in rs]))
        a2 = float(np.mean([r["A2"] for r in rs]))
        if best is None or t < best[0]: best = (t, a2, lr)
    return best

if __name__ == "__main__":
    jobs = [(n, e, k, H, st) for (_, k, H, st) in settings for n, e in forms.items()]
    with Pool() as p: res = p.map(job, jobs)
    i = 0
    for title, k, H, st in settings:
        print(f"\n== {title} ==  (toplam puan, dusuk=iyi | unutma A2 | lr)")
        for n in forms:
            t, a2, lr = res[i]; i += 1
            print(f"  {n:26} {t:7.3f} | {a2:6.3f} | lr={lr}")
