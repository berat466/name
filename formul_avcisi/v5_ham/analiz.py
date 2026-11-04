import glob,collections,statistics as st,struct,math,sys
def med(x): return st.median(x)
D=collections.defaultdict(list)   # (H,mode,b,t)->list of dict per rep
GR=collections.defaultdict(dict)  # (H,mode,b,rep,t,i)->mse
OZ={}
for f in sorted(glob.glob('/tmp/v5log/*_rep*.txt')):
    rep=int(f.split('rep')[1].split('.')[0])
    for l in open(f):
        p=l.strip().split('|')
        if p[0]=='ADIM':
            H=int(p[6]);t=int(p[7]);key=(H,p[1],int(p[2]),t)
            D[key].append(dict(to=float(p[9]),tn=float(p[10]),ts=float(p[11]),ao=float(p[12]),an=float(p[13]),as_=float(p[14]),tf=float(p[15]),tu=float(p[16]),te=float(p[17])))
        elif p[0]=='GOREV':
            GR[(None,p[1],int(p[2]),rep,int(p[4]),int(p[5]))]=float(p[6]) if False else float(p[6])
        elif p[0]=='OZET_ISLEM':
            OZ.setdefault((int(p[3].split('=')[1]),p[1],int(p[2].split('=')[1])),[]).append((float(p[4].split('=')[1]),float(p[5].split('=')[1]),p[6].split('=')[1]))
# GOREV keyed with H: reparse properly
G=collections.defaultdict(dict)
for f in sorted(glob.glob('/tmp/v5log/*_rep*.txt')):
    rep=int(f.split('rep')[1].split('.')[0]); H=int(f.split('_H')[1].split('_')[0])
    for l in open(f):
        p=l.strip().split('|')
        if p[0]=='GOREV': G[(H,p[1],int(p[2]),rep)][(int(p[4]),int(p[5]))]=float(p[6])
def forget(H,mode,b,rep,t):
    g=G[(H,mode,b,rep)]; return sum(g[(t,i)]-g[(i,i)] for i in range(1,t))/(t-1)
def rdw(H,mode,b,t):
    def rd(tag):
        raw=open(f'/tmp/v5w/{tag}_t{t}.bin','rb').read(); return struct.unpack(f'{len(raw)//8}d',raw)
    a=rd(f'{mode}_b{b}_H{H}_s1'); r=rd(f'kesin_b0_H{H}_s1')
    num=math.sqrt(sum((x-y)**2 for x,y in zip(a,r))); den=math.sqrt(sum(y*y for y in r)); mx=max(abs(x-y) for x,y in zip(a,r))
    return num/den,mx
man={}
for l in open('/tmp/v5log/manifest.tsv'):
    p=l.rstrip('\n').split('\t'); cmd=p[0].split(); H=int(cmd[2]); mode=cmd[1]; b=int(cmd[6]) if mode=='rls' else 0
    man.setdefault((H,mode,b),[]).append((float(p[3].split('=')[1]),float(p[6].split('=')[1])))
cfgs=[('kesin',0),('bastan',0)]+[('rls',b) for b in (1,16,64,256,1024)]
for H in (256,512):
    print('=== H',H)
    for mode,b in cfgs:
        rows=[]
        for t in (1,2,3):
            L=D[(H,mode,b,t)]
            tf=med([x['tf'] for x in L]); tu=med([x['tu'] for x in L]); te=med([x['te'] for x in L])
            rows.append((tf,tu,te))
        ttot=sum(sum(r) for r in rows); tupd=sum(r[1]+r[2] for r in rows)
        L3=D[(H,mode,b,3)]
        fg=med([forget(H,mode,b,rep,3) for rep in (1,2,3)])
        dw=rdw(H,mode,b,3) if not (mode=='kesin') else (0,0)
        wall=med([w for w,_ in man[(H,mode,b)]]); ram=max(r for _,r in man[(H,mode,b)])
        print(f"{mode:7} b={b:4} n={len(man[(H,mode,b)])} t3: test_old={L3[0]['to']:.5f} new={L3[0]['tn']:.5f} seen={L3[0]['ts']:.5f} acc_seen={L3[0]['as_']*100:.2f} forget={fg:.5f} | feat_total={sum(r[0] for r in rows):.3f} update_total(upd+solve)={tupd:.3f} per-task upd+solve=[{', '.join(f'{r[1]+r[2]:.3f}' for r in rows)}] | dW_rel={dw[0]:.2e} dW_max={dw[1]:.2e} | wall={wall:.2f}s ram={ram:.0f}MB")
