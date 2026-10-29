// V60: MATEMATIKSEL OLCEKLEME TESTI (model egitimi DEGIL). Sentetik Zipf+kume yapili bigram sayim matrisi uzerinde seyrek kNN cekirdeginin
// (a) dogrulugu (kucuk V: ayni toplama sirasiyla yogun naif hesapla bit-esit), (b) buyuk V'de (50k, 100k) suresi/FLOP'u/RAM'i.
// Kullanim: ./test_knn_sparse V T_carpan [verify]
#include <bits/stdc++.h>
#include <sys/resource.h>
#include "knn_sparse.h"
using namespace std;
static double now() { return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count(); }

// V61: MATEMATIKSEL OLCEKLEME TESTI (model egitimi DEGIL): seyrek kNN + s(K)=Q.E hesabi (Q hic olusmadan), sentetik Zipf+kume bigram sayimi.
static int V; static int g_knn39 = 10; static double g_qkap53 = 3, g_bq46 = 0.9; static int g_qpi53 = 1;
struct Pool39 { vector<int> nb; vector<float> wt; };
struct CountCSR { vector<long> off; vector<int> col; vector<float> val; };
static void v61_stab(const CountCSR& C, const Pool39& P, const vector<double>& Ptr_, const vector<float>& Et, bool shuf, vector<float>& S) {
    double t0 = now(); vector<double> nw(V, 0.0);
#pragma omp parallel for schedule(static)
    for (int w = 0; w < V; w++) for (long e = C.off[w]; e < C.off[w + 1]; e++) nw[w] += C.val[e];
    mt19937_64 gs(9001); vector<int> pm(V); iota(pm.begin(), pm.end(), 0); if (shuf) shuffle(pm.begin(), pm.end(), gs);
    auto dot64 = [&](const vector<double>& vec, vector<double>& out, double& sum) { out.assign(64, 0.0); sum = 0; for (int v = 0; v < V; v++) { sum += vec[v]; const float* e = &Et[(size_t)v * 64]; double a = vec[v]; for (int j = 0; j < 64; j++) out[j] += a * e[j]; } };
    vector<double> pi(V, 0.0), PiEt, PtrEt; double sumPi = 0, sumPtr = 0;
    if (g_qkap53 > 0) { if (g_qpi53 == 1) { double t = 0; for (long e = 0; e < (long)C.col.size(); e++) if (C.val[e] > 0) pi[C.col[e]] += 1.0; for (int v = 0; v < V; v++) { pi[v] += 1.0; t += pi[v]; } for (int v = 0; v < V; v++) pi[v] /= t; } else pi = Ptr_; dot64(pi, PiEt, sumPi); }
    dot64(Ptr_, PtrEt, sumPtr);
    S.assign((size_t)V * 64, 0.f); const double bq = g_bq46; const int kn = g_knn39;
#pragma omp parallel
    { vector<double> N(V, 0.0), Cw(V, 0.0); vector<char> inS(V, 0); vector<int> sup;
#pragma omp for schedule(dynamic, 16)
        for (int w = 0; w < V; w++) { sup.clear(); double ws = 0;
            for (int qk = 0; qk < kn; qk++) { int nb = P.nb[(size_t)w * kn + qk]; if (nb < 0) continue; nb = pm[nb]; if (nw[nb] <= 0) continue; double a = P.wt[(size_t)w * kn + qk]; ws += a; for (long e = C.off[nb]; e < C.off[nb + 1]; e++) { int v = C.col[e]; N[v] += a * C.val[e] / nw[nb]; if (!inS[v]) { inS[v] = 1; sup.push_back(v); } } }
            bool hasN = ws > 0; if (hasN) for (int v : sup) N[v] /= ws;
            if (nw[w] > 0) for (long e = C.off[w]; e < C.off[w + 1]; e++) { int v = C.col[e]; Cw[v] = C.val[e]; if (!inS[v]) { inS[v] = 1; sup.push_back(v); } }
            sort(sup.begin(), sup.end()); vector<double> qv(sup.size(), 0.0); double sum = 0;
            if (nw[w] > 0 && hasN) for (size_t t = 0; t < sup.size(); t++) qv[t] = (1 - bq) * Cw[sup[t]] / nw[w] + bq * N[sup[t]];
            else if (nw[w] > 0) for (size_t t = 0; t < sup.size(); t++) qv[t] = Cw[sup[t]] / nw[w];
            else if (hasN) for (size_t t = 0; t < sup.size(); t++) qv[t] = N[sup[t]];
            for (size_t t = 0; t < sup.size(); t++) sum += qv[t];
            float* so = &S[(size_t)w * 64];
            if (!(sum > 0) || (nw[w] <= 0 && !hasN)) { for (int j = 0; j < 64; j++) so[j] = (float)(PtrEt[j] / sumPtr); }   // q = Ptr (normalize)
            else { for (size_t t = 0; t < sup.size(); t++) qv[t] /= sum; double sumq = 0; double acc[64] = {0}; for (size_t t = 0; t < sup.size(); t++) { sumq += qv[t]; const float* e = &Et[(size_t)sup[t] * 64]; double a = qv[t]; for (int j = 0; j < 64; j++) acc[j] += a * e[j]; }
                if (g_qkap53 > 0 && nw[w] > 0) { double lam = nw[w] / (nw[w] + g_qkap53); double sum2 = lam * sumq + (1 - lam) * sumPi; for (int j = 0; j < 64; j++) so[j] = (float)((lam * acc[j] + (1 - lam) * PiEt[j]) / sum2); }
                else for (int j = 0; j < 64; j++) so[j] = (float)acc[j]; }
            for (int v : sup) { N[v] = 0; Cw[v] = 0; inS[v] = 0; } } }
    }

