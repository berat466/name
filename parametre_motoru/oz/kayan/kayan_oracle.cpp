// BAGIMSIZ ORACLE (derleyici turleriyle): long double (x87 genisletilmis) ve __float128 (IEEE binary128) islemleri. Motorun C++ referansindan (src/anlam_wfp.h) ayri yazilmistir.
// Sayisal sonuclar DERLEYICININ aritmetiginden gelir; yalniz motor tanimi olan kenar kurallari (NaN yayilimi, varsayilan NaN, kayan->tamsayi gecersiz degeri) burada ayrica uygulanir.
// Kullanim: oracle veri80|veri128|vergin N   -> PMA data satiri (virgullu isaretli 64 bit sozcuk cifleri)
//           oracle bin80|bin128 N    -> cift (i,j) basina: add sub mul div (lo hi) eq ne lt le
//           oracle un80|un128 N      -> deger basina: neg abs sqrt (lo hi) tobool, i64 u64 (i128 lo hi) (u128 lo hi) i8 u8 i16 u16 i32 u32, f64 f32 f16 f128/f80 (lo hi)
//           oracle dbl N / int N     -> double bit desenleri / 64 bit tamsayi listesi (veri) ; cd80|cd128 N: double->wide ; ci80|ci128 N: tamsayi->wide (i64 u64 i128 u128)
// Gereksinimler: g++ -std=gnu++20 (_Float16, __bf16), -lquadmath
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <quadmath.h>
typedef long double LD; typedef __float128 Q; typedef unsigned __int128 U; typedef __int128 S;
struct W { uint64_t lo, hi; };
static uint64_t rng = 0x9E3779B97F4A7C15ULL;
static uint64_t rnd() { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }
static LD toLD(W w) { LD v = 0; std::memcpy(&v, &w.lo, 8); uint16_t h = uint16_t(w.hi); std::memcpy(reinterpret_cast<char*>(&v) + 8, &h, 2); return v; }
static W fromLD(LD v) { W w; std::memcpy(&w.lo, &v, 8); uint16_t h; std::memcpy(&h, reinterpret_cast<char*>(&v) + 8, 2); w.hi = h; return w; }
static Q toQ(W w) { Q v; uint64_t a[2] = {w.lo, w.hi}; std::memcpy(&v, a, 16); return v; }
static W fromQ(Q v) { uint64_t a[2]; std::memcpy(a, &v, 16); return W{a[0], a[1]}; }
static bool isnan80(W w) { unsigned ef = w.hi & 0x7FFF; if (ef == 0x7FFF) return (w.lo << 1) != 0; return ef != 0 && !(w.lo >> 63); }
static bool isnan128(W w) { unsigned ef = (w.hi >> 48) & 0x7FFF; return ef == 0x7FFF && (((w.hi & ((1ULL << 48) - 1)) | w.lo) != 0); }
template <int F> static bool isnanw(W w) { return F == 0 ? isnan80(w) : isnan128(w); }
template <int F> static W quietw(W w) { if (F == 0) { w.lo |= 1ULL << 62; return w; } w.hi |= 1ULL << 47; return w; }
template <int F> static W defnanw() { return F == 0 ? W{0xC000000000000000ULL, 0xFFFF} : W{0, 0xFFFF800000000000ULL}; }
template <int F> static W put(typename std::conditional<F == 0, LD, Q>::type v) { return F == 0 ? fromLD(LD(v)) : fromQ(Q(v)); }
template <int F> struct T { typedef typename std::conditional<F == 0, LD, Q>::type type; static type get(W w) { if constexpr (F == 0) { if ((w.hi & 0x7FFF) == 0x7FFF && (w.lo << 1) == 0) w.lo = 1ULL << 63; return toLD(w); } else return toQ(w); } };
static bool bad(W w, bool f80) { return false; (void)w; (void)f80; }
// ---- deger uretimi (kanonik kodlamalar: f80'de ussu 0 ise tam sayi biti 0, ussu >0 ise 1; sahte sonsuz / sahte alt normal YOK; unnormal yalniz NaN sinamasi icin birkac tane)
template <int F> static std::vector<W> values(int n) {
  std::vector<W> v;
  auto mk = [&](int s, unsigned ef, U frac) -> W {   // frac: F==0 -> 63 bit kesir ; F==1 -> 112 bit kesir
    if (F == 0) { uint64_t m = uint64_t(frac) | (ef ? (1ULL << 63) : 0); if (ef == 0x7FFF) m = uint64_t(frac) | (1ULL << 63); return W{m, (uint64_t(s) << 15) | ef}; }
    return W{uint64_t(frac), (uint64_t(s) << 63) | (uint64_t(ef) << 48) | uint64_t(frac >> 64)};
  };
  const int KB = F == 0 ? 63 : 112; const U ONE = 1, FM = (ONE << KB) - 1;
  for (int s = 0; s < 2; s++) {
    v.push_back(mk(s, 0, 0)); v.push_back(mk(s, 0x7FFF, 0)); v.push_back(mk(s, 1, 0)); v.push_back(mk(s, 0x7FFE, FM)); v.push_back(mk(s, 16383, 0)); v.push_back(mk(s, 16383, ONE << (KB - 1)));
    v.push_back(mk(s, 0, 1)); v.push_back(mk(s, 0, FM)); v.push_back(mk(s, 16383 + 63, 0)); v.push_back(mk(s, 16383 + 64, 12345)); v.push_back(mk(s, 16383 + 112, 7)); v.push_back(mk(s, 16383 - 1, FM));
  }
  v.push_back(defnanw<F>()); v.push_back(mk(0, 0x7FFF, ONE << (KB - 1))); v.push_back(mk(1, 0x7FFF, 12345));
  if (F == 0) { v.push_back(W{0x0000000000000001ULL, 0x3FFF}); v.push_back(W{0x7FFFFFFFFFFFFFFFULL, 0x4005}); }   // unnormal (NaN sayilir)
  while (int(v.size()) < n) {
    if (v.size() > 30 && rnd() % 3 == 0) {   // komsu: oncekinin ussu/mantisi az degistirilmis (iptal, es-uzaklik, yapiskan bit sinamasi)
      W p = v.back(); if (!isnanw<F>(p)) {
        int dE = int(rnd() % 7) - 3; if (rnd() % 4 == 0) dE = -int(KB + 1 + rnd() % 3); if (rnd() % 8 == 0) dE = -int(2 * KB + rnd() % 5);
        unsigned ef0 = F == 0 ? (p.hi & 0x7FFF) : unsigned((p.hi >> 48) & 0x7FFF); int ef1 = int(ef0) + dE; if (ef1 < 1) ef1 = 1; if (ef1 > 0x7FFE) ef1 = 0x7FFE;
        U fr0 = F == 0 ? U(p.lo & ((1ULL << 63) - 1)) : ((U(p.hi & ((1ULL << 48) - 1)) << 64) | p.lo);
        if (rnd() % 2) fr0 ^= U(1) << (rnd() % KB); else if (rnd() % 2) fr0 = FM; 
        v.push_back(mk(int(rnd() % 2), unsigned(ef1), fr0 & FM)); continue; }
    }
    uint64_t r = rnd(); int s = r & 1; unsigned ef; int k = (r >> 1) % 10;
    if (k < 2) ef = unsigned(rnd() % 40); else if (k < 4) ef = unsigned(0x7FFE - rnd() % 40); else if (k < 8) ef = unsigned(16383 + int(rnd() % 301) - 150); else ef = unsigned(rnd() % 0x7FFF);
    U fr = ((U(rnd()) << 64) | rnd()) & FM; if (rnd() % 5 == 0) fr = (fr >> (rnd() % 64)) << (rnd() % 60);
    v.push_back(mk(s, ef, fr));
  }
  v.resize(n); return v;
}
static void pr(const char* sep, uint64_t x) { printf("%s%lld", sep, (long long)(int64_t)x); }
static void prW(W w) { printf(" %lld %lld", (long long)(int64_t)w.lo, (long long)(int64_t)w.hi); }
template <int F> static W binop(int code, W x, W y) {
  typedef typename T<F>::type X;
  if (isnanw<F>(x)) return quietw<F>(x); if (isnanw<F>(y)) return quietw<F>(y);
  X a = T<F>::get(x), b = T<F>::get(y), r;
  switch (code) { case 0: r = a + b; break; case 1: r = a - b; break; case 2: r = a * b; break; default: r = a / b; }
  if (r != r) return defnanw<F>(); return put<F>(r);
}
template <int F> static void bin(int n) {
  typedef typename T<F>::type X;
  auto v = values<F>(n);
  for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) {
    W x = v[i], y = v[j]; printf("%s", ""); bool first = true;
    for (int c = 0; c < 4; c++) { W r = binop<F>(c, x, y); if (!first) printf(" "); first = false; printf("%lld %lld", (long long)(int64_t)r.lo, (long long)(int64_t)r.hi); }
    bool un = isnanw<F>(x) || isnanw<F>(y); X a = T<F>::get(x), b = T<F>::get(y);
    int e = un ? 0 : a == b, ne = un ? 1 : a != b, lt = un ? 0 : a < b, le = un ? 0 : a <= b;
    printf(" %d %d %d %d\n", e, ne, lt, le);
  }
}
template <int F> static bool toint(typename T<F>::type v, bool sg, int bits, U& out) {
  typedef typename T<F>::type X;
  if (v != v) return false;
  X t; if constexpr (F == 0) t = truncl(v); else t = truncq(v);
  X lim = ldexp((X)1, 200); if constexpr (F == 1) lim = ldexpq((X)1, 200);
  if (t > lim || t < -lim) return false;
  auto p2 = [&](int e) -> X { if constexpr (F == 0) return ldexpl(1.0L, e); else return ldexpq((Q)1, e); };
  if (sg) { if (t < -p2(bits - 1) || !(t < p2(bits - 1))) return false; out = bits == 128 ? U(S(t)) : U(int64_t(t)); return true; }
  if (t < 0 || !(t < p2(bits))) return false; out = bits == 128 ? U(t) : U(uint64_t(t)); return true;
}
static uint64_t dbits(double d) { uint64_t b; std::memcpy(&b, &d, 8); return b; }
template <int F> static void un(int n) {
  typedef typename T<F>::type X;
  auto v = values<F>(n);
  for (int i = 0; i < n; i++) {
    W x = v[i]; bool nan = isnanw<F>(x); X a = T<F>::get(x);
    W ng = x; ng.hi ^= F == 0 ? (1ULL << 15) : (1ULL << 63); W ab = x; ab.hi &= ~(F == 0 ? (1ULL << 15) : (1ULL << 63));
    prW(ng); prW(ab);
    W sq; if (nan) sq = quietw<F>(x); else { X r; if constexpr (F == 0) r = sqrtl(a); else r = sqrtq(a); sq = (r != r) ? defnanw<F>() : put<F>(r); } prW(sq);
    printf(" %d", nan ? 1 : (a != 0));
    for (int k = 0; k < 4; k++) {   // i64 u64 i128 u128
      bool sg = k % 2 == 0; int bits = k < 2 ? 64 : 128; U o = 0; bool ok = !nan && toint<F>(a, sg, bits, o);
      if (!ok) o = sg ? (bits == 128 ? (U(1) << 127) : (U(1) << 63)) : 0;
      if (bits == 64) printf(" %lld", (long long)(int64_t)uint64_t(o)); else prW(W{uint64_t(o), uint64_t(o >> 64)});
    }
    for (int nb : {8, 16, 32}) for (int sg = 1; sg >= 0; sg--) {   // i8 u8 i16 u16 i32 u32 (once 64 bit, sonra sar)
      U o = 0; bool ok = !nan && toint<F>(a, sg, 64, o); if (!ok) o = sg ? (U(1) << 63) : 0; uint64_t r = uint64_t(o);
      if (nb == 8) r = sg ? uint64_t(int64_t(int8_t(r))) : uint64_t(uint8_t(r)); else if (nb == 16) r = sg ? uint64_t(int64_t(int16_t(r))) : uint64_t(uint16_t(r)); else r = sg ? uint64_t(int64_t(int32_t(r))) : uint64_t(uint32_t(r));
      printf(" %lld", (long long)(int64_t)r);
    }
    // f64 f32 f16 bf16: NaN yuku kisaltilir (motor tanimi) ; sonuc double bit deseni
    auto nanbits = [&](int keep) -> uint64_t {
      uint64_t s = F == 0 ? (x.hi >> 15) & 1 : x.hi >> 63;
      uint64_t fr = F == 0 ? ((x.lo & ((1ULL << 63) - 1)) >> 11) : uint64_t((((U(x.hi & ((1ULL << 48) - 1)) << 64) | x.lo)) >> 60);
      int sh = 52 - keep; fr = (fr >> sh) << sh; return (s << 63) | (0x7FFULL << 52) | fr | (1ULL << 51); };
    pr(" ", nan ? nanbits(52) : dbits(double(a)));
    pr(" ", nan ? nanbits(23) : dbits(double(float(a))));
    pr(" ", nan ? nanbits(10) : dbits(double(float(_Float16(a)))));
    pr(" ", nan ? nanbits(7) : dbits(double(float(__bf16(a)))));
    // f80 <-> f128
    if (F == 0) {
      W o; if (nan) { uint64_t s = (x.hi >> 15) & 1; U fr = U(x.lo & ((1ULL << 63) - 1)) << 49; fr |= U(1) << 111; o = W{uint64_t(fr), (s << 63) | (0x7FFFULL << 48) | uint64_t(fr >> 64)}; } else o = fromQ(Q(a));
      prW(o);
    } else {
      W o; if (nan) { uint64_t s = x.hi >> 63; U fr = (U(x.hi & ((1ULL << 48) - 1)) << 64 | x.lo) >> 49; o = W{uint64_t(fr) | (1ULL << 62) | (1ULL << 63), (s << 15) | 0x7FFF}; } else o = fromLD(LD(a));
      prW(o);
    }
    printf("\n");
  }
}
template <int F> static void cd(int n) {   // double -> wide
  rng = 0x1234567ULL; 
  for (int i = 0; i < n; i++) {
    uint64_t b; switch (i) { case 0: b = 0; break; case 1: b = 1ULL << 63; break; case 2: b = 0x7FF0000000000000ULL; break; case 3: b = 0xFFF0000000000000ULL; break; case 4: b = 0x7FF8000000000000ULL; break; case 5: b = 0xFFF0000000000001ULL; break;
      case 6: b = 1; break; case 7: b = 0x000FFFFFFFFFFFFFULL; break; case 8: b = 0x0010000000000000ULL; break; case 9: b = 0x7FEFFFFFFFFFFFFFULL; break; case 10: b = 0x3FF0000000000000ULL; break;
      default: { uint64_t r = rnd(); b = r; if (i % 3 == 0) b = (b & ~(0x7FFULL << 52)) | (uint64_t(1023 + int(rnd() % 120) - 60) << 52); } }
    double d; std::memcpy(&d, &b, 8); W o;
    if ((b >> 52 & 0x7FF) == 0x7FF && (b & ((1ULL << 52) - 1))) {
      uint64_t s = b >> 63, fr = b & ((1ULL << 52) - 1);
      if (F == 0) o = W{(fr << 11) | (1ULL << 62) | (1ULL << 63), (s << 15) | 0x7FFF};
      else { U f = U(fr) << 60; f |= U(1) << 111; o = W{uint64_t(f), (s << 63) | (0x7FFFULL << 48) | uint64_t(f >> 64)}; }
    } else o = put<F>(typename T<F>::type(d));
    prW(o); printf("\n");
  }
}
template <int F> static void ci(int n) {   // tamsayi -> wide
  rng = 0xABCDEF1ULL;
  for (int i = 0; i < n; i++) {
    uint64_t b; switch (i) { case 0: b = 0; break; case 1: b = 1; break; case 2: b = ~0ULL; break; case 3: b = 1ULL << 63; break; case 4: b = (1ULL << 63) - 1; break; case 5: b = (1ULL << 53) + 1; break; case 6: b = (1ULL << 63) + 1; break;
      default: b = rnd() >> (rnd() % 64); }
    U w128 = (U(rnd()) << 64) | b; if (i % 2) w128 = (U(rnd()) << 64 | rnd()) >> (rnd() % 128); if (i == 2) w128 = ~U(0); if (i == 3) w128 = U(1) << 127; if (i == 0) w128 = 0;
    typedef typename T<F>::type X;
    prW(put<F>(X(int64_t(b)))); prW(put<F>(X(b))); prW(put<F>(X(S(w128)))); prW(put<F>(X(w128))); printf("\n");
  }
}
template <int F> static void veri(int n) {
  auto v = values<F>(n); std::string s;
  for (size_t i = 0; i < v.size(); i++) { char b[64]; snprintf(b, 64, "%s%lld,%lld", i ? "," : "", (long long)(int64_t)v[i].lo, (long long)(int64_t)v[i].hi); s += b; }
  puts(s.c_str());
}
int main(int argc, char** argv) {
  if (argc < 3) return 2; std::string m = argv[1]; int n = atoi(argv[2]);
  if (m == "veri80") veri<0>(n); else if (m == "veri128") veri<1>(n);
  else if (m == "bin80") bin<0>(n); else if (m == "bin128") bin<1>(n);
  else if (m == "un80") un<0>(n); else if (m == "un128") un<1>(n);
  else if (m == "cd80") cd<0>(n); else if (m == "cd128") cd<1>(n);
  else if (m == "ci80") ci<0>(n); else if (m == "ci128") ci<1>(n);
  else if (m == "dbl" || m == "int") {   // veri: double bit desenleri / 64 bit tamsayilar (cd / ci ile ayni uretim)
    std::string s; rng = m == "dbl" ? 0x1234567ULL : 0xABCDEF1ULL;
    for (int i = 0; i < n; i++) {
      uint64_t b;
      if (m == "dbl") { switch (i) { case 0: b = 0; break; case 1: b = 1ULL << 63; break; case 2: b = 0x7FF0000000000000ULL; break; case 3: b = 0xFFF0000000000000ULL; break; case 4: b = 0x7FF8000000000000ULL; break; case 5: b = 0xFFF0000000000001ULL; break;
        case 6: b = 1; break; case 7: b = 0x000FFFFFFFFFFFFFULL; break; case 8: b = 0x0010000000000000ULL; break; case 9: b = 0x7FEFFFFFFFFFFFFFULL; break; case 10: b = 0x3FF0000000000000ULL; break;
        default: { uint64_t r = rnd(); b = r; if (i % 3 == 0) b = (b & ~(0x7FFULL << 52)) | (uint64_t(1023 + int(rnd() % 120) - 60) << 52); } } }
      else { switch (i) { case 0: b = 0; break; case 1: b = 1; break; case 2: b = ~0ULL; break; case 3: b = 1ULL << 63; break; case 4: b = (1ULL << 63) - 1; break; case 5: b = (1ULL << 53) + 1; break; case 6: b = (1ULL << 63) + 1; break;
        default: b = rnd() >> (rnd() % 64); }
        U w128 = (U(rnd()) << 64) | b; if (i % 2) w128 = (U(rnd()) << 64 | rnd()) >> (rnd() % 128); if (i == 2) w128 = ~U(0); if (i == 3) w128 = U(1) << 127; if (i == 0) w128 = 0;
        char bb[96]; snprintf(bb, 96, "%s%lld,%lld,%lld", i ? "," : "", (long long)(int64_t)b, (long long)(int64_t)uint64_t(w128), (long long)(int64_t)uint64_t(w128 >> 64)); s += bb; continue; }
      char bb[64]; snprintf(bb, 64, "%s%lld", i ? "," : "", (long long)(int64_t)b); s += bb;
    }
    puts(s.c_str());
  } else return 2;
  return 0;
}
