// Plan Bolum 01: genis kayan nokta (f80 = x87 long double, f128 = __float128). C++ gercekleme DERLEYICININ turleriyle (long double, __float128, libquadmath sqrtq): bagimsiz referans.
// Motor tanimi (donanimdan farkli, tabloyla ayni): NaN islenen -> ilk NaN islenenin ham sozcukleri sessizlestirilmis; gecersiz islem -> varsayilan NaN (isaret 1, sessiz, yuk 0);
// f80'de ussu 0 ya da tam-1 olmayan ve tam sayi biti 0 olan desen (unnormal) NaN sayilir; kayan -> tamsayi: sifira dogru kes, gecersiz (NaN/sonsuz/aralik disi): isaretli -> en kucuk deger, isaretsiz -> 0;
// genis -> dar (f32/f16/bf16): TEK yuvarlama (tamsayi algoritmasi pm_round_core); NaN donusumlerinde kesir yuku kaydirilir, sessiz bit konur, isaret korunur.
#pragma once
#include <quadmath.h>
typedef long double PmLD; typedef __float128 PmQ;
enum WFPK { WFP_ADD, WFP_SUB, WFP_MUL, WFP_DIV, WFP_SQRT, WFP_NEG, WFP_ABS, WFP_MOV, WFP_PACK, WFP_LO, WFP_HI, WFP_EQ, WFP_NE, WFP_LT, WFP_LE, WFP_TOBOOL,
            WFP_FROM_F64, WFP_TO_F64, WFP_TO_OTHER, WFP_TO_NARROW, WFP_FROM_I64, WFP_FROM_U64, WFP_FROM_I128, WFP_FROM_U128, WFP_TO_INT_S, WFP_TO_INT_U,
            WFP_I128_TO_NARROW_S, WFP_I128_TO_NARROW_U, WFP_F64_TO_I128_S, WFP_F64_TO_I128_U };   // son dort: 128 bit tamsayi <-> dar kayan (double kabi); p2: 0 f32 1 f16 2 bf16 3 f64
