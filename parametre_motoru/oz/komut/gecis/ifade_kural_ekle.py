#!/usr/bin/env python3
"""Plan Bolum 02 (duzeltme): komut tablosuna (PMK1 okunabilir gorunum, stdin -> stdout) bolum 12 = IFADE KURAL TABLOSU ekler/yeniler.
ifx.pmb (sayisal PMX1 derleyicisi) bu bolumu CALISMA ANINDA okur: tur ozellikleri, ikili/karsilastirma operatorlerinin komut eslemesi, donusum/normallestirme komut adlari.
Kayit: sinif nstr (uzunluk karakter...)*nstr nnum sayilar*nnum ;  yuk: kayit_sayisi kayit...
 sinif 1 TUR       dizgeler [ad, normallestirme_komutu]  sayilar [tur_kodu, bit, isaretli, sinif(0 tam,1 kayan), derece]
 sinif 2 IKILI     dizgeler [simge, tam_komut, isaretsiz64_komut, kayan_komut, f80_komut, f128_komut]  sayilar [tur(1 aritm,2 bol/mod,3 bit,4 kaydirma), isaretsiz_secimi(0 yok,1 yalniz u64,2 her isaretsiz), normallestir]
 sinif 3 KARSILASTIR dizgeler [simge, isaretli, isaretsiz, kayan, f80, f128]  sayilar [isaretsizde_takas, kayanda_takas, kayanda_tersle, f80de_takas, isaretlide_takas]  (takma komut gt/ge/fgt/fge KULLANILMAZ: lt/le/flt/fle + operand takasi)
 sinif 5 YAZMAC    sayilar [i64 u64 f64 bool i8 i16 i32 u8 u16 u32 f32 f80 devam f128]: yazmac turu -> TYPES tur kimligi (PMT1 notu icin)
 sinif 4 AD        dizgeler [i2f u2f f2i f2u rnd_f32 neg fneg not feq | add_f80 sub_f80 mul_f80 div_f80 neg_f80 eq_f80 lt_f80 le_f80 ne_f80 tobool_f80 cv_f64_f80 cv_f80_f64 cv_f80_f32 cv_i64_f80 cv_u64_f80 cv_f80_i64 cv_f80_u64 pack_f80 lo64_f80 hi64_f80 | f128: add sub mul div neg eq lt le ne tobool cv_f64 cv_to_f64 cv_to_f32 cv_i64 cv_u64 cv_to_i64 cv_to_u64 pack lo64 hi64 cv_f80_f128 cv_f128_f80 | dolgu (64 toplam)]  sayilar []
Bolum bayrak bit0 = 0 (istege bagli)."""
import sys
def rec(cls, strs, nums):
    r = [cls, len(strs)]
    for s in strs: r += [len(s)] + [ord(c) for c in s]
    return r + [len(nums)] + nums
R = []
T = [(1, 'bool', '', 8, 0, 0, 1), (2, 'char', 'sext8', 8, 1, 0, 2), (3, 'schar', 'sext8', 8, 1, 0, 2), (4, 'uchar', 'zext8', 8, 0, 0, 2), (5, 'short', 'sext16', 16, 1, 0, 3), (6, 'ushort', 'zext16', 16, 0, 0, 3),
     (7, 'int', 'sext32', 32, 1, 0, 4), (8, 'uint', 'zext32', 32, 0, 0, 4), (9, 'long', '', 64, 1, 0, 5), (10, 'ulong', '', 64, 0, 0, 5), (11, 'llong', '', 64, 1, 0, 6), (12, 'ullong', '', 64, 0, 0, 6),
     (13, 'float', '', 32, 1, 1, 0), (14, 'double', '', 64, 1, 1, 0), (15, 'ldouble', '', 128, 1, 1, 0), (24, 'float128', '', 128, 1, 1, 0)]
for (c, n, nm, b, sg, cl, rk) in T: R.append(rec(1, [n, nm], [c, b, sg, cl, rk]))
B = [('+', 'add', '', 'fadd', 'add_f80', 'add_f128', 1, 0, 1), ('-', 'sub', '', 'fsub', 'sub_f80', 'sub_f128', 1, 0, 1), ('*', 'mul', '', 'fmul', 'mul_f80', 'mul_f128', 1, 0, 1), ('/', 'div', 'divu', 'fdiv', 'div_f80', 'div_f128', 2, 1, 1), ('%', 'mod', 'modu', '', '', '', 2, 1, 1),
     ('&', 'and', '', '', '', '', 3, 0, 0), ('|', 'or', '', '', '', '', 3, 0, 0), ('^', 'xor', '', '', '', '', 3, 0, 0), ('<<', 'shl', '', '', '', '', 4, 0, 1), ('>>', 'sar', 'shr', '', '', '', 4, 2, 0)]
for (t, i, u, f, f8, f16, k, w, nn) in B: R.append(rec(2, [t, i, u, f, f8, f16], [k, w, nn]))
C = [('==', 'eq', 'eq', 'feq', 'eq_f80', 'eq_f128', 0, 0, 0, 0, 0), ('!=', 'ne', 'ne', 'feq', 'ne_f80', 'ne_f128', 0, 0, 1, 0, 0), ('<', 'lt', 'ltu', 'flt', 'lt_f80', 'lt_f128', 0, 0, 0, 0, 0), ('>', 'lt', 'ltu', 'flt', 'lt_f80', 'lt_f128', 1, 1, 0, 1, 1), ('<=', 'le', 'leu', 'fle', 'le_f80', 'le_f128', 0, 0, 0, 0, 0), ('>=', 'le', 'leu', 'fle', 'le_f80', 'le_f128', 1, 1, 0, 1, 1)]
for (t, s_, u, f, f8, f16, a, b, c, d, e) in C: R.append(rec(3, [t, s_, u, f, f8, f16], [a, b, c, d, e]))
N = ['i2f', 'u2f', 'f2i', 'f2u', 'rnd_f32', 'neg', 'fneg', 'not', 'feq', 'add_f80', 'sub_f80', 'mul_f80', 'div_f80', 'neg_f80', 'eq_f80', 'lt_f80', 'le_f80', 'ne_f80', 'tobool_f80', 'cv_f64_f80', 'cv_f80_f64', 'cv_f80_f32', 'cv_i64_f80', 'cv_u64_f80', 'cv_f80_i64', 'cv_f80_u64', 'pack_f80', 'lo64_f80', 'hi64_f80', 'add_f128', 'sub_f128', 'mul_f128', 'div_f128', 'neg_f128', 'eq_f128', 'lt_f128', 'le_f128', 'ne_f128', 'tobool_f128', 'cv_f64_f128', 'cv_f128_f64', 'cv_f128_f32', 'cv_i64_f128', 'cv_u64_f128', 'cv_f128_i64', 'cv_f128_u64', 'pack_f128', 'lo64_f128', 'hi64_f128', 'cv_f80_f128', 'cv_f128_f80'] + [''] * 13
R.append(rec(4, N, []))
R.append(rec(5, [], [4, 9, 24, 11, 1, 2, 3, 6, 7, 8, 23, 25, 255, 26]))
pay = [len(R)]
for r in R: pay += r
L = sys.stdin.read().rstrip('\n').split('\n')
L = [l for l in L if not l.startswith('section 12 ')]
L.append('section 12 0 1 ' + ' '.join(map(str, pay)))
print('\n'.join(L))
