#!/usr/bin/env python3
# Plan Bolum 01 (program duzeyi tur sistemi): CAST d,r = tur degistirme (yeniden yorumlama) komutu. Anlami MOV ile ayni (deger kopyalanir); fark TUR denetimindedir:
# CAST yalniz TYPES bolumunde kayitli ve komut kimligi -1 (ozdes) olan donusumlerde (kaynak tur -> hedef tur) gecerlidir. Gorunum (stdin) -> gorunum (stdout).
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' CAST ' in l for l in L): sys.exit('tur5_ekle: tablo zaten genisletilmis')
op_last = max(i for i, l in enumerate(L) if l.startswith('op '))
oid = int(L[op_last].split()[1]) + 1
sem_last = max(i for i, l in enumerate(L) if l.startswith('sem '))
out = L[:op_last + 1] + ['op %d CAST cast d,r' % oid] + L[op_last + 1:sem_last + 1] + ['sem CAST rd t1 b ; wr a t1'] + L[sem_last + 1:]
print('\n'.join(out), end='')
