import time, numpy as np, model53 as M
t=time.time(); F,Fv,Fe=M.load(); m=M.Fit(F,Fv,Fe,85.0); W=m.WF(9000); Zv=m.logits(W,'val')
ce,a,e=M.read_ce(Zv,M.yv); print('model okuma val CE %.4f a=%.2f e=%g (C++ 4.6644)'%(ce,a,e))
Cn=M.counts(M.w1t,M.yt); nb,wt=M.graph(Cn); Q=M.qtab(Cn,nb,wt); Pv=M.read_P(Zv,0.9,1e-5)
for al in (0.1,0.2,0.3): print('son karisim alpha=%.1f val CE %.4f'%(al,M.ce_of((1-al)*Pv+al*Q[M.w1v],M.yv)),'(C++ alpha=0.2: 4.6334)' if al==0.2 else '')
print(round(time.time()-t,1),'s')
