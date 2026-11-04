#!/usr/bin/env python3
"""Plan Bolum 02 (ifadeler/operatorler): komut tablosuna (PMK1 okunabilir gorunum, stdin -> stdout) bolum 11 = IFADE OPERATOR TABLOSU ekler (varsa yenisiyle degistirir).
Kayit: uzunluk(kalan jeton)  sinif  oncelik  birlesim  toklen  tok...   ;  yuk: kayit_sayisi kayit...
sinif 1 ikili (oncelik 3..13, birlesim 0 sol), 2 atama (oncelik 2, birlesim 1 sag), 3 onek (oncelik 14), 4 sonek (oncelik 15), 5 uclu ?: (oncelik 2, sag), 6 virgul (oncelik 1).
Bolum bayrak bit0 = 0 (istege bagli): eski araclar olduklari gibi korur. Bu betik TEK DOGRULUK KAYNAGIdir; src/ifade_ops_gen.h tablodan uretilir (ifade_ops_h.py)."""
import sys
OPS = []
def add(sinif, prec, assoc, toks):
    for t in toks.split(): OPS.append((sinif, prec, assoc, t))
add(1, 3, 0, '||'); add(1, 4, 0, '&&'); add(1, 5, 0, '|'); add(1, 6, 0, '^'); add(1, 7, 0, '&'); add(1, 8, 0, '== !='); add(1, 9, 0, '< > <= >=')
add(1, 10, 0, '<=>'); add(1, 11, 0, '<< >>'); add(1, 12, 0, '+ -'); add(1, 13, 0, '* / %')
add(2, 2, 1, '= += -= *= /= %= &= |= ^= <<= >>=')
add(3, 14, 1, '+ - ! ~ * & ++ --')
add(4, 15, 0, '++ -- [ ( . ->')
add(5, 2, 1, '?:'); add(6, 1, 0, ',')
pay = [len(OPS)]
for (s, p, a, t) in OPS:
    pay += [4 + len(t), s, p, a, len(t)] + [ord(c) for c in t]
L = sys.stdin.read().rstrip('\n').split('\n')
L = [l for l in L if not l.startswith('section 11 ')]
L.append('section 11 0 1 ' + ' '.join(map(str, pay)))
print('\n'.join(L))
