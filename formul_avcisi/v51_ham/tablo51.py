import re,glob,sys
def row(f):
    t=open(f).read(); out={}
    for m in re.finditer(r'V50_SECIM\|([^|]*)\|rho=(\S+)\|alpha=(\S+) \(dogrulamayla\)\|[^|]*\|dogr CE=([\d.]+) PPL=[\d.]+ Top1=([\d.]+) Top5=([\d.]+)\|TEST CE=[\d.]+ PPL=([\d.]+) Top1=([\d.]+) Top5=([\d.]+)\|\(alpha=0 test PPL=([\d.]+)\)\|agirlik cozme\(rho\)=([\d.]+)s',t):
        key=m.group(1)[:1]+('s' if 'KARISIK' in m.group(1) else '')
        out[key]=dict(rho=m.group(2),alpha=m.group(3),val=m.group(4),test=m.group(7),t1=m.group(8),t5=m.group(9),a0=m.group(10))
    k=re.search(r'V51_CEKIRDEK\|[^|]*sure=([\d.]+)s',t); h=re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t)
    return out,(k.group(1) if k else '-'),(h.group(1) if h else '-')
for f in sys.argv[1:]:
    o,k,h=row(f)
    print('==',f,'cekirdek_kurulum=%ss hazirlik(F+havuz+kuvvet+Gram)=%ss'%(k,h))
    for key in ['B','Bs','C','Cs','A']:
        if key in o: d=o[key]; print('   %-3s rho=%-5s alpha=%-4s val CE=%s test PPL=%s Top1=%s Top5=%s (alpha=0 test %s)'%(key,d['rho'],d['alpha'],d['val'],d['test'],d['t1'],d['t5'],d['a0']))
