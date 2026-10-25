// V54: glibc 2.39 tanhf/expm1f (fdlibm, float) algoritmasinin dallanmasiz, vektorlestirilebilir hali.
// Amac: libm tanhf ile BIT-OZDES sonuc (tum 2^32 float girdide dogrulanir: tanhf_test.cpp). Aritmetik islem sirasi fdlibm kaynagiyle ayni.
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
static inline uint32_t tv_f2u(float x) { uint32_t u; memcpy(&u, &x, 4); return u; }
static inline float tv_u2f(uint32_t u) { float x; memcpy(&x, &u, 4); return x; }
// beklenen alan: |x| < 44 (tanhf icin 2|x| < 44), bitis/NaN yok; x < 2^-25 -> x
#ifndef TV_ATTR
#define TV_ATTR __attribute__((optimize("fp-contract=off")))
#endif
TV_ATTR static inline float tv_expm1f(float x) {
    const float ln2_hi = 6.9313812256e-01f, ln2_lo = 9.0580006145e-06f, invln2 = 1.4426950216e+00f;
    const float Q1 = -3.3333335072e-02f, Q2 = 1.5873016091e-03f, Q3 = -7.9365076090e-05f, Q4 = 4.0082177293e-06f, Q5 = -2.0109921195e-07f;
    uint32_t hx = tv_f2u(x); bool neg = (hx & 0x80000000u) != 0; hx &= 0x7fffffffu;
    bool big = hx > 0x3eb17218u, mid = hx < 0x3F851592u, tiny = hx < 0x33000000u;
    float hi1 = neg ? x + ln2_hi : x - ln2_hi, lo1 = neg ? -ln2_lo : ln2_lo; int k1 = neg ? -1 : 1;
    int kg = (int)(invln2 * x + (neg ? -0.5f : 0.5f)); float tt = (float)kg; float hig = x - tt * ln2_hi, log_ = tt * ln2_lo;
    int k = big ? (mid ? k1 : kg) : 0; float hi = mid ? hi1 : hig, lo = mid ? lo1 : log_;
    float xr = hi - lo; float c = (hi - xr) - lo; float x2 = big ? xr : x, cc = big ? c : 0.0f;
    float hfx = 0.5f * x2, hxs = x2 * hfx;
    float r1 = 1.0f + hxs * (Q1 + hxs * (Q2 + hxs * (Q3 + hxs * (Q4 + hxs * Q5))));
    float t = 3.0f - r1 * hfx; float e = hxs * ((r1 - t) / (6.0f - x2 * t));
    float res0 = x2 - (x2 * e - hxs);
    float e2 = (x2 * (e - cc) - cc); e2 -= hxs;
    float resm1 = 0.5f * (x2 - e2) - 0.5f;
    float res1 = (x2 < -0.25f) ? -2.0f * (e2 - (x2 + 0.5f)) : 1.0f + 2.0f * (x2 - e2);
    uint32_t ku = (uint32_t)k << 23;
    float ya = 1.0f - (e2 - x2); float resbig = tv_u2f(tv_f2u(ya) + ku) - 1.0f;
    int ks = k & 31; float tk = tv_u2f(0x3f800000u - (0x1000000u >> ks)); float yb = tk - (e2 - x2); float reslt = tv_u2f(tv_f2u(yb) + ku);
    float t2 = tv_u2f((uint32_t)(0x7f - k) << 23); float yc = x2 - (e2 + t2); yc += 1.0f; float resge = tv_u2f(tv_f2u(yc) + ku);
    float r = (k <= -2 || k > 56) ? resbig : (k < 23 ? reslt : resge);
    r = (k == 1) ? res1 : r; r = (k == -1) ? resm1 : r; r = (k == 0) ? res0 : r; r = tiny ? x : r;
    return r;
}
// tek eleman (vektorlestirici dallanmasiz dongude kullanir); normal alan disi (|x|>=22, |x|<2^-55, nan) -> libm
TV_ATTR static inline float tv_tanhf(float x) {
    uint32_t jx = tv_f2u(x), ix = jx & 0x7fffffffu; float ax = fabsf(x);
    bool ge1 = ix >= 0x3f800000u; float arg = ge1 ? 2.0f * ax : -2.0f * ax;
    float t = tv_expm1f(arg);
    float z = ge1 ? (1.0f - 2.0f / (t + 2.0f)) : (-t / (t + 2.0f));
    return ((int32_t)jx >= 0) ? z : -z;
}
#if defined(__AVX512F__)
#include <immintrin.h>
// AVX-512 (16 serit) elle yazilmis surum: islem sirasi skaler surumle ayni, FMA yok -> libm tanhf ile bit-ozdes (tanhf_test.cpp ile 2^32 girdide dogrulanir)
TV_ATTR static inline __m512 tv_expm1f16(__m512 x) {
    const __m512 ln2_hi = _mm512_set1_ps(6.9313812256e-01f), ln2_lo = _mm512_set1_ps(9.0580006145e-06f), invln2 = _mm512_set1_ps(1.4426950216e+00f);
    const __m512 Q1 = _mm512_set1_ps(-3.3333335072e-02f), Q2 = _mm512_set1_ps(1.5873016091e-03f), Q3 = _mm512_set1_ps(-7.9365076090e-05f), Q4 = _mm512_set1_ps(4.0082177293e-06f), Q5 = _mm512_set1_ps(-2.0109921195e-07f);
    const __m512 one = _mm512_set1_ps(1.0f), half = _mm512_set1_ps(0.5f), nhalf = _mm512_set1_ps(-0.5f);
    __m512i xi = _mm512_castps_si512(x), hx = _mm512_and_si512(xi, _mm512_set1_epi32(0x7fffffff));
    __mmask16 neg = _mm512_cmplt_epi32_mask(xi, _mm512_setzero_si512()), big = _mm512_cmpgt_epu32_mask(hx, _mm512_set1_epi32(0x3eb17218)), mid = _mm512_cmplt_epu32_mask(hx, _mm512_set1_epi32(0x3F851592)), tiny = _mm512_cmplt_epu32_mask(hx, _mm512_set1_epi32(0x33000000));
    __m512 hi1 = _mm512_mask_blend_ps(neg, _mm512_sub_ps(x, ln2_hi), _mm512_add_ps(x, ln2_hi)), lo1 = _mm512_mask_blend_ps(neg, ln2_lo, _mm512_sub_ps(_mm512_setzero_ps(), ln2_lo));
    __m512i k1 = _mm512_mask_blend_epi32(neg, _mm512_set1_epi32(1), _mm512_set1_epi32(-1));
    __m512i kg = _mm512_cvttps_epi32(_mm512_add_ps(_mm512_mul_ps(invln2, x), _mm512_mask_blend_ps(neg, half, nhalf)));
    __m512 tt = _mm512_cvtepi32_ps(kg), hig = _mm512_sub_ps(x, _mm512_mul_ps(tt, ln2_hi)), log_ = _mm512_mul_ps(tt, ln2_lo);
    __m512i k = _mm512_maskz_mov_epi32(big, _mm512_mask_blend_epi32(mid, kg, k1));
    __m512 hi = _mm512_mask_blend_ps(mid, hig, hi1), lo = _mm512_mask_blend_ps(mid, log_, lo1);
    __m512 xr = _mm512_sub_ps(hi, lo), c = _mm512_sub_ps(_mm512_sub_ps(hi, xr), lo);
    __m512 x2 = _mm512_mask_blend_ps(big, x, xr), cc = _mm512_maskz_mov_ps(big, c);
    __m512 hfx = _mm512_mul_ps(half, x2), hxs = _mm512_mul_ps(x2, hfx);
    __m512 p = _mm512_add_ps(Q4, _mm512_mul_ps(hxs, Q5)); p = _mm512_add_ps(Q3, _mm512_mul_ps(hxs, p)); p = _mm512_add_ps(Q2, _mm512_mul_ps(hxs, p)); p = _mm512_add_ps(Q1, _mm512_mul_ps(hxs, p));
    __m512 r1 = _mm512_add_ps(one, _mm512_mul_ps(hxs, p));
    __m512 t = _mm512_sub_ps(_mm512_set1_ps(3.0f), _mm512_mul_ps(r1, hfx));
    __m512 e = _mm512_mul_ps(hxs, _mm512_div_ps(_mm512_sub_ps(r1, t), _mm512_sub_ps(_mm512_set1_ps(6.0f), _mm512_mul_ps(x2, t))));
    __m512 res0 = _mm512_sub_ps(x2, _mm512_sub_ps(_mm512_mul_ps(x2, e), hxs));
    __m512 e2 = _mm512_sub_ps(_mm512_mul_ps(x2, _mm512_sub_ps(e, cc)), cc); e2 = _mm512_sub_ps(e2, hxs);
    __m512 resm1 = _mm512_sub_ps(_mm512_mul_ps(half, _mm512_sub_ps(x2, e2)), half);
    __mmask16 lt = _mm512_cmp_ps_mask(x2, _mm512_set1_ps(-0.25f), _CMP_LT_OQ);
    __m512 res1 = _mm512_mask_blend_ps(lt, _mm512_add_ps(one, _mm512_mul_ps(_mm512_set1_ps(2.0f), _mm512_sub_ps(x2, e2))), _mm512_mul_ps(_mm512_set1_ps(-2.0f), _mm512_sub_ps(e2, _mm512_add_ps(x2, half))));
    __m512i ku = _mm512_slli_epi32(k, 23);
    __m512 ya = _mm512_sub_ps(one, _mm512_sub_ps(e2, x2)); __m512 resbig = _mm512_sub_ps(_mm512_castsi512_ps(_mm512_add_epi32(_mm512_castps_si512(ya), ku)), one);
    __m512i ks = _mm512_and_si512(k, _mm512_set1_epi32(31)); __m512 tk = _mm512_castsi512_ps(_mm512_sub_epi32(_mm512_set1_epi32(0x3f800000), _mm512_srlv_epi32(_mm512_set1_epi32(0x1000000), ks)));
    __m512 yb = _mm512_sub_ps(tk, _mm512_sub_ps(e2, x2)); __m512 reslt = _mm512_castsi512_ps(_mm512_add_epi32(_mm512_castps_si512(yb), ku));
    __m512 t2 = _mm512_castsi512_ps(_mm512_slli_epi32(_mm512_sub_epi32(_mm512_set1_epi32(0x7f), k), 23)); __m512 yc = _mm512_sub_ps(x2, _mm512_add_ps(e2, t2)); yc = _mm512_add_ps(yc, one); __m512 resge = _mm512_castsi512_ps(_mm512_add_epi32(_mm512_castps_si512(yc), ku));
    __mmask16 kbig = _mm512_kor(_mm512_cmple_epi32_mask(k, _mm512_set1_epi32(-2)), _mm512_cmpgt_epi32_mask(k, _mm512_set1_epi32(56))), klt = _mm512_cmplt_epi32_mask(k, _mm512_set1_epi32(23));
    __m512 r = _mm512_mask_blend_ps(kbig, _mm512_mask_blend_ps(klt, resge, reslt), resbig);
    r = _mm512_mask_blend_ps(_mm512_cmpeq_epi32_mask(k, _mm512_set1_epi32(1)), r, res1); r = _mm512_mask_blend_ps(_mm512_cmpeq_epi32_mask(k, _mm512_set1_epi32(-1)), r, resm1); r = _mm512_mask_blend_ps(_mm512_cmpeq_epi32_mask(k, _mm512_setzero_si512()), r, res0);
    return _mm512_mask_blend_ps(tiny, r, x); }
