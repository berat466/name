#!/usr/bin/env python3
"""BAGIMSIZ REFERANS (Plan Bolum 01, dar kayan nokta f32/f16/bf16): sonuclari TAM RASYONEL aritmetikle (fractions + buyuk tamsayi) hesaplar; motorun ve C++ derleyicisinin hicbir parcasini kullanmaz.
  ref_dar.py veri  TUR N          : deneme degerleri (double bit deseni, isaretli int64 olarak), TUR = rnd | fmt32 | fmt16 | fmtbf16 | int
  ref_dar.py bekle FORMAT OP N    : beklenen cikti satirlari (FORMAT f32|f16|bf16 ; OP rnd add sub mul div sqrt i2f u2f)
Cikti degeri = sonucun double bit deseni (motor puti ile ayni)."""
import sys, struct
from fractions import Fraction
from math import isqrt
FM = {'f32': (23, -126, 127), 'f16': (10, -14, 15), 'bf16': (7, -126, 127)}
def d2b(x): return struct.unpack('<q', struct.pack('<d', x))[0]
def b2d(b): return struct.unpack('<d', struct.pack('<q', b))[0]
def u2s(v): v &= (1 << 64) - 1; return v - (1 << 64) if v >> 63 else v
def lcg(seed):
    s = seed
    while True:
        s = (s * 6364136223846793005 + 1442695040888963407) & ((1 << 64) - 1); yield s
def round_frac(a, mb, emin, emax):
    """a >= 0 Fraction -> en yakin (cifte) izgara degeri Fraction ya da None (= sonsuz)"""
    if a == 0: return Fraction(0)
    e = a.numerator.bit_length() - a.denominator.bit_length()
    if Fraction(2) ** e > a: e -= 1
    q = max(e - mb, emin - mb)
    r = a / (Fraction(2) ** q)
    n = r.numerator // r.denominator; rem = r - n
    if rem > Fraction(1, 2) or (rem == Fraction(1, 2) and n % 2 == 1): n += 1
    v = Fraction(n) * (Fraction(2) ** q)
    return None if v >= Fraction(2) ** (emax + 1) else v
def frac_to_bits(v, neg):
    if v is None: return d2b(float('-inf') if neg else float('inf'))
    x = float(v)
    return d2b(-x if neg else x)
NAN_DEFAULT = -2251799813685248   # 0xFFF8000000000000 (x86 varsayilan NaN)
def rnd_bits(b, f):
    mb, emin, emax = FM[f]; x = b2d(b)
    u = b & ((1 << 64) - 1); sign = u >> 63; e = (u >> 52) & 2047; mant = u & ((1 << 52) - 1)
    if e == 2047:
        if mant == 0: return b
        s = 52 - mb; return u2s((sign << 63) | (2047 << 52) | ((mant >> s) << s) | (1 << 51))
    if u << 1 & ((1 << 64) - 1) == 0: return b
    return frac_to_bits(round_frac(abs(Fraction(x)), mb, emin, emax), sign == 1)
