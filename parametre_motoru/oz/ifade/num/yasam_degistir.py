#!/usr/bin/env python3
"""Test yardimcisi: PMK1 sayi akisinda (stdin) bolum 12 sinif 6 (YASAM DONGUSU) kaydinin INDEKS. parametresini YENI yapar (yasam_degistir.py INDEKS YENI < komutlar.pmk > degisik.pmk).
Yalniz sayilari yeniden yazar; hicbir kaynak/ikili yeniden uretilmez -- ifx.pmb ayni kalir, yasam dongusu davranisi tablodan degisir."""
import sys
idx, new = int(sys.argv[1]), int(sys.argv[2])
nums = [int(x) for x in sys.stdin.read().split()]
pat = [6, 0, 20, 2, 4, 26]
hits = [i for i in range(len(nums) - len(pat)) if nums[i:i + len(pat)] == pat]
assert len(hits) == 1, 'yasam kaydi bulunamadi (%d)' % len(hits)
nums[hits[0] + 3 + idx] = new
print(' '.join(map(str, nums)))
