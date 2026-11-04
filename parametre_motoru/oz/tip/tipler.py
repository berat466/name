#!/usr/bin/env python3
"""Test yardimcisi (gelistirme araci; motor bagimliligi DEGIL): okunabilir tur bildirimini PMT1 sayilarina cevirir.
  tipler.py TUR_DOKUMU(komut @@tur ciktisi) [--pad LISTE IMZA] < bildirim   -> PMT1 sayilari (stdout)
bildirim satiri:  islev_no donus : tur tur tur ...   (donus: ad ya da '-' ; tur adlari TYPES adlari, 'dyn' = 254, '+' = DEVAM 255)
--pad: bildirilen yazmac sayisi programdakinden azsa kalan yazmaclar i32 (kullanilmayan yazmac) olarak tamamlanir
ornek:            0 i64 : i32 i32 i64 dyn"""
import sys
ids = {}
for l in open(sys.argv[1]):
    t = l.split()
    if t and t[0] == 'tur': ids[t[2]] = int(t[1])
ids['dyn'] = 254; ids['+'] = 255; ids['-'] = 0
nregs = None
if len(sys.argv) > 4 and sys.argv[2] == '--pad':
    lst = [int(x) for x in open(sys.argv[3]).read().split()]
    im = [int(x) for x in open(sys.argv[4]).read().split()]
    nk = {}
    nops = im[0]
    for o in range(nops): nk[im[1 + 11 * o]] = im[1 + 11 * o + 1]
    p = 3; nc = lst[p]; p += 1 + nc; nd = lst[p]; p += 1 + nd; p += 1; nf = lst[p]; p += 1
    nregs = []
    for f in range(nf):
        npar, nr, ncode = lst[p], lst[p + 1], lst[p + 2]; p += 3
        nregs.append(nr)
        for _ in range(ncode): p += 1 + nk[lst[p]]
ents = []
for l in sys.stdin:
    l = l.split('#')[0].strip()
    if not l: continue
    h, regs = l.split(':'); f, rt = h.split()
    rr = [ids[x] if x in ids else int(x) for x in regs.split()]
    if nregs is not None and int(f) < len(nregs):
        while len(rr) < nregs[int(f)]: rr.append(ids['i32'])
    ents.append([int(f), ids[rt], len(rr)] + rr)
out = [1347441713, 1, 0, len(ents)]
for e in ents: out += e
print(' '.join(map(str, out)))
