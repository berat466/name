import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_search as o
from optim_search import V, C, U, B

o.LR_GRID = [0.0003, 0.001, 0.003, 0.01, 0.03, 0.1, 0.3]   # alt kenar genisletildi
adam = o.BASELINES["Adam"]
sg = U('sign', B('-', B('-', C(1.10413), V('w')), V('w')))
def adam_minus(k): return B('-', adam, B('*', C(k), V('w')))   # "anti weight decay": agirliklari buyutur
forms = {
  "SGD": o.BASELINES["SGD"], "Adam": adam,
  "Adam+sign(1.1-2w) [kesif]": B('+', adam, sg),
  "Adam-0.2w (ag. buyutme)": adam_minus(0.2),
  "Adam-0.5w (ag. buyutme)": adam_minus(0.5),
  "Adam-1.0w (ag. buyutme)": adam_minus(1.0),
}
SEEDS = (301, 302, 303, 304, 305)
settings = [("spiral H=24 100adim", "spiral", 24, 100),
            ("spiral H=64 500adim", "spiral", 64, 500),
            ("daire H=16 500adim", "circles", 16, 500)]

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
        print(f"\n== {title} ==  (toplam puan, dusuk=iyi | unutma A2 | en iyi lr)")
        for n in forms:
            t, a2, lr = res[i]; i += 1
            print(f"  {n:28} {t:7.3f} | {a2:6.3f} | lr={lr}")
