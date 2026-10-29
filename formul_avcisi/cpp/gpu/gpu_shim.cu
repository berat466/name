// GPU kaydirmasi (SHIM): programdaki scipy_cblas_sgemm cagrilarini cuBLAS'a yonlendirir. Baska hicbir hesap degismez.
// Kucuk matris carpimlari CPU'da (scipy OpenBLAS) kalir; buyukler GPU'ya gider. FP64 (dgemm), Cholesky, grup cozumleri CPU'da kalir.
// DIKKAT: Bu dosya GPU'suz sunucuda yazildi; derlenmedi/calistirilmadi. Mantik (satir-ana -> sutun-ana cevirimi) CPU'da test_map.cpp ile dogrulandi.
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cstdio>
#include <cstdlib>
#include "sgemm_map.h"

extern "C" void scipy_cblas_sgemm(int, int, int, int, int, int, float, const float*, int, const float*, int, float, float*, int);

static cublasHandle_t g_h = nullptr; static float *dA = nullptr, *dB = nullptr, *dC = nullptr; static size_t cA = 0, cB = 0, cC = 0;
static void ensure(float*& p, size_t& cap, size_t need) { if (need > cap) { if (p) cudaFree(p); cudaMalloc(&p, need * sizeof(float)); cap = need; } }
#define CK(x) do { cudaError_t e = (x); if (e != cudaSuccess) { fprintf(stderr, "CUDA hata %s:%d %s\n", __FILE__, __LINE__, cudaGetErrorString(e)); exit(1); } } while (0)

extern "C" void gpu_sgemm(int order, int ta, int tb, int M, int N, int K, float alpha, const float* A, int lda, const float* B, int ldb, float beta, float* C, int ldc) {
    const double work = (double)M * N * K; const double kThresh = getenv("GPU_MIN_WORK") ? atof(getenv("GPU_MIN_WORK")) : 5e7;
    if (order != 101 || work < kThresh) { scipy_cblas_sgemm(order, ta, tb, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc); return; }
    if (!g_h) cublasCreate(&g_h);
    size_t nA = (ta == 111) ? (size_t)(M - 1) * lda + K : (size_t)(K - 1) * lda + M;       // A: MxK (NoTrans) veya KxM (Trans), satir-ana
    size_t nB = (tb == 111) ? (size_t)(K - 1) * ldb + N : (size_t)(N - 1) * ldb + K;
    size_t nC = (size_t)(M - 1) * ldc + N;
    ensure(dA, cA, nA); ensure(dB, cB, nB); ensure(dC, cC, nC);
    CK(cudaMemcpy(dA, A, nA * sizeof(float), cudaMemcpyHostToDevice)); CK(cudaMemcpy(dB, B, nB * sizeof(float), cudaMemcpyHostToDevice)); if (beta != 0.f) CK(cudaMemcpy(dC, C, nC * sizeof(float), cudaMemcpyHostToDevice));
    auto cm = [&](bool opX, bool opY, int m, int n, int k, float al, const float* X, int ldx, const float* Y, int ldy, float be, float* Z, int ldz) {
        cublasSgemm(g_h, opX ? CUBLAS_OP_T : CUBLAS_OP_N, opY ? CUBLAS_OP_T : CUBLAS_OP_N, m, n, k, &al, X, ldx, Y, ldy, &be, Z, ldz); };
    rowmajor_sgemm_via_colmajor(cm, ta, tb, M, N, K, alpha, dA, lda, dB, ldb, beta, dC, ldc);
    CK(cudaMemcpy(C, dC, nC * sizeof(float), cudaMemcpyDeviceToHost));
}