int main(int argc, char** argv) {
    V = atoi(argv[1]); double tm = atof(argv[2]); long T = (long)(tm * V); mt19937_64 rng(12345);
    vector<double> cdf(V); double z = 0; for (int i = 0; i < V; i++) { z += 1.0 / (i + 1); cdf[i] = z; }
    auto zipf = [&]() { double r = uniform_real_distribution<double>(0, z)(rng); return (int)(lower_bound(cdf.begin(), cdf.end(), r) - cdf.begin()); };
    double t0 = now(); vector<long> keys; keys.reserve(T); int prev = zipf();
    for (long t = 0; t < T; t++) { int c = prev, y = (rng() & 1) ? zipf() : (int)(((long)c * 7919 + zipf() % 50) % V); keys.push_back((long)c * V + y); prev = y; }
    sort(keys.begin(), keys.end()); CountCSR C; C.off.assign(V + 1, 0); for (size_t i = 0; i < keys.size();) { size_t j = i; while (j < keys.size() && keys[j] == keys[i]) j++; C.col.push_back((int)(keys[i] % V)); C.val.push_back((float)(j - i)); C.off[keys[i] / V + 1]++; i = j; }
    for (int u = 0; u < V; u++) C.off[u + 1] += C.off[u]; keys.clear(); keys.shrink_to_fit();
    SpCSR R; R.V = V; R.off = C.off; R.col = C.col; R.val.resize(C.val.size()); vector<int> cnt(V, 0);
    for (int u = 0; u < V; u++) { double nn = 0; for (long e = C.off[u]; e < C.off[u + 1]; e++) { float x = sqrtf(C.val[e]); nn += (double)x * x; cnt[u] += (int)C.val[e]; R.val[e] = x; } nn = sqrt(nn) + 1e-9; for (long e = C.off[u]; e < C.off[u + 1]; e++) R.val[e] /= (float)nn; }
    double tgen = now() - t0; t0 = now(); vector<float> ov; vector<int> oi, ok; SpKnnStats st; knn_sparse_topk(R, cnt, g_knn39, 5, 384, ov, oi, ok, &st); double tk = now() - t0;
    Pool39 P; P.nb.assign((size_t)V * g_knn39, -1); P.wt.assign((size_t)V * g_knn39, 0.f);
    for (int u = 0; u < V; u++) { double sm = 0; for (int q = 0; q < ok[u]; q++) sm += ov[(size_t)u * g_knn39 + q]; for (int q = 0; q < ok[u]; q++) { P.nb[(size_t)u * g_knn39 + q] = oi[(size_t)u * g_knn39 + q]; P.wt[(size_t)u * g_knn39 + q] = (float)(ov[(size_t)u * g_knn39 + q] / max(sm, 1e-9)); } }
    vector<float> Et((size_t)V * 64); { mt19937 r2(7); normal_distribution<float> Nd(0, 1); for (auto& x : Et) x = Nd(r2) * 0.125f; }
    vector<double> Ptr(V); { double s = 0; for (int v = 0; v < V; v++) { Ptr[v] = 1.0 / (v + 10); s += Ptr[v]; } for (auto& x : Ptr) x /= s; }
    vector<float> S; t0 = now(); v61_stab(C, P, Ptr, Et, false, S); double ts = now() - t0; bool fin = true; for (float x : S) if (!std::isfinite(x)) fin = false;
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    printf("Q_OLCEKLEME_TESTI|V=%d nnz=%zu|uretim=%.2fs|seyrek kNN=%.2fs (%.1f GF)|s(K)=Q.E (Q olusmadan)=%.2fs sonlu=%d|tepe RAM=%.0f MB|yogun Q tablosu olsaydi: %.1f GB/tablo (x6 = %.0f GB)|yogun Cn(float): %.1f GB\n", V, C.col.size(), tgen, tk, st.flops * 1e-9, ts, (int)fin, ru.ru_maxrss / 1024.0, (double)V * V * 8 / 1e9, 6.0 * V * V * 8 / 1e9, (double)V * V * 4 / 1e9);
}
