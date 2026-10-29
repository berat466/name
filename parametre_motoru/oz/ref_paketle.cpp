// Referans (dogrulama araci, bagimsiz C++): sayi listesi -> LEB128/zigzag baytlar, TAM 64 bit.
// N: 0..2^64-1 ; zN: -2^63..2^63-1 ; '#' yorum. Gecersiz/tasan simge -> cikis kodu 1.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
typedef unsigned long long u64;
int main() {
  std::string all; int ch; while ((ch = getchar()) != EOF) all.push_back(char(ch));
  size_t i = 0, n = all.size(); std::string out;
  while (i < n) {
    char c = all[i];
    if (c == '#') { while (i < n && all[i] != '\n') i++; continue; }
    if ((unsigned char)c <= 32) { i++; continue; }
    bool z = false, neg = false; size_t s = i;
    if (all[i] == 'z') { z = true; i++; }
    if (i < n && all[i] == '-') { neg = true; i++; }
    size_t d0 = i; u64 v = 0;
    while (i < n && all[i] >= '0' && all[i] <= '9') { unsigned d = all[i] - '0'; if (v > (~0ULL - d) / 10) { fprintf(stderr, "tasma\n"); return 1; } v = v * 10 + d; i++; }
    if (i == d0 || (neg && !z) || (i < n && (unsigned char)all[i] > 32 && all[i] != '#')) { fprintf(stderr, "gecersiz simge '%s'\n", all.substr(s, 20).c_str()); return 1; }
    u64 enc = v;
    if (z) {
      if (neg ? (v > (1ULL << 63)) : (v >= (1ULL << 63))) { fprintf(stderr, "aralik disi\n"); return 1; }
      long long sv = neg ? (long long)(0 - v) : (long long)v;
      enc = ((u64)sv << 1) ^ (u64)(sv >> 63);
    }
    while (enc >= 128) { out.push_back(char((enc & 127) | 128)); enc >>= 7; }
    out.push_back(char(enc));
  }
  fwrite(out.data(), 1, out.size(), stdout); return 0;
}
