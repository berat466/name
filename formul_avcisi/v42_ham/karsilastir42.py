import re,glob,numpy as np,os
def info(f):
    t=open(f).read()
    m=re.search(r'SONUC\|ORTAK_KIMLIK\+4F\(secilen\)\|lamC=(\S+) lamF=(\S+)\|alpha.*?dogr CE=([\d.]+) PPL=([\d.]+)\|test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t)
    r=re.search(r'COZUM_REZIDUSU\|\|\|[^=]*= ([\d.e+-]+)',t); g=re.search(r'V42_REZIDU[^=]*= ([\d.e+-]+)',t)
    h=float(re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t).group(1)); c=float(re.search(r'SURE\|lamC=\S+ Cholesky\(C blogu\)\+Schur hazirlik=([\d.]+)s',t).group(1)); m6=float(re.search(r'TT\|M6[^|]*\|([\d.]+)s',t).group(1))
    return dict(valCE=m.group(3),testPPL=m.group(6),top1=m.group(7),top5=m.group(8),res=r.group(1) if r else '-',grp=g.group(1) if g else '-',t=(h,c,m6))
print('seed lamC lamF | W_C maks.fark  bagil.Frob | W_F maks.fark bagil.Frob | tam-sistem rezidu dense/grp | grup rezidu | val CE (d/g) | test PPL (d/g) | Top1 | Top5')
for f in sorted(glob.glob('log_s*_dense.txt')):
    tag=f[4:-10]; s,c,l=re.match(r's(\d)_c(\d+)_f(\d+)',tag).groups()
    if not os.path.exists(f'log_{tag}_grp.txt'): continue
    d=info(f); g=info(f'log_{tag}_grp.txt'); out=[]
    for k in ['WC','WF']:
        a=np.fromfile(f'/tmp/d42_{tag}_dense_{k}.bin',dtype=np.float32); b=np.fromfile(f'/tmp/d42_{tag}_grp_{k}.bin',dtype=np.float32)
        out.append('%.2e %.2e'%(np.abs(a-b).max(),np.linalg.norm(a-b)/np.linalg.norm(a)))
    print(s,c,l,'|',out[0],'|',out[1],'|',d['res'],'/',g['res'],'|',g['grp'],'|',d['valCE'],'/',g['valCE'],'|',d['testPPL'],'/',g['testPPL'],'|',d['top1'],'/',g['top1'],'|',d['top5'],'/',g['top5'])
print()
import statistics as st
for m in ['dense','grp']:
    T=[]
    for r in (1,2,3):
        f=f'time_{m}_r{r}.txt'
        if os.path.exists(f): x=info(f)['t']; T.append(x+(sum(x),))
    if T: print(m,'tekrarlar [F+havuz+Gram, Chol+Y0+X+Schur, lamF cozum, TOPLAM]:',[tuple(round(v,2) for v in x) for x in T],'toplam medyan=%.2f min=%.2f'%(st.median(x[3] for x in T),min(x[3] for x in T)))
