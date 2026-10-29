# V53 / Asama C: Q_havuz sifir-kutle duzeltmesi (frekans-bilincli geri cekilme). Yalniz son karisim (egitime girmez -> OOF/sizinti kaygisi yok; kappa yalniz dogrulamayla).
import numpy as np, model53 as M, sys
V=M.V; r=np.arange(M.nv)
F,Fv,Fe=M.load(); lamC=float(sys.argv[1]) if len(sys.argv)>1 else 85.0; lamF=float(sys.argv[2]) if len(sys.argv)>2 else 9000.0
m=M.Fit(F,Fv,Fe,lamC); W=m.WF(lamF); Zv=m.logits(W,'val'); ce0,a,e=M.read_ce(Zv,M.yv); Pv=M.read_P(Zv,a,e); print('model okuma val CE %.4f (a=%.2f e=%g)'%(ce0,a,e))
Cn=M.counts(M.w1t,M.yt); nb,wt=M.graph(Cn); Q=M.qtab(Cn,nb,wt); nw=Cn.sum(1)
def best(Qs,als=(0.1,0.15,0.2,0.25,0.3,0.35,0.4,0.5)): return min((M.ce_of((1-al)*Pv+al*Qs[M.w1v],M.yv),al) for al in als)
def zm(Qs): return (Qs[M.w1v][r,M.yv]==0).mean(), (Qs[M.w1v]==0).mean()
print('Q mevcut: hedefe sifir kutle (val satiri) %.3f ; Q satir-ici sifir sutun orani %.3f ; son karisim'%zm(Q),best(Q))
uni=M.uni; ncont=(Cn>0).sum(0).astype(float); pcont=(ncont+1)/(ncont+1).sum()      # Kneser-Ney tarzi devam eden unigram
P=Cn/np.maximum(nw,1)[:,None]; nbw=np.array([nw[nb[u][nb[u]>=0]].sum() for u in range(V)])  # komsularin toplam sayimi
out=[]
for tag,pi in (('unigram',uni),('KN-devam',pcont)):
    for kap in (1,3,10,30,100,300):
        lam=(nw+0.0)/(nw+kap)          # kendi sayima gore (kelime frekansi)
        Qs=lam[:,None]*Q+(1-lam)[:,None]*pi[None,:]; Qs/=Qs.sum(1,keepdims=True); c,al=best(Qs); out.append((c,'Dirichlet-kendi',tag,kap,al,zm(Qs)[0]))
        lam2=(nw+nbw)/(nw+nbw+kap)     # kendi + komsu sayimi
        Qs=lam2[:,None]*Q+(1-lam2)[:,None]*pi[None,:]; Qs/=Qs.sum(1,keepdims=True); c,al=best(Qs); out.append((c,'Dirichlet-kendi+komsu',tag,kap,al,zm(Qs)[0]))
    for d in (0.1,0.3,0.5,0.7,0.9):    # mutlak indirim: gorulen (w,y) icin d kadar azalt, kutleyi pi'ya ver
        Pd=np.maximum(Cn-d,0)/np.maximum(nw,1)[:,None]; left=1-Pd.sum(1); left[nw==0]=1
        Pw=Pd+left[:,None]*pi[None,:]; N=Q-(1-0.9)*P   # komsu bileseni
        Qs=0.1*Pw+N; Qs[nw==0]=Q[nw==0]; Qs/=Qs.sum(1,keepdims=True); c,al=best(Qs); out.append((c,'mutlak-indirim(kendi kismi)',tag,d,al,zm(Qs)[0]))
out.sort(key=lambda x:x[0])
for o in out[:12]: print('%.4f %-30s %-9s param=%-6s alpha=%.2f hedef-sifir=%.3f'%o)
print('en kotu: %.4f'%out[-1][0])
