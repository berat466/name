// Karsilastirma: ayni ozyinelemeli fib(35), yerel C++ (-O2). Derleyici ozyinelemeyi kismen acabilir; bu bir ust sinir olcumudur.
#include <cstdio>
#include <cstdint>
static int64_t fib(int64_t n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
int main() { printf("%lld\n", (long long)fib(35)); }
