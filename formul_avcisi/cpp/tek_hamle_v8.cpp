// Tek hamle V8: METIN uzerinde sonraki-kelime tahmini. Gradyan/Adam/epok/RLS yok.
//   Giris: onceki c kelimenin PPMI-SVD gommelerinin (egitimden) SIRA KORUYARAK birlestirilmesi (K = c*64).
//   W1,b1 (mod "cift"): egitim satirlarindan, HEDEFLERI FARKLI iki satirin farki (birim norm); bias ~ N(0, sigma_b^2) (V3 kazanan yontemi).
//   W1,b1 (mod "rastgele"): V2 rastgele ELM: W~N(0,1/K), b~N(0,0.5^2).
//   F = [1, tanh(gamma (X W1 + b1))],  W2 = (F^T F + lambda I)^-1 F^T T   (T: V=2001 sinifli one-hot)  -> Cholesky, 256 satirlik FP64 birikim.
//   Olasilik icin ham skorlara softmax(z/T) uygulanir; T (tek skaler) dogrulamada secilir (agirlik ogrenmesi degil).
// Kullanim: ./tek_hamle_v8 MOD c H TOHUM [veri=/tmp/v8data]
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle_v8 tek_hamle_v8.cpp
#include <bits/stdc++.h>
#include <omp.h>
#include <sys/resource.h>
using namespace std;
using Clock = chrono::steady_clock;
static double now() { return chrono::duration<double>(Clock::now().time_since_epoch()).count(); }
static double peak_mb() { rusage u; getrusage(RUSAGE_SELF, &u); return u.ru_maxrss / 1024.0; }
static const int IB = 16, JB = 256;
template <class T> vector<T> readbin(const string& p, size_t n) {
    vector<T> v(n); FILE* f = fopen(p.c_str(), "rb"); if (!f) { cerr << "acilamadi: " << p << "\n"; exit(1); }
    if (fread(v.data(), sizeof(T), n, f) != n) { cerr << "kisa dosya: " << p << "\n"; exit(1); } fclose(f); return v;
}
// C[n x N] = A[n x K] * B[K x N]  (satir ana, ldc)
static void gemm_f(const float* A, int n, int K, const float* B, int N, float* C, int ldc) {
    int nib = (n + IB - 1) / IB, njb = (N + JB - 1) / JB;
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int bi = 0; bi < nib; bi++) for (int bj = 0; bj < njb; bj++) {
        int i0 = bi * IB, j0 = bj * JB, ni = min(IB, n - i0), nj = min(JB, N - j0);
        alignas(64) float acc[IB][JB]; memset(acc, 0, sizeof acc);
        for (int k = 0; k < K; k++) { const float* b = B + (size_t)k * N + j0;
            for (int r = 0; r < ni; r++) { float a = A[(size_t)(i0 + r) * K + k]; float* ar = acc[r];
                #pragma omp simd
                for (int j = 0; j < nj; j++) ar[j] += a * b[j]; } }
        for (int r = 0; r < ni; r++) memcpy(C + (size_t)(i0 + r) * ldc + j0, acc[r], nj * sizeof(float));
    }
}
// S (ust ucgen) += F^T F ; 256 satirlik float parca, parcalar arasi double (V7: FP64 birikim)
static void syrk_add(const float* F, int n, int m, double* Sd) {
    int nib = (m + IB - 1) / IB, njb = (m + JB - 1) / JB;
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int bi = 0; bi < nib; bi++) for (int bj = 0; bj < njb; bj++) {
        int i0 = bi * IB, j0 = bj * JB; if (j0 + JB <= i0) continue;
        int ni = min(IB, m - i0), nj = min(JB, m - j0);
        alignas(64) float acc[IB][JB]; memset(acc, 0, sizeof acc);
        for (int k = 0; k < n; k++) { const float* row = F + (size_t)k * m; const float* b = row + j0;
            for (int r = 0; r < ni; r++) { float a = row[i0 + r]; float* ar = acc[r];
                #pragma omp simd
                for (int j = 0; j < nj; j++) ar[j] += a * b[j]; } }
        for (int r = 0; r < ni; r++) for (int j = 0; j < nj; j++) Sd[(size_t)(i0 + r) * m + j0 + j] += acc[r][j];
    }
}
static void syrk_flush256(const float* F, int n, int m, double* S) { for (int s = 0; s < n; s += 256) syrk_add(F + (size_t)s * m, min(256, n - s), m, S); }
// R[m x V] += F^T T (T one-hot)
static void rhs_add(const float* F, const int* y, int n, int m, int V, double* R) {
    int nt = omp_get_max_threads(); vector<vector<double>> loc(nt, vector<double>((size_t)m * V, 0.0));
    #pragma omp parallel
    { auto& L = loc[omp_get_thread_num()];
      #pragma omp for schedule(static)
      for (int k = 0; k < n; k++) { const float* row = F + (size_t)k * m; int c = y[k]; for (int j = 0; j < m; j++) L[(size_t)j * V + c] += row[j]; } }
    for (auto& L : loc) for (size_t i = 0; i < L.size(); i++) R[i] += L[i];
}
static bool cholesky(vector<double>& A, int m) {
    for (int j = 0; j < m; j++) {
        double* Lj = &A[(size_t)j * m]; double s = A[(size_t)j * m + j];
        for (int k = 0; k < j; k++) s -= Lj[k] * Lj[k];
        if (s <= 0) return false; double d = sqrt(s); A[(size_t)j * m + j] = d;
        #pragma omp parallel for schedule(static) if (m - j > 256)
        for (int i = j + 1; i < m; i++) { double* Li = &A[(size_t)i * m]; double t = A[(size_t)j * m + i]; double acc = 0; for (int k = 0; k < j; k++) acc += Li[k] * Lj[k]; Li[j] = (t - acc) / d; }
    }
    return true;
}
// L L^T W = R ; R,W: m x V  (sutun dilimleri uzerinde paralel)
static void chol_solve_multi(const vector<double>& L, int m, const vector<double>& R, int V, vector<float>& Wf) {
    Wf.assign((size_t)m * V, 0.f);
    int nt = omp_get_max_threads(); int chunk = (V + nt - 1) / nt;
    #pragma omp parallel for schedule(static)
    for (int t = 0; t < nt; t++) {
        int c0 = t * chunk, c1 = min(V, c0 + chunk); if (c0 >= c1) continue; int w = c1 - c0; vector<double> X((size_t)m * w);
        for (int i = 0; i < m; i++) memcpy(&X[(size_t)i * w], &R[(size_t)i * V + c0], w * sizeof(double));
        for (int i = 0; i < m; i++) { double* xi = &X[(size_t)i * w]; for (int k = 0; k < i; k++) { double l = L[(size_t)i * m + k]; const double* xk = &X[(size_t)k * w]; for (int c = 0; c < w; c++) xi[c] -= l * xk[c]; } double d = L[(size_t)i * m + i]; for (int c = 0; c < w; c++) xi[c] /= d; }
        for (int i = m - 1; i >= 0; i--) { double* xi = &X[(size_t)i * w]; for (int k = i + 1; k < m; k++) { double l = L[(size_t)k * m + i]; const double* xk = &X[(size_t)k * w]; for (int c = 0; c < w; c++) xi[c] -= l * xk[c]; } double d = L[(size_t)i * m + i]; for (int c = 0; c < w; c++) xi[c] /= d; }
        for (int i = 0; i < m; i++) for (int c = 0; c < w; c++) Wf[(size_t)i * V + c0 + c] = (float)X[(size_t)i * w + c];
    }
}

