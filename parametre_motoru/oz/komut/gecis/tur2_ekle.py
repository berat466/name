#!/usr/bin/env python3
# Plan Bolum 01 (bool + nullptr_t): komut tablosuna (okunabilir gorunum, stdin -> stdout) MANTIKSAL komutlar ekler. Yeni mikro-islem YOK (cmp_ne/cmp_eq/alu_*/lit yeterli).
#   tobool d,r : r != 0 -> 0/1 ; bnot d,r : r == 0 -> 0/1 ; band/bor/bxor d,r,r : her iki girdi once bool'a indirgenir ; nullb d,r : nullptr_t -> bool (her zaman false)
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' TOBOOL ' in l for l in L): sys.exit('tur2_ekle: tablo zaten genisletilmis')
op_last = max(i for i, l in enumerate(L) if l.startswith('op '))
oid = int(L[op_last].split()[1])
newop, newsem = [], []
def op(name, fmt, sem):
    global oid
    oid += 1; newop.append('op %d %s %s %s' % (oid, name.upper(), name, fmt)); newsem.append('sem %s %s' % (name.upper(), sem))
op('tobool', 'd,r', 'rd t1 b ; lit t2 0 ; cmp_ne t3 t1 t2 ; wr a t3')
op('bnot', 'd,r', 'rd t1 b ; lit t2 0 ; cmp_eq t3 t1 t2 ; wr a t3')
for n, p in (('band', 'alu_and'), ('bor', 'alu_or'), ('bxor', 'alu_xor')):
    op(n, 'd,r,r', 'rd t1 b ; rd t2 c ; lit t3 0 ; cmp_ne t4 t1 t3 ; cmp_ne t3 t2 t3 ; %s t4 t4 t3 ; wr a t4' % p)
op('nullb', 'd,r', 'rd t1 b ; lit t2 0 ; wr a t2')
sem_last = max(i for i, l in enumerate(L) if l.startswith('sem '))
out = L[:op_last + 1] + newop + L[op_last + 1:sem_last + 1] + newsem + L[sem_last + 1:]
print('\n'.join(out), end='')
