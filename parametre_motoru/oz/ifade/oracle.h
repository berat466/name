// g++ oracle on eki: ifade tests (.cx) bu baslikla g++ ile derlenir; pm ifade ayni isimleri yerlesik olarak taniyor (pi/pc/pd).
#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <compare>
#include <typeinfo>
static void pi(long long x) { printf("%lld\n", x); }
static void pc(int c) { putchar(c); }
static void pd(double d) { printf("%.15g\n", d); }