struct Data { int n = 0; vector<float> X; vector<int> y; };
static int V, K;
static Data load(const string& dir, int c, const string& sp, int n) { Data d; d.n = n; d.X = readbin<float>(dir + "/c" + to_string(c) + "_" + sp + "_X.f32", (size_t)n * K); d.y = readbin<int>(dir + "/c" + to_string(c) + "_" + sp + "_y.i32", n); return d; }

struct Feat { vector<float> W, b; int H; };
static Feat make_w1(const string& mode, const Data& tr, int H, int seed) {
    Feat f; f.H = H; f.W.assign((size_t)K * H, 0.f); f.b.assign(H, 0.f); mt19937_64 g(seed), gb(seed + 7777); normal_distribution<double> N(0, 1);
    if (mode == "rastgele") { for (auto& w : f.W) w = (float)(N(g) / sqrt((double)K)); for (auto& b : f.b) b = (float)(N(g) * 0.5); return f; }
    vector<double> pb(H), w(K); uniform_int_distribution<int> U(0, tr.n - 1);
    for (int j = 0; j < H; j++) {
        int a = U(g), b; do b = U(g); while (b == a || tr.y[b] == tr.y[a]);                                  // hedefleri farkli iki egitim satiri
        const float *xa = &tr.X[(size_t)a * K], *xb = &tr.X[(size_t)b * K]; double nn = 0; for (int k = 0; k < K; k++) { double d = xa[k] - xb[k]; nn += d * d; }
        double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0; for (int k = 0; k < K; k++) { w[k] = (xa[k] - xb[k]) * inv; dot += (xa[k] + xb[k]) * w[k]; f.W[(size_t)k * H + j] = (float)w[k]; }
        pb[j] = -dot / 2;
    }
    double mu = 0, sd = 0; for (double v : pb) mu += v; mu /= H; for (double v : pb) sd += (v - mu) * (v - mu); sd = sqrt(sd / H);
    for (int j = 0; j < H; j++) f.b[j] = (float)(N(gb) * sd);
    return f;
}
// F: n x m, kolon 0 = 1
static void features(const Data& d, const Feat& f, double gam, vector<float>& F) {
    int H = f.H, m = H + 1; F.resize((size_t)d.n * m); gemm_f(d.X.data(), d.n, K, f.W.data(), H, F.data() + 1, m); const float g = (float)gam;
    #pragma omp parallel for
    for (int i = 0; i < d.n; i++) { float* r = &F[(size_t)i * m]; for (int j = 1; j <= H; j++) r[j] = tanhf(g * (r[j] + f.b[j - 1])); r[0] = 1.f; }
}
struct Metrics { double mse_raw, mse_prob, ce, ppl, top1, top5; };
// Fe: n x m, W: m x V, y ; T: softmax sicakligi
static Metrics evaluate(const vector<float>& Fe, const vector<float>& W, int n, int m, const int* y, double T) {
    vector<float> Z((size_t)n * V); gemm_f(Fe.data(), n, m, W.data(), V, Z.data(), V);
    double sraw = 0, sprob = 0, sce = 0; long c1 = 0, c5 = 0;
    #pragma omp parallel for reduction(+ : sraw, sprob, sce, c1, c5) schedule(static)
    for (int i = 0; i < n; i++) {
        const float* z = &Z[(size_t)i * V]; int yy = y[i]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]);
        double se = 0, raw = 0; for (int v = 0; v < V; v++) { double d = z[v] - (v == yy ? 1.0 : 0.0); raw += d * d; se += exp((z[v] - mx) / T); }
        double lse = log(se), pp = 0, py = 0; for (int v = 0; v < V; v++) { double p = exp((z[v] - mx) / T - lse); double d = p - (v == yy ? 1.0 : 0.0); pp += d * d; if (v == yy) py = p; }
        sraw += raw / V; sprob += pp / V; sce += -log(max(py, 1e-300));
        int rank = 0; for (int v = 0; v < V; v++) if (z[v] > z[yy] || (z[v] == z[yy] && v < yy)) rank++;
        c1 += (rank == 0); c5 += (rank < 5);
    }
    Metrics M; M.mse_raw = sraw / n; M.mse_prob = sprob / n; M.ce = sce / n; M.ppl = exp(M.ce); M.top1 = (double)c1 / n; M.top5 = (double)c5 / n; return M;
}

