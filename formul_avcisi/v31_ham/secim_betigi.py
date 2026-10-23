import re,glob
best=None; bid=None
for p in glob.glob('/tmp/v31log/v31_sweep*_H512_s1.txt'):
    for l in open(p):
        m=re.match(r'ORTAK_TARAMA\|lamC=(\S+) lamF=(\S+)\|dogr CE=([\d.]+)',l)
        if m and float(m.group(2))<1e8:
            v=float(m.group(3))
            if best is None or v<best[0]: best=(v,m.group(1),m.group(2))
        m=re.match(r'YALNIZ_KIMLIK_TARAMA\|lamC=(\S+)\|dogr CE=([\d.]+)',l)
        if m:
            v=float(m.group(2))
            if bid is None or v<bid[0]: bid=(v,m.group(1))
print(best[1],best[2],bid[1],best[0],bid[0])
