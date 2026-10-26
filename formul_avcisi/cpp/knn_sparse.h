// V60: tam-esdeger SEYREK kNN cekirdegi. S = Nn Nn^T yogun V^3 sgemm yerine yalniz ortak sutunu olan (u,w) ciftleri toplanir.
// Toplama: sutun (k) artan sirada, her K-blok icinde tek FMA zinciri, bloklar C=C+blok seklinde (OpenBLAS sgemm birikimi emulasyonu);
// ortak sutunu olmayan ciftlerin S degeri tam 0'dir -> top-k'da (deger,indeks) siralamasiyla en kucuk uygun indeksler dolgu olur (yogun yolla ayni).
// Maliyet: sum_k nnz(sutun k)^2  (yogun: V^3).  Girdi CSR (satir basina artan sutun).
#pragma once
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>
#include <omp.h>
struct SpCSR { int V = 0; std::vector<long> off; std::vector<int> col; std::vector<float> val; };
struct SpKnnStats { double flops = 0; long touched = 0; };
// out_v/out_i: V*knn (deger float, indeks); out_k[u]: bulunan sayi
static inline std::vector<int> sp_kblocks(int K, int Q) { std::vector<int> b; for (int ls = 0; ls < K;) { int l = K - ls; if (l >= 2 * Q) l = Q; else if (l > Q) l = l / 2; b.push_back(ls); ls += l; } b.push_back(K); return b; }
static void knn_sparse_topk(const SpCSR& R, const std::vector<int>& cnt, int knn, int minc, int Q, std::vector<float>& out_v, std::vector<int>& out_i, std::vector<int>& out_k, SpKnnStats* st = nullptr) {
    const int V = R.V; std::vector<long> coff(V + 1, 0);
    for (long e = 0; e < (long)R.col.size(); e++) coff[R.col[e] + 1]++;
    for (int v = 0; v < V; v++) coff[v + 1] += coff[v];
    std::vector<int> crow(R.col.size()); std::vector<float> cval(R.col.size()); { std::vector<long> pos(coff.begin(), coff.end() - 1); for (int u = 0; u < V; u++) for (long e = R.off[u]; e < R.off[u + 1]; e++) { long p = pos[R.col[e]]++; crow[p] = u; cval[p] = R.val[e]; } }
    std::vector<int> kb = sp_kblocks(V, Q), blk(V); for (size_t b = 0; b + 1 < kb.size(); b++) for (int v = kb[b]; v < kb[b + 1]; v++) blk[v] = (int)b;
    out_v.assign((size_t)V * knn, 0.f); out_i.assign((size_t)V * knn, -1); out_k.assign(V, 0); double fl = 0; long tch = 0;
#pragma omp parallel reduction(+ : fl, tch)
    { std::vector<float> tot(V, 0.f), bacc(V, 0.f); std::vector<int> stampT(V, -1); std::vector<long> stampB(V, -1); const long NB = (long)kb.size(); std::vector<int> touched, btouched;
#pragma omp for schedule(dynamic, 8)
        for (int u = 0; u < V; u++) { touched.clear(); btouched.clear(); int curb = -1;
            auto flush = [&]() { for (int w : btouched) { tot[w] = fmaf(1.f, bacc[w], tot[w]); bacc[w] = 0.f; } btouched.clear(); };
            for (long e = R.off[u]; e < R.off[u + 1]; e++) { int v = R.col[e]; float a = R.val[e]; int b = blk[v]; if (b != curb) { flush(); curb = b; }
                for (long p = coff[v]; p < coff[v + 1]; p++) { int w = crow[p];                     if (stampT[w] != u) { stampT[w] = u; touched.push_back(w); tot[w] = 0.f; }
                    if (stampB[w] != (long)u * NB + b) { stampB[w] = (long)u * NB + b; btouched.push_back(w); bacc[w] = 0.f; }
                    bacc[w] = fmaf(a, cval[p], bacc[w]); } fl += 2.0 * (coff[v + 1] - coff[v]); }
            flush(); tch += (long)touched.size();
            std::pair<float, int> top[64]; int k = 0;
            for (int w : touched) { if (w == u || cnt[w] < minc) continue; std::pair<float, int> x{-tot[w], w};
                if (k < knn || x < top[k - 1]) { int pos = k < knn ? k++ : k - 1; while (pos > 0 && x < top[pos - 1]) { top[pos] = top[pos - 1]; pos--; } top[pos] = x; } }
            if (k < knn) { for (int w = 0; w < V && k < knn; w++) { if (w == u || cnt[w] < minc || stampT[w] == u) continue; top[k++] = {-0.f, w}; } }   // S==0 adaylari: artan indeks
            for (int q = 0; q < k; q++) { out_v[(size_t)u * knn + q] = -top[q].first; out_i[(size_t)u * knn + q] = top[q].second; } out_k[u] = k; } }
    if (st) { st->flops = fl; st->touched = tch; }
}
