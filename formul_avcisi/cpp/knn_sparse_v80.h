// V60: tam-esdeger SEYREK kNN cekirdegi. S = Nn Nn^T yogun V^3 sgemm yerine yalniz ortak sutunu olan (u,w) ciftleri toplanir.
// Toplama: sutun (k) artan sirada, her K-blok icinde tek FMA zinciri, bloklar C=C+blok seklinde (OpenBLAS sgemm birikimi emulasyonu);
// ortak sutunu olmayan ciftlerin S degeri tam 0'dir -> top-k'da (deger,indeks) siralamasiyla en kucuk uygun indeksler dolgu olur (yogun yolla ayni).
// Maliyet: sum_k nnz(sutun k)^2  (yogun: V^3).  Girdi CSR (satir basina artan sutun).
#pragma once
#include <x86intrin.h>
static double g_knn_cyc[2] = {0, 0};
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
    { std::vector<float> tot(V, 0.f), bacc(V, 0.f); const long NB = (long)kb.size(); std::vector<int> touched(V + 1), btouched(V + 1); int nT = 0, nB = 0;
#pragma omp for schedule(dynamic, 8)
        for (int u = 0; u < V; u++) { const unsigned long long tc0 = __rdtsc(); nT = 0; nB = 0; int curb = -1;
            auto flush = [&]() { for (int i = 0; i < nB; i++) { int w = btouched[i]; touched[nT] = w; nT += (tot[w] == 0.f); tot[w] = fmaf(1.f, bacc[w], tot[w]); bacc[w] = 0.f; } nB = 0; };   // V80: tum degerler > 0 -> 'dokunuldu' damgasi (stampT/stampB) yerine tot/bacc != 0
            for (long e = R.off[u]; e < R.off[u + 1]; e++) { int v = R.col[e]; float a = R.val[e]; int b = blk[v]; if (b != curb) { flush(); curb = b; }
                for (long p = coff[v]; p < coff[v + 1]; p++) { int w = crow[p]; btouched[nB] = w; nB += (bacc[w] == 0.f);
                    bacc[w] = fmaf(a, cval[p], bacc[w]); } fl += 2.0 * (coff[v + 1] - coff[v]); }
            flush(); tch += (long)nT; const unsigned long long tc1 = __rdtsc();
            std::pair<float, int> top[64]; int k = 0;
            float thr = -1.f;   // V80: k dolunca esik = en kotu tutulan benzerlik; tot < esik -> kesin elenir (cift karsilastirmasi (-tot, w) ile ayni sonuc)
            for (int ti = 0; ti < nT; ti++) { int w = touched[ti]; if (tot[w] < thr) continue; if (w == u || cnt[w] < minc) continue; std::pair<float, int> x{-tot[w], w};
                if (k < knn || x < top[k - 1]) { int pos = k < knn ? k++ : k - 1; while (pos > 0 && x < top[pos - 1]) { top[pos] = top[pos - 1]; pos--; } top[pos] = x; if (k == knn) thr = -top[k - 1].first; } }
            if (k < knn) { for (int w = 0; w < V && k < knn; w++) { if (w == u || cnt[w] < minc || tot[w] != 0.f) continue; top[k++] = {-0.f, w}; } }   // S==0 adaylari: artan indeks
            for (int q = 0; q < k; q++) { out_v[(size_t)u * knn + q] = -top[q].first; out_i[(size_t)u * knn + q] = top[q].second; } out_k[u] = k; for (int ti = 0; ti < nT; ti++) tot[touched[ti]] = 0.f;
#pragma omp atomic
            g_knn_cyc[0] += (double)(tc1 - tc0);
#pragma omp atomic
            g_knn_cyc[1] += (double)(__rdtsc() - tc1); } }
    if (st) { st->flops = fl; st->touched = tch; }
}
