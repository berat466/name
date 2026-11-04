#!/usr/bin/env python3
"""Plan Bolum 01 (karmasik sayilar c64 c128 c160): komutlar = OP + SEM (mevcut mikro-islemler). Mikro-programlar ../../kayan/{cx,ops,sf}.py ureteclerinden gelir.
Anlam: ISO C CX_LIMITED_RANGE (bilesen bilesen duz formuller). Girdi: okunabilir gorunum (stdin) -> stdout ; ayrica src/anlam_wcx_gen.h (C++ referans makrolari).
Komutlar (C = c64 c128 c160): add_C sub_C mul_C div_C (Q,q,q | c160: O,o,o)  neg_C conj_C mov_C (Q,q | O,o)  eq_C ne_C (d,q,q | d,o,o)  tobool_C (d,q | d,o)  mk_C (Q,f,f | c160: O,q,q)
  real_C imag_C (d,q | c160: Q,o)  cv_C_D (karmasik donusumleri)  cv_f64_C (Q,f | c160: O,f)  cv_f80_C (Q,q | c160: O,q)"""
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'kayan'))
import sf, ops, cx
L = sys.stdin.read().split('\n')
GUNCELLE = '--guncelle' in sys.argv
if not GUNCELLE and any(l.startswith('op ') and ' ADD_C64 ' in l for l in L): sys.exit('tur12_ekle: tablo zaten genisletilmis (--guncelle: mevcut komutlarin SEM kayitlarini yeniden uret)')
ol = max(i for i, l in enumerate(L) if l.startswith('op ')); oid = int(L[ol].split()[1])
sl = max(i for i, l in enumerate(L) if l.startswith('sem '))
for l in L:
    if l.startswith('sem RND_F32 '): cx.RND32 = l.split(' ', 2)[2].split(' ; ')
assert cx.RND32
nop, nsem, cxx = [], [], []
def op(name, sig, g, w):
    global oid
    oid += 1; nop.append('op %d %s %s %s' % (oid, name.upper(), name, sig)); nsem.append('sem %s %s' % (name.upper(), g.text())); cxx.append((name.upper(), w))
CODE = {'add': 'WCX_ADD', 'sub': 'WCX_SUB', 'mul': 'WCX_MUL', 'div': 'WCX_DIV', 'neg': 'WCX_NEG', 'conj': 'WCX_CONJ', 'mov': 'WCX_MOV', 'eq': 'WCX_EQ', 'ne': 'WCX_NE', 'tobool': 'WCX_TOBOOL', 'mk': 'WCX_MK', 'real': 'WCX_REAL', 'imag': 'WCX_IMAG'}
for ti, C in enumerate(('c64', 'c128', 'c160')):
    big = C == 'c160'; Q, q = ('O', 'o') if big else ('Q', 'q')
    for k in ('add', 'sub', 'mul', 'div'): op('%s_%s' % (k, C), '%s,%s,%s' % (Q, q, q), cx.build(C, k), (CODE[k], ti, 0))
    for k in ('neg', 'conj', 'mov'): op('%s_%s' % (k, C), '%s,%s' % (Q, q), cx.build(C, k), (CODE[k], ti, 0))
    for k in ('eq', 'ne'): op('%s_%s' % (k, C), 'd,%s,%s' % (q, q), cx.build(C, k), (CODE[k], ti, 0))
    op('tobool_' + C, 'd,' + q, cx.build(C, 'tobool'), (CODE['tobool'], ti, 0))
    op('mk_' + C, 'O,q,q' if big else 'Q,f,f', cx.build(C, 'mk'), (CODE['mk'], ti, 0))
    for k in ('real', 'imag'): op('%s_%s' % (k, C), 'Q,o' if big else 'd,q', cx.build(C, k), (CODE[k], ti, 0))
names = ('c64', 'c128', 'c160')
for si, s in enumerate(names):
    for di, d in enumerate(names):
        if si == di: continue
        op('cv_%s_%s' % (s, d), '%s,%s' % ('O' if d == 'c160' else 'Q', 'o' if s == 'c160' else 'q'), cx.conv(s, d, cx.RND32), ('WCX_CVT', si, di))
for di, d in enumerate(names):
    op('cv_f64_' + d, '%s,f' % ('O' if d == 'c160' else 'Q'), cx.from_real('f64', d, cx.RND32), ('WCX_FROM_REAL', di, 0))
    op('cv_f80_' + d, '%s,q' % ('O' if d == 'c160' else 'Q'), cx.from_real('f80', d, cx.RND32), ('WCX_FROM_REAL', di, 1))
hdr = ['// KARMASIK SAYI komutlarinin C++ anlam makrolari -- oz/komut/gecis/tur12_ekle.py ile uretilir (tablodaki OP adlariyla bire bir).']
for nm, w in cxx: hdr.append('#define SEM_%s(M) M##_WCX(%s,%d,%d)' % (nm, w[0], w[1], w[2]))
hdr.append('#define PM_SEM_OPS_WCX(X) \\\n  ' + ' '.join('X(%s)' % nm for nm, w in cxx))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'src', 'anlam_wcx_gen.h'), 'w').write('\n'.join(hdr) + '\n')
if GUNCELLE:   # mevcut komutlarin SEM satirlarini yeniden uretilenlerle degistir (OP/TYPES degismez)
    yeni = {l.split(' ', 2)[1]: l for l in nsem}; out = []
    for l in L:
        t = l.split(' ', 2)
        out.append(yeni[t[1]] if t[0] == 'sem' and len(t) > 1 and t[1] in yeni else l)
    print('\n'.join(out), end=''); sys.exit(0)
ins = {ol: nop, sl: nsem}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
