// BAGIMSIZ ORACLE (Plan Bolum 01, donusum matrisi): static_cast sonuclarini yerel C++ turleriyle hesaplar. Motorun hicbir parcasini kullanmaz.
//   donusum_oracle veri KAYNAK         : kaynak turun deneme degerleri (motorun yazmac gosterimi: isaretli int64, isaretsiz turler sifir-genisletilmis, u64 ham bit)
//   donusum_oracle don KAYNAK HEDEF    : her deger icin static_cast<HEDEF>(deger) (ayni gosterim; her satir bir sayi)
// Tur adlari tablodaki adlardir: i8 i16 i32 i64 u8 u16 u32 u64 bool
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
using namespace std;
static const int64_t BASE[] = {0, 1, 2, 3, 7, 8, 100, 126, 127, 128, 129, 200, 254, 255, 256, 257, 1000, 32767, 32768, 32769, 65535, 65536, 65537, 100000, 2147483647LL, 2147483648LL, 2147483649LL, 4294967295LL, 4294967296LL, 4294967297LL,
  -1, -2, -3, -7, -8, -100, -127, -128, -129, -200, -255, -256, -257, -1000, -32767, -32768, -32769, -65535, -65536, -65537, -100000, -2147483647LL, -2147483648LL, -2147483649LL, -4294967295LL, -4294967296LL,
  INT64_MAX, INT64_MAX - 1, INT64_MIN, INT64_MIN + 1, 0x7F7F7F7F7F7F7F7FLL, (int64_t)0x8080808080808080ULL, 0x00FF00FF00FF00FFLL, (int64_t)0xFFFF0000FFFF0000ULL, 1000000007, -1000000007, 6148914691236517205LL, -6148914691236517206LL};
template <class T> static int64_t rep(T v) { return (int64_t)(uint64_t)(typename conditional<is_signed<T>::value, int64_t, uint64_t>::type)v; }
template <class F> static vector<int64_t> src() { vector<int64_t> r; for (int64_t b : BASE) { F v = (F)b; r.push_back(rep<F>(v)); } return r; }
template <class F, class T> static void don() { for (int64_t b : BASE) { F v = (F)b; printf("%lld\n", (long long)rep<T>((T)v)); } }
template <class F> static void veri() { auto r = src<F>(); for (size_t i = 0; i < r.size(); i++) printf("%s%lld", i ? ", " : "", (long long)r[i]); printf("\n"); }
#define PAIR(FN, TN, F, T) if (f == FN && t == TN) { don<F, T>(); return 0; }
#define ALLTO(FN, F) PAIR(FN, "i8", F, int8_t) PAIR(FN, "i16", F, int16_t) PAIR(FN, "i32", F, int32_t) PAIR(FN, "i64", F, int64_t) PAIR(FN, "u8", F, uint8_t) PAIR(FN, "u16", F, uint16_t) PAIR(FN, "u32", F, uint32_t) PAIR(FN, "u64", F, uint64_t) PAIR(FN, "bool", F, bool)
int main(int argc, char** argv) {
  string m = argc > 1 ? argv[1] : "";
  if (m == "veri" && argc > 2) { string f = argv[2];
    if (f == "i8") veri<int8_t>(); else if (f == "i16") veri<int16_t>(); else if (f == "i32") veri<int32_t>(); else if (f == "i64") veri<int64_t>();
    else if (f == "u8") veri<uint8_t>(); else if (f == "u16") veri<uint16_t>(); else if (f == "u32") veri<uint32_t>(); else if (f == "u64") veri<uint64_t>(); else if (f == "bool") veri<bool>(); else return 2;
    return 0; }
  if (m == "don" && argc > 3) { string f = argv[2], t = argv[3];
    ALLTO("i8", int8_t) ALLTO("i16", int16_t) ALLTO("i32", int32_t) ALLTO("i64", int64_t) ALLTO("u8", uint8_t) ALLTO("u16", uint16_t) ALLTO("u32", uint32_t) ALLTO("u64", uint64_t) ALLTO("bool", bool)
    return 2; }
  return 2;
}
