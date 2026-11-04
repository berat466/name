// CPU testi: sutun-ana naif sgemm ile satir-ana donusumu, scipy OpenBLAS sonucuyla karsilastirir (4 devrik kombinasyonu, ld > boyut).
#include <cstdio>
#include <cmath>
#include <vector>
#include <random>
#include "sgemm_map.h"
extern "C" void scipy_cblas_sgemm(int, int, int, int, int, int, float, const float*, int, const float*, int, float, float*, int);
static void cm_ref(bool opX, bool opY, int m, int n, int k, float al, const float* X, int ldx, const float* Y, int ldy, float be, float* Z, int ldz) {
    for (int j = 0; j < n; j++) for (int i = 0; i < m; i++) { double s = 0; for (int p = 0; p < k; p++) { float x = opX ? X[p + (size_t)i * ldx] : X[i + (size_t)p * ldx]; float y = opY ? Y[j + (size_t)p * ldy] : Y[p + (size_t)j * ldy]; s += (double)x * y; } Z[i + (size_t)j * ldz] = (float)(al * s + be * Z[i + (size_t)j * ldz]); } }
int main() { std::mt19937 g(1); std::normal_distribution<float> N01(0, 1); int bad = 0;
    for (int ta : {111, 112}) for (int tb : {111, 112}) for (float be : {0.f, 0.7f}) { int M = 37, N = 29, K = 41, lda = (ta == 111 ? K : M) + 3, ldb = (tb == 111 ? N : K) + 5, ldc = N + 2;
        std::vector<float> A((size_t)(ta == 111 ? M : K) * lda), B((size_t)(tb == 111 ? K : N) * ldb), C0((size_t)M * ldc), C1, C2; for (auto& x : A) x = N01(g); for (auto& x : B) x = N01(g); for (auto& x : C0) x = N01(g); C1 = C0; C2 = C0;
        scipy_cblas_sgemm(101, ta, tb, M, N, K, 1.3f, A.data(), lda, B.data(), ldb, be, C1.data(), ldc);
        rowmajor_sgemm_via_colmajor(cm_ref, ta, tb, M, N, K, 1.3f, A.data(), lda, B.data(), ldb, be, C2.data(), ldc);
        double md = 0; for (int i = 0; i < M; i++) for (int j = 0; j < N; j++) md = std::max(md, (double)std::fabs(C1[(size_t)i * ldc + j] - C2[(size_t)i * ldc + j]));
        printf("ta=%d tb=%d beta=%.1f  maks fark=%.2e %s\n", ta, tb, be, md, md < 1e-3 ? "OK" : "HATA"); if (md >= 1e-3) bad++; }
    printf(bad ? "BASARISIZ\n" : "TUM TESTLER GECTI\n"); return bad; }
