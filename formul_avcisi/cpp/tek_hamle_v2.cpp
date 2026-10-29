// Tek hamlede agirlik uretimi V2 (MNIST). GRADYAN YOK, ADAM YOK, EPOK YOK.
//   mod "cift"  : W1,b1 egitim verisinden HESAPLANIR (her ozellik icin farkli sinif ciftine ait iki ornek):
//                   w_j = (xa-xb)/(|xa-xb|+eps),  b_j = -(xa+xb)^T w_j / 2
//   mod "rastgele": referans ELM (W1 ~ N(0,1/d), b1 ~ N(0,0.5^2)), AYNI kod, AYNI izgara.
//   F  = [tanh(gamma (X W1 + b1)), 1]
//   W2 = (F^T F + lambda I)^-1 F^T T   -> ters alinmaz, Cholesky (S W2 = R)
// Hiperparametreler (gamma, lambda) 5000 ornekli DOGRULAMA kumesinde secilir (55000 fit / 5000 dogrulama).
// Cift secimi yalniz fit kumesinden; test verisi yalniz sonda, secilen ayarla BIR kez kullanilir.
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle_v2 tek_hamle_v2.cpp
// Calistirma: ./tek_hamle_v2 cift|rastgele H [tohum=1] [veri_klasoru=/tmp/mnist_bin] [gamma listesi a,b,c,d (ek tarama)]
#include <bits/stdc++.h>
#include <omp.h>
#include <sys/resource.h>
using namespace std;
using Clock = chrono::steady_clock;
static double now() { return chrono::duration<double>(Clock::now().time_since_epoch()).count(); }

static const int D = 784, NC = 10, NTR = 60000, NTE = 10000, NFIT = 55000;
static const int IB = 16, JB = 256, CH = 2048;

template <class T> vector<T> readbin(const string& p, size_t n) {
    vector<T> v(n); FILE* f = fopen(p.c_str(), "rb"); if (!f) { cerr << "acilamadi: " << p << "\n"; exit(1); }
    if (fread(v.data(), sizeof(T), n, f) != n) { cerr << "kisa dosya: " << p << "\n"; exit(1); } fclose(f); return v;
}

// Z[n x H] = X[n x D] * W[D x H]   (satir ana, ldz = Z satir adimi)
static void gemm_nn(const float* X, int n, const float* W, int H, float* Z, int ldz) {
    int nib = (n + IB - 1) / IB, njb = (H + JB - 1) / JB;
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int bi = 0; bi < nib; bi++) for (int bj = 0; bj < njb; bj++) {
        int i0 = bi * IB, j0 = bj * JB, ni = min(IB, n - i0), nj = min(JB, H - j0);
        alignas(64) float acc[IB][JB]; memset(acc, 0, sizeof acc);
        for (int k = 0; k < D; k++) {
            const float* b = W + (size_t)k * H + j0;
            for (int r = 0; r < ni; r++) { float a = X[(size_t)(i0 + r) * D + k]; float* ar = acc[r];
                #pragma omp simd
                for (int j = 0; j < nj; j++) ar[j] += a * b[j]; }
        }
        for (int r = 0; r < ni; r++) memcpy(Z + (size_t)(i0 + r) * ldz + j0, acc[r], nj * sizeof(float));
    }
}

// Sd[m x m] (ust ucgen) += F^T F ; F[n x m] satir ana. Parca ici float, parcalar arasi double.
static void syrk_add(const float* F, int n, int m, double* Sd) {
    int nib = (m + IB - 1) / IB, njb = (m + JB - 1) / JB;
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int bi = 0; bi < nib; bi++) for (int bj = 0; bj < njb; bj++) {
        int i0 = bi * IB, j0 = bj * JB; if (j0 + JB <= i0) continue;     // tamamen alt ucgen
        int ni = min(IB, m - i0), nj = min(JB, m - j0);
        alignas(64) float acc[IB][JB]; memset(acc, 0, sizeof acc);
        for (int k = 0; k < n; k++) {
            const float* row = F + (size_t)k * m; const float* b = row + j0;
            for (int r = 0; r < ni; r++) { float a = row[i0 + r]; float* ar = acc[r];
                #pragma omp simd
                for (int j = 0; j < nj; j++) ar[j] += a * b[j]; }
        }
        for (int r = 0; r < ni; r++) for (int j = 0; j < nj; j++) Sd[(size_t)(i0 + r) * m + j0 + j] += acc[r][j];
    }
}

