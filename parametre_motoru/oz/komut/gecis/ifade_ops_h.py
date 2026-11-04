#!/usr/bin/env python3
"""Komut tablosunun (okunabilir gorunum, stdin) bolum 11 IFADE OPERATOR TABLOSU'ndan src/ifade_ops_gen.h uretir (stdout). Elle yazilmaz."""
import sys
sec = None; s12 = None
for l in sys.stdin.read().split('\n'):
    if l.startswith('section 11 '): sec = list(map(int, l.split()[4:]))
    if l.startswith('section 12 '): s12 = list(map(int, l.split()[4:]))
if sec is None: sys.exit('bolum 11 yok')
if s12 is None: sys.exit('bolum 12 yok')
n = sec[0]; p = 1; rows = []
for _ in range(n):
    ln = sec[p]; s, pr, a, tl = sec[p + 1:p + 5]; tok = ''.join(chr(c) for c in sec[p + 5:p + 5 + tl])
    assert ln == 4 + tl; rows.append((s, pr, a, tok)); p += 1 + ln
assert p == len(sec)
print('// OTOMATIK URETILDI: oz/komut/gecis/ifade_ops_h.py <- komutlar.pmk bolum 11 (IFADE OPERATOR TABLOSU). Elle degistirmeyin.')
print('#define PM_IFADE_OPS(X) \\')
print(' \\\n'.join('  X(%d, %d, %d, "%s")' % r for r in rows))
# bolum 12 sinif 6 (YASAM DONGUSU parametreleri): front-end onsozu (korutin blok ayirici) ayni sayilari kullanir
life = None; q = 1
for _ in range(s12[0]):
    cls = s12[q]; ns = s12[q + 1]; q += 2
    for _ in range(ns): q += 1 + s12[q]
    nn = s12[q]; nums = s12[q + 1:q + 1 + nn]; q += 1 + nn
    if cls == 6: life = nums
if life is None: sys.exit('bolum 12 sinif 6 (YASAM DONGUSU) yok')
print('// komutlar.pmk bolum 12 sinif 6 (YASAM DONGUSU)')
print('static const long long kLife[] = {' + ', '.join(str(x) + 'LL' for x in life) + '};')