def vals_double(n):
    sp = [0.0, -0.0, float('inf'), float('-inf'), 1.0, -1.0, 0.5, 0.1, -0.1, 1/3, 2.0, 3.0, 65504.0, 65519.99999999999, 65520.0, 65536.0, 3.4028234663852886e38, 3.4028235677973362e38, 3.4028235677973366e38, 3.5e38, -3.4028235e38,
          1e-8, 5.960464477539063e-08, 2.9802322387695312e-08, 2.98023223876953125e-08 * 1.0000001, 1.401298464324817e-45, 7.006492321624085e-46, 7.0064923216240854e-46 * 1.0000001, 1.1754943508222875e-38, 6.103515625e-05, 6.097555160522461e-05,
          1e-300, 4.9e-324, 2.2250738585072014e-308, 1.7976931348623157e308, 3.3895313892515355e38, 3.3961775190e38, 3.3961775e38, 0.00390625, 1.00390625, 1.001953125, 1.0009765625, 2049.0, 2050.0, 2051.0, 16777217.0, 16777219.0, 1.0000000596046448, 1.00000005960464478]
    for (mb_, emin_, emax_) in FM.values():
        ulp_sub = 2.0 ** (emin_ - mb_)
        for k in (0, 1, 2, 3):
            for d_ in (0.5, 0.4999999, 0.5000001, 1.0, 1.5): sp.append((k + d_) * ulp_sub)
        for k in (0, 1, 2, 3, (1 << mb_) - 1):
            for d_ in (0.5, 0.4999999, 0.5000001): sp.append((2.0 ** emin_) * (1 + (k + d_) / (1 << mb_)))      # ilk normal bant ve komsulari
        for e_ in (0, 1, emax_ - 1):
            for k in (0, 1, (1 << mb_) - 1):
                for d_ in (0.5, 0.4999999, 0.5000001): sp.append((2.0 ** e_) * (1 + (k + d_) / (1 << mb_)))
        mx = (2.0 - 2.0 ** -mb_) * 2.0 ** emax_
        for d_ in (0.4999999, 0.5, 0.5000001): sp.append(mx + d_ * 2.0 ** (emax_ - mb_))
        sp += [-v for v in sp[-12:]]
    sp += [2.0**31, 2147483647.0, 2147483648.5, 3e9, 4e9, 4294967295.0, 4294967296.0, 2.0**53, 2.0**62, 9.223372036854775e18, 9223372036854775808.0, 1.5 * 2.0**63, 1.8446744073709548e19, 18446744073709551616.0, 1e19, 1e20, -2147483649.0, -3e9, -4e9, -9.2e18, -9223372036854775808.0, -1e19, -0.5, 0.5, -1.5, 1.9999999, 255.5, 256.0, 65535.5, 128.0, -128.5, -129.0, 127.5, 32768.0, 32767.9, -32769.0]
    bits = [d2b(v) for v in sp] + [0x7FF8000000000000, 0x7FF0000000000001, u2s(0xFFF8DEADBEEF0000), u2s(0xFFFFFFFFFFFFFFFF), 0x7FF7FFFFFFFFFFFF]
    g = lcg(12345)
    while len(bits) < max(n, len(bits)) and n > 0 and len(bits) < n:
        r = next(g); ex = (r >> 52) % 80 + 970 if (r & 3) else (r >> 52) % 2047
        bits.append(u2s(((r >> 7) & (1 << 63)) | (ex << 52) | (r & ((1 << 52) - 1))))
    return bits if n <= 0 else bits[:n] if n < len(bits) else bits
