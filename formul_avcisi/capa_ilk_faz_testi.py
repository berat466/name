"""Capa yalniz gorev degisiminden sonra devreye girerse (ilk gorevde cekme yok) tum-gorev kaybi duzeliyor mu?"""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
from optim_search import BASELINES

NAMES = ["Adam", "SGD", "AdamW", "Adam+Capa(0.1)", "Adam+Capa(sqrt v)"]

def job(a):
    mode, name = a
    s.ANCHOR_FIRST_PHASE = mode
    seeds = [10**7 + i for i in range(200)]
    return s.suite_scores((BASELINES[name], seeds))

if __name__ == "__main__":
    seeds = [10**7 + i for i in range(200)]
    cont = np.array([len(s.make_task(sd)["phases"]) > 1 for sd in seeds])
    gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(x, 1e-4)))))
    with Pool() as p:
        for mode in ("init", "none"):
            res = p.map(job, [(mode, n) for n in NAMES])
            ad = res[0]
            print(f"\n== ilk gorevde capa: {mode} ==   (tum | ardisik) kalan kayip, Adam'a gore")
            for n, (g, lr, rs) in zip(NAMES, res):
                print(f"  {n:20} lr={lr:<4} tum={g:.4f} ({(1-g/ad[0])*100:+6.1f}%)   ardisik={gm(rs[cont]):.4f} ({(1-gm(rs[cont])/gm(ad[2][cont]))*100:+6.1f}%)")