// R[m x NC] += F^T T
static void rhs_add(const float* F, const int* y, int n, int m, double* R) {
    int nt = omp_get_max_threads(); vector<vector<double>> loc(nt, vector<double>((size_t)m * NC, 0.0));
    #pragma omp parallel
    { auto& L = loc[omp_get_thread_num()];
      #pragma omp for schedule(static)
      for (int k = 0; k < n; k++) { const float* row = F + (size_t)k * m; int c = y[k]; for (int j = 0; j < m; j++) L[(size_t)j * NC + c] += row[j]; } }
    for (auto& L : loc) for (size_t i = 0; i < L.size(); i++) R[i] += L[i];
}

// Cholesky: A = L L^T (A: m x m, ust ucgen dolu; yerinde alt ucgene L yazilir). Paralel, satir ana.
static bool cholesky(vector<double>& A, int m) {
    for (int j = 0; j < m; j++) {
        double* Lj = &A[(size_t)j * m]; double s = A[(size_t)j * m + j];
        for (int k = 0; k < j; k++) s -= Lj[k] * Lj[k];
        if (s <= 0) return false; double d = sqrt(s); A[(size_t)j * m + j] = d;
        #pragma omp parallel for schedule(static) if (m - j > 256)
        for (int i = j + 1; i < m; i++) {
            double* Li = &A[(size_t)i * m]; double t = A[(size_t)j * m + i];   // A[i][j] icin ust ucgen: A[j][i]
            double acc = 0; for (int k = 0; k < j; k++) acc += Li[k] * Lj[k];
            Li[j] = (t - acc) / d;
        }
    }
    return true;
}
// S W = R ; A'nin alt ucgeni L (yukaridaki cholesky cikisi)
static void chol_solve(const vector<double>& L, int m, const vector<double>& R, vector<double>& W) {
    W = R;
    for (int i = 0; i < m; i++) { for (int c = 0; c < NC; c++) { double s = W[(size_t)i * NC + c]; for (int k = 0; k < i; k++) s -= L[(size_t)i * m + k] * W[(size_t)k * NC + c]; W[(size_t)i * NC + c] = s / L[(size_t)i * m + i]; } }
    for (int i = m - 1; i >= 0; i--) { for (int c = 0; c < NC; c++) { double s = W[(size_t)i * NC + c]; for (int k = i + 1; k < m; k++) s -= L[(size_t)k * m + i] * W[(size_t)k * NC + c]; W[(size_t)i * NC + c] = s / L[(size_t)i * m + i]; } }
}

static double peak_mb() { rusage u; getrusage(RUSAGE_SELF, &u); return u.ru_maxrss / 1024.0; }

