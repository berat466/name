import re,json,statistics as st,sys
def rd(p):
    t=open(p+'.txt').read(); tm=open(p+'.time').read()
    g=lambda k:float(re.search(r'TH\|'+re.escape(k)+r'\|([0-9.]+)',t).group(1))
    w=float(re.search(r'WALL ([0-9.]+)',tm).group(1)); u=float(re.search(r'USER ([0-9.]+)',tm).group(1)); s=float(re.search(r'SYS ([0-9.]+)',tm).group(1)); r=int(re.search(r'MAXRSS_KB (\d+)',tm).group(1))
    hz=float(re.search(r'MODEL_HAZIR\|([0-9.]+)',t).group(1))
    val=g('v75 val asama1 (logit+lin_table)')+g('v75 val asama2 (logit+8 alfa birlesik)'); te=g('v75 test (logit+fuse)')
    ce=re.search(r'TEST CE=([0-9.]+) PPL=([0-9.]+) Top1=([0-9.]+) Top5=([0-9.]+)',t).groups(); vce=re.search(r'VAL CE=([0-9.]+)',t).group(1)
    return dict(wall=w,user=u,sys=s,rss=r/1024,model=hz,val=val,test=te,tce=ce[0],ppl=ce[1],t1=ce[2],t5=ce[3],vce=vce)
n=int(sys.argv[1]) if len(sys.argv)>1 else 5
A=[rd(f'ham/A76_{i}') for i in range(1,n+1)]; B=[rd(f'ham/B77_{i}') for i in range(1,n+1)]
out={}
for nm,L in (('V76',A),('V77',B)):
    out[nm]={k:dict(med=st.median([x[k] for x in L]),mn=min(x[k] for x in L),mx=max(x[k] for x in L)) for k in ('wall','user','sys','rss','model','val','test')}
    out[nm]['CE']=sorted(set((x['vce'],x['tce'],x['ppl'],x['t1'],x['t5']) for x in L))
out['cift_farki_wall']=[a['wall']-b['wall'] for a,b in zip(A,B)]
out['cift_orani_wall']=[b['wall']/a['wall'] for a,b in zip(A,B)]
json.dump(out,open('ab_ozet.json','w'),indent=1)
print(json.dumps(out,indent=1))
