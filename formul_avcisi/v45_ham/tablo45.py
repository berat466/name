import re,glob,sys
for f in sorted(glob.glob('[ABCD]_*.txt'))+sorted(glob.glob('E_*.txt')):
    t=open(f).read()
    m=re.search(r'SONUC\|ORTAK[^|]*\|lamC=(\S+) lamF=(\S+)\|.*?dogr CE=([\d.]+) PPL=([\d.]+)\|test CE=[\d.]+ PPL=([\d.]+) top1=([\d.]+) top5=([\d.]+)',t)
    h=re.search(r'V31_ZINCIR_OZELLIK\|[^|]*\|hazirlik=([\d.]+)s',t); c=re.search(r'Schur hazirlik=([\d.]+)s',t); l=re.search(r'TT\|M6[^|]*\|([\d.]+)s',t)
    d=re.search(r'V45_DAGILIM\|(beta_w: [^;]*)',t)
    tot=(float(h.group(1))+float(c.group(1))+float(l.group(1))) if h and c and l else 0
    if m: print('%-18s valCE=%s valPPL=%s testPPL=%s top1=%s top5=%s toplam_sure=%.2fs | %s'%(f[:-4],m.group(3),m.group(4),m.group(5),m.group(6),m.group(7),tot,d.group(1) if d else ''))
    else: print(f,'NO RESULT')