TV_ATTR static inline __m512 tv_tanhf16(__m512 x) {
    const __m512i ABS = _mm512_set1_epi32(0x7fffffff); const __m512 one = _mm512_set1_ps(1.0f), two = _mm512_set1_ps(2.0f), sgn = _mm512_castsi512_ps(_mm512_set1_epi32(0x80000000u));
    __m512i jx = _mm512_castps_si512(x), ix = _mm512_and_si512(jx, ABS); __m512 ax = _mm512_castsi512_ps(ix);
    __mmask16 ge1 = _mm512_cmpge_epu32_mask(ix, _mm512_set1_epi32(0x3f800000));
    __m512 arg = _mm512_mask_blend_ps(ge1, _mm512_mul_ps(_mm512_set1_ps(-2.0f), ax), _mm512_mul_ps(two, ax));
    __m512 t = tv_expm1f16(arg);
    __m512 z = _mm512_mask_blend_ps(ge1, _mm512_div_ps(_mm512_xor_ps(t, sgn), _mm512_add_ps(t, two)), _mm512_sub_ps(one, _mm512_div_ps(two, _mm512_add_ps(t, two))));
    __mmask16 negx = _mm512_cmplt_epi32_mask(jx, _mm512_setzero_si512());
    return _mm512_mask_blend_ps(negx, z, _mm512_xor_ps(z, sgn)); }
