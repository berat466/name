import re
for f,name in [('A_kontrol','A kontrol'),('B0_olcek2','2A olcek (s=2)'),('B_kuvvet','B kuvvet A,A2,A3'),('C_etkilesim','C etkilesim A,A2,AB')]:
    t=open(f+'.txt').read()
    d=re.search(r'V49_DONUSUM\|([^|]*)\|([^|]*)\|([^|]*)\|([^\n]*)',t)
    lfs=[float(x) for x in re.findall(r'ORTAK_TARAMA\|lamC=85 lamF=(\S+)\|',t)]
    # ORTAK_TARAMA (rho=0) val CE per lamF
    tar={}
    for m in re.finditer(r'ORTAK_TARAMA\|lamC=85 lamF=(\S+)\|dogr CE=([\d.]+)',t): tar[float(m.group(1))]=float(m.group(2))
    sec=re.findall(r'V46_SECIM\|mod=0[^|]*\|rho=(\S+) \(dogrulamayla\)\|dogr CE=([\d.]+) PPL=([\d.]+)\|test CE=[\d.]+ PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)\|sinyal sure=([\d.]+)s',t)
    hz=re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t); ch=re.search(r'Schur hazirlik=([\d.]+)s',t)
    print('==',name, '| donusum:',(d.group(4) if d else '-')[:80], '| hazirlik=%s chol/schur=%s'%(hz.group(1) if hz else '-',ch.group(1) if ch else '-'))
    print('   rho=0 (V44) val CE by lamF:',{k:tar[k] for k in sorted(tar)})
    for lf,s_ in zip(sorted(tar),sec): print('   lamF=%g  V46 sinyal rho=%s val CE=%s test PPL=%s top1=%s top5=%s sinyal_sure=%s'%(lf,s_[0],s_[1],s_[3],s_[4],s_[5],s_[6]))
