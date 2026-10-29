import glob,collections,statistics as st,struct,math,sys,re
med=st.median
LOG=sys.argv[1] if len(sys.argv)>1 else '/tmp/v6log'; WD=sys.argv[2] if len(sys.argv)>2 else '/tmp/v6w'
D=collections.defaultdict(list); G=collections.defaultdict(dict); X=collections.defaultdict(dict)
for f in sorted(glob.glob(f'{LOG}/*_rep*.txt')):
    rep=int(f.split('rep')[1].split('.')[0])
    for l in open(f):
        p=l.strip().split('|')
        if p[0]=='ADIM':
            mode=p[1];H=int(p[5]);t=int(p[6])
            D[(H,mode,t)].append(dict(to=float(p[8]),tn=float(p[9]),ts=float(p[10]),ao=float(p[11]),an=float(p[12]),as_=float(p[13]),tf=float(p[14]),tst=float(p[15]),tfa=float(p[16]),tso=float(p[17]),tu=float(p[18])))
            X[(H,mode)][t]=p[19]
        elif p[0]=='GOREV': G[(int(p[3]),p[1],rep)][(int(p[4]),int(p[5]))]=float(p[6])
def forget(H,m,rep,t): g=G[(H,m,rep)]; return sum(g[(t,i)]-g[(i,i)] for i in range(1,t))/(t-1)
def rd(tag,t):
    raw=open(f'{WD}/{tag}_t{t}.bin','rb').read(); return struct.unpack(f'{len(raw)//8}d',raw)
def rel(a,r): return math.sqrt(sum((x-y)**2 for x,y in zip(a,r)))/math.sqrt(sum(y*y for y in r))
man={}
for l in open(f'{LOG}/manifest.tsv'):
    p=l.rstrip('\n').split('\t'); c=p[0].split()
    if c[1]=='mikro': continue
    key=(int(c[2]),c[1],int(c[7]),int(c[8])); man.setdefault(key,[]).append((float(p[3].split('=')[1]),float(p[6].split('=')[1])))
MODES=['v4f32','v4f64','v4f32c256','v6dw_ref','v6dw_pcg','v6chol']
if __name__=='__main__':
    for H in (256,512,1024):
        print(f'=== H={H}')
        for md in MODES:
            reps=sorted({int(f.split('rep')[1].split('.')[0]) for f in glob.glob(f'{LOG}/{md}_H{H}_rep*.txt')})
            pt=[]
            for t in range(1,6):
                L=D[(H,md,t)]; pt.append(dict(tf=med([x['tf'] for x in L]),tst=med([x['tst'] for x in L]),tfa=med([x['tfa'] for x in L]),tso=med([x['tso'] for x in L]),tu=med([x['tu'] for x in L])))
            L5=D[(H,md,5)][0]
            fg=med([forget(H,md,r,5) for r in reps])
            wd=[rel(rd(f'{md}_H{H}_s1',t),rd(f'v4f64_H{H}_s1',t)) for t in range(1,6)]
            tot_u=sum(p['tu'] for p in pt); tot_f=sum(p['tf'] for p in pt)
            wl=med([w for w,_ in man[(H,md,0,60)]]); ram=max(r for _,r in man[(H,md,0,60)])
            print(f"{md:10} reps={len(reps)} t5: old={L5['to']:.5f} new={L5['tn']:.5f} seen={L5['ts']:.5f} acc={L5['as_']*100:.2f} forget={fg:.5f} | upd/task=[{', '.join(f'{p['tu']:.4f}' for p in pt)}] stat/fac/sol(sum)={sum(p['tst'] for p in pt):.3f}/{sum(p['tfa'] for p in pt):.3f}/{sum(p['tso'] for p in pt):.3f} upd_total={tot_u:.3f} feat_total={tot_f:.3f} | dW_vs_ref64 t1..5=[{', '.join(f'{w:.1e}' for w in wd)}] | wall={wl:.2f}s ram={ram:.0f}MB | extra={X[(H,md)]}")
