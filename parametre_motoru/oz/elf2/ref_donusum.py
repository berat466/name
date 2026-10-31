#!/usr/bin/env python3
"""BAGIMSIZ REFERANS (Plan Bolum 01, kayan nokta donusumleri): tamsayi <-> kayan nokta (f16 bf16 f32 f64) ve kayan <-> kayan donusumlerinin beklenen sonuclari, TAM rasyonel aritmetikle.
  ref_donusum.py veri KAYNAK HEDEF N     : kaynak turun deneme degerleri (motorun yazmac gosterimi: isaretli int64 ya da double bit deseni)
  ref_donusum.py bekle KAYNAK HEDEF N    : beklenen satirlar
Tamsayi -> kayan: en yakina-cifte; kayan -> tamsayi: sifira dogru kes, int64'e sigan degerler icin hedef turun araligina SAR (motor tanimi; C++'ta aralik disi tanimsiz), u64 hedefi doyurmali (f2u); kayan -> bool: != 0 (NaN dahil)."""
import sys
from fractions import Fraction
from ref_dar import FM, d2b, b2d, u2s, rnd_bits, round_frac, frac_to_bits, vals_double, vals_fmt, ints, sample
INT = {'i8': (8, 1), 'i16': (16, 1), 'i32': (32, 1), 'i64': (64, 1), 'u8': (8, 0), 'u16': (16, 0), 'u32': (32, 0), 'u64': (64, 0), 'bool': (1, 0)}
FL = ('f16', 'bf16', 'f32', 'f64')
def norm(v, t):   # herhangi bir tamsayiyi tur araligina sar; motor gosterimi (isaretli int64) dondur
    b, sg = INT[t]
    if t == 'bool': return 1 if v & 1 else 0
    v &= (1 << b) - 1
    if sg and v >> (b - 1): v -= 1 << b
    return u2s(v) if b == 64 else v
def srcvals(t, n):
    if t in INT:
        vs = []; [vs.append(x) for x in (norm(v, t) for v in ints(0)) if x not in vs]
        if t == 'bool': vs = [0, 1]
        return sample(vs, n)
    if t == 'f64': return sample([b for b in vals_double(0)], n)
    return vals_fmt(t, n)
def value(v, t):  # tamsayi turde yazmac degerinin matematiksel degeri
    b, sg = INT[t]
    return (v if sg else v & ((1 << b) - 1)) if t != 'bool' else v
def bits_of_float(t, b): return Fraction(b2d(b))
def main():
    mode, s, d, n = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4])
    if s in FL and d in INT:   # kayan -> tamsayi: yalniz aralik icindekiler
        out = []
        for b in srcvals(s, 0):
            x = b2d(b); u = b & ((1 << 64) - 1); nan = ((u >> 52) & 2047) == 2047 and (u & ((1 << 52) - 1)) != 0; inf = ((u >> 52) & 2047) == 2047 and not nan
            if d == 'bool': out.append((b, 1 if (nan or x != 0) else 0)); continue
            bb, sg = INT[d]
            if d == 'u64':      # f2u: doyurmali (NaN ve <= -1 -> 0, >= 2^64 -> 2^64-1, sonsuz dahil)
                if nan: q = 0
                elif inf: q = (1 << 64) - 1 if x > 0 else 0
                else:
                    q = int(x)
                    q = 0 if x <= -1 else min(q, (1 << 64) - 1) if q >= 0 else 0
                out.append((b, u2s(q))); continue
            if nan or inf: continue
            q = int(x)   # sifira dogru kes
            if d == 'i64':
                if abs(q) < (1 << 63): out.append((b, q))
                continue
            if abs(q) < (1 << 63): out.append((b, norm(q, d)))   # int64'e kes, sonra tur araligina sar (motor tanimi; C++'ta aralik disi tanimsiz)
        out = sample(out, n)
        for b, e in out: print(b if mode == 'veri' else e)
        return
    vals = srcvals(s, n)
    for v in vals:
        if mode == 'veri': print(v); continue
        if s in INT and d in FL:
            mb, emin, emax = FM[d] if d != 'f64' else (52, -1022, 1023)
            val = value(v, s)
            print(frac_to_bits(round_frac(abs(Fraction(val)), mb, emin, emax), val < 0) if val != 0 else 0)
        else:   # kayan -> kayan
            print(rnd_bits(v, d) if d != 'f64' else v)
main()
