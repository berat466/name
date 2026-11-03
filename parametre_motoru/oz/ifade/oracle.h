// g++ oracle on eki: ifade tests (.cx) bu baslikla g++ ile derlenir; pm ifade ayni isimleri yerlesik olarak taniyor (pi/pc/pd).
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <compare>
#include <typeinfo>
#include <alloca.h>
#include <coroutine>
static void pi(long long x) { printf("%lld\n", x); }
static void pc(int c) { putchar(c); }
static void pd(double d) { printf("%.15g\n", d); }
static void pl(long double x) { unsigned long long m; unsigned short e; memcpy(&m, &x, 8); memcpy(&e, (char*)&x + 8, 2); printf("%lld %d\n", (long long)m, (int)e); }
static void pq(__float128 x) { long long a[2]; memcpy(a, &x, 16); printf("%lld %lld\n", a[0], a[1]); }
