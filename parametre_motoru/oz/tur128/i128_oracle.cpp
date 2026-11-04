// BAGIMSIZ ORACLE (Plan Bolum 01, 128 bit tamsayilar): beklenen ciktilar derleyicinin __int128 / unsigned __int128 turleriyle. Motorun hicbir parcasini kullanmaz.
//   i128_oracle veri N          : N tane 128 bit deger (dusuk, yuksek sozcuk cifti, virgullu isaretli int64) -- PMA `data` satiri icin
//   i128_oracle cift N          : tum (x,y) ciftleri icin: add sub mul and or xor (i ve u adlariyla ayni) eq ne lt_i le_i lt_u le_u
//   i128_oracle tek N           : her x icin: neg not, kaydirmalar (sayac kumesi), donusumler
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
using namespace std;
typedef unsigned __int128 U; typedef __int128 S;
static vector<U> degerler(int n) {
  vector<U> v;
  auto P = [&](unsigned long long hi, unsigned long long lo) { v.push_back((U(hi) << 64) | lo); };
  P(0, 0); P(0, 1); P(~0ULL, ~0ULL); P(0, 0x7FFFFFFFFFFFFFFFULL); P(0, 0x8000000000000000ULL); P(0, ~0ULL); P(1, 0); P(1, 1); P(0x7FFFFFFFFFFFFFFFULL, ~0ULL); P(0x8000000000000000ULL, 0);
  P(0x8000000000000000ULL, 1); P(~0ULL, 0); P(~0ULL, 0x8000000000000000ULL); P(0, 0xFFFFFFFFULL); P(0, 0x100000000ULL); P(2, 3); P(0xDEADBEEFCAFEBABEULL, 0x0123456789ABCDEFULL);
  P(0x0000000100000000ULL, 0xFFFFFFFF00000000ULL); P(0x7F7F7F7F7F7F7F7FULL, 0x8080808080808080ULL); P(0, 0x80); P(0, 0xFF); P(0, 0x7F); P(~0ULL, ~0ULL - 0x7E); P(~0ULL, 0x8000000000000000ULL - 1);
  unsigned long long s = 88172645463325252ULL;
  while ((int)v.size() < 40) { s ^= s << 13; s ^= s >> 7; s ^= s << 17; unsigned long long a = s; s ^= s << 13; s ^= s >> 7; s ^= s << 17; P(a >> (s % 40), s >> (a % 50)); }
  if (n > 1 && (size_t)n < v.size()) { vector<U> w; for (int i = 0; i < n; i++) w.push_back(v[(size_t)i * (v.size() - 1) / (n - 1)]); v = w; }
  return v;
}
static void pr(U x) { printf(" %lld %lld", (long long)(uint64_t)x, (long long)(uint64_t)(x >> 64)); }
static const int SAY[] = {0, 1, 5, 31, 32, 63, 64, 65, 100, 127, 128, 200, -1};
int main(int argc, char** argv) {
  string m = argc > 1 ? argv[1] : ""; int n = argc > 2 ? atoi(argv[2]) : 0; auto v = degerler(n);
  if (m == "veri") { for (size_t i = 0; i < v.size(); i++) printf("%s%lld, %lld", i ? ", " : "", (long long)(uint64_t)v[i], (long long)(uint64_t)(v[i] >> 64)); printf("\n"); return 0; }
  if (m == "cift") {
    for (U x : v) for (U y : v) {
      for (int k = 0; k < 2; k++) { pr(x + y); pr(x - y); pr(x * y); pr(x & y); pr(x | y); pr(x ^ y); }
      printf(" %d %d %d %d %d %d", (int)(x == y), (int)(x != y), (int)(S(x) < S(y)), (int)(S(x) <= S(y)), (int)(x < y), (int)(x <= y));
      if (y != 0) {   // bolme/kalan (y != 0); isaretli: buyukluk uzerinden, INT128_MIN / -1 sarar (motor tanimi: bolum INT128_MIN, kalan 0)
        printf(" |");
        U ax = S(x) < 0 ? U(0) - x : x, ay = S(y) < 0 ? U(0) - y : y; U q = ax / ay, r = ax % ay;
        if ((S(x) < 0) != (S(y) < 0)) q = U(0) - q; if (S(x) < 0) r = U(0) - r;
        pr(q); pr(r); pr(x / y); pr(x % y);
      }
      printf("\n");
    }
    return 0;
  }
  if (m == "tek") {
    for (U x : v) {
      for (int k = 0; k < 2; k++) { pr(U(0) - x); pr(~x); }
      for (int c : SAY) { int k = c < 0 ? 77 : c; printf(" |"); pr(x << (k & 127)); pr(x << (k & 127)); pr(x >> (k & 127)); pr(U(S(x) >> (k & 127))); }
      printf(" |"); pr(U(S(int64_t(uint64_t(x))))); pr(U(uint64_t(x))); pr(U(S(int64_t(uint64_t(x))))); pr(U(uint64_t(x)));   // sext64_i128 zext64_i128 sext64_u128 zext64_u128 (dusuk sozcuk)
      printf(" |");
      for (int k = 0; k < 2; k++) printf(" %lld %lld %lld %lld %lld %lld %lld %lld %d %lld", (long long)(int8_t)x, (long long)(int16_t)x, (long long)(int32_t)x, (long long)(int64_t)x,
             (long long)(uint8_t)x, (long long)(uint16_t)x, (long long)(uint32_t)x, (long long)(uint64_t)x, (int)(x != 0), (long long)(uint64_t)(x >> 64));
      printf(" |");
      for (int k = 0; k < 2; k++) { pr(U(S(int64_t(5)))); pr(U(S(int64_t(-5)))); pr(U(S(int64_t(2147483647)))); pr(U(S(int64_t(-2147483647 - 1)))); }   // ldi_i128 / ldi_u128 (imm32 isaret genislemeli)
      printf(" |"); pr(__builtin_bswap128(x));   // bswap128
      printf("\n");
    }
    return 0;
  }
  return 2;
}
