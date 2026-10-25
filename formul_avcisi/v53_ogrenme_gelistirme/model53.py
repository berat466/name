# V51b analitik cozumunun Python kopyasi (V53): veri dizini parametreli; F dokumunden ayni Gram/RHS/Schur/Ridge + dogrusal okuma + son Q karisimi.
import numpy as np, scipy.sparse as sp, scipy.linalg as sl, os
V = 2001
D = os.environ.get('M53_D', '/tmp/v53A_yeni'); FD = os.environ.get('M53_FD', '/tmp/v53FA')
def rows(s):
    ids = np.load(f'{D}/ids_{s}.npy'); nb = ids.shape[0]
    return ids[:, :-2].reshape(-1), ids[:, 1:-1].reshape(-1), ids[:, 2:].reshape(-1), np.repeat(np.arange(nb), 498), ids
w2t, w1t, yt, bt, idt = rows('train'); w2v, w1v, yv, bv, idv = rows('val'); w2e, w1e, ye, be, ide = rows('test')
n, nv, ne = len(yt), len(yv), len(ye)
uni = np.fromfile(f'{D}/uni_tr.f64')
def pair_slots(top_n=2000):
    key = w2t.astype(np.int64) * V + w1t; kk, cc = np.unique(key, return_counts=True); order = np.lexsort((kk, -cc)); top = kk[order[:top_n]]; pidx = {int(k): i for i, k in enumerate(top)}
    f = lambda a, b: np.array([pidx.get(int(x) * V + int(y), -1) for x, y in zip(a, b)])
    return f(w2t, w1t), f(w2v, w1v), f(w2e, w1e)
pt, pv, pe = pair_slots()
NC = V + 2000 + 1
def Cmat(w1, p, nc=NC):
    m = len(w1); r = np.concatenate([np.arange(m), np.arange(m)[p >= 0], np.arange(m)]); c = np.concatenate([w1, V + p[p >= 0], np.full(m, nc - 1)]); return sp.csr_matrix((np.ones(len(r)), (r, c)), shape=(m, nc))
Ct, Cv, Ce = Cmat(w1t, pt), Cmat(w1v, pv), Cmat(w1e, pe)
def load(tag=None):
    tag = tag or FD
    g = lambda nm, m: np.fromfile(f'{tag}/{nm}', dtype=np.float32).reshape(m, 513)[:, 1:].astype(np.float64)
    return g('Fc.f32', n), g('Fcv.f32', nv), g('Fce.f32', ne)
GA = (0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.85, 0.9, 0.93, 0.95, 0.97, 0.98, 0.99, 1.0); GE = (1e-7, 1e-6, 1e-5, 1e-4, 3e-4, 1e-3, 3e-3, 1e-2)
def read_P(Z, a, e, pi=None):
    pi = uni if pi is None else pi; S = np.maximum(Z, e); return (1 - a) * pi[None, :] + a * S / S.sum(1, keepdims=True)
def read_ce(Z, y, pi=None):
    pi = uni if pi is None else pi; best = (1e9, None, None); r = np.arange(len(y))
    for e in GE:
        S = np.maximum(Z, e); zy = S[r, y] / S.sum(1)
        for a in GA:
            ce = -np.log((1 - a) * pi[y] + a * zy).mean()
            if ce < best[0]: best = (ce, a, e)
    return best
ce_of = lambda P, y: -np.log(P[np.arange(len(y)), y]).mean()
# --- Q_havuz (V39 grafi, b=0.9, K=10, aday sayim>=5)
def counts(w1, y): C = np.zeros((V, V)); np.add.at(C, (w1, y), 1); return C
def graph(Cn, K=10):
    nw = Cn.sum(1); Nn = np.sqrt(Cn); Nn /= np.maximum(np.linalg.norm(Nn, axis=1, keepdims=True), 1e-9); S = Nn @ Nn.T
    cand = np.where(nw >= 5)[0]; nb = -np.ones((V, K), int); wt = np.zeros((V, K)); Sc = S[:, cand].copy()
    for u in range(V):
        s = Sc[u].copy(); s[cand == u] = -1; kk = min(K, len(cand)); o = np.argsort(-s, kind='stable')[:kk]; nb[u, :kk] = cand[o]; w = np.maximum(s[o], 0); wt[u, :kk] = w / max(w.sum(), 1e-9)
    return nb, wt
def qtab(Cn, nb, wt, b=0.9):
    nw = Cn.sum(1); P = Cn / np.maximum(nw, 1)[:, None]; Q = np.zeros((V, V))
    for u in range(V):
        ok = nb[u] >= 0; ws = wt[u][ok].sum(); N = (wt[u][ok, None] * P[nb[u][ok]]).sum(0) / ws if ws > 0 else None
        if nw[u] > 0 and N is not None: Q[u] = (1 - b) * P[u] + b * N
        elif nw[u] > 0: Q[u] = P[u]
        elif N is not None: Q[u] = N
        else: Q[u] = uni
        Q[u] /= Q[u].sum()
    return Q
class Fit:
    """lamC: skaler ya da uzunluk-NC vektor (C blogu icin slot-basi lambda)."""
    def __init__(s, F, Fv, Fe, lamC=85.0, Ct_=None, Cv_=None, Ce_=None):
        s.Ct, s.Cv, s.Ce = (Ct if Ct_ is None else Ct_), (Cv if Cv_ is None else Cv_), (Ce if Ce_ is None else Ce_); nc = s.Ct.shape[1]
        s.F, s.Fv, s.Fe = F, Fv, Fe
        s.ACC = (s.Ct.T @ s.Ct).toarray(); s.ACF = np.asarray(s.Ct.T @ F); s.AFF = F.T @ F
        Y = sp.csr_matrix((np.ones(n), (np.arange(n), yt)), shape=(n, V)); s.RC = np.asarray((s.Ct.T @ Y).todense()); s.RF = np.zeros((512, V))
        for j in range(512): s.RF[j] = np.bincount(yt, weights=F[:, j], minlength=V)
        lam = np.full(nc, float(lamC)) if np.isscalar(lamC) else np.asarray(lamC, float)
        s.lam = lam; s.set_lam(lam)
    def set_lam(s, lam):
        A = s.ACC + np.diag(lam); s.cho = sl.cho_factor(A, lower=True)
        s.Y0 = sl.cho_solve(s.cho, s.RC); s.X = sl.cho_solve(s.cho, s.ACF)
        s.S = s.AFF - s.ACF.T @ s.X; s.rs = s.RF - s.ACF.T @ s.Y0
        s.CvY0 = np.asarray(s.Cv @ s.Y0); s.Fvr = s.Fv - np.asarray(s.Cv @ s.X); s.CeY0 = np.asarray(s.Ce @ s.Y0); s.Fer = s.Fe - np.asarray(s.Ce @ s.X)
    def WF(s, lamF): return sl.solve(s.S + lamF * np.eye(512), s.rs, assume_a='pos')
    def logits(s, W, split='val'):
        return (s.CvY0 + s.Fvr @ W) if split == 'val' else (s.CeY0 + s.Fer @ W)
