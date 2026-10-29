// V81_LOCAL_BENCHMARK: derleyici komut satirindan on-eklenir (-include / /FI). V81 kaynak dosyalari DEGISTIRILMEZ.
#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstdio>
// 1) BLAS adlari: kaynak, scipy'nin on-ekli OpenBLAS'ini (scipy_cblas_*) cagirir. Standart OpenBLAS icin adlar eslenir (TH_STD_BLAS).
#ifdef TH_STD_BLAS
#define scipy_cblas_sgemm cblas_sgemm
#define scipy_cblas_dgemm cblas_dgemm
#define scipy_cblas_dsyrk cblas_dsyrk
#define scipy_cblas_ssyrk cblas_ssyrk
#define scipy_cblas_dtrsm cblas_dtrsm
#define scipy_cblas_sgemv cblas_sgemv
#define scipy_cblas_sger cblas_sger
#define scipy_dpotrf_ dpotrf_
#define scipy_dsyev_ dsyev_
#define scipy_openblas_set_num_threads openblas_set_num_threads
#define scipy_openblas_get_num_threads openblas_get_num_threads
#endif
// openblas_set_num_threads_local: scipy'nin OpenBLAS'inde var, standart paketlerde olmayabilir. Yoksa bos taklit: yalnizca ic-ice BLAS is parcacigi sayisini etkiler, sonucu degil.
#ifdef TH_SHIM_OB_LOCAL
extern "C" inline int openblas_set_num_threads_local(int) { return 0; }
#endif
// 2) CPU korumasi: AVX-512/AVX2 komutlari DESTEKLENMEYEN CPU'da calistirilmaz (acik hata ile cikar).
#if defined(__GNUC__) && !defined(TH_NO_CPU_GUARD)
#if defined(__AVX512F__)
#define TH_ISA_NAME "avx512 (F,CD,BW,DQ,VL + AVX2 + FMA)"
static void __attribute__((constructor)) th_cpu_guard() { __builtin_cpu_init(); if (!(__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512dq") && __builtin_cpu_supports("avx512bw") && __builtin_cpu_supports("avx512vl") && __builtin_cpu_supports("avx512cd") && __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma"))) { fprintf(stderr, "TH_HATA: bu ikili AVX-512 ile derlendi ama CPU AVX-512 (F/CD/BW/DQ/VL) desteklemiyor. AVX2 surumunu (tek_hamle_v81_avx2) kullanin.\n"); _Exit(97); } }
#else
#define TH_ISA_NAME "avx2 (+FMA; AVX-512 komutlari skaler taklitle)"
static void __attribute__((constructor)) th_cpu_guard() { __builtin_cpu_init(); if (!(__builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma"))) { fprintf(stderr, "TH_HATA: bu ikili AVX2+FMA gerektirir; CPU desteklemiyor. TH_ISA=generic ile yeniden derleyin.\n"); _Exit(97); } }
#endif
#endif
#include "th_avx2_emu.h"
