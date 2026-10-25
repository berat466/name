// Satir-ana (C/CBLAS 101) sgemm cagrisini sutun-ana (cuBLAS/BLAS) cagrisina cevirir.
// C = alpha*op(A)*op(B) + beta*C  (C: MxN, ldc), CblasNoTrans=111, CblasTrans=112.
// Hile: satir-ana C, sutun-ana C^T gibi gorunur; C^T = op(B)^T op(A)^T => A ve B yer degistirir.
#pragma once
template <class CMGemm>   // cm_gemm(opX_is_T, opY_is_T, m, n, k, alpha, X, ldx, Y, ldy, beta, Z, ldz) : sutun-ana Z = opX(X) opY(Y) + beta Z
inline void rowmajor_sgemm_via_colmajor(CMGemm cm_gemm, int ta, int tb, int M, int N, int K, float alpha, const float* A, int lda, const float* B, int ldb, float beta, float* C, int ldc) {
    bool opX = (tb == 112), opY = (ta == 112);          // X = B, Y = A
    cm_gemm(opX, opY, N, M, K, alpha, B, ldb, A, lda, beta, C, ldc);
}
