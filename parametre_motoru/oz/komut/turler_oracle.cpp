// BAGIMSIZ ORACLE (Plan Bolum 01, TYPES): tur ozelliklerini (boyut, hiza, bit, isaret, siniflar, limitler, ABI veri modeli, bayt sirasi) DERLEYICIDEN okur:
// sizeof/alignof/std::numeric_limits ve <float.h> onislemci sabitleri (__FLT16_MAX__ ...). Motorun hicbir parcasini kullanmaz.
// Cikti satiri (komut @@tur 'tur' satirinin ayni sutunlari, kimlik haric):  AD sinif bit isaret boyut hiza nsoz endian usbit mantbit  min_lo min_hi max_lo max_hi tiny_lo tiny_hi eps_lo eps_hi dmin_lo dmin_hi
//   abi satirlari: abi 1 C_TUR_ADI boyut isaret(1/0) kayan(1/0)    (C tur adi = bosluklu ad; sonda boyut isaret kayan)
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <limits>
#include <stdfloat>
#include <complex>
#include <string>
using namespace std;
typedef __int128 i128; typedef unsigned __int128 u128;
static void pw(int64_t v) { printf(" %lld", (long long)v); }
static void pair128(u128 v) { pw((int64_t)(uint64_t)v); pw((int64_t)(uint64_t)(v >> 64)); }
template <class T> static void ilim() {   // tamsayi benzeri: min,max sign-genisletmeli iki sozcuk; kalan 6 sozcuk 0
  numeric_limits<T> L; i128 mn = (i128)L.min(), mx = (i128)L.max();
  pw((int64_t)(uint64_t)(u128)mn); pw((int64_t)(uint64_t)((u128)mn >> 64)); pw((int64_t)(uint64_t)(u128)mx); pw((int64_t)(uint64_t)((u128)mx >> 64));
  for (int i = 0; i < 6; i++) pw(0);
}
template <class T> static void rawf(T v) {   // kayan desen: ilk 16 baytin dusuk/yuksek sozcugu; 80 bit genisletilmis icin yalniz 10 bayt anlamli
  unsigned char b[16] = {0}; memcpy(b, &v, sizeof(T) > 10 && sizeof(T) == 16 ? 16 : sizeof(T));
  if (sizeof(T) == 16 && sizeof(T) == sizeof(long double) && is_same<T, long double>::value) { b[10] = b[11] = b[12] = b[13] = b[14] = b[15] = 0; }
  uint64_t lo = 0, hi = 0; for (int i = 0; i < 8; i++) lo |= (uint64_t)b[i] << (8 * i); for (int i = 0; i < 8; i++) hi |= (uint64_t)b[8 + i] << (8 * i);
  pw((int64_t)lo); pw((int64_t)hi);
}
template <class T> static void flim(T lowest, T mx, T tiny, T eps, T dmin) { rawf<T>(lowest); rawf<T>(mx); rawf<T>(tiny); rawf<T>(eps); rawf<T>(dmin); }
#define HEAD(ad, cls, bits, sg, T, nw, eb, mb) printf("%s %d %d %d %d %d %d %d %d %d", ad, cls, bits, sg, (int)sizeof(T), (int)alignof(T), nw, (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? 0 : 1), eb, mb)
#define INTT(ad, cls, T) do { HEAD(ad, cls, (int)numeric_limits<T>::digits + (numeric_limits<T>::is_signed ? 1 : 0), (int)numeric_limits<T>::is_signed, T, sizeof(T) > 8 ? 2 : 1, 0, 0); ilim<T>(); printf("\n"); } while (0)
#define FLT(ad, T, bits, nw, eb, mb, MX, MN, EP, DM) do { HEAD(ad, 5, bits, 1, T, nw, eb, mb); flim<T>(-(MX), (MX), (MN), (EP), (DM)); printf("\n"); } while (0)
#define CPX(ad, T, bits, nw, eb, mb) do { HEAD(ad, 6, bits, 1, T, nw, eb, mb); for (int i = 0; i < 10; i++) pw(0); printf("\n"); } while (0)
int main() {
  INTT("i8", 1, int8_t); INTT("i16", 1, int16_t); INTT("i32", 1, int32_t); INTT("i64", 1, int64_t); INTT("i128", 1, i128);
  INTT("u8", 1, uint8_t); INTT("u16", 1, uint16_t); INTT("u32", 1, uint32_t); INTT("u64", 1, uint64_t); INTT("u128", 1, u128);
  { HEAD("bool", 2, 1, 0, bool, 1, 0, 0); ilim<bool>(); printf("\n"); }
  INTT("char", 3, char); INTT("schar", 3, signed char); INTT("uchar", 3, unsigned char); INTT("wchar", 3, wchar_t); INTT("char8", 3, char8_t); INTT("char16", 3, char16_t); INTT("char32", 3, char32_t);
  { HEAD("byte", 4, 8, 0, byte, 1, 0, 0); for (int i = 0; i < 4; i++) pw(i == 2 ? 255 : 0); for (int i = 0; i < 6; i++) pw(0); printf("\n"); }
  { HEAD("nullptr", 7, 64, 0, nullptr_t, 1, 0, 0); for (int i = 0; i < 10; i++) pw(0); printf("\n"); }
  // kayan nokta: mantis bit = (digits-1) saklanan, genisletilmis 80 bitte tam sayi biti dahil 64
  FLT("f16", _Float16, 16, 1, 5, 10, (_Float16)__FLT16_MAX__, (_Float16)__FLT16_MIN__, (_Float16)__FLT16_EPSILON__, (_Float16)__FLT16_DENORM_MIN__);
  FLT("bf16", __bf16, 16, 1, 8, 7, (__bf16)__BFLT16_MAX__, (__bf16)__BFLT16_MIN__, (__bf16)__BFLT16_EPSILON__, (__bf16)__BFLT16_DENORM_MIN__);
  FLT("f32", float, 32, 1, 8, 23, __FLT_MAX__, __FLT_MIN__, __FLT_EPSILON__, __FLT_DENORM_MIN__);
  FLT("f64", double, 64, 1, 11, 52, __DBL_MAX__, __DBL_MIN__, __DBL_EPSILON__, __DBL_DENORM_MIN__);
  FLT("f80", long double, 80, 2, 15, 64, __LDBL_MAX__, __LDBL_MIN__, __LDBL_EPSILON__, __LDBL_DENORM_MIN__);
  FLT("f128", __float128, 128, 2, 15, 112, __FLT128_MAX__, __FLT128_MIN__, __FLT128_EPSILON__, __FLT128_DENORM_MIN__);
  CPX("c64", float _Complex, 64, 2, 8, 23); CPX("c128", double _Complex, 128, 2, 11, 52); CPX("c160", long double _Complex, 160, 4, 15, 64);
  // ABI veri modeli (Linux x86-64): C tur adi -> boyut isaret kayan
#define AB(ad, T) printf("abi 1 %s|%d %d %d\n", ad, (int)sizeof(T), (int)(numeric_limits<T>::is_signed ? 1 : 0), (int)(numeric_limits<T>::is_integer ? 0 : 1))
  AB("char", char); AB("signed char", signed char); AB("unsigned char", unsigned char); AB("short", short); AB("unsigned short", unsigned short); AB("int", int); AB("unsigned int", unsigned);
  AB("long", long); AB("unsigned long", unsigned long); AB("long long", long long); AB("unsigned long long", unsigned long long); AB("float", float); AB("double", double); AB("long double", long double);
  AB("bool", bool); AB("wchar_t", wchar_t); AB("char8_t", char8_t); AB("char16_t", char16_t); AB("char32_t", char32_t); printf("abi 1 void*|%d 0 0\n", (int)sizeof(void*)); AB("size_t", size_t); AB("ptrdiff_t", ptrdiff_t);
  AB("intptr_t", intptr_t); AB("uintptr_t", uintptr_t); AB("int8_t", int8_t); AB("int16_t", int16_t); AB("int32_t", int32_t); AB("int64_t", int64_t); AB("uint8_t", uint8_t); AB("uint16_t", uint16_t);
  AB("uint32_t", uint32_t); AB("uint64_t", uint64_t); AB("__int128", i128); AB("unsigned __int128", u128);
  return 0;
}
