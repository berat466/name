"""Ekleme agi buyumesini kucultme: yeni blok genisligi Hn ve yanal baglanti rutbesi r (kimlik biliniyor)."""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
import ekleme_agi as E

CONFIGS = [("tam (Hn=H, yanal tam)", 1.0, "full"), ("Hn=H/2, yanal rutbe 4", 0.5, 4),
           ("Hn=H/4, yanal rutbe 2", 0.25, 2), ("Hn=H/4, yanal YOK", 0.25, 0), ("Hn=H/8, yanal rutbe 2", 0.125, 2)]
gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))

def run(task, lr, frac, rank):
    H, out, reg = task["H"], task["out"], task["reg"]
    Hn = max(2, int(round(H * frac)))
    (XA, yA, _), (XB, yB, _) = task["phases"]
    net = s.Net(H, out, task["seed"] % 1000); PA = [p.copy() for p in net.parts()]
    f = lambda X, P: np.tanh(X @ P[0] + P[1]) @ P[2] + P[3]
    l0A, l0B = E.loss_of(f(XA, PA), yA, reg), E.loss_of(f(XB, PA), yB, reg)
    opt = E.AdamV([p.shape for p in PA], lr); n = len(yA)
    with np.errstate(all="ignore"):
        for _ in range(task["steps"]):
            a = np.tanh(XA @ PA[0] + PA[1]); z = a @ PA[2] + PA[3]
            e = ((2 * (z[:, 0] - yA))[:, None] if reg else E.softmax_e(z, yA)) / n
            da = e @ PA[2].T * (1 - a * a)
            opt.step(PA, [XA.T @ da, da.sum(0), a.T @ e, e.sum(0)])
        if not all(np.isfinite(p).all() for p in PA): return None
    lossA = E.loss_of(f(XA, PA), yA, reg) / max(l0A, 1e-6)          # A donuk: bozulmaz
    rng = np.random.default_rng(task["seed"] + 11)
    aA = np.tanh(XB @ PA[0] + PA[1])
    P = [rng.normal(0, .7, (2, Hn)), np.zeros(Hn)]
    if rank == "full": P += [rng.normal(0, .7 / np.sqrt(H), (H, Hn))]
    elif rank: P += [rng.normal(0, .7 / np.sqrt(H), (H, rank)), rng.normal(0, .7 / np.sqrt(rank), (rank, Hn))]
    P += [rng.normal(0, .7 / np.sqrt(Hn), (Hn, out)), np.zeros(out)]
    nl = 0 if not rank else (1 if rank == "full" else 2)
    def fwd(X, aA_):
        h = X @ P[0] + P[1]
        if rank == "full": h = h + aA_ @ P[2]
        elif rank: h = h + (aA_ @ P[2]) @ P[3]
        a = np.tanh(h); return a, a @ P[2 + nl] + P[3 + nl]
    opt = E.AdamV([p.shape for p in P], lr); n = len(yB)
    with np.errstate(all="ignore"):
        for _ in range(task["steps"]):
            a, z = fwd(XB, aA)
            e = ((2 * (z[:, 0] - yB))[:, None] if reg else E.softmax_e(z, yB)) / n
            da = e @ P[2 + nl].T * (1 - a * a)
            G = [XB.T @ da, da.sum(0)]
            if rank == "full": G += [aA.T @ da]
            elif rank: G += [aA.T @ (da @ P[3].T), (aA @ P[2]).T @ da]
            G += [a.T @ e, e.sum(0)]
            opt.step(P, G)
        if not all(np.isfinite(p).all() for p in P): return None
    lossB = E.loss_of(fwd(XB, aA)[1], yB, reg) / max(l0B, 1e-6)
    base = 3 * H + H * out + out; new = sum(p.size for p in P)
    return min(s.CLIP, lossA), min(s.CLIP, lossB), (base + new) / base

def job(a):
    name, frac, rank = a
    tasks = [t for t in (s.make_task(10**7 + i) for i in range(200)) if len(t["phases"]) == 2]
    best = None
    for lr in (0.03, 0.1, 0.3):
        r = [x for x in (run(t, lr, frac, rank) for t in tasks) if x]
        arr = np.array(r); sc = gm(arr[:, :2].mean(1))
        if best is None or sc < best[0]: best = (sc, lr, gm(arr[:, 0]), gm(arr[:, 1]), arr[:, 2].mean())
    return name, best

if __name__ == "__main__":
    with Pool() as p: res = p.map(job, CONFIGS)
    print("93 ardisik gorev, kimlik biliniyor. Referans: Adam+capa ortalama 0.244 (parametre 1x)\n")
    print(f"{'ekleme bicimi':26} {'lr':>5} {'ESKI':>7} {'YENI':>7} {'ortalama':>9} {'toplam param (temel=1)':>24}")
    for name, (sc, lr, A, B, g) in res:
        print(f"{name:26} {lr:>5} {A:7.4f} {B:7.4f} {sc:9.4f} {g:20.2f}x")
