#!/usr/bin/env python3
"""Tam rasyonel referans (GELISTIRME araci; motor dismi): f80 / f128 bit desenleri <-> Fraction, en yakina-cifte yuvarlama, islemler."""
from fractions import Fraction
from math import isqrt
import random
FM = {'f128': dict(P=113, bias=16383, ex=False), 'f80': dict(P=64, bias=16383, ex=True)}
M64 = (1 << 64) - 1
def decode(fmt, lo, hi):
    """-> ('zero'|'fin'|'inf'|'nan', sign, Fraction)"""
    f = FM[fmt]
    if f['ex']:
        s = (hi >> 15) & 1; ef = hi & 0x7FFF; mant = lo
        if ef == 0x7FFF: return ('inf' if (mant << 1) & M64 == 0 else 'nan', s, None)
        if ef == 0: return ('zero', s, Fraction(0)) if mant == 0 else ('fin', s, Fraction(mant) * Fraction(2) ** (1 - f['bias'] - 63))
        if not (mant >> 63): return ('nan', s, None)   # unnormal
        return ('fin', s, Fraction(mant) * Fraction(2) ** (ef - f['bias'] - 63))
    s = (hi >> 63) & 1; ef = (hi >> 48) & 0x7FFF; frac = ((hi & ((1 << 48) - 1)) << 64) | lo
    if ef == 0x7FFF: return ('inf' if frac == 0 else 'nan', s, None)
    if ef == 0: return ('zero', s, Fraction(0)) if frac == 0 else ('fin', s, Fraction(frac) * Fraction(2) ** (1 - f['bias'] - 112))
    return ('fin', s, Fraction((1 << 112) | frac) * Fraction(2) ** (ef - f['bias'] - 112))
def rnd(fmt, s, a):
    """a >= 0 Fraction (kesin deger) -> (lo, hi) en yakina-cifte; tasma sonsuz; a == 0 -> sifir"""
    f = FM[fmt]; P = f['P']; bias = f['bias']; emin = 1 - bias; emax = bias
    if a == 0: return pack(fmt, s, 0, 0)
    e = a.numerator.bit_length() - a.denominator.bit_length()
    if Fraction(2) ** e > a: e -= 1
    q = max(e, emin) - (P - 1)
    r = a / (Fraction(2) ** q); n = r.numerator // r.denominator; rem = r - n
    if rem > Fraction(1, 2) or (rem == Fraction(1, 2) and n % 2 == 1): n += 1
    if n >= (1 << P): n >>= 1; q += 1
    # n * 2^q; us alani
    ee = q + (P - 1)   # n'nin en yuksek bitinin ussu (n tam P bit ise)
    if n == 0: return pack(fmt, s, 0, 0)
    if ee > emax: return pack(fmt, s, 'inf', 0)
    if n >> (P - 1): return pack(fmt, s, ee + bias, n)
    return pack(fmt, s, 0, n)
def pack(fmt, s, ef, mant):
    f = FM[fmt]
    if f['ex']:
        if ef == 'inf': return (1 << 63, (s << 15) | 0x7FFF)
        return (mant & M64, (s << 15) | ef)
    if ef == 'inf': return (0, (s << 63) | (0x7FFF << 48))
    frac = mant & ((1 << 112) - 1)
    return (frac & M64, (s << 63) | (ef << 48) | (frac >> 64))
def defnan(fmt): return (0xC000000000000000, 0xFFFF) if FM[fmt]['ex'] else (0, 0xFFFF800000000000)
def quiet(fmt, lo, hi):
    return (lo | (1 << 62), hi) if FM[fmt]['ex'] else (lo, hi | (1 << 47))
