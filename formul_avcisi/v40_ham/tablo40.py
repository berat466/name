import re,glob
for f in sorted(glob.glob('f*.txt')):
    t=open(f).read()
    m=re.search(r'SONUC\|ORTAK_KIMLIK\+\dF\(secilen\)\|lamC=(\S+) lamF=(\S+)\|alpha.*?dogr CE=([\d.]+) PPL=([\d.]+)\|test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t)
    h=re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t); sv=re.search(r'V39_HAVUZ.*?sure=([\d.]+)s',t)
    sc=re.findall(r'SURE\|lamC=(\S+) Cholesky\(C blogu\)\+Schur hazirlik=([\d.]+)s',t)
    if m: print(f[:-4],'lamC=%s lamF=%s valCE=%s valPPL=%s testPPL=%s top1=%s top5=%s | F+gram hazirlik=%ss havuz=%s | chol+schur(lamC basina)=%s'%(m.group(1),m.group(2),m.group(3),m.group(4),m.group(6),m.group(7),m.group(8),h.group(1) if h else '-',sv.group(1) if sv else '-',sc))
    else: print(f,'NO RESULT')
