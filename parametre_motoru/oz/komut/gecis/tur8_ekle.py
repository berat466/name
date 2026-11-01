#!/usr/bin/env python3
# 128 bit yardimcilari: hi64_<t> d,q (ust sozcuk), pack_<t> Q,r,r (dusuk, yuksek sozcuklerden olustur). Gorunum -> gorunum.
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' HI64_I128 ' in l for l in L): sys.exit('tur8_ekle: tablo zaten genisletilmis')
ol = max(i for i, l in enumerate(L) if l.startswith('op ')); oid = int(L[ol].split()[1])
sl = max(i for i, l in enumerate(L) if l.startswith('sem '))
nop, nsem = [], []
for ty in ('i128', 'u128'):
    oid += 1; nop.append('op %d HI64_%s hi64_%s d,q' % (oid, ty.upper(), ty)); nsem.append('sem HI64_%s rdw t1 b 1 ; wr a t1' % ty.upper())
    oid += 1; nop.append('op %d PACK_%s pack_%s Q,r,r' % (oid, ty.upper(), ty)); nsem.append('sem PACK_%s rd t1 b ; rd t2 c ; wrw a 0 t1 ; wrw a 1 t2' % ty.upper())
ins = {ol: nop, sl: nsem}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
