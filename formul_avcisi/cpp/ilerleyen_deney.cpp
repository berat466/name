// V78 deney 1: "ilerleyen hesap" (onceki sonuctan turet) ile dogrudan carpma -- bagimsiz, calisma-zamani girdileriyle
// Durumlar: (1) ardisik 1..N  (2) rastgele istenenler  (3) ortak carpan, (4) bagimsiz islemler, (5) bir sonucun cok kullanimi
// Yontemler: A dogrudan, B ilerleyen (onceki sonuca ekle), C akilli secim (seyrek -> dogrudan, ardisik -> ilerleyen)
#include <immintrin.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
using namespace std;
static double now() { return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count(); }
static volatile uint64_t g_sink;
template <class F> static double med(F f, int rep = 15) { vector<double> t; for (int r = 0; r < rep; r++) { double a = now(); f(); t.push_back(now() - a); } sort(t.begin(), t.end()); return t[t.size() / 2]; }
// ---- int64 ----
static void direct_seq(int64_t b, int64_t n, int64_t* o) { for (int64_t k = 1; k <= n; k++) o[k - 1] = b * k; }
static void prog_seq_scalar(int64_t b, int64_t n, int64_t* o) { int64_t s = 0; for (int64_t k = 0; k < n; k++) { s += b; o[k] = s; } }
static void prog_seq_avx(int64_t b, int64_t n, int64_t* o) {   // 8 serit, her serit kendi zincirinde: v += 8b
    __m512i v = _mm512_mullo_epi64(_mm512_set_epi64(8, 7, 6, 5, 4, 3, 2, 1), _mm512_set1_epi64(b)); /* ilk 8: tek sefer carp */ const __m512i st = _mm512_set1_epi64(8 * b);
    int64_t k = 0; for (; k + 8 <= n; k += 8) { _mm512_storeu_si512(o + k, v); v = _mm512_add_epi64(v, st); } for (; k < n; k++) o[k] = b * (k + 1); }
static void prog_seq_avx4(int64_t b, int64_t n, int64_t* o) {   // 4 bagimsiz zincir (gecikme gizleme)
    __m512i s0 = _mm512_mullo_epi64(_mm512_set_epi64(8, 7, 6, 5, 4, 3, 2, 1), _mm512_set1_epi64(b)); const __m512i st = _mm512_set1_epi64(32 * b);
    __m512i s1 = _mm512_add_epi64(s0, _mm512_set1_epi64(8 * b)), s2 = _mm512_add_epi64(s0, _mm512_set1_epi64(16 * b)), s3 = _mm512_add_epi64(s0, _mm512_set1_epi64(24 * b));
    int64_t k = 0; for (; k + 32 <= n; k += 32) { _mm512_storeu_si512(o + k, s0); _mm512_storeu_si512(o + k + 8, s1); _mm512_storeu_si512(o + k + 16, s2); _mm512_storeu_si512(o + k + 24, s3); s0 = _mm512_add_epi64(s0, st); s1 = _mm512_add_epi64(s1, st); s2 = _mm512_add_epi64(s2, st); s3 = _mm512_add_epi64(s3, st); } for (; k < n; k++) o[k] = b * (k + 1); }
static void direct_avx(int64_t b, int64_t n, int64_t* o) { const __m512i vb = _mm512_set1_epi64(b); __m512i kk = _mm512_set_epi64(8, 7, 6, 5, 4, 3, 2, 1), c8 = _mm512_set1_epi64(8); int64_t k = 0; for (; k + 8 <= n; k += 8) { _mm512_storeu_si512(o + k, _mm512_mullo_epi64(kk, vb)); kk = _mm512_add_epi64(kk, c8); } for (; k < n; k++) o[k] = b * (k + 1); }
// ---- double ----
static void directd(double b, int64_t n, double* o) { for (int64_t k = 1; k <= n; k++) o[k - 1] = b * (double)k; }
static void progd(double b, int64_t n, double* o) { double s = 0; for (int64_t k = 0; k < n; k++) { s += b; o[k] = s; } }
// ---- rastgele istenenler: indeks listesi idx (artan olmayan). B: en yakin onceki hesaplanmis sonuca "k - prev" farkini ekleyerek turetir ----
static void direct_rand(int64_t b, const int64_t* idx, int64_t m, int64_t* o) { for (int64_t i = 0; i < m; i++) o[i] = b * idx[i]; }
static void prog_rand_sorted(int64_t b, const int64_t* idx, const int32_t* ord, int64_t m, int64_t* o) {   // idx siralandiktan sonra ardisik farklardan ilerle: sonuc += b*(fark) ... fark carpimi yine carpma (!)
    int64_t prev = 0, pv = 0; for (int64_t j = 0; j < m; j++) { int64_t i = ord[j]; int64_t d = idx[i] - prev; pv += b * d; o[i] = pv; prev = idx[i]; } }
