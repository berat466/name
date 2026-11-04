#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#define BARRIER(x) asm volatile("" : "+r"(x))
static int64_t fib(int64_t n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
template <class F> static void timeit(const char* name, F f) {
  double best = 1e9; long long res = 0;
  for (int i = 0; i < 3; i++) { auto t0 = std::chrono::steady_clock::now(); res = f(); best = std::min(best, std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count()); }
  printf("%-10s %.4f s   sonuc=%lld\n", name, best, res);
}
int main() {
  timeit("fib35", [] { return (long long)fib(35); });
  timeit("loop_sum", [] { int64_t s = 0; for (int64_t i = 0; i < 100000000; i++) { s += i; BARRIER(s); } return (long long)s; });
  timeit("sieve10m", [] {
    const int64_t n = 10000000; std::vector<uint64_t> fl(n + 1, 1); fl[0] = fl[1] = 0;
    for (int64_t i = 2; i * i <= n; i++) if (fl[i]) for (int64_t j = i * i; j <= n; j += i) fl[j] = 0;
    long long c = 0; for (int64_t k = 0; k <= n; k++) c += fl[k]; return c; });
  timeit("floatloop", [] { double sum = 0, sign = 1; for (int64_t k = 0; k < 20000000; k++) { double den = double(2 * k + 1); double t = sign / den; sum += t; BARRIER(sum); sign = -sign; } return (long long)(sum * 4.0 * 1e12); });
  timeit("memsum", [] { std::vector<uint64_t> a(1000000); for (int64_t i = 0; i < 1000000; i++) a[i] = i; int64_t s = 0;
    for (int r = 0; r < 50; r++) for (int64_t i = 0; i < 1000000; i++) { s += a[i]; BARRIER(s); } return (long long)s; });
}