def inf(fmt, s): return pack(fmt, s, 'inf', 0)
def zero(fmt, s): return pack(fmt, s, 0, 0)
def rand_vals(fmt, n, seed=1):
    rnd_ = random.Random(seed); out = []
    f = FM[fmt]
    def mk(ef, mant, s): return pack(fmt, s, ef, mant)
    P = f['P']
    for s in (0, 1):
        out += [zero(fmt, s), inf(fmt, s), mk(1, (1 << (P - 1)), s), mk(0x7FFE, (1 << P) - 1, s), mk(f['bias'], 1 << (P - 1), s), mk(f['bias'] + 1, (1 << (P - 1)) | 1, s),
                mk(0, 1, s), mk(0, (1 << (P - 1)) - 1, s), mk(f['bias'] - 1, (1 << P) - 1, s), mk(f['bias'] + 63, 1 << (P - 1), s), mk(f['bias'] + 64, (1 << (P - 1)) | 12345, s)]
    out.append(defnan(fmt)); out.append(quiet(fmt, 0, 0x7FFF << 48) if not f['ex'] else (0xC000000000000001, 0x7FFF))
    while len(out) < n:
        k = rnd_.random()
        s = rnd_.randrange(2)
        if k < 0.15: ef = rnd_.randrange(0, 40)
        elif k < 0.3: ef = rnd_.randrange(0x7FFE - 40, 0x7FFF)
        elif k < 0.6: ef = f['bias'] + rnd_.randrange(-130, 131)
        else: ef = rnd_.randrange(0, 0x7FFF)
        mant = rnd_.getrandbits(P - 1)
        if f['ex']: mant |= (1 << 63) if ef != 0 else 0
        elif ef != 0: mant |= 1 << 112
        if rnd_.random() < 0.15: mant = (mant >> rnd_.randrange(1, 60)) << rnd_.randrange(0, 60) if False else mant
        out.append(mk(ef, mant, s))
    return out
# ---- islem referanslari (tam rasyonel; yuvarlama en yakina-cifte)
def _nanres(fmt, ops):
    for (lo, hi) in ops:
        c = decode(fmt, lo, hi)[0]
        if c == 'nan': return quiet(fmt, lo, hi)
    return None
def op_addsub(fmt, x, y, sub=False):
    y0 = y
    if sub: y = (y[0], y[1] ^ ((1 << 15) if FM[fmt]['ex'] else (1 << 63)))   # isaret cevir (ham)
    cx, sx, vx = decode(fmt, *x); cy, sy, vy = decode(fmt, *y)
    if cx == 'nan' or cy == 'nan':
        return quiet(fmt, *(x if cx == 'nan' else y0))   # NaN islenen ham haliyle (cevrilmemis) sessizlestirilir
    if cx == 'inf' or cy == 'inf':
        if cx == 'inf' and cy == 'inf' and sx != sy: return defnan(fmt)
        return inf(fmt, sx if cx == 'inf' else sy)
    if cx == 'zero' and cy == 'zero': return zero(fmt, sx & sy)
    if cx == 'zero': return y
    if cy == 'zero': return x
    a = (-vx if sx else vx) + (-vy if sy else vy)
    if a == 0: return zero(fmt, 0)
    return rnd(fmt, 1 if a < 0 else 0, abs(a))
def near_pairs(fmt, n, seed=11):
    """Yonlendirilmis cift uretimi: ayni/yakin us, iptal, yapiskan bit, es-uzaklik (tie), alt-normal"""
    f = FM[fmt]; P = f['P']; r = random.Random(seed); out = []
    def mant(): 
        m = r.getrandbits(P - 1)
        return m | (1 << (P - 1))
    for _ in range(n):
        s1 = r.randrange(2); s2 = r.randrange(2)
        k = r.random()
        if k < 0.3: e1 = r.choice([1, 2, 3, f['bias'], 0x7FFD, 0x7FFE, r.randrange(1, 0x7FFE)])
        else: e1 = r.randrange(1, 0x7FFE)
        mode = r.randrange(7)
        m1 = mant()
        if mode == 0: e2, m2 = e1, mant()
        elif mode == 1: e2, m2 = e1, m1 ^ r.getrandbits(r.randrange(1, P))
        elif mode == 2: e2, m2 = e1 + r.choice([-1, 1]), mant()
        elif mode == 3: e2 = e1 - r.choice([P - 1, P, P + 1, P + 2, P + 3, 2 * P, 130, 140]); m2 = mant()
        elif mode == 4: e2 = e1 - r.choice([P, P + 1, P + 2]); m2 = 1 << (P - 1)   # tam es (tie) ve alt-es
        elif mode == 5: e1 = r.randrange(0, 3); e2 = r.randrange(0, 3); m1 = r.getrandbits(P - 1) | ((1 << (P - 1)) if e1 else 0); m2 = r.getrandbits(P - 1) | ((1 << (P - 1)) if e2 else 0)
        else: e2 = e1 + r.randrange(-3, 4); m2 = m1
        if not (0 <= e2 < 0x7FFF): e2 = max(0, min(0x7FFE, e2))
        if e2 == 0 and f['ex']: m2 &= (1 << 63) - 1
        if e2 == 0 and not f['ex']: m2 &= (1 << 112) - 1
        out.append((pack(fmt, s1, e1, m1), pack(fmt, s2, e2, m2)))
    return out