int main(int argc, char** argv) {
    if (argc < 5) { cerr << "kullanim: tek_hamle_v8 cift|rastgele c H TOHUM [veri_klasoru]\n"; return 1; }
    string mode = argv[1]; int c = atoi(argv[2]), H = atoi(argv[3]), seed = atoi(argv[4]); string dir = argc > 5 ? argv[5] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); const int m = H + 1;
    printf("V8 mod=%s c=%d H=%d tohum=%d V=%d K=%d egitim=%d dogrulama=%d test=%d (%d cekirdek)\n", mode.c_str(), c, H, seed, V, K, ntr, nva, nte, omp_get_max_threads());
    double T0 = now(); double t0 = now(); Feat f = make_w1(mode, tr, H, seed); double t_w1 = now() - t0;
    const double GAMS[4] = {0.5, 1, 2, 4}, LAMS[4] = {1, 10, 100, 1000}, TS[9] = {0.003, 0.005, 0.01, 0.02, 0.03, 0.05, 0.07, 0.1, 0.2};
    struct Best { double ce = 1e30, gam = 0, lam = 0, T = 0; int gi = 0; } best; vector<vector<double>> Sall(4), Rall(4); vector<double> t_stats(4);
    vector<float> Ftr, Fva;
    for (int gi = 0; gi < 4; gi++) {
        double gam = GAMS[gi]; t0 = now(); features(tr, f, gam, Ftr); features(va, f, gam, Fva);
        vector<double> S((size_t)m * m, 0.0), R((size_t)m * V, 0.0); syrk_flush256(Ftr.data(), ntr, m, S.data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, R.data()); t_stats[gi] = now() - t0;
        for (int li = 0; li < 4; li++) {
            vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += LAMS[li]; if (!cholesky(A, m)) { printf("  gamma=%g lambda=%g Cholesky basarisiz\n", gam, LAMS[li]); continue; }
            vector<float> W; chol_solve_multi(A, m, R, V, W);
            double pce = 1e30, pT = 0; for (double T : TS) { Metrics M = evaluate(Fva, W, nva, m, va.y.data(), T); if (M.ce < pce) { pce = M.ce; pT = T; } if (M.ce < best.ce) best = {M.ce, gam, LAMS[li], T, gi}; }
            printf("  gamma=%g lambda=%g en_iyi_T=%g val_CE=%.4f\n", gam, LAMS[li], pT, pce); fflush(stdout);
        }
        // dogrulama istatistigini ekle (egitim+dogrulama son model icin, toplanabilir)
        syrk_flush256(Fva.data(), nva, m, S.data()); rhs_add(Fva.data(), va.y.data(), nva, m, V, R.data()); Sall[gi] = move(S); Rall[gi] = move(R);
    }
    double t_search = now() - T0;
    // son model: egitim+dogrulama istatistigi, secilen (gamma, lambda); T secilen
    t0 = now(); vector<double> A = Sall[best.gi]; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += best.lam; cholesky(A, m); vector<float> W; chol_solve_multi(A, m, Rall[best.gi], V, W); double t_final = now() - t0;
    double t_stat_final = t_stats[best.gi] * (double)(ntr + nva) / ntr;
    vector<float> Fte; features(te, f, best.gam, Fte); Metrics M = evaluate(Fte, W, nte, m, te.y.data(), best.T);
    double T1 = now() - T0;
    printf("SECILEN gamma=%g lambda=%g T=%g val_CE=%.4f\n", best.gam, best.lam, best.T, best.ce);
    printf("TEST mse_ham=%.6f mse_olasilik=%.6f CE=%.4f PPL=%.2f top1=%.4f top5=%.4f\n", M.mse_raw, M.mse_prob, M.ce, M.ppl, M.top1, M.top5);
    double t_train = t_w1 + t_stat_final + t_final;
    printf("SURE W1_uretim=%.3fs tek_model_egitim(W1+istatistik(egitim+dogrulama)+1 Cholesky/cozum)=%.2fs ayar_aramasi_dahil_toplam=%.2fs\n", t_w1, t_train, T1);
    printf("OZET|%s|c=%d|H=%d|tohum=%d|gam=%g|lam=%g|T=%g|valCE=%.4f|mse_ham=%.6f|mse_olas=%.6f|CE=%.4f|PPL=%.2f|top1=%.4f|top5=%.4f|egitim_sn=%.2f|toplam_sn=%.2f|RAM_MB=%.0f\n", mode.c_str(), c, H, seed, best.gam, best.lam, best.T, best.ce, M.mse_raw, M.mse_prob, M.ce, M.ppl, M.top1, M.top5, t_train, T1, peak_mb());
    return 0;
}