int main(int argc, char** argv) {
    if (argc < 3) { cerr << "kullanim: tek_hamle_v2 cift|rastgele H [tohum] [veri_klasoru]\n"; return 1; }
    string mod = argv[1]; int H = atoi(argv[2]); int seed = argc > 3 ? atoi(argv[3]) : 1; string dir = argc > 4 ? argv[4] : "/tmp/mnist_bin";
    if (mod != "cift" && mod != "rastgele") { cerr << "mod: cift|rastgele\n"; return 1; }
    auto Xtr = readbin<float>(dir + "/Xtr.f32", (size_t)NTR * D); auto Xte = readbin<float>(dir + "/Xte.f32", (size_t)NTE * D);
    auto ytr = readbin<int>(dir + "/ytr.i32", NTR); auto yte = readbin<int>(dir + "/yte.i32", NTE);
    const int m = H + 1;
    double gammas[4] = {0.5, 1, 2, 4}; const double lams[] = {1, 10, 100, 1000};
    if (argc > 5) { double a, b, c, d; if (sscanf(argv[5], "%lf,%lf,%lf,%lf", &a, &b, &c, &d) == 4) { gammas[0] = a; gammas[1] = b; gammas[2] = c; gammas[3] = d; } }   // yalniz ek tarama icin
    printf("MOD=%s H=%d tohum=%d  (%d cekirdek)  fit=%d dogrulama=%d test=%d\n", mod.c_str(), H, seed, omp_get_max_threads(), NFIT, NTR - NFIT, NTE);
    double T0 = now();

    // ---- 1) W1, b1
    vector<float> W1((size_t)D * H), b1(H); mt19937_64 g(seed);
    double tw0 = now();
    if (mod == "rastgele") {
        normal_distribution<double> N(0, 1); for (auto& w : W1) w = (float)(N(g) / sqrt((double)D)); for (auto& b : b1) b = (float)(N(g) * 0.5);
    } else {
        vector<vector<int>> byc(NC); for (int i = 0; i < NFIT; i++) byc[ytr[i]].push_back(i);   // YALNIZ fit kumesi
        uniform_int_distribution<int> UC(0, NC - 1); const double eps = 1e-8;
        for (int j = 0; j < H; j++) {
            int ca = UC(g), cb; do cb = UC(g); while (cb == ca);                                  // farkli sinif
            int a = byc[ca][uniform_int_distribution<int>(0, (int)byc[ca].size() - 1)(g)], b = byc[cb][uniform_int_distribution<int>(0, (int)byc[cb].size() - 1)(g)];
            const float *xa = &Xtr[(size_t)a * D], *xb = &Xtr[(size_t)b * D]; double nn = 0; for (int k = 0; k < D; k++) { double d = xa[k] - xb[k]; nn += d * d; }
            double inv = 1.0 / (sqrt(nn) + eps), dot = 0;
            for (int k = 0; k < D; k++) { double w = (xa[k] - xb[k]) * inv; W1[(size_t)k * H + j] = (float)w; dot += (xa[k] + xb[k]) * w; }
            b1[j] = (float)(-dot / 2);
        }
    }
    double t_w = now() - tw0;

    // ---- 2) ozellik + istatistik (parca parca; Z her gamma icin yeniden hesaplanir -> dusuk RAM)
    auto features = [&](const float* X, int n, double gam, vector<float>& F) {
        F.resize((size_t)n * m); gemm_nn(X, n, W1.data(), H, F.data(), m);
        #pragma omp parallel for
        for (int i = 0; i < n; i++) { float* r = &F[(size_t)i * m]; for (int j = 0; j < H; j++) r[j] = tanhf((float)gam * (r[j] + b1[j])); r[H] = 1.f; }
    };
    auto accumulate = [&](int lo, int hi, double gam, vector<double>& S, vector<double>& R) {
        vector<float> F;
        for (int s = lo; s < hi; s += CH) { int n = min(CH, hi - s); features(&Xtr[(size_t)s * D], n, gam, F); syrk_add(F.data(), n, m, S.data()); rhs_add(F.data(), &ytr[s], n, m, R.data()); }
    };
    auto accuracy = [&](const float* X, const int* y, int n, double gam, const vector<double>& W) {
        vector<float> F; long ok = 0;
        for (int s = 0; s < n; s += CH) { int nn = min(CH, n - s); features(X + (size_t)s * D, nn, gam, F);
            #pragma omp parallel for reduction(+:ok)
            for (int i = 0; i < nn; i++) { double z[NC] = {0}; const float* f = &F[(size_t)i * m]; for (int j = 0; j < m; j++) for (int c = 0; c < NC; c++) z[c] += f[j] * W[(size_t)j * NC + c];
                int bc = 0; for (int c = 1; c < NC; c++) if (z[c] > z[bc]) bc = c; ok += (bc == y[s + i]); } }
        return (double)ok / n;
    };

    struct Best { double val = -1, gam = 0, lam = 0; } best;
    vector<vector<double>> Sall(4), Rall(4); vector<double> t_stats(4);
    printf("\n  gamma   lambda   dogrulama_dogruluk\n");
    for (int gi = 0; gi < 4; gi++) {
        double gam = gammas[gi], t1 = now();
        vector<double> Sf((size_t)m * m, 0.0), Rf((size_t)m * NC, 0.0), Sv((size_t)m * m, 0.0), Rv((size_t)m * NC, 0.0);
        accumulate(0, NFIT, gam, Sf, Rf); accumulate(NFIT, NTR, gam, Sv, Rv);
        t_stats[gi] = now() - t1;
        for (int li = 0; li < 4; li++) {
            vector<double> A = Sf; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lams[li];
            // ust ucgen -> cholesky satirlari icin: A[j][i] (j<i) kullanir, ust ucgen dolu
            vector<double> W; bool ok = cholesky(A, m); double va = -1;
            if (ok) chol_solve(A, m, Rf, W);
            if (ok) { va = accuracy(&Xtr[(size_t)NFIT * D], &ytr[NFIT], NTR - NFIT, gam, W); }
            printf("  %6.3f  %7.0f   %.4f%s\n", gam, lams[li], va, ok ? "" : "  (Cholesky basarisiz)"); fflush(stdout);
            if (va > best.val) best = {va, gam, lams[li]};
        }
        for (size_t i = 0; i < Sf.size(); i++) Sf[i] += Sv[i]; for (size_t i = 0; i < Rf.size(); i++) Rf[i] += Rv[i];
        Sall[gi] = move(Sf); Rall[gi] = move(Rf);                      // S_fit + S_dogrulama (toplanabilir)
    }
    double t_search = now() - T0;

    // ---- 3) son cozum: secilen ayar, 60k tum egitim, TEST bir kez
    int bg = 0; for (int i = 0; i < 4; i++) if (gammas[i] == best.gam) bg = i;
    double t3 = now(); vector<double> A = Sall[bg]; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += best.lam;
    vector<double> W; cholesky(A, m); chol_solve(A, m, Rall[bg], W); double t_final_solve = now() - t3;
    double test = accuracy(Xte.data(), yte.data(), NTE, best.gam, W);
    double T1 = now() - T0;
    double t_fit_single = t_w + t_stats[bg] + t_final_solve;   // tek model: W1 + (fit+dogr. istatistik) + 1 cozum
    printf("\nSECILEN (dogrulamaya gore): gamma=%.3f lambda=%.0f  dogrulama=%.4f\n", best.gam, best.lam, best.val);
    printf("TEST DOGRULUGU: %.4f\n", test);
    printf("W1,b1 uretim suresi: %.2f sn\n", t_w);
    printf("TEK MODEL egitim suresi (W1 + 60k istatistik + 1 Cholesky; ayar aramasi HARIC): %.1f sn\n", t_fit_single);
    printf("TOPLAM sure (ayar izgarasi 16 nokta + son cozum + test): %.1f sn\n", T1);
    printf("Tepe RAM (RSS): %.0f MB   | S matrisi (double) %.0f MB x (4 gamma saklanan + calisma kopyalari)\n", peak_mb(), (double)m * m * 8 / 1e6);
    printf("OZET|%s|%d|%d|%.3f|%.0f|%.4f|%.4f|%.1f|%.1f|%.0f\n", mod.c_str(), H, seed, best.gam, best.lam, best.val, test, t_fit_single, T1, peak_mb());
    return 0;
}