def sample(lst, n):
    if n <= 0 or n >= len(lst): return lst
    return [lst[i * (len(lst) - 1) // (n - 1)] for i in range(n)]
def vals_fmt(f, n):
    out = []
    for b in vals_double(0 if n <= 0 else 400):
        u = b & ((1 << 64) - 1)
        if ((u >> 52) & 2047) == 2047 and (u & ((1 << 52) - 1)): continue
        r = rnd_bits(b, f); ru = r & ((1 << 64) - 1)
        out.append(r)
    seen = []; [seen.append(v) for v in out if v not in seen]
    return sample(seen, n)
def ints(n):
    base = [0, 1, 2, 3, 7, 8, 255, 256, 2047, 2048, 2049, 4095, 65504, 65519, 65520, 65535, 65536, 16777215, 16777216, 16777217, 16777219, 33554433, 2**31 - 1, 2**31, 2**32, 2**53 - 1, 2**53, 2**53 + 1, 2**53 + 2, 2**54 + 3,
            2**62 + 2**38, 2**62 + 2**38 + 1, 2**62 + 2**39, 2**63 - 1, 2**63 - 1025, 2**63 - 512, 2**63 - 513, 2**64 - 1, 2**64 - 2**39, 2**64 - 2**38 - 1, 2**63, 2**63 + 1, 2**63 + 2**39, 2**63 + 2**39 + 1, 0x8000000000000401]
    for p_ in (24, 25, 30, 31, 32, 33, 35, 36, 40, 45, 52, 53, 54, 58, 62, 63):
        for mb_ in (23, 10, 7):
            t_ = p_ - mb_ - 1
            if t_ < 0: continue
            for low in (0, 1, 2**11, 2**11 + 1, 2**10, 2**10 + 1, 2**(t_ - 1) if t_ > 0 else 0, 2**(t_ + 1)):
                for extra in (0, 1):
                    v_ = 2**p_ + 2**t_ + low + extra
                    if v_ < 2**64: base.append(v_)
                    if 2**p_ + low + extra < 2**64: base.append(2**p_ + low + extra)
    base += [-v for v in base if v < 2**63] + [-(2**63)]
    g = lcg(777)
    seen_ = set(); base = [v for v in base if not (v in seen_ or seen_.add(v))]
    while len(base) < len(base) + 0 and False: pass
    n0 = len(base)
    while len(base) < n0 + 40:
        r = next(g); base.append(r >> (r % 40))
    return sample([u2s(v) for v in base], n)
def sgn(f): return f
def main():
  mode = sys.argv[1]
  if mode == 'veri':
      kind, n = sys.argv[2], int(sys.argv[3])
      lst = vals_double(n) if kind == 'rnd' else ints(n) if kind == 'int' else vals_fmt(kind[3:], n)
      print(', '.join(str(v) for v in lst))
  elif mode == 'bekle':
      f, op, n = sys.argv[2], sys.argv[3], int(sys.argv[4]); mb, emin, emax = FM[f]
      if op == 'rnd':
          for b in vals_double(n): print(rnd_bits(b, f))
      elif op in ('i2f', 'u2f'):
          for v in ints(n):
              u = v & ((1 << 64) - 1); val = (v if op == 'i2f' else u)
              print(frac_to_bits(round_frac(abs(Fraction(val)), mb, emin, emax), val < 0))
      else:
          L = vals_fmt(f, n)
          def isnan(b): return ((b >> 52) & 2047) == 2047 and (b & ((1 << 52) - 1)) != 0
          def fin(b): return ((b >> 52) & 2047) != 2047
          for xb in L:
              ys = L if op in ('add', 'sub', 'mul', 'div') else [None]
              for yb in ys:
                  x = b2d(xb)
                  if op == 'sqrt':
                      if x != x: res = xb
                      elif x < 0: res = NAN_DEFAULT
                      elif x == 0 or x == float('inf'): res = xb
                      else:
                          fr = Fraction(x); p, q = fr.numerator, fr.denominator; K = 80
                          N = (p * q) << (2 * K); r = isqrt(N); den = q << K
                          v = Fraction(2 * r + 1, 2 * den) if r * r != N else Fraction(r, den)
                          res = frac_to_bits(round_frac(v, mb, emin, emax), False)
                  else:
                      y = b2d(yb)
                      inf = float('inf')
                      if x != x or y != y: res = NAN_DEFAULT if False else None
                      if op in ('add', 'sub') and fin(xb) and fin(yb):
                          t = Fraction(x) + (Fraction(y) if op == 'add' else -Fraction(y))
                          if t == 0: res = d2b(x + y if op == 'add' else x - y)   # isaret kurali (+0/-0) donanimdan: ayni ifade IEEE
                          else: res = frac_to_bits(round_frac(abs(t), mb, emin, emax), t < 0)
                      elif op == 'mul' and fin(xb) and fin(yb):
                          t = Fraction(x) * Fraction(y)
                          res = d2b(x * y) if t == 0 else frac_to_bits(round_frac(abs(t), mb, emin, emax), t < 0)
                      elif op == 'div' and fin(xb) and fin(yb) and y != 0:
                          t = Fraction(x) / Fraction(y)
                          res = d2b(x / y) if t == 0 else frac_to_bits(round_frac(abs(t), mb, emin, emax), t < 0)
                      else:
                          # sonsuz / sifir / NaN girdileri: IEEE kurali double'da (isaret ve NaN varsayilanlari) -- sonuc zaten bicimde gosterilir (sonsuz/sifir/NaN)
                          try:
                              z = {'add': x + y, 'sub': x - y, 'mul': x * y, 'div': (x / y if y != 0 else (float('nan') if x == 0 or x != x else (inf if (x > 0) == (str(y)[0] != '-') else -inf)))}[op]
                          except Exception: z = float('nan')
                          res = NAN_DEFAULT if z != z else d2b(z)
                  print(res)
if __name__ == '__main__': main()
