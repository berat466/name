import re,glob
rows=[]
for f in sorted(glob.glob('[ABCD]_*.txt')):
    t=open(f).read()
    m=re.search(r'SONUC\|ORTAK_KIMLIK\+4F\(secilen\)\|lamC=(\S+) lamF=(\S+)\|alpha.*?dogr CE=([\d.]+) PPL=([\d.]+)\|test CE=([\d.]+) PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t)
    h=re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t)
    pr=re.findall(r'RANK\|F\d\|.*?efektif_rank\(PR\)=([\d.]+)',t)
    oof=re.search(r'V36_OOF.*?sure=([\d.]+)s',t)
    if m: print(f[:-4],'lamC=%s lamF=%s valCE=%s valPPL=%s testPPL=%s top1=%s top5=%s | hazirlik=%ss oof=%s | PR=%s'%(m.group(1),m.group(2),m.group(3),m.group(4),m.group(6),m.group(7),m.group(8),h.group(1) if h else '-',oof.group(1) if oof else '-','/'.join(pr)))
    else: print(f,'NO RESULT')
