import re,sys
def info(f):
    t=open(f).read(); d={}
    for m in re.finditer(r'V50_SECIM\|([^|]*)\|rho=(\S+)\|alpha=(\S+) \(dogrulamayla\)\|okuma alpha=(\S+) eps=\S+\|dogr CE=([\d.]+) PPL=[\d.]+ Top1=([\d.]+) Top5=([\d.]+)\|TEST CE=[\d.]+ PPL=([\d.]+) Top1=([\d.]+) Top5=([\d.]+)\|\(alpha=0 test PPL=([\d.]+)\)\|agirlik cozme\(rho\)=([\d.]+)s ; logit\(val\+test\)=([\d.]+)s ; birlestirme\+PPL\(val\+test\)=([\d.]+)s',t):
        k=m.group(1).split()[0]+('s' if 'KARISIK' in m.group(1) else ''); d[k]=dict(rho=m.group(2),al=m.group(3),val=m.group(5),test=m.group(8),t1=m.group(9),t5=m.group(10),a0=m.group(11),tsol=m.group(12),tlog=m.group(13),tfu=m.group(14))
    for m in re.finditer(r'V50_SABIT\|([^|]*)\|alpha=0.2 \(sabit\)\|dogr CE=([\d.]+)\|TEST PPL=([\d.]+) Top1=([\d.]+) Top5=([\d.]+)',t):
        k=m.group(1).split()[0]+('s' if 'KARISIK' in m.group(1) else ''); d.setdefault(k,{})['sab']=(m.group(2),m.group(3))
    a0={}
    for m in re.finditer(r'V50_ALFA\|([^|]*)\|alpha=0\|dogr CE=([\d.]+)',t): a0[m.group(1).split()[0]+('s' if 'KARISIK' in m.group(1) else '')]=m.group(2)
    kd=re.search(r'V51_CEKIRDEK\|[^|]*sure=([\d.]+)s',t); hz=re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t); rs=re.search(r'V50_SURE\|[^|]*tepe RSS=([\d.]+)',t)
    return d,a0,(kd.group(1) if kd else '-'),(hz.group(1) if hz else '-'),(rs.group(1) if rs else '-')
if __name__=='__main__':
    for f in sys.argv[1:]:
        d,a0,kd,hz,rs=info(f); print('==',f,'cekirdek=%ss hazirlik=%ss RSS=%sMB'%(kd,hz,rs))
        for k in ['B','Bs','C','Cs','A']:
            if k in d: x=d[k]; print('  %-2s rho=%-5s a=%-4s val=%s test=%s T1=%s T5=%s | a=0 val=%s | sabit a=0.2: %s | t_coz=%s t_log=%s t_fuse=%s'%(k,x['rho'],x['al'],x['val'],x['test'],x['t1'],x['t5'],a0.get(k,'-'),x.get('sab','-'),x['tsol'],x['tlog'],x['tfu']))
