// V60: MATEMATIKSEL OLCEKLEME TESTI (model egitimi DEGIL). Sentetik Zipf+kume yapili bigram sayim matrisi uzerinde seyrek kNN cekirdeginin
// (a) dogrulugu (kucuk V: ayni toplama sirasiyla yogun naif hesapla bit-esit), (b) buyuk V'de (50k, 100k) suresi/FLOP'u/RAM'i.
// Kullanim: ./test_knn_sparse V T_carpan [verify]
#include <bits/stdc++.h>
#include <sys/resource.h>
#include "knn_sparse.h"
using namespace std;
static double now() { return chrono::duration<double>(chrono::steady_clock::now().time_since_epoch()).count(); }
int main(int argc, char** argv) {
    int V = atoi(argv[1]); double tm = atof(argv[2]); bool verify = argc > 3; long T = (long)(tm * V); mt19937_64 rng(12345);
    vector<double> cdf(V); double z = 0; for (int i = 0; i < V; i++) { z += 1.0 / (i + 1); cdf[i] = z; }
    auto zipf = [&]() { double r = uniform_real_distribution<double>(0, z)(rng); return (int)(lower_bound(cdf.begin(), cdf.end(), r) - cdf.begin()); };
    double t0 = now(); vector<long> keys; keys.reserve(T); int prev = zipf();
    for (long t = 0; t < T; t++) { int c = prev, y = (rng() & 1) ? zipf() : (int)(((long)c * 7919 + zipf() % 50) % V); keys.push_back((long)c * V + y); prev = y; }
    sort(keys.begin(), keys.end()); SpCSR R; R.V = V; R.off.assign(V + 1, 0); vector<int> cnt(V, 0); vector<float> cv;
    { size_t i = 0; vector<int> ccol; vector<float> cval; vector<int> rowof; while (i < keys.size()) { size_t j = i; while (j < keys.size() && keys[j] == keys[i]) j++; int u = keys[i] / V, v = keys[i] % V; float c = (float)(j - i); R.col.push_back(v); cv.push_back(c); rowof.push_back(u); cnt[u] += (int)c; i = j; }
      for (int u : rowof) R.off[u + 1]++; for (int u = 0; u < V; u++) R.off[u + 1] += R.off[u]; R.val.resize(R.col.size());
      for (int u = 0; u < V; u++) { double nn = 0; for (long e = R.off[u]; e < R.off[u + 1]; e++) { float x = sqrtf(cv[e]); nn += (double)x * x; R.val[e] = x; } nn = sqrt(nn) + 1e-9; for (long e = R.off[u]; e < R.off[u + 1]; e++) R.val[e] /= (float)nn; } }
    double tgen = now() - t0; long nnz = R.col.size();
    vector<float> ov; vector<int> oi, ok; SpKnnStats st; t0 = now(); knn_sparse_topk(R, cnt, 10, 5, 384, ov, oi, ok, &st); double tk = now() - t0;
    struct rusage ru; getrusage(RUSAGE_SELF, &ru);
    printf("OLCEKLEME_TESTI|V=%d token=%ld nnz=%ld (satir basina %.1f; dolu oran %.3e)|uretim=%.2fs|seyrek kNN=%.3fs flop=%.3f GF dokunulan_cift=%ld (V^2'nin %.3f%%)|yogun V^3 esdegeri=%.1f GF|oran=%.2e|tepe RAM=%.0f MB\n", V, T, nnz, (double)nnz / V, (double)nnz / ((double)V * V), tgen, tk, st.flops * 1e-9, st.touched, 100.0 * st.touched / ((double)V * V), 2.0 * V * V * V * 1e-9, st.flops / (2.0 * V * V * V), ru.ru_maxrss / 1024.0);
    if (verify) { vector<int> kb = sp_kblocks(V, 384); long bad = 0; vector<float> row(V);
        for (int u = 0; u < V; u++) { vector<float> d(V, 0.f); vector<float> du(V, 0.f); for (long e = R.off[u]; e < R.off[u + 1]; e++) du[R.col[e]] = R.val[e];
            for (int w = 0; w < V; w++) { vector<float> dw(V, 0.f); (void)dw; }
            vector<float> tot(V, 0.f);
            for (int w = 0; w < V; w++) { float t = 0; long ew = R.off[w]; vector<float> ww; for (size_t b = 0; b + 1 < kb.size(); b++) { float acc = 0; for (long e = ew; e < R.off[w + 1]; e++) { int v = R.col[e]; if (v >= kb[b] && v < kb[b + 1]) acc = fmaf(du[v], R.val[e], acc); } t = fmaf(1.f, acc, t); } tot[w] = t; }
            pair<float, int> top[64]; int k = 0; for (int w = 0; w < V; w++) { if (w == u || cnt[w] < 5) continue; pair<float, int> x{-tot[w], w}; if (k < 10 || x < top[k - 1]) { int pos = k < 10 ? k++ : k - 1; while (pos > 0 && x < top[pos - 1]) { top[pos] = top[pos - 1]; pos--; } top[pos] = x; } }
            if (k != ok[u]) bad++; else for (int q = 0; q < k; q++) if (top[q].second != oi[(size_t)u * 10 + q] || -top[q].first != ov[(size_t)u * 10 + q]) { bad++; break; } }
        printf("DOGRULAMA|yogun naif (ayni toplama sirasi) ile karsilastirma: farkli satir=%ld / %d %s\n", bad, V, bad == 0 ? "(BIT-ESDEGER)" : "(FARK)"); }
}
