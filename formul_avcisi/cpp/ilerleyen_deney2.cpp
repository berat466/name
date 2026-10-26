// V78 deney 2: (6) 14x2 -> 14x6 -> 14x8 (calisma-zamani carpanlar), (7) boyut taramasi, (8) double'da ilerleyen toplamanin kayma/ULP hatasi
#include <immintrin.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>
using namespace std;
static double now() { return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count(); }
static volatile uint64_t g_sink;
template <class F> static double med(F f, int rep = 15) { vector<double> t; for (int r = 0; r < rep; r++) { double a = now(); f(); t.push_back(now() - a); } sort(t.begin(), t.end()); return t[t.size() / 2]; }
int main(int argc, char** argv) {
    int64_t salt = argc > 1 ? atoll(argv[1]) : 0;   // calisma zamani
    // (6) m farkli taban b_i icin {k1,k2,k3} carpimlari; carpanlar CALISMA ZAMANINDA (derleyici strength-reduction yapamasin)
    int64_t m = 4000000; mt19937_64 g(1 + salt); vector<int64_t> b(m); for (auto& x : b) x = g() % 100000; volatile int64_t K1 = 2 + salt, K2 = 6 + salt, K3 = 8 + salt; const int64_t k1 = K1, k2 = K2, k3 = K3;
    vector<int64_t> r1(m), r2(m), r3(m), q1(m), q2(m), q3(m);
    auto direct = [&] { for (int64_t i = 0; i < m; i++) { r1[i] = b[i] * k1; r2[i] = b[i] * k2; r3[i] = b[i] * k3; } };
    // ilerleyen: 14x6 = 14x2 * 3 ?  (carpma) ya da  r6 = r2 + r2 + r2 ; r8 = r6 + r2  (carpanlar farki k3-k2 = k1 oldugundan bu CALISMA ZAMANINDA dogrulanir)
    bool chain = (k3 - k2 == k1) && (k2 % k1 == 0) && (k2 / k1 == 3);   // 6 = 3*2, 8 = 6+2
    auto prog = [&] { const int64_t t3 = k2 / k1; for (int64_t i = 0; i < m; i++) { int64_t a = b[i] * k1; q1[i] = a; int64_t c = a * t3; /* 1 carpma */ q2[i] = c; q3[i] = c + a; } };
    auto prog_add = [&] { for (int64_t i = 0; i < m; i++) { int64_t a = b[i] * k1; q1[i] = a; int64_t c = a + a + a; q2[i] = c; q3[i] = c + a; } };   // yalniz toplama (3 carpim yerine 1 carpma + 3 toplama)
    direct(); prog(); if (chain && (q1 != r1 || q2 != r2 || q3 != r3)) { printf("HATA(6)\n"); return 1; } prog_add(); if (chain && (q2 != r2 || q3 != r3)) { printf("HATA(6b)\n"); return 1; }
    double tD = med(direct), tP = med(prog), tPA = med(prog_add);
    printf("(6) 14x2, 14x6, 14x8 (b_i degisken, m=%lld; carpanlar calisma-zamani; zincir gecerli=%d)\n   dogrudan 3 carpma/b: %.2f ms (%.3f ns/b)\n   ilerleyen: 1 carpma + 1 carpma(x3) + 1 toplama: %.2f ms (%.3f ns/b)\n   ilerleyen: 1 carpma + 3 toplama: %.2f ms (%.3f ns/b)\n", (long long)m, (int)chain, tD * 1e3, tD * 1e9 / m, tP * 1e3, tP * 1e9 / m, tPA * 1e3, tPA * 1e9 / m);
    // derleyicinin sabit carpanla yaptigi: k sabit 2,6,8
    { auto cst = [&] { for (int64_t i = 0; i < m; i++) { r1[i] = b[i] * 2; r2[i] = b[i] * 6; r3[i] = b[i] * 8; } }; double tc = med(cst); printf("   (kiyas) carpanlar DERLEME-ZAMANI sabit: %.2f ms (%.3f ns/b)  -- derleyici kendisi kaydirma/lea ile 'iliskiyi' kullaniyor\n", tc * 1e3, tc * 1e9 / m); }
    // tek 'b'=14 icin gecikme zinciri: 3 sonuc birbirine bagli mi? (siralanmis bagimlilik)
    { volatile int64_t bb = 14; int64_t acc = 0; double t = med([&] { int64_t x = bb; for (int r = 0; r < 20000000; r++) { x = x * k1; x ^= bb; acc += x; } g_sink += acc; }, 5); double t2 = med([&] { int64_t x = bb; for (int r = 0; r < 20000000; r++) { x = x + x; x ^= bb; acc += x; } g_sink += acc; }, 5); printf("   gecikme zinciri (bagimli): imul %.3f ns/adim | add %.3f ns/adim\n", t / 2e7 * 1e9, t2 / 2e7 * 1e9); }
    // (7) boyut taramasi: ardisik b*k, k=1..N, int64 direct(vectorized) vs prog 4 zincir AVX-512
    printf("(7) ardisik 1..N (int64) boyut taramasi, ns/eleman:\n");
    for (int64_t N : {int64_t(256), int64_t(2048), int64_t(16384), int64_t(131072), int64_t(1048576), int64_t(16777216), int64_t(100000000)}) {
        vector<int64_t> o((size_t)N); int64_t b0 = 14 + salt; volatile int64_t vb = b0; const int64_t bb = vb; int reps = max<int64_t>(3, 20000000 / N);
        auto dire = [&] { for (int rr = 0; rr < reps; rr++) { for (int64_t k = 1; k <= N; k++) o[k - 1] = bb * k; g_sink += o[rr % N]; } };
        auto progv = [&] { for (int rr = 0; rr < reps; rr++) { __m512i s0 = _mm512_mullo_epi64(_mm512_set_epi64(8, 7, 6, 5, 4, 3, 2, 1), _mm512_set1_epi64(bb)); const __m512i st = _mm512_set1_epi64(32 * bb); __m512i s1 = _mm512_add_epi64(s0, _mm512_set1_epi64(8 * bb)), s2 = _mm512_add_epi64(s0, _mm512_set1_epi64(16 * bb)), s3 = _mm512_add_epi64(s0, _mm512_set1_epi64(24 * bb)); int64_t k = 0; int64_t* p = o.data();
              for (; k + 32 <= N; k += 32) { _mm512_storeu_si512(p + k, s0); _mm512_storeu_si512(p + k + 8, s1); _mm512_storeu_si512(p + k + 16, s2); _mm512_storeu_si512(p + k + 24, s3); s0 = _mm512_add_epi64(s0, st); s1 = _mm512_add_epi64(s1, st); s2 = _mm512_add_epi64(s2, st); s3 = _mm512_add_epi64(s3, st); } for (; k < N; k++) p[k] = bb * (k + 1); g_sink += o[rr % N]; } };
        double a = med(dire, 7) / reps / N * 1e9, c = med(progv, 7) / reps / N * 1e9; printf("   N=%10lld: dogrudan %.3f | ilerleyen AVX512 %.3f  (oran %.2f)\n", (long long)N, a, c, c / a); }
    // (8) double'da ilerleyen toplama: b ondalikli -> b*k ile s+=b arasindaki ULP kaymasi
    { printf("(8) double, b=0.1 vb: ilerleyen s+=b ile dogrudan b*k farki (ULP):\n"); for (double bd : {0.1, 14.0, 1.0 / 3.0}) { int64_t N = 100000; double s = 0, mx = 0; for (int64_t k = 1; k <= N; k++) { s += bd; double d = bd * (double)k; double ulp = nextafter(d, 1e300) - d; mx = max(mx, fabs(s - d) / ulp); } printf("   b=%.17g N=%lld: maks fark = %.1f ULP\n", bd, (long long)N, mx); } }
    return (int)(g_sink & 0);
}
