#!/usr/bin/env python3
"""Bolum 04 duzeltmesi (host RT cagrisi, DERINLIK): host_rt / host_rtr sablonlari RT cagrisi icin kullanici cagri derinligini (r13) KULLANMAMALI.
Referans pm'de host cagrisi cerceve/derinlik tuketmez; ELF'te RT islevi ve alt cagrilari derinligi artirdigi icin 65535. duzeyde putf gereksiz DEPTH tuzagi veriyordu (pm vermez).
DUZELTME: sablon r13'u kaydeder (push r13), sifirlar (xor r13,r13: RT kendi derinlik sayacini 0'dan baslatir), donuste geri yukler (pop r13). Derinlik denetimi (cmp/jae/inc/dec) sablondan kalkar.
okunabilir gorunum stdin -> stdout."""
import sys
L = sys.stdin.read().split('\n')
A = ' 49 81 FD 00 00 01 00 0F 83 REL_DEPTH 49 FF C5'
B = ' 41 55 4D 31 ED'
C = ' 49 FF CD 48 8B 03'
D = ' 41 5D 48 8B 03'
n = 0
for i, l in enumerate(L):
    if l.startswith('tpl sp ') and (' host_rt :' in l or ' host_rtr :' in l):
        if A not in l or C not in l: sys.exit('host_rt_derinlik: sablon beklenen bicimde degil (zaten uygulanmis?)')
        L[i] = l.replace(A, B).replace(C, D); n += 1
if n != 2: sys.exit('host_rt_derinlik: host_rt/host_rtr sablonlari bulunamadi')
sys.stdout.write('\n'.join(L))