namespace wfp {
typedef unsigned __int128 U; typedef __int128 S;
static inline bool isnanw(int f, uint64_t lo, uint64_t hi) {
  if (f == 0) { unsigned ef = unsigned(hi & 0x7FFF); if (ef == 0x7FFF) return (lo << 1) != 0; return ef != 0 && !(lo >> 63); }
  unsigned ef = unsigned((hi >> 48) & 0x7FFF); return ef == 0x7FFF && (((hi & ((1ULL << 48) - 1)) | lo) != 0);
}
static inline void quiet(int f, uint64_t lo, uint64_t hi, uint64_t& rl, uint64_t& rh) { if (f == 0) { rl = lo | (1ULL << 62); rh = hi; } else { rl = lo; rh = hi | (1ULL << 47); } }
static inline void defnan(int f, uint64_t& rl, uint64_t& rh) { if (f == 0) { rl = 0xC000000000000000ULL; rh = 0xFFFF; } else { rl = 0; rh = 0xFFFF800000000000ULL; } }
static inline PmLD get80(uint64_t lo, uint64_t hi) { PmLD v = 0; std::memcpy(&v, &lo, 8); uint16_t h = uint16_t(hi); std::memcpy(reinterpret_cast<char*>(&v) + 8, &h, 2); return v; }
static inline void put80(PmLD v, uint64_t& lo, uint64_t& hi) { std::memcpy(&lo, &v, 8); uint16_t h; std::memcpy(&h, reinterpret_cast<char*>(&v) + 8, 2); hi = h; }
static inline PmQ get128(uint64_t lo, uint64_t hi) { PmQ v; uint64_t w[2] = {lo, hi}; std::memcpy(&v, w, 16); return v; }
static inline void put128(PmQ v, uint64_t& lo, uint64_t& hi) { uint64_t w[2]; std::memcpy(w, &v, 16); lo = w[0]; hi = w[1]; }
// tur-bagimsiz yardimcilar
template <class T> struct Ops;
template <> struct Ops<PmLD> {
  static const int F = 0;
  static PmLD get(uint64_t lo, uint64_t hi) { if ((hi & 0x7FFF) == 0x7FFF && (lo << 1) == 0) lo = 1ULL << 63; return get80(lo, hi); }   // sonsuz: kanonik tam sayi biti
  static void put(PmLD v, uint64_t& lo, uint64_t& hi) { put80(v, lo, hi); }
  static PmLD sqrt_(PmLD x) { return sqrtl(x); }
  static PmLD trunc_(PmLD x) { return truncl(x); }
  static PmLD p2(int e) { return ldexpl(1.0L, e); }
};
template <> struct Ops<PmQ> {
  static const int F = 1;
  static PmQ get(uint64_t lo, uint64_t hi) { return get128(lo, hi); }
  static void put(PmQ v, uint64_t& lo, uint64_t& hi) { put128(v, lo, hi); }
  static PmQ sqrt_(PmQ x) { return sqrtq(x); }
  static PmQ trunc_(PmQ x) { return truncq(x); }
  static PmQ p2(int e) { return ldexpq(__float128(1), e); }
};
template <class T> static inline bool toint(T v, bool sgn, int bits, U& out) {
  // gecerli ise true (out = kesilmis deger, iki tamamlayan genislikte)
  if (v != v) return false;
  T t = Ops<T>::trunc_(v);
  if (t > Ops<T>::p2(200) || t < -Ops<T>::p2(200)) return false;   // sonsuz ya da cok buyuk
  if (sgn) {
    if (t < -Ops<T>::p2(bits - 1) || !(t < Ops<T>::p2(bits - 1))) return false;
    out = bits == 128 ? U(S(t)) : U(int64_t(t)); return true;
  }
  if (t < 0 || !(t < Ops<T>::p2(bits))) return false;
  out = bits == 128 ? U(t) : U(uint64_t(t)); return true;
}
template <class T> static inline void run(int code, int p2, uint64_t* R, int a, int b, int c) {
  const int F = Ops<T>::F;
  auto L = [&](int r) { return std::pair<uint64_t, uint64_t>(R[r], R[r + 1]); };
  switch (code) {
    case WFP_ADD: case WFP_SUB: case WFP_MUL: case WFP_DIV: {
      auto x = L(b), y = L(c); uint64_t rl, rh;
      if (isnanw(F, x.first, x.second)) quiet(F, x.first, x.second, rl, rh);
      else if (isnanw(F, y.first, y.second)) quiet(F, y.first, y.second, rl, rh);
      else {
        T p = Ops<T>::get(x.first, x.second), q = Ops<T>::get(y.first, y.second), r;
        switch (code) { case WFP_ADD: r = p + q; break; case WFP_SUB: r = p - q; break; case WFP_MUL: r = p * q; break; default: r = p / q; }
        if (r != r) defnan(F, rl, rh); else Ops<T>::put(r, rl, rh);
      }
      R[a] = rl; R[a + 1] = rh; break; }
    case WFP_SQRT: {
      auto x = L(b); uint64_t rl, rh;
      if (isnanw(F, x.first, x.second)) quiet(F, x.first, x.second, rl, rh);
      else { T r = Ops<T>::sqrt_(Ops<T>::get(x.first, x.second)); if (r != r) defnan(F, rl, rh); else Ops<T>::put(r, rl, rh); }
      R[a] = rl; R[a + 1] = rh; break; }
    case WFP_NEG: { uint64_t lo = R[b], hi = R[b + 1]; hi ^= F == 0 ? (1ULL << 15) : (1ULL << 63); R[a] = lo; R[a + 1] = hi; break; }
    case WFP_ABS: { uint64_t lo = R[b], hi = R[b + 1]; hi &= ~(F == 0 ? (1ULL << 15) : (1ULL << 63)); R[a] = lo; R[a + 1] = hi; break; }
    case WFP_MOV: { uint64_t lo = R[b], hi = R[b + 1]; R[a] = lo; R[a + 1] = hi; break; }
    case WFP_PACK: { uint64_t lo = R[b], hi = R[c]; if (F == 0) hi &= 0xFFFF; R[a] = lo; R[a + 1] = hi; break; }
    case WFP_LO: R[a] = R[b]; break;
    case WFP_HI: R[a] = F == 0 ? (R[b + 1] & 0xFFFF) : R[b + 1]; break;
    case WFP_EQ: case WFP_NE: case WFP_LT: case WFP_LE: {
      auto x = L(b), y = L(c); bool un = isnanw(F, x.first, x.second) || isnanw(F, y.first, y.second); bool r = false;
      if (un) r = code == WFP_NE;
      else { T p = Ops<T>::get(x.first, x.second), q = Ops<T>::get(y.first, y.second); r = code == WFP_EQ ? p == q : code == WFP_NE ? p != q : code == WFP_LT ? p < q : p <= q; }
      R[a] = r; break; }
    case WFP_TOBOOL: { auto x = L(b); R[a] = isnanw(F, x.first, x.second) ? 1 : (Ops<T>::get(x.first, x.second) != 0); break; }
    case WFP_FROM_F64: {
      uint64_t d = R[b]; uint64_t rl, rh;
      if ((d >> 52 & 0x7FF) == 0x7FF && (d & ((1ULL << 52) - 1))) {   // NaN: kesir yuku sola kaydirilir, sessiz bit
        uint64_t s = d >> 63, fr = d & ((1ULL << 52) - 1);
        if (F == 0) { rl = (fr << 11) | (1ULL << 62) | (1ULL << 63); rh = (s << 15) | 0x7FFF; }
        else { U f = U(fr) << 60; f |= U(1) << 111; rl = uint64_t(f); rh = (s << 63) | (0x7FFFULL << 48) | uint64_t(f >> 64); }
      } else { double dd; std::memcpy(&dd, &d, 8); Ops<T>::put(T(dd), rl, rh); }
      R[a] = rl; R[a + 1] = rh; break; }
    case WFP_TO_F64: {
      auto x = L(b); uint64_t o;
      if (isnanw(F, x.first, x.second)) {
        uint64_t s = F == 0 ? (x.second >> 15) & 1 : x.second >> 63;
        uint64_t fr = F == 0 ? ((x.first & ((1ULL << 63) - 1)) >> 11) : uint64_t((U(x.second & ((1ULL << 48) - 1)) << 64 | x.first) >> 60);
        o = (s << 63) | (0x7FFULL << 52) | fr | (1ULL << 51);
      } else { double dd = double(Ops<T>::get(x.first, x.second)); std::memcpy(&o, &dd, 8); }
      R[a] = o; break; }
    case WFP_TO_NARROW: {   // p2: 0 f32, 1 f16, 2 bf16 (PM_FMT sirasi)
      auto x = L(b); uint64_t o; const PmFmt& nf = PM_FMT[p2];
      uint64_t s = F == 0 ? (x.second >> 15) & 1 : x.second >> 63;
      if (isnanw(F, x.first, x.second)) {
        uint64_t fr = F == 0 ? ((x.first & ((1ULL << 63) - 1)) >> 11) : uint64_t((U(x.second & ((1ULL << 48) - 1)) << 64 | x.first) >> 60);
        int sh = 52 - nf.mb; fr = (fr >> sh) << sh; o = (s << 63) | (0x7FFULL << 52) | fr | (1ULL << 51);
      } else {
        unsigned ef; U m;
        if (F == 0) { ef = unsigned(x.second & 0x7FFF); m = x.first; if (ef == 0x7FFF) m = 0; }
        else { ef = unsigned((x.second >> 48) & 0x7FFF); m = (U(x.second & ((1ULL << 48) - 1)) << 64) | x.first; if (ef == 0x7FFF) m = 0; else if (ef) m |= U(1) << 112; }
        bool isinf = ef == 0x7FFF;
        double r;
        if (isinf) r = HUGE_VAL;
        else if (m == 0) r = 0.0;
        else { int E = (ef ? int(ef) : 1) - 16383 - (F == 0 ? 63 : 112); r = pm_round_core(m, E, nf); }
        uint64_t rb; std::memcpy(&rb, &r, 8); o = (s << 63) | rb;
      }
      R[a] = o; break; }
    case WFP_TO_OTHER: {   // f80 <-> f128 (p2 yok): kaynak bicim F
      auto x = L(b); uint64_t rl, rh;
      if (F == 0) {
        if (isnanw(0, x.first, x.second)) {
          uint64_t s = (x.second >> 15) & 1; U fr = U(x.first & ((1ULL << 63) - 1)) << 49; fr |= U(1) << 111;
          rl = uint64_t(fr); rh = (s << 63) | (0x7FFFULL << 48) | uint64_t(fr >> 64);
        } else Ops<PmQ>::put(PmQ(Ops<PmLD>::get(x.first, x.second)), rl, rh);
      } else {
        if (isnanw(1, x.first, x.second)) {
          uint64_t s = x.second >> 63; U fr = (U(x.second & ((1ULL << 48) - 1)) << 64 | x.first) >> 49; uint64_t f63 = uint64_t(fr) | (1ULL << 62);
          rl = f63 | (1ULL << 63); rh = (s << 15) | 0x7FFF;
        } else Ops<PmLD>::put(PmLD(Ops<PmQ>::get(x.first, x.second)), rl, rh);
      }
      R[a] = rl; R[a + 1] = rh; break; }
    case WFP_FROM_I64: { uint64_t rl, rh; Ops<T>::put(T(int64_t(R[b])), rl, rh); R[a] = rl; R[a + 1] = rh; break; }
    case WFP_FROM_U64: { uint64_t rl, rh; Ops<T>::put(T(uint64_t(R[b])), rl, rh); R[a] = rl; R[a + 1] = rh; break; }
    case WFP_FROM_I128: { U v = (U(R[b + 1]) << 64) | R[b]; uint64_t rl, rh; Ops<T>::put(T(S(v)), rl, rh); R[a] = rl; R[a + 1] = rh; break; }
    case WFP_FROM_U128: { U v = (U(R[b + 1]) << 64) | R[b]; uint64_t rl, rh; Ops<T>::put(T(v), rl, rh); R[a] = rl; R[a + 1] = rh; break; }
    case WFP_TO_INT_S: case WFP_TO_INT_U: {   // p2 = hedef bit (8,16,32,64,128)
      auto x = L(b); bool sg = code == WFP_TO_INT_S; int bits = p2 == 128 ? 128 : 64; U v = 0; bool ok = false;
      if (!isnanw(F, x.first, x.second)) ok = toint<T>(Ops<T>::get(x.first, x.second), sg, bits, v);
      if (!ok) v = sg ? (bits == 128 ? (U(1) << 127) : U(1) << 63) : U(0);
      if (p2 == 128) { R[a] = uint64_t(v); R[a + 1] = uint64_t(v >> 64); }
      else {
        uint64_t r = uint64_t(v);
        if (p2 == 8) r = sg ? uint64_t(int64_t(int8_t(r))) : uint64_t(uint8_t(r));
        else if (p2 == 16) r = sg ? uint64_t(int64_t(int16_t(r))) : uint64_t(uint16_t(r));
        else if (p2 == 32) r = sg ? uint64_t(int64_t(int32_t(r))) : uint64_t(uint32_t(r));
        R[a] = r;
      }
      break; }
  }
}
}  // namespace wfp
static inline int pm_wfp(int code, int p1, int p2, uint64_t* R, int a, int b, int c) {
  if (code == WFP_I128_TO_NARROW_S || code == WFP_I128_TO_NARROW_U) {
    wfp::U v = (wfp::U(R[b + 1]) << 64) | R[b]; bool neg = code == WFP_I128_TO_NARROW_S && wfp::S(v) < 0; wfp::U m = neg ? wfp::U(0) - v : v; double r;
    if (m == 0) r = 0.0; else if (p2 == 3) r = double(m); else r = pm_round_core(m, 0, PM_FMT[p2]);
    if (neg) r = -r; uint64_t o; std::memcpy(&o, &r, 8); R[a] = o; return 0;
  }
  if (code == WFP_F64_TO_I128_S || code == WFP_F64_TO_I128_U) {
    double d; std::memcpy(&d, &R[b], 8); bool sg = code == WFP_F64_TO_I128_S; wfp::U v = 0; bool ok = false;
    if (d == d) { double t = std::trunc(d); if (std::fabs(t) < 1e300) { if (sg) { if (t >= -std::ldexp(1.0, 127) && t < std::ldexp(1.0, 127)) { v = wfp::U(wfp::S(t)); ok = true; } } else if (t > -1.0 && t < std::ldexp(1.0, 128)) { v = wfp::U(t); ok = true; } } }
    if (!ok) v = sg ? (wfp::U(1) << 127) : wfp::U(0);
    R[a] = uint64_t(v); R[a + 1] = uint64_t(v >> 64); return 0;
  }
  if (p1 == 0) wfp::run<PmLD>(code, p2, R, a, b, c); else wfp::run<PmQ>(code, p2, R, a, b, c);
  return 0;
}