def op_mul(fmt, x, y):
    cx, sx, vx = decode(fmt, *x); cy, sy, vy = decode(fmt, *y)
    if cx == 'nan' or cy == 'nan': return quiet(fmt, *(x if cx == 'nan' else y))
    s = sx ^ sy
    if cx == 'inf' or cy == 'inf':
        if cx == 'zero' or cy == 'zero': return defnan(fmt)
        return inf(fmt, s)
    if cx == 'zero' or cy == 'zero': return zero(fmt, s)
    return rnd(fmt, s, abs(vx) * abs(vy))
def op_div(fmt, x, y):
    cx, sx, vx = decode(fmt, *x); cy, sy, vy = decode(fmt, *y)
    if cx == 'nan' or cy == 'nan': return quiet(fmt, *(x if cx == 'nan' else y))
    s = sx ^ sy
    if cx == 'inf':
        return defnan(fmt) if cy == 'inf' else inf(fmt, s)
    if cy == 'inf': return zero(fmt, s)
    if cy == 'zero': return defnan(fmt) if cx == 'zero' else inf(fmt, s)
    if cx == 'zero': return zero(fmt, s)
    return rnd(fmt, s, abs(vx) / abs(vy))
def op_sqrt(fmt, x):
    cx, sx, vx = decode(fmt, *x)
    if cx == 'nan': return quiet(fmt, *x)
    if cx == 'zero': return x
    if sx: return defnan(fmt)
    if cx == 'inf': return x
    # sqrt(vx) en yakina-cifte: tam tamsayi karekok ile
    n, d = vx.numerator, vx.denominator
    # vx = n/d ; sqrt = sqrt(n*d)/d ; yeterli bit icin olcekle: K cift
    K = 2 * 400
    r = isqrt(n * d * (1 << K)); exact = (r * r == n * d * (1 << K))
    a = Fraction(r, d * (1 << (K // 2)))
    if not exact:
        # yapiskan: bir alt bit ekle (sonlu rasyonel yaklasimin tam deger olmadigini yuvarlama icin korur)
        a = Fraction(2 * r + 1, 2 * d * (1 << (K // 2)))
    return rnd(fmt, 0, a)
# ---- f64 (tek sozcuk) ve donusum referanslari
FM['f64'] = dict(P=53, bias=1023, ex=False, one=True)
_decode_w = decode; _pack_w = pack
def decode(fmt, lo, hi):
    if fmt == 'f64':
        s = lo >> 63; ef = (lo >> 52) & 0x7FF; fr = lo & ((1 << 52) - 1)
        if ef == 0x7FF: return ('inf' if fr == 0 else 'nan', s, None)
        if ef == 0: return ('zero', s, Fraction(0)) if fr == 0 else ('fin', s, Fraction(fr) * Fraction(2) ** (1 - 1023 - 52))
        return ('fin', s, Fraction((1 << 52) | fr) * Fraction(2) ** (ef - 1023 - 52))
    return _decode_w(fmt, lo, hi)
def pack(fmt, s, ef, mant):
    if fmt == 'f64':
        if ef == 'inf': return ((s << 63) | (0x7FF << 52), 0)
        return ((s << 63) | (ef << 52) | (mant & ((1 << 52) - 1)), 0)
    return _pack_w(fmt, s, ef, mant)
FRACW = {'f64': 52, 'f80': 63, 'f128': 112}
def fraction(fmt, lo, hi):
    if fmt == 'f64': return lo & ((1 << 52) - 1)
    if fmt == 'f80': return lo & ((1 << 63) - 1)
    return ((hi & ((1 << 48) - 1)) << 64) | lo
def nan_to(fmt, s, fr):
    fr |= 1 << (FRACW[fmt] - 1)
    if fmt == 'f64': return ((s << 63) | (0x7FF << 52) | fr, 0)
    if fmt == 'f80': return (fr | (1 << 63), (s << 15) | 0x7FFF)
    return (fr & M64, (s << 63) | (0x7FFF << 48) | (fr >> 64))
def nan_conv(src, dst, lo, hi):
    c, s, _ = decode(src, lo, hi); fr = fraction(src, lo, hi); ws, wd = FRACW[src], FRACW[dst]
    fr = (fr << (wd - ws)) if wd >= ws else (fr >> (ws - wd))
    return nan_to(dst, s, fr)
def op_w2w(src, dst, x):
    c, s, v = decode(src, *x)
    if c == 'nan': return nan_conv(src, dst, *x)
    if c == 'inf': return inf(dst, s)
    if c == 'zero': return zero(dst, s)
    return rnd(dst, s, abs(v))
def op_w2n(src, nar, x):
    NP = {'f32': (24, -126, 127), 'f16': (11, -14, 15), 'bf16': (8, -126, 127)}[nar]
    c, s, v = decode(src, *x)
    if c == 'nan':
        lo, hi = nan_conv(src, 'f64', *x)
        wn = NP[0] - 1; fr = (lo & ((1 << 52) - 1)); fr = (fr >> (52 - wn)) << (52 - wn); fr |= 1 << 51
        return ((s << 63) | (0x7FF << 52) | fr, 0)
    if c == 'inf': return inf('f64', s)
    if c == 'zero': return zero('f64', s)
    P, emin, emax = NP; a = abs(v)
    e = a.numerator.bit_length() - a.denominator.bit_length()
    if Fraction(2) ** e > a: e -= 1
    q = max(e, emin) - (P - 1)
    r = a / (Fraction(2) ** q); n = r.numerator // r.denominator; rem = r - n
    if rem > Fraction(1, 2) or (rem == Fraction(1, 2) and n % 2 == 1): n += 1
    val = Fraction(n) * Fraction(2) ** q
    if n == 0: return zero('f64', s)
    if val >= Fraction(2) ** (emax + 1): return inf('f64', s)
    return rnd('f64', s, val)
def op_cmp(fmt, kind, x, y):
    cx, sx, vx = decode(fmt, *x); cy, sy, vy = decode(fmt, *y)
    if cx == 'nan' or cy == 'nan': return 1 if kind == 'ne' else 0
    def key(c, s, v): return (float('-inf') if s else float('inf')) if c == 'inf' else (-v if s else v)
    kx, ky = key(cx, sx, vx), key(cy, sy, vy)
    return int({'eq': kx == ky, 'ne': kx != ky, 'lt': kx < ky, 'le': kx <= ky}[kind])
def op_i2w(fmt, v, signed=True, bits=64):
    if signed and v >= 1 << (bits - 1): v -= 1 << bits
    if v == 0: return zero(fmt, 0)
    return rnd(fmt, 1 if v < 0 else 0, Fraction(abs(v)))
def op_w2i(fmt, x, bits, signed):
    c, s, v = decode(fmt, *x); M = (1 << bits) - 1
    ind = (1 << (bits - 1)) if signed else 0
    if c in ('nan', 'inf'): return ind
    t = int(abs(v)); t = -t if s else t
    if signed:
        if -(1 << (bits - 1)) <= t < (1 << (bits - 1)): return t & M
        return ind
    if 0 <= t < (1 << bits): return t
    return 0
