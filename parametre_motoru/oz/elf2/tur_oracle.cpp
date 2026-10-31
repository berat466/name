// BAGIMSIZ ORACLE (Plan Bolum 01): tamsayi aileleri icin beklenen ciktilari yerel C++ turleriyle (int8_t .. uint64_t) hesaplar. Motorun hicbir parcasini kullanmaz.
//   tur_oracle veri N              : N tane deneme degeri (int64) satiri (PMA `data tab = ...` icin)
//   tur_oracle dm W s              : W bit, s=1 isaretli / 0 isaretsiz kucuk tur ailesi: tum (x,y) ciftleri icin add sub mul [div mod] neg shl (not) sonuclari + donusumler
//   tur_oracle u64 N               : uint64 islemleri (divu modu ltu leu gtu geu) tum ciftler
//   tur_oracle u2f N / f2u         : uint64 <-> double (bit deseni olarak)
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
using namespace std;
static vector<int64_t> degerler(int n) {
  vector<int64_t> v = {0, 1, 2, 3, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128, 129, 255, 256, 257, 32767, 32768, 32769, 65535, 65536, 65537, 2147483647LL, 2147483648LL, 2147483649LL, 4294967295LL, 4294967296LL,
    -1, -2, -3, -7, -8, -127, -128, -129, -255, -256, -257, -32767, -32768, -32769, -65535, -65536, -2147483647LL, -2147483648LL, -2147483649LL, -4294967295LL, -4294967296LL,
    INT64_MAX, INT64_MAX - 1, INT64_MIN, INT64_MIN + 1, 0x7F7F7F7F7F7F7F7FLL, (int64_t)0x8080808080808080ULL, 0x00FF00FF00FF00FFLL, (int64_t)0xFFFF0000FFFF0000ULL, 1000000007, -1000000007,
    9007199254740993LL, 9007199254740995LL, (int64_t)0x8000000000000401ULL, (int64_t)0x8000000000000C01ULL, (int64_t)0xFFFFFFFFFFFFFBFFULL, (int64_t)0xFFFFFFFFFFFFFFFFULL};   // 2^53+1 / 2^63+1025: u2f'de baglanti yuvarlama (yapiskan bit) siniri
  uint64_t s = 88172645463325252ULL;
  for (int i = 0; i < 24; i++) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; v.push_back((int64_t)s); }
  if (n > 1 && (size_t)n < v.size()) { vector<int64_t> w; for (int i = 0; i < n; i++) w.push_back(v[(size_t)i * (v.size() - 1) / (n - 1)]); v = w; }   // n < tum liste: esit aralikli ornek (hem kucuk hem buyuk degerler, ilk ve son dahil)
  else if (n == 1) v.resize(1);
  return v;
}
template <class T> static void aile(int64_t xi, int64_t yi, int64_t raw_y, bool sg) {
  T x = (T)xi, y = (T)yi; int64_t X = (int64_t)x, Y = (int64_t)y;
  uint64_t UX = (uint64_t)X, c = (uint64_t)raw_y & 63;
  printf("%lld %lld %lld ", (long long)(T)(X + Y), (long long)(T)(X - Y), (long long)(T)((uint64_t)X * (uint64_t)Y));
  if (sg) { if (Y != 0) printf("%lld %lld ", (long long)(T)(X / Y), (long long)(T)(X % Y)); else printf("- - "); }
  printf("%lld %lld", (long long)(T)(0 - (uint64_t)X), (long long)(T)(UX << c));
  if (!sg) printf(" %lld", (long long)(T)(~UX));
  printf("\n");
}
int main(int argc, char** argv) {
  if (argc < 2) return 2;
  string m = argv[1];
  if (m == "veri") { auto v = degerler(argc > 2 ? atoi(argv[2]) : 0); for (size_t i = 0; i < v.size(); i++) printf("%s%lld", i ? ", " : "", (long long)v[i]); printf("\n"); return 0; }
  if (m == "dm") {
    int W = atoi(argv[2]); bool sg = atoi(argv[3]); auto v = degerler(argc > 4 ? atoi(argv[4]) : 0);
    // 1) donusumler: sextW / zextW
    for (auto x : v) {
      if (W == 8) printf("%lld %lld\n", (long long)(int8_t)x, (long long)(uint8_t)x);
      if (W == 16) printf("%lld %lld\n", (long long)(int16_t)x, (long long)(uint16_t)x);
      if (W == 32) printf("%lld %lld\n", (long long)(int32_t)x, (long long)(uint32_t)x);
    }
    // 2) ikili ve tekli islemler (girdiler once sextW / zextW ile normallestirilir)
    for (auto x : v) for (auto y : v) {
      if (sg) { if (W == 8) aile<int8_t>(x, y, y, true); if (W == 16) aile<int16_t>(x, y, y, true); if (W == 32) aile<int32_t>(x, y, y, true); }
      else { if (W == 8) aile<uint8_t>(x, y, y, false); if (W == 16) aile<uint16_t>(x, y, y, false); if (W == 32) aile<uint32_t>(x, y, y, false); }
    }
    return 0;
  }
  if (m == "u64") {
    auto v = degerler(argc > 2 ? atoi(argv[2]) : 0);
    for (auto a : v) for (auto b : v) {
      uint64_t x = (uint64_t)a, y = (uint64_t)b;
      if (y) printf("%lld %lld ", (long long)(int64_t)(x / y), (long long)(int64_t)(x % y)); else printf("- - ");
      printf("%d %d %d %d\n", x < y, x <= y, x > y, x >= y);
    }
    return 0;
  }
  if (m == "u2f") {
    auto v = degerler(argc > 2 ? atoi(argv[2]) : 0);
    for (auto a : v) { double d = (double)(uint64_t)a; int64_t b; memcpy(&b, &d, 8); printf("%lld\n", (long long)b); }
    return 0;
  }
  if (m == "fveri") {   // f2u girdi desenleri (double bit deseni)
    double d[] = {0.0, -0.0, 0.5, 1.0, 1.5, 2.0, 1e9, 4294967296.0, 4503599627370496.0, 9007199254740992.0, 9223372036854774784.0, 9223372036854775808.0, 9223372036854777856.0, 1.8446744073709549568e19, 1.8446744073709551616e19, 1e30, 1e300, -0.5, -0.99999, -1.0, -1e30, INFINITY, -INFINITY, NAN, 123456789012345678.0, 3.0e18};
    for (size_t i = 0; i < sizeof d / sizeof *d; i++) { int64_t b; memcpy(&b, &d[i], 8); printf("%s%lld", i ? ", " : "", (long long)b); }
    printf("\n"); return 0;
  }
  if (m == "f2u") {
    double d[] = {0.0, -0.0, 0.5, 1.0, 1.5, 2.0, 1e9, 4294967296.0, 4503599627370496.0, 9007199254740992.0, 9223372036854774784.0, 9223372036854775808.0, 9223372036854777856.0, 1.8446744073709549568e19, 1.8446744073709551616e19, 1e30, 1e300, -0.5, -0.99999, -1.0, -1e30, INFINITY, -INFINITY, NAN, 123456789012345678.0, 3.0e18};
    for (double f : d) {
      uint64_t r;
      if (f != f) r = 0; else if (f <= -1.0) r = 0; else if (f >= 18446744073709551616.0) r = UINT64_MAX; else r = (uint64_t)f;
      printf("%lld\n", (long long)(int64_t)r);
    }
    return 0;
  }
  return 2;
}
