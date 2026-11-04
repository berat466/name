// UCUNCU REFERANS (donanim/derleyici): ref_dar.py'nin tam rasyonel sonuclarini C++ donusumleri/islemleri ile capraz denetler.
//   donanim_dar rnd|add|sub|mul|div|sqrt|i2f|u2f  f32|f16|bf16   < veri (virgullu isaretli int64 bit desenleri)   > beklenen satirlar (bit desenleri, ref_dar.py ile ayni sira/bicim)
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <stdfloat>
using namespace std;
typedef long long ll;
static double b2d(int64_t b) { double d; memcpy(&d, &b, 8); return d; }
static int64_t d2b(double d) { int64_t b; memcpy(&b, &d, 8); return b; }
template <class F> static int64_t cv(double x) { return d2b((double)(F)x); }
template <class F> static void run(const string& op, const vector<int64_t>& v) {
  if (op == "rnd") { for (auto b : v) printf("%lld\n", (ll)cv<F>(b2d(b))); return; }
  if (op == "i2f") { for (auto b : v) printf("%lld\n", (ll)d2b((double)(F)(int64_t)b)); return; }
  if (op == "u2f") { for (auto b : v) printf("%lld\n", (ll)d2b((double)(F)(uint64_t)b)); return; }
  if (op == "sqrt") { for (auto b : v) { F x = (F)b2d(b); F r = (F)sqrt((double)x); double d = (double)r; if (d != d) printf("%lld\n", (ll)-2251799813685248LL); else printf("%lld\n", (ll)d2b(d)); } return; }
  for (auto xb : v) for (auto yb : v) { F x = (F)b2d(xb), y = (F)b2d(yb); F r = op == "add" ? (F)(x + y) : op == "sub" ? (F)(x - y) : op == "mul" ? (F)(x * y) : (F)(x / y);
    double d = (double)r; if (d != d) printf("%lld\n", (ll)-2251799813685248LL); else printf("%lld\n", (ll)d2b(d)); }
}
int main(int argc, char** argv) {
  if (argc < 3) return 2; string op = argv[1], f = argv[2]; vector<int64_t> v; char* line = nullptr; size_t cap = 0;
  while (getline(&line, &cap, stdin) > 0) { char* p = line; while (*p) { while (*p == ' ' || *p == ',') p++; if (!*p || *p == '\n') break; char* e; v.push_back(strtoll(p, &e, 10)); p = e; } }
  if (f == "f32") run<float>(op, v); else if (f == "f16") run<_Float16>(op, v); else if (f == "bf16") run<__bf16>(op, v); else return 2;
  return 0;
}
