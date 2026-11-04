// Plan Bolum 01: karmasik sayilar c64 (iki f32), c128 (iki f64), c160 (iki f80). C++ gercekleme DERLEYICININ float/double/long double turleriyle (bilesen bilesen, CX_LIMITED_RANGE duz formuller);
// c160 bilesenleri icin f80 referansi (anlam_wfp.h) kullanilir (NaN kurallari motor tanimi). Yazmac temsili: c64/c128 = 2 sozcuk (gercek, sanal; c64'te f32 degeri double kabinda), c160 = 4 sozcuk.
#pragma once
#include "anlam_wfp.h"
enum WCXK { WCX_ADD, WCX_SUB, WCX_MUL, WCX_DIV, WCX_NEG, WCX_CONJ, WCX_MOV, WCX_EQ, WCX_NE, WCX_TOBOOL, WCX_MK, WCX_REAL, WCX_IMAG, WCX_CVT, WCX_FROM_REAL };
namespace wcx {
static inline double dasF(uint64_t b) { double d; std::memcpy(&d, &b, 8); return d; }
static inline uint64_t dfromF(double d) { uint64_t b; std::memcpy(&b, &d, 8); return b; }
template <class T> static inline void cop(int code, T a, T b, T c, T d, T& rr, T& ri) {
  switch (code) {
    case WCX_ADD: rr = a + c; ri = b + d; break;
    case WCX_SUB: rr = a - c; ri = b - d; break;
    case WCX_MUL: { T p1 = a * c, p2 = b * d, p3 = a * d, p4 = b * c; rr = p1 - p2; ri = p3 + p4; break; }
    default: { T dn = c * c + d * d; T nr = a * c + b * d; T ni = b * c - a * d; rr = nr / dn; ri = ni / dn; }
  }
}
// f80 bilesen islemi (motor kurallariyla): x, y 2 sozcuk -> sonuc 2 sozcuk
static inline void w80(int code, const uint64_t* x, const uint64_t* y, uint64_t* out) {
  uint64_t R[6] = {x[0], x[1], y[0], y[1], 0, 0}; wfp::run<PmLD>(code, 0, R, 4, 0, 2); out[0] = R[4]; out[1] = R[5];
}
static inline int run(int code, int ty, int p2, uint64_t* R, int a, int b, int c) {
  if (ty == 2) {   // c160: 4 sozcuk
    auto X = [&](int r, int k) { return R + r + k * 2; };
    uint64_t t[4][2]; uint64_t res[4];
    switch (code) {
      case WCX_ADD: case WCX_SUB: { int op = code == WCX_ADD ? WFP_ADD : WFP_SUB; w80(op, X(b, 0), X(c, 0), t[0]); w80(op, X(b, 1), X(c, 1), t[1]); res[0] = t[0][0]; res[1] = t[0][1]; res[2] = t[1][0]; res[3] = t[1][1]; break; }
      case WCX_MUL: {
        uint64_t p1[2], p2_[2], p3[2], p4[2], rr[2], ri[2];
        w80(WFP_MUL, X(b, 0), X(c, 0), p1); w80(WFP_MUL, X(b, 1), X(c, 1), p2_); w80(WFP_MUL, X(b, 0), X(c, 1), p3); w80(WFP_MUL, X(b, 1), X(c, 0), p4);
        w80(WFP_SUB, p1, p2_, rr); w80(WFP_ADD, p3, p4, ri); res[0] = rr[0]; res[1] = rr[1]; res[2] = ri[0]; res[3] = ri[1]; break; }
      case WCX_DIV: {
        uint64_t p1[2], p2_[2], dn[2], p3[2], p4[2], nr[2], p5[2], p6[2], ni[2], rr[2], ri[2];
        w80(WFP_MUL, X(c, 0), X(c, 0), p1); w80(WFP_MUL, X(c, 1), X(c, 1), p2_); w80(WFP_ADD, p1, p2_, dn);
        w80(WFP_MUL, X(b, 0), X(c, 0), p3); w80(WFP_MUL, X(b, 1), X(c, 1), p4); w80(WFP_ADD, p3, p4, nr);
        w80(WFP_MUL, X(b, 1), X(c, 0), p5); w80(WFP_MUL, X(b, 0), X(c, 1), p6); w80(WFP_SUB, p5, p6, ni);
        w80(WFP_DIV, nr, dn, rr); w80(WFP_DIV, ni, dn, ri); res[0] = rr[0]; res[1] = rr[1]; res[2] = ri[0]; res[3] = ri[1]; break; }
      case WCX_NEG: res[0] = R[b]; res[1] = R[b + 1] ^ 0x8000; res[2] = R[b + 2]; res[3] = R[b + 3] ^ 0x8000; break;
      case WCX_CONJ: res[0] = R[b]; res[1] = R[b + 1]; res[2] = R[b + 2]; res[3] = R[b + 3] ^ 0x8000; break;
      case WCX_MOV: for (int k = 0; k < 4; k++) res[k] = R[b + k]; break;
      case WCX_EQ: case WCX_NE: {
        uint64_t Q[6]; Q[0] = R[b]; Q[1] = R[b + 1]; Q[2] = R[c]; Q[3] = R[c + 1]; wfp::run<PmLD>(WFP_EQ, 0, Q, 4, 0, 2); uint64_t e1 = Q[4];
        Q[0] = R[b + 2]; Q[1] = R[b + 3]; Q[2] = R[c + 2]; Q[3] = R[c + 3]; wfp::run<PmLD>(WFP_EQ, 0, Q, 4, 0, 2); uint64_t e2 = Q[4];
        uint64_t e = e1 & e2; R[a] = code == WCX_EQ ? e : (e ^ 1); return 0; }
      case WCX_TOBOOL: { uint64_t Q[4] = {R[b], R[b + 1], 0, 0}; wfp::run<PmLD>(WFP_TOBOOL, 0, Q, 2, 0, 0); uint64_t n1 = Q[2]; uint64_t Q2[4] = {R[b + 2], R[b + 3], 0, 0}; wfp::run<PmLD>(WFP_TOBOOL, 0, Q2, 2, 0, 0); R[a] = n1 | Q2[2]; return 0; }
      case WCX_MK: res[0] = R[b]; res[1] = R[b + 1] & 0xFFFF; res[2] = R[c]; res[3] = R[c + 1] & 0xFFFF; break;
      case WCX_REAL: R[a] = R[b]; R[a + 1] = R[b + 1]; return 0;
      case WCX_IMAG: R[a] = R[b + 2]; R[a + 1] = R[b + 3]; return 0;
      default: return 0;
    }
    for (int k = 0; k < 4; k++) R[a + k] = res[k];
    return 0;
  }
  // c64 (float) ve c128 (double): bilesenler double kabinda
  auto get = [&](int r, int k) -> double { return dasF(R[r + k]); };
  auto fl = [&](double x) { return ty == 0 ? double(float(x)) : x; };
  switch (code) {
    case WCX_ADD: case WCX_SUB: case WCX_MUL: case WCX_DIV: {
      double rr, ri;
      if (ty == 0) { float x, y; cop<float>(code, float(get(b, 0)), float(get(b, 1)), float(get(c, 0)), float(get(c, 1)), x, y); rr = x; ri = y; }
      else cop<double>(code, get(b, 0), get(b, 1), get(c, 0), get(c, 1), rr, ri);
      R[a] = dfromF(rr); R[a + 1] = dfromF(ri); break; }
    case WCX_NEG: { uint64_t x = R[b] ^ (1ULL << 63), y = R[b + 1] ^ (1ULL << 63); R[a] = x; R[a + 1] = y; break; }
    case WCX_CONJ: { uint64_t x = R[b], y = R[b + 1] ^ (1ULL << 63); R[a] = x; R[a + 1] = y; break; }
    case WCX_MOV: { uint64_t x = R[b], y = R[b + 1]; R[a] = x; R[a + 1] = y; break; }
    case WCX_EQ: R[a] = get(b, 0) == get(c, 0) && get(b, 1) == get(c, 1); break;
    case WCX_NE: R[a] = !(get(b, 0) == get(c, 0) && get(b, 1) == get(c, 1)); break;
    case WCX_TOBOOL: R[a] = !(get(b, 0) == 0 && get(b, 1) == 0); break;
    case WCX_MK: { uint64_t x = dfromF(fl(dasF(R[b]))), y = dfromF(fl(dasF(R[c]))); R[a] = x; R[a + 1] = y; break; }
    case WCX_REAL: R[a] = R[b]; break;
    case WCX_IMAG: R[a] = R[b + 1]; break;
    default: break;
  }
  return 0;
}
// donusumler: p2 = hedef tur (0 c64, 1 c128, 2 c160) ; kaynak tur = ty
static inline int cvt(int ty, int dst, uint64_t* R, int a, int b) {
  auto narrow = [&](uint64_t x) { return dfromF(double(float(dasF(x)))); };   // f64 kabi -> f32 degerine yuvarla (NaN yuku icin pm_rndfmt)
  auto to80 = [&](uint64_t d, uint64_t* o) { uint64_t Q[4] = {d, 0, 0, 0}; wfp::run<PmLD>(WFP_FROM_F64, 0, Q, 2, 0, 0); o[0] = Q[2]; o[1] = Q[3]; };
  auto from80 = [&](const uint64_t* x, int how) -> uint64_t { uint64_t Q[4] = {x[0], x[1], 0, 0}; wfp::run<PmLD>(how == 0 ? WFP_TO_F64 : WFP_TO_NARROW, 0, Q, 2, 0, 0); return Q[2]; };
  uint64_t o[4];
  if (ty == 2) {
    if (dst == 2) { for (int k = 0; k < 4; k++) o[k] = R[b + k]; }
    else { int how = dst == 1 ? 0 : 1; o[0] = from80(R + b, how); o[1] = from80(R + b + 2, how); if (dst == 0) { o[0] = o[0]; } }
  } else if (dst == 2) { uint64_t t[2]; to80(R[b], t); uint64_t u[2]; to80(R[b + 1], u); o[0] = t[0]; o[1] = t[1]; o[2] = u[0]; o[3] = u[1]; }
  else if (ty == 0 && dst == 1) { o[0] = R[b]; o[1] = R[b + 1]; }
  else if (ty == 1 && dst == 0) { o[0] = pm_f64_to_f32bits(R[b]); o[1] = pm_f64_to_f32bits(R[b + 1]); }
  else { o[0] = R[b]; o[1] = R[b + 1]; }
  (void)narrow;
  int nw = dst == 2 ? 4 : 2; for (int k = 0; k < nw; k++) R[a + k] = o[k];
  return 0;
}
}  // namespace wcx
static inline int pm_wcx(int code, int p1, int p2, uint64_t* R, int a, int b, int c) {
  if (code == WCX_CVT) return wcx::cvt(p1, p2, R, a, b);
  if (code == WCX_FROM_REAL) {   // p1 = hedef tur ; p2 = kaynak (0: f64 kabi, 1: f80 genis)
    uint64_t re[2] = {R[b], 0}; if (p2 == 1) re[1] = R[b + 1];
    uint64_t out[4];
    if (p2 == 0) {
      if (p1 == 2) { uint64_t Q[4] = {re[0], 0, 0, 0}; wfp::run<PmLD>(WFP_FROM_F64, 0, Q, 2, 0, 0); out[0] = Q[2]; out[1] = Q[3]; out[2] = 0; out[3] = 0; }
      else if (p1 == 0) { out[0] = pm_f64_to_f32bits(re[0]); out[1] = 0; }
      else { out[0] = re[0]; out[1] = 0; }
    } else {
      if (p1 == 2) { out[0] = re[0]; out[1] = re[1]; out[2] = 0; out[3] = 0; }
      else { uint64_t Q[4] = {re[0], re[1], 0, 0}; wfp::run<PmLD>(p1 == 1 ? WFP_TO_F64 : WFP_TO_NARROW, 0, Q, 2, 0, 0); out[0] = Q[2]; out[1] = 0; }
    }
    int nw = p1 == 2 ? 4 : 2; for (int k = 0; k < nw; k++) R[a + k] = out[k];
    return 0;
  }
  return wcx::run(code, p1, p2, R, a, b, c);
}