int main(int argc, char** argv) {
    int64_t b = argc > 1 ? atoll(argv[1]) : 14; b += (int64_t)(now() * 0) + (getenv("HOME") ? 0 : 1); int64_t N = argc > 2 ? atoll(argv[2]) : 100000;   // b calisma zamaninda
    vector<int64_t> o1(N), o2(N), o3(N), o4(N), o5(N); printf("b=%lld N=%lld\n", (long long)b, (long long)N);
    auto chk = [&](const vector<int64_t>& x, const vector<int64_t>& y, const char* nm) { if (x != y) { printf("HATA: %s farkli\n", nm); exit(1); } };
    // (1) ardisik 1..N, int64
    direct_seq(b, N, o1.data()); prog_seq_scalar(b, N, o2.data()); prog_seq_avx(b, N, o3.data()); prog_seq_avx4(b, N, o4.data()); direct_avx(b, N, o5.data()); chk(o1, o2, "scalar"); chk(o1, o3, "avx"); chk(o1, o4, "avx4"); chk(o1, o5, "directavx");
    int reps = 400;   // N=100000 (800 KB, L2) -- tekrar edip birlikte olc
    auto run = [&](auto f) { return med([&] { for (int r = 0; r < reps; r++) { f(); g_sink += o1[(size_t)r % N]; } }) / reps; };
    double tA = run([&] { direct_seq(b + 0, N, o1.data()); }), tAv = run([&] { direct_avx(b, N, o5.data()); }), tBs = run([&] { prog_seq_scalar(b, N, o2.data()); }), tBv = run([&] { prog_seq_avx(b, N, o3.data()); }), tB4 = run([&] { prog_seq_avx4(b, N, o4.data()); });
    printf("(1) ardisik 1..%lld int64  [us/ tum dizi, ns/eleman]\n  A dogrudan skaler (derleyici vektorlestirebilir): %8.2f us  %.3f ns\n  A dogrudan AVX-512 vpmullq                  : %8.2f us  %.3f ns\n  B ilerleyen skaler (s+=b)                   : %8.2f us  %.3f ns\n  B ilerleyen AVX-512 (8 serit, 1 zincir)     : %8.2f us  %.3f ns\n  B ilerleyen AVX-512 (32 serit, 4 zincir)    : %8.2f us  %.3f ns\n", (long long)N, tA * 1e6, tA * 1e9 / N, tAv * 1e6, tAv * 1e9 / N, tBs * 1e6, tBs * 1e9 / N, tBv * 1e6, tBv * 1e9 / N, tB4 * 1e6, tB4 * 1e9 / N);
    // double
    { vector<double> d1(N), d2(N); directd((double)b, N, d1.data()); progd((double)b, N, d2.data()); int ne = 0; for (int64_t i = 0; i < N; i++) ne += d1[i] != d2[i]; double tdA = run([&] { directd((double)b, N, d1.data()); g_sink += (uint64_t)d1[5]; }), tdB = run([&] { progd((double)b, N, d2.data()); g_sink += (uint64_t)d2[5]; });
      printf("(1b) ardisik 1..%lld double: A dogrudan %.2f us (%.3f ns/el) | B ilerleyen s+=b %.2f us (%.3f ns/el) | farkli eleman=%d (b tam sayi -> tam)\n", (long long)N, tdA * 1e6, tdA * 1e9 / N, tdB * 1e6, tdB * 1e9 / N, ne); }
    // (2) rastgele istenenler: m istek, indeksler [1,N]
    for (int64_t m : {int64_t(100), int64_t(1000), int64_t(10000), N}) { mt19937_64 g(12345 + m); vector<int64_t> idx(m); for (auto& x : idx) x = 1 + (int64_t)(g() % N); vector<int32_t> ord(m); for (int i = 0; i < m; i++) ord[i] = i; vector<int64_t> r1(m), r2(m);
        double tD = med([&] { for (int r = 0; r < reps / 4; r++) { direct_rand(b, idx.data(), m, r1.data()); g_sink += r1[r % m]; } }) / (reps / 4);
        double tS = med([&] { sort(ord.begin(), ord.end(), [&](int a, int c) { return idx[a] < idx[c]; }); }, 15); /* siralama maliyeti (bir kez) */
        sort(ord.begin(), ord.end(), [&](int a, int c) { return idx[a] < idx[c]; }); double tP = med([&] { for (int r = 0; r < reps / 4; r++) { prog_rand_sorted(b, idx.data(), ord.data(), m, r2.data()); g_sink += r2[r % m]; } }) / (reps / 4); direct_rand(b, idx.data(), m, r1.data()); prog_rand_sorted(b, idx.data(), ord.data(), m, r2.data()); if (r1 != r2) { printf("HATA rastgele\n"); return 1; }
        printf("(2) rastgele %6lld istek: A dogrudan %.2f us | B ilerleyen (siralama %.2f us + farklardan ilerle %.2f us; farklar yine carpilir) | esit\n", (long long)m, tD * 1e6, tS * 1e6, tP * 1e6); }
    // (3) ortak carpan: c ile carpilan K farkli sayi x_j (14 x_j): ortak b -> hepsi dogrudan; "ilerleyen" yok (x_j bagimsiz). Paylasilan: b*(x1+x2) = b*x1 + b*x2 (dagilma)
    { int64_t m = N; mt19937_64 g(7); vector<int64_t> x(m); for (auto& v : x) v = (int64_t)(g() % 1000000); vector<int64_t> p(m); double t1 = med([&] { for (int r = 0; r < reps / 4; r++) { for (int64_t i = 0; i < m; i++) p[i] = b * x[i]; g_sink += p[r % m]; } }) / (reps / 4);
      // toplam sorusu: sum_i b*x_i  (dogrudan m carpma+m toplama) vs b*(sum x_i) (1 carpma + m toplama): ortak carpan cikarma -- gercek matematiksel iliski
      volatile int64_t bb = b; int64_t s1 = 0, s2 = 0; double t2 = med([&] { for (int r = 0; r < reps / 4; r++) { int64_t s = 0; for (int64_t i = 0; i < m; i++) s += bb * x[i]; s1 = s; g_sink += s; } }) / (reps / 4);
      double t3 = med([&] { for (int r = 0; r < reps / 4; r++) { int64_t s = 0; for (int64_t i = 0; i < m; i++) s += x[i]; s2 = bb * s; g_sink += s2; } }) / (reps / 4);
      printf("(3) ortak carpan: m=%lld bagimsiz b*x_i dizisi %.2f us (turetilebilecek ilerleyen iliski yok) | toplam sum b*x_i: dogrudan %.2f us -> b*sum(x_i) %.2f us (esit=%d)\n", (long long)m, t1 * 1e6, t2 * 1e6, t3 * 1e6, (int)(s1 == s2)); }
    // (4) bagimsiz islemler: a_i*c_i  (ortak yapi yok): ilerleyen hesap uygulanamaz -> dogrudan
    { int64_t m = N; mt19937_64 g(9); vector<int64_t> a(m), c(m), p(m); for (int64_t i = 0; i < m; i++) { a[i] = g() % 100000; c[i] = g() % 100000; } double t = med([&] { for (int r = 0; r < reps / 4; r++) { for (int64_t i = 0; i < m; i++) p[i] = a[i] * c[i]; g_sink += p[r % m]; } }) / (reps / 4); printf("(4) bagimsiz a_i*c_i m=%lld: dogrudan %.2f us (%.3f ns/el); ilerleyen yontemin kullanabilecegi onceki sonuc iliskisi yok\n", (long long)m, t * 1e6, t * 1e9 / m); }
    // (5) bir sonucun cok kullanimi: P = b*c bir kez; sonra P kullanan m islem x_i + P  (P'yi her seferinde yeniden carpmak vs bir kez)
    { int64_t m = N; mt19937_64 g(11); vector<int64_t> x(m), y1(m), y2(m); for (auto& v : x) v = g() % 1000; volatile int64_t c = 97; double t1 = med([&] { for (int r = 0; r < reps / 4; r++) { for (int64_t i = 0; i < m; i++) y1[i] = x[i] + b * c; g_sink += y1[r % m]; } }) / (reps / 4);
      double t2 = med([&] { for (int r = 0; r < reps / 4; r++) { int64_t P = b * c; for (int64_t i = 0; i < m; i++) y2[i] = x[i] + P; g_sink += y2[r % m]; } }) / (reps / 4); printf("(5) P=b*c, m=%lld kullanim: her seferinde %.2f us | bir kez + tasi %.2f us | esit=%d (derleyici bunu zaten disari alir: volatile c ile bile)\n", (long long)m, t1 * 1e6, t2 * 1e6, (int)(y1 == y2)); }
    return (int)(g_sink & 0);
}
