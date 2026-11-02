#!/usr/bin/env python3
"""Test yardimcisi: PMK1 sayi akisinda (stdin) bolum 12'deki bir IKILI kuralin tam-sayi komutunu degistirir  (kural_degistir.py SIMGE ESKI YENI < komutlar.pmk > degisik.pmk).
Yalniz sayilari yeniden yazar; hicbir kaynak/ikili yeniden uretilmez -- ifx.pmb ayni kalir, davranis tablodan degisir."""
import sys
tok, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
v = sys.stdin.read().split(); assert len(old) == len(new)
pat = [2, 4, len(tok)] + [ord(c) for c in tok] + [len(old)] + [ord(c) for c in old]
nums = [int(x) for x in v]
n = 0
for i in range(len(nums) - len(pat)):
    if nums[i:i + len(pat)] == pat:
        nums[i + len(pat) - len(old):i + len(pat)] = [ord(c) for c in new]; n += 1
assert n == 1, 'kural bulunamadi (%d)' % n
print(' '.join(map(str, nums)))
