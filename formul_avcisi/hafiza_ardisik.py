"""Tek-hamle (ELM) ogrenmenin TOPLANABILIR istatistikleri: yeni gorev eskisine EKLENIR, hicbir sey ezilmez.
 S = F^T F ,  R = F^T T  ;  S_yeni = S_eski + S_B ,  R_yeni = R_eski + R_B ;  beta = (S + lam I)^-1 R
Ardisik gorevlerde TEST verisinde (her faz %70 egitim / %30 test) karsilastirma:
 Adam ince ayar (gorev basina en iyi lr = ayricalikli) | ELM yalniz son gorevle | ELM istatistik biriktirerek"""
import sys
sys.path.insert(0, ".")
from multiprocessing import Pool
import numpy as np
import optim_suite as s
import hafiza_ogrenme as H0

gm = lambda x: float(np.exp(np.mean(np.log(np.maximum(np.array(x), 1e-4)))))

def split_phase(task, i):
    X, y, _ = task["phases"][i]
    rng = np.random.default_rng(task["seed"] + 777 + i)
    idx = rng.permutation(len(y)); ntr = int(0.7 * len(y))
    return X[idx[:ntr]], y[idx[:ntr]], X[idx[ntr:]], y[idx[ntr:]]

def init_loss(task, X, y):
    return s.Net(task["H"], task["out"], task["seed"] % 1000).loss_grad(X, y, task["reg"], False)

def adam_seq(task, sp, lr):
    reg = task["reg"]; net = s.Net(task["H"], task["out"], task["seed"] % 1000)
    l0 = [net.loss_grad(sp[i][2], sp[i][3], reg, False) for i in range(2)]
    m = np.zeros_like(net.w); v = np.zeros_like(net.w); t = 0
    with np.errstate(all="ignore"):
        for i in range(2):
            Xtr, ytr = sp[i][0], sp[i][1]
            for _ in range(task["steps"]):
                t += 1; _, g = net.loss_grad(Xtr, ytr, reg)
                m = 0.9 * m + 0.1 * g; v = 0.999 * v + 0.001 * g * g
                net.w = net.w - lr * (m / (1 - 0.9 ** t)) / (np.sqrt(v / (1 - 0.999 ** t)) + 1e-8)
                if not np.isfinite(net.w).all(): return (s.CLIP, s.CLIP)
    lf = [net.loss_grad(sp[i][2], sp[i][3], reg, False) for i in range(2)]
    return tuple(min(s.CLIP, f / max(a, 1e-6)) if np.isfinite(f) else s.CLIP for f, a in zip(lf, l0))

def one(seed):
    t = s.make_task(seed)
    if len(t["phases"]) != 2: return None
    reg, out = t["reg"], t["out"]
    sp = [split_phase(t, i) for i in range(2)]
    l0 = [init_loss(t, sp[i][2], sp[i][3]) for i in range(2)]
    adam = min((adam_seq(t, sp, lr) for lr in (0.001, 0.003, 0.01, 0.03, 0.1, 0.3)), key=lambda r: np.mean(r))
    rng = np.random.default_rng(t["seed"] + 5); Hf = min(512, t["H"] * 8)
    Win = rng.normal(0, 1.5, (2, Hf)); b = rng.normal(0, 1.0, Hf)
    F = lambda X: np.hstack([np.tanh(X @ Win + b), np.ones((len(X), 1))])
    T = [(sp[i][1][:, None].astype(float) if reg else np.eye(out)[sp[i][1]]) for i in range(2)]
    Fs = [F(sp[i][0]) for i in range(2)]
    S = [f.T @ f for f in Fs]; R = [f.T @ tt for f, tt in zip(Fs, T)]          # toplanabilir istatistikler
    def evaluate(Smat, Rmat):
        best = None
        for lam in (1e-4, 1e-2, 1.0):
            beta = np.linalg.solve(Smat + lam * np.eye(Hf + 1), Rmat)
            if reg:
                sc = float(np.mean([((Fs[i] @ beta)[:, 0] - sp[i][1]) ** 2 for i in range(2)][0]))
                if best is None or sc < best[0]: best = (sc, beta, None)
            else:
                for tau in (1, 2, 4, 8, 16):
                    sc = np.mean([H0.ce_from_probs(H0.softmax((Fs[i] @ beta) * tau), sp[i][1]) for i in range(2)])
                    if best is None or sc < best[0]: best = (sc, beta, tau)
        _, beta, tau = best; r = []
        for i in range(2):
            Sc = F(sp[i][2]) @ beta
            lf = float(((Sc[:, 0] - sp[i][3]) ** 2).mean()) if reg else H0.ce_from_probs(H0.softmax(Sc * tau), sp[i][3])
            r.append(min(s.CLIP, lf / max(l0[i], 1e-6)))
        return tuple(r)
    elm_last = evaluate(S[1], R[1])                       # yalniz B
    elm_acc = evaluate(S[0] + S[1], R[0] + R[1])          # A + B istatistikleri toplandi
    return adam, elm_last, elm_acc

if __name__ == "__main__":
    seeds = [10**7 + i for i in range(200)]
    with Pool() as p: res = [r for r in p.map(one, seeds) if r]
    names = ["Adam ince ayar (en iyi lr, ayricalikli)", "ELM yalniz son gorev (eskiyi atar)", "ELM istatistik biriktirme (EKLE)"]
    print(f"{len(res)} ardisik gorev, TEST verisinde kalan kayip orani (dusuk iyi)\n")
    print(f"{'yontem':42} {'ESKI':>8} {'YENI':>8} {'ortalama':>9}")
    for k, nm in enumerate(names):
        A = [r[k][0] for r in res]; B = [r[k][1] for r in res]
        print(f"{nm:42} {gm(A):8.4f} {gm(B):8.4f} {gm(np.array([A, B]).mean(0)):9.4f}")
