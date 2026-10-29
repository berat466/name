import itertools, numpy as np, b_countaware as B
ni=B.ni; out=[]
for lam0,al,kap in itertools.product((60,85,120,170),(-0.25,-0.1,0.0,0.05,0.1),(100,)):
    lam=lam0*(1+ni/kap)**(-al)
    ce,(fce,fal)=B.metric(lam,9000); out.append((fce,lam0,al,kap,ce)); print('lam0=%d alpha=%.2f kappa=%d | okuma %.4f | son %.4f'%(lam0,al,kap,ce,fce),flush=True)
out.sort(); print('EN IYI',out[:3])
lam=85*(1+ni/100)**(-0.25); print('df sabit',B.df(np.full(B.M.NC,85.0))); print('df alpha=.25',B.df(lam))
