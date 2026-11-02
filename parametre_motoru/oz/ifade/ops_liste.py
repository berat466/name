#!/usr/bin/env python3
"""komut tablosunun okunabilir gorunumunden (stdin) bolum 11 operator tablosunu 'sinif oncelik birlesim tok' satirlari olarak yazar (pm ifade --ops ile karsilastirilir)."""
import sys
sec = None
for l in sys.stdin.read().split('\n'):
    if l.startswith('section 11 '): sec = list(map(int, l.split()[4:]))
if sec is None: sys.exit('bolum 11 yok')
n = sec[0]; p = 1
for _ in range(n):
    ln = sec[p]; s, pr, a, tl = sec[p + 1:p + 5]; print(s, pr, a, ''.join(chr(c) for c in sec[p + 5:p + 5 + tl])); p += 1 + ln
