#!/usr/bin/env python3
# Plan Bolum 01 (endianness / temsil): bayt sirasi cevirme komutlari bswap16/32/64 (okunabilir gorunum, stdin -> stdout). Yeni mikro-islem YOK: kaydirma/maske ile bayt bayt.
#   Sonuc isaretsiz (sifir genisletilmis). Ana makine kucuk-sonlu oldugundan bswapN = ana makine <-> buyuk-sonlu temsil donusumudur (TYPES bolumu: endian alani).
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' BSWAP16 ' in l for l in L): sys.exit('tur4_ekle: tablo zaten genisletilmis')
op_last = max(i for i, l in enumerate(L) if l.startswith('op '))
oid = int(L[op_last].split()[1])
newop, newsem = [], []
def op(name, fmt, sem):
    global oid
    oid += 1; newop.append('op %d %s %s %s' % (oid, name.upper(), name, fmt)); newsem.append('sem %s %s' % (name.upper(), sem))
for w in (16, 32, 64):
    n = w // 8; s = ['rd t1 b', 'lit t4 0']
    for k in range(n):
        s += ['lit t2 %d' % (8 * k), 'alu_shr t3 t1 t2', 'lit t2 255', 'alu_and t3 t3 t2', 'lit t2 %d' % (8 * (n - 1 - k)), 'alu_shl t3 t3 t2', 'alu_or t4 t4 t3']
    s += ['wr a t4']
    op('bswap%d' % w, 'd,r', ' ; '.join(s))
sem_last = max(i for i, l in enumerate(L) if l.startswith('sem '))
out = L[:op_last + 1] + newop + L[op_last + 1:sem_last + 1] + newsem + L[sem_last + 1:]
print('\n'.join(out), end='')
