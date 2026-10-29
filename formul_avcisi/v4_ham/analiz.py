import glob,collections,sys
def mean(x): return sum(x)/len(x)
def load(dirn,g,l):
    A=collections.defaultdict(dict)   # (method,seed,t)->dict
    G=collections.defaultdict(dict)   # (method,seed,t,i)->(test_mse,train_mse,acc)
    for f in sorted(glob.glob(f'{dirn}/*_g{g}_l{l}_s*.txt')):
        for line in open(f):
            p=line.strip().split('|')
            if p[0]=='ADIM':
                A[(p[2],int(p[3]),int(p[6]))]=dict(m=int(p[7]),to=float(p[8]),tn=float(p[9]),ts=float(p[10]),ta=float(p[11]),ro=float(p[12]),rn=float(p[13]),rs=float(p[14]),ao=float(p[15]),an=float(p[16]),as_=float(p[17]),tnew=float(p[18]),tm=float(p[19]),te=float(p[20]),lamu=float(p[21]),fail=int(p[22]))
            elif p[0]=='GOREV':
                G[(p[2],int(p[3]),int(p[4]),int(p[5]))]=(float(p[6]),float(p[7]),float(p[8]))
    return A,G
METH=['SABIT_1024','SABIT_2048','BUYUME_SIFIR','BUYUME_DONUK','BUYUME_ORNEK20','BUYUME_ORNEK100','BUYUME_TAMPON20','BUYUME_TAMPON100','BUYUME_TAVAN','V3_1024','V3_2048']
def forget(G,meth,seed,t):
    if t<2: return None
    return mean([G[(meth,seed,t,i)][0]-G[(meth,seed,i,i)][0] for i in range(1,t)])
def table(dirn,g,l,t):
    A,G=load(dirn,g,l); rows=[]
    for me in METH:
        ss=[s for s in (1,2,3) if (me,s,t) in A]
        if not ss: continue
        f=lambda k: mean([A[(me,s,t)][k] for s in ss])
        fg=mean([forget(G,me,s,t) for s in ss]) if t>1 else float('nan')
        fl=sum(A[(me,s,t)]['fail'] for s in ss); lm=max(A[(me,s,t)]['lamu'] for s in ss)
        rows.append((me,f('m'),f('to'),f('tn'),f('ts'),f('ta'),f('ro'),f('rn'),f('rs'),fg,f('ao')*100,f('an')*100,f('as_')*100,f('tm'),lm,fl,len(ss)))
    return rows
if __name__=='__main__':
    d,g,l=sys.argv[1],sys.argv[2],sys.argv[3]
    for t in (2,3,4,5):
        print(f'--- t={t} g={g} l={l}')
        print('method m test_old test_new test_seen test_all | train_old train_new train_seen | forget acc_old acc_new acc_seen t_method lam_max fail n')
        for r in table(d,g,l,t): print(r[0],int(r[1]),' '.join(f'{x:.4f}' if isinstance(x,float) else str(x) for x in r[2:]))