TV_ATTR static inline void tv_tanhf_array(const float* in, float* out, size_t n) {
    size_t i = 0;
    for (; i + 16 <= n; i += 16) { __m512 x = _mm512_loadu_ps(in + i); __m512 y = tv_tanhf16(x);
        __m512i ix = _mm512_and_si512(_mm512_castps_si512(x), _mm512_set1_epi32(0x7fffffff));
        __mmask16 odd = _mm512_kor(_mm512_cmpge_epu32_mask(ix, _mm512_set1_epi32(0x41b00000)), _mm512_cmplt_epu32_mask(ix, _mm512_set1_epi32(0x24000000)));
        _mm512_storeu_ps(out + i, y); if (odd) for (int l = 0; l < 16; l++) if ((odd >> l) & 1) out[i + l] = tanhf(in[i + l]); }   // alan disi (|x|>=22, |x|<2^-55, sifir, nan) -> libm
    for (; i < n; i++) out[i] = tanhf(in[i]); }
#else
TV_ATTR static inline void tv_tanhf_array(const float* in, float* out, size_t n) { for (size_t i = 0; i < n; i++) out[i] = tanhf(in[i]); }
#endif

// calisma zamani oz-denetimi: vektor tanhf libm ile ayni mi? (farkli glibc/CPU'da otomatik olarak libm'e doner)
static inline bool tv_selftest() {
    const size_t N = 1 << 18; std::vector<float> in(N + 16), out(N + 16); uint64_t st = 88172645463325252ull;
    for (size_t i = 0; i < N; i++) { st ^= st << 13; st ^= st >> 7; st ^= st << 17; float u = (float)((st >> 40) & 0xffffff) / 16777216.0f; in[i] = (u - 0.5f) * (i % 4 == 0 ? 60.0f : (i % 4 == 1 ? 4.0f : (i % 4 == 2 ? 0.5f : 24.0f))); }
    const float sp[] = {0.0f, -0.0f, 1e-30f, -1e-30f, 2.9e-8f, 1.0f, -1.0f, 0.99999994f, 1.0000001f, 0.34657359f, 0.5198603f, 21.99f, 22.0f, -22.0f, 9.5f, 11.0f, 11.3f, 19.7f, 19.8f, 0.3465f, 0.5f, 1.04f, 1.5f};
    for (size_t i = 0; i < sizeof(sp) / sizeof(float); i++) in[i] = sp[i];
    tv_tanhf_array(in.data(), out.data(), N); for (size_t i = 0; i < N; i++) if (tv_f2u(out[i]) != tv_f2u(tanhf(in[i]))) return false; return true; }
