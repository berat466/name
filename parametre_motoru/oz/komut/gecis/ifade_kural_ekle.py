#!/usr/bin/env python3
"""Plan Bolum 02 (duzeltme): komut tablosuna (PMK1 okunabilir gorunum, stdin -> stdout) bolum 12 = IFADE KURAL TABLOSU ekler/yeniler.
ifx.pmb (sayisal PMX1 derleyicisi) bu bolumu CALISMA ANINDA okur: tur ozellikleri, ikili/karsilastirma operatorlerinin komut eslemesi, donusum/normallestirme komut adlari.
Kayit: sinif nstr (uzunluk karakter...)*nstr nnum sayilar*nnum ;  yuk: kayit_sayisi kayit...
 sinif 1 TUR       dizgeler [ad, normallestirme_komutu]  sayilar [tur_kodu, bit, isaretli, sinif(0 tam,1 kayan), derece]
 sinif 2 IKILI     dizgeler [simge, tam_komut, isaretsiz64_komut, kayan_komut]  sayilar [tur(1 aritm,2 bol/mod,3 bit,4 kaydirma), isaretsiz_secimi(0 yok,1 yalniz u64,2 her isaretsiz), normallestir]
 sinif 3 KARSILASTIR dizgeler [simge, isaretli, isaretsiz, kayan]  sayilar [isaretsizde_takas, kayanda_takas, kayanda_tersle]
 sinif 4 AD        dizgeler [i2f u2f f2i f2u rnd_f32 neg fneg not feq + bos dolgu (16)]  sayilar []
Bolum bayrak bit0 = 0 (istege bagli)."""
import sys
def rec(cls, strs, nums):
    r = [cls, len(strs)]
    for s in strs: r += [len(s)] + [ord(c) for c in s]
    return r + [len(nums)] + nums
R = []
T = [(1, 'bool', '', 8, 0, 0, 1), (2, 'char', 'sext8', 8, 1, 0, 2), (3, 'schar', 'sext8', 8, 1, 0, 2), (4, 'uchar', 'zext8', 8, 0, 0, 2), (5, 'short', 'sext16', 16, 1, 0, 3), (6, 'ushort', 'zext16', 16, 0, 0, 3),
     (7, 'int', 'sext32', 32, 1, 0, 4), (8, 'uint', 'zext32', 32, 0, 0, 4), (9, 'long', '', 64, 1, 0, 5), (10, 'ulong', '', 64, 0, 0, 5), (11, 'llong', '', 64, 1, 0, 6), (12, 'ullong', '', 64, 0, 0, 6),
     (13, 'float', '', 32, 1, 1, 0), (14, 'double', '', 64, 1, 1, 0), (15, 'ldouble', '', 128, 1, 1, 0)]
for (c, n, nm, b, sg, cl, rk) in T: R.append(rec(1, [n, nm], [c, b, sg, cl, rk]))
B = [('+', 'add', '', 'fadd', 1, 0, 1), ('-', 'sub', '', 'fsub', 1, 0, 1), ('*', 'mul', '', 'fmul', 1, 0, 1), ('/', 'div', 'divu', 'fdiv', 2, 1, 1), ('%', 'mod', 'modu', '', 2, 1, 1),
     ('&', 'and', '', '', 3, 0, 0), ('|', 'or', '', '', 3, 0, 0), ('^', 'xor', '', '', 3, 0, 0), ('<<', 'shl', '', '', 4, 0, 1), ('>>', 'sar', 'shr', '', 4, 2, 0)]
for (t, i, u, f, k, w, nn) in B: R.append(rec(2, [t, i, u, f], [k, w, nn]))
C = [('==', 'eq', 'eq', 'feq', 0, 0, 0), ('!=', 'ne', 'ne', 'feq', 0, 0, 1), ('<', 'lt', 'ltu', 'flt', 0, 0, 0), ('>', 'gt', 'ltu', 'fgt', 1, 0, 0), ('<=', 'le', 'leu', 'fle', 0, 0, 0), ('>=', 'ge', 'leu', 'fge', 1, 0, 0)]
for (t, s, u, f, a, b, c) in C: R.append(rec(3, [t, s, u, f], [a, b, c]))
N = ['i2f', 'u2f', 'f2i', 'f2u', 'rnd_f32', 'neg', 'fneg', 'not', 'feq'] + [''] * 7
R.append(rec(4, N, []))
pay = [len(R)]
for r in R: pay += r
L = sys.stdin.read().rstrip('\n').split('\n')
L = [l for l in L if not l.startswith('section 12 ')]
L.append('section 12 0 1 ' + ' '.join(map(str, pay)))
print('\n'.join(L))
