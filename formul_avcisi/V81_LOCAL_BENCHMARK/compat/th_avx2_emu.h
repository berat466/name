// AVX-512 olmayan CPU/derleme icin: kaynakta kullanilan _mm512_*_pd / _mm256_mask_*_epi32 komutlarinin SKALER, ayni-islemli taklidi.
// Her serit IEEE double ile ayni islemi yapar (fmadd = std::fma, max/cmp Intel semantigi); sonuc AVX-512 ile BIT-OZDES olmasi hedeflenir.
// Yalnizca __AVX512F__ tanimli DEGILSE devreye girer. Tek-dosya, bagimsiz; V81 kaynagi degistirilmez (-include ile on-eklenir).
#pragma once
#if !defined(__AVX512F__)
#include <immintrin.h>
#include <cmath>
#include <cstdint>
#include <cstring>
struct th_m512d { double v[8]; };
#define __m512d th_m512d
static inline th_m512d th_set1_pd(double x) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = x; return r; }
static inline th_m512d th_setzero_pd() { return th_set1_pd(0.0); }
static inline th_m512d th_loadu_pd(const double* p) { th_m512d r; memcpy(r.v, p, sizeof r.v); return r; }
static inline void th_storeu_pd(double* p, th_m512d a) { memcpy(p, a.v, sizeof a.v); }
static inline th_m512d th_add_pd(th_m512d a, th_m512d b) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = a.v[i] + b.v[i]; return r; }
static inline th_m512d th_sub_pd(th_m512d a, th_m512d b) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = a.v[i] - b.v[i]; return r; }
static inline th_m512d th_mul_pd(th_m512d a, th_m512d b) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = a.v[i] * b.v[i]; return r; }
static inline th_m512d th_div_pd(th_m512d a, th_m512d b) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = a.v[i] / b.v[i]; return r; }
static inline th_m512d th_max_pd(th_m512d a, th_m512d b) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = (a.v[i] > b.v[i]) ? a.v[i] : b.v[i]; return r; }   // Intel: a>b ? a : b
static inline th_m512d th_fmadd_pd(th_m512d a, th_m512d b, th_m512d c) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = std::fma(a.v[i], b.v[i], c.v[i]); return r; }
static inline th_m512d th_fnmadd_pd(th_m512d a, th_m512d b, th_m512d c) { th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = std::fma(-a.v[i], b.v[i], c.v[i]); return r; }
static inline unsigned char th_cmp_pd_mask(th_m512d a, th_m512d b, int imm) { unsigned m = 0; for (int i = 0; i < 8; i++) { bool t; switch (imm & 31) { case 17: t = a.v[i] < b.v[i]; break; /* _CMP_LT_OQ */ case 30: t = a.v[i] > b.v[i]; break; /* _CMP_GT_OQ */ case 18: t = a.v[i] <= b.v[i]; break; /* LE_OQ */ case 29: t = a.v[i] >= b.v[i]; break; /* GE_OQ */ case 0: t = a.v[i] == b.v[i]; break; default: t = false; } if (t) m |= 1u << i; } return (unsigned char)m; }
static inline th_m512d th_cvtps_pd(__m256 x) { float f[8]; _mm256_storeu_ps(f, x); th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = (double)f[i]; return r; }
// GCC'nin _mm512_reduce_add_pd sirasi: (a0+a4),(a1+a5),(a2+a6),(a3+a7) -> (t0+t2),(t1+t3) -> toplam
static inline double th_reduce_add_pd(th_m512d a) { double t0 = a.v[0] + a.v[4], t1 = a.v[1] + a.v[5], t2 = a.v[2] + a.v[6], t3 = a.v[3] + a.v[7]; return (t0 + t2) + (t1 + t3); }
static inline th_m512d th_i32gather_pd(__m256i idx, const double* base, int scale) { int ix[8]; _mm256_storeu_si256((__m256i*)ix, idx); th_m512d r; for (int i = 0; i < 8; i++) r.v[i] = *(const double*)((const char*)base + (long long)ix[i] * scale); return r; }
static inline __m256i th_mask_add_epi32(__m256i src, unsigned char k, __m256i a, __m256i b) { int s[8], x[8], y[8]; _mm256_storeu_si256((__m256i*)s, src); _mm256_storeu_si256((__m256i*)x, a); _mm256_storeu_si256((__m256i*)y, b); for (int i = 0; i < 8; i++) if ((k >> i) & 1) s[i] = (int)((unsigned)x[i] + (unsigned)y[i]); return _mm256_loadu_si256((const __m256i*)s); }
static inline __m256i th_mask_sub_epi32(__m256i src, unsigned char k, __m256i a, __m256i b) { int s[8], x[8], y[8]; _mm256_storeu_si256((__m256i*)s, src); _mm256_storeu_si256((__m256i*)x, a); _mm256_storeu_si256((__m256i*)y, b); for (int i = 0; i < 8; i++) if ((k >> i) & 1) s[i] = (int)((unsigned)x[i] - (unsigned)y[i]); return _mm256_loadu_si256((const __m256i*)s); }
#define _mm512_set1_pd th_set1_pd
#define _mm512_setzero_pd th_setzero_pd
#define _mm512_loadu_pd th_loadu_pd
#define _mm512_storeu_pd th_storeu_pd
#define _mm512_add_pd th_add_pd
#define _mm512_sub_pd th_sub_pd
#define _mm512_mul_pd th_mul_pd
#define _mm512_div_pd th_div_pd
#define _mm512_max_pd th_max_pd
#define _mm512_fmadd_pd th_fmadd_pd
#define _mm512_fnmadd_pd th_fnmadd_pd
#define _mm512_cmp_pd_mask th_cmp_pd_mask
#define _mm512_cvtps_pd th_cvtps_pd
#define _mm512_reduce_add_pd th_reduce_add_pd
#define _mm512_i32gather_pd th_i32gather_pd
#define _mm256_mask_add_epi32 th_mask_add_epi32
#define _mm256_mask_sub_epi32 th_mask_sub_epi32
#endif
