"""Ardisik gorevlerde: eskiyi koruma (A) ve yeniyi ogrenme (B) AYRI ayri. Capa sadece B'yi mi bogduruyor?"""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
from optim_search import BASELINES, evaluate

FORMS = {"Adam": ("Adam", 0.3), "SGD": ("SGD", 0.1), "AdamW": ("AdamW", 0.1),
         "Adam+Capa(0.1)": ("Adam+Capa(0.1)", 0.1), "Adam+Capa(sqrt v)": ("Adam+Capa(sqrt v)", 0.1)}

def run_phases(args):
    name, lr, seed = args
    task = s.make_task(seed)
    if len(task["phases"]) < 2:
        return None
    expr = BASELINES[name]
    net = s.Net(task["H"], task["out"], task["seed"] % 1000); reg = task["reg"]
    l0 = [net.loss_grad(X, y, reg, False) for X, y, _ in task["phases"]]
    m = np.zeros_like(net.w); v = np.zeros_like(net.w); gp = np.zeros_like(net.w); t = 0
    with np.errstate(all="ignore"):
        for X, y, _ in task["phases"]:
            anchor = net.w.copy()
            for _ in range(task["steps"]):
                t += 1
                _, g = net.loss_grad(X, y, reg)
                m = 0.9*m + 0.1*g; v = 0.999*v + 0.001*g*g
                env = {"g": g, "m": m/(1-0.9**t), "v": v/(1-0.999**t), "w": net.w, "d": g-gp, "a": anchor}
                gp = g
                net.w = net.w - lr*evaluate(expr, env)
                if not np.isfinite(net.w).all():
                    return (s.CLIP, s.CLIP)
    lf = [net.loss_grad(X, y, reg, False) for X, y, _ in task["phases"]]
    r = [min(s.CLIP, f/max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0)]
    return (r[0], r[1])   # r[0]: ESKI gorev (korundu mu), r[1]: YENI gorev (ogrenildi mi)

if __name__ == "__main__":
    seeds = [10**7 + i for i in range(200)]
    gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(x, 1e-4)))))
    with Pool() as p:
        for label, (name, lr) in FORMS.items():
            res = [r for r in p.map(run_phases, [(name, lr, sd) for sd in seeds]) if r]
            A = [r[0] for r in res]; B = [r[1] for r in res]
            print(f"{label:20} ESKI gorev kalan kayip={gm(A):.4f}   YENI gorev kalan kayip={gm(B):.4f}   ({len(res)} ardisik gorev)")
