// Tek hamle V5: ONCEKI AGIRLIKTAN DEVAM (RLS) vs BASTAN/KESIN COZUM. Gradyan/Adam/epok yok.
// RLS: K = P F^T (I + F P F^T)^-1 ; W += K (T - F W) ; P -= K F P ; baslangic W=0, P=I/lambda.
// Ozellik matrisi sabit (V4 sabit-W1: W1 yalniz gorev-1 verisinden). V4 dosyalarina dokunulmadi (kernel/yardimcilar V4'ten kopya).
// Kullanim: ./tek_hamle_v5 MOD H TOHUM [gamma=0.125] [lambda=1] [blok=64] [gorev_sayisi=3] [veri_klasoru]
//   MOD: kesin (V4 sabit-W1: yalniz yeni gorev istatistigi eklenir + Cholesky) | bastan (her gorevde gorulen TUM veriden sifirdan) | rls | ref64 (kesin ile ayni, ama S,R double toplamla: dogruluk referansi)
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle_v5 tek_hamle_v5.cpp
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
static vector<float> Xtr, Xte; static vector<int> ytr, yte;
static vector<vector<int>> byc;                                  // sinif -> fit satirlari (ilk NFIT)
static double GAM = 0.125, LAM = 1.0;

// ---------------------------------------------------------------- ozellik bloklari (W1 buyur)
struct Block { vector<float> W; vector<float> b; int h = 0; };      // W: D x h (satir ana)
struct Feat { vector<Block> blocks; int H = 0; vector<float> Wc, bc; };
static void rebuild(Feat& f) {                                      // bloklari tek D x H matrise birlestir
    f.H = 0; for (auto& bl : f.blocks) f.H += bl.h; f.Wc.assign((size_t)D * f.H, 0.f); f.bc.assign(f.H, 0.f); int off = 0;
    for (auto& bl : f.blocks) { for (int k = 0; k < D; k++) memcpy(&f.Wc[(size_t)k * f.H + off], &bl.W[(size_t)k * bl.h], bl.h * sizeof(float)); memcpy(&f.bc[off], bl.b.data(), bl.h * sizeof(float)); off += bl.h; }
}
// h yeni yon: verilen siniflardaki fit satirlarindan cekilen farkli-sinif ciftlerinin farki (birim norm); bias ~ N(0, sigma_b^2), sigma_b = ayni ciftlerin orta-nokta bias'larinin std'si
static Block make_block(const vector<int>& classes, int h, uint64_t seed) {
    Block o; o.h = h; o.W.assign((size_t)D * h, 0.f); o.b.assign(h, 0.f);
    mt19937_64 gp(seed), gb(seed + 7777); normal_distribution<double> N(0, 1); int na = classes.size();
    auto pick = [&](int c) { return byc[c][uniform_int_distribution<int>(0, (int)byc[c].size() - 1)(gp)]; };
    vector<double> pb(h); vector<double> w(D);
    for (int j = 0; j < h; j++) {
        int ca = classes[uniform_int_distribution<int>(0, na - 1)(gp)], cb = ca;
        if (na > 1) { do cb = classes[uniform_int_distribution<int>(0, na - 1)(gp)]; while (cb == ca); }
        int a = pick(ca), b = pick(cb); if (na == 1) { while (b == a) b = pick(cb); }
        const float *xa = &Xtr[(size_t)a * D], *xb = &Xtr[(size_t)b * D]; double nn = 0; for (int k = 0; k < D; k++) { double d = xa[k] - xb[k]; nn += d * d; }
        double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0; for (int k = 0; k < D; k++) { w[k] = (xa[k] - xb[k]) * inv; dot += (xa[k] + xb[k]) * w[k]; o.W[(size_t)k * h + j] = (float)w[k]; }
        pb[j] = -dot / 2;
    }
    double mu = 0, sd = 0; for (double v : pb) mu += v; mu /= h; for (double v : pb) sd += (v - mu) * (v - mu); sd = sqrt(sd / h);
    for (int j = 0; j < h; j++) o.b[j] = (float)(N(gb) * sd);
    return o;
}
// F: n x m, m = H+1 ; kolon 0 = bias=1
static void features(const float* X, int n, const Feat& f, vector<float>& F) {
    int H = f.H, m = H + 1; F.resize((size_t)n * m); gemm_nn(X, n, f.Wc.data(), H, F.data() + 1, m);
    const float g = (float)GAM;
    #pragma omp parallel for
    for (int i = 0; i < n; i++) { float* r = &F[(size_t)i * m]; for (int j = 1; j <= H; j++) r[j] = tanhf(g * (r[j] + f.bc[j - 1])); r[0] = 1.f; }
}
// satir listesi (Xtr satirlari) icin S (ust ucgen) += F^T F, R += F^T T
static void stats_rows(const vector<int>& idx, const Feat& f, vector<double>& S, vector<double>& R) {
    int m = f.H + 1; vector<float> Xb, F; vector<int> yb;
    for (size_t s = 0; s < idx.size(); s += CH) {
        int n = min((size_t)CH, idx.size() - s); Xb.resize((size_t)n * D); yb.resize(n);
        for (int i = 0; i < n; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)idx[s + i] * D], D * sizeof(float)); yb[i] = ytr[idx[s + i]]; }
        features(Xb.data(), n, f, F); syrk_add(F.data(), n, m, S.data()); rhs_add(F.data(), yb.data(), n, m, R.data());
    }
}
static int retries = 0; static double lam_used = 0; static int last_fail = 0;
static void solve_sys(const vector<double>& S, const vector<double>& R, int m, vector<double>& W) {
    double lam = LAM;
    for (int tr = 0; tr < 6; tr++) {
        vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam;
        if (cholesky(A, m)) { chol_solve(A, m, R, W); lam_used = lam; last_fail = 0; return; }
        retries++; lam *= 10;
    }
    fprintf(stderr, "Cholesky basarisiz\n"); W.assign((size_t)m * NC, 0.0); lam_used = -1; last_fail = 1;
}
static void pad_add(const vector<double>& So, int mo, vector<double>& Sn, int mn) { for (int i = 0; i < mo; i++) for (int j = i; j < mo; j++) Sn[(size_t)i * mn + j] += So[(size_t)i * mo + j]; }
static void padR_add(const vector<double>& Ro, int mo, vector<double>& Rn) { for (size_t i = 0; i < (size_t)mo * NC; i++) Rn[i] += Ro[i]; }   // R satirlari bastan hizali

// ---------------------------------------------------------------- degerlendirme (sinif basina SSE/adet/dogru)
struct Ev { array<double, NC> sse, cnt, cor; };
static vector<Ev> eval_ws(const float* Xbase, const int* ylab, const vector<int>* idx, int n, const Feat& f, const vector<const vector<double>*>& Ws) {
    int m = f.H + 1; vector<Ev> out(Ws.size()); for (auto& e : out) { e.sse.fill(0); e.cnt.fill(0); e.cor.fill(0); }
    vector<float> Xb, F;
    for (int s = 0; s < n; s += CH) {
        int nn = min(CH, n - s); const float* Xp; vector<int> yb(nn);
        if (idx) { Xb.resize((size_t)nn * D); for (int i = 0; i < nn; i++) { memcpy(&Xb[(size_t)i * D], &Xbase[(size_t)(*idx)[s + i] * D], D * sizeof(float)); yb[i] = ylab[(*idx)[s + i]]; } Xp = Xb.data(); }
        else { Xp = Xbase + (size_t)s * D; for (int i = 0; i < nn; i++) yb[i] = ylab[s + i]; }
        features(Xp, nn, f, F);
        for (size_t w = 0; w < Ws.size(); w++) {
            const vector<double>& W = *Ws[w]; vector<array<double, 3>> acc((size_t)omp_get_max_threads() * NC, array<double, 3>{0, 0, 0});
            #pragma omp parallel
            { int t = omp_get_thread_num();
              #pragma omp for
              for (int i = 0; i < nn; i++) { double z[NC] = {0}; const float* fr = &F[(size_t)i * m]; for (int j = 0; j < m; j++) { double fj = fr[j]; const double* wj = &W[(size_t)j * NC]; for (int c = 0; c < NC; c++) z[c] += fj * wj[c]; }
                  int y = yb[i]; double se = 0; int bc = 0; for (int c = 0; c < NC; c++) { double d = z[c] - (c == y ? 1.0 : 0.0); se += d * d; if (z[c] > z[bc]) bc = c; }
                  auto& a = acc[(size_t)t * NC + y]; a[0] += se; a[1] += 1; a[2] += (bc == y); } }
            for (int t = 0; t < omp_get_max_threads(); t++) for (int c = 0; c < NC; c++) { out[w].sse[c] += acc[(size_t)t * NC + c][0]; out[w].cnt[c] += acc[(size_t)t * NC + c][1]; out[w].cor[c] += acc[(size_t)t * NC + c][2]; }
        }
    }
    return out;
}
struct Gr { double mse, acc; };
static Gr grp(const Ev& e, const vector<int>& cls) { double s = 0, c = 0, k = 0; for (int x : cls) { s += e.sse[x]; c += e.cnt[x]; k += e.cor[x]; } return c > 0 ? Gr{s / (c * NC), k / c} : Gr{-1, -1}; }

static vector<vector<int>> TASKS = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}};
static vector<int> cls_of(int lo, int hi) { vector<int> v; for (int t = lo; t < hi; t++) for (int c : TASKS[t]) v.push_back(c); return v; }           // gorevler [lo,hi)
static vector<int> rows_of(const vector<int>& cls) { vector<int> r; for (int c : cls) r.insert(r.end(), byc[c].begin(), byc[c].end()); return r; }

struct MethodState { string name; vector<double> S, R, W; int m = 0; double t_method = 0; double lamu = 0; int fail = 0; };


// ---------------------------------------------------------------- RLS
struct RLS { int m; vector<double> P, W; };
static void rls_init(RLS& r, int m, double lam) { r.m = m; r.P.assign((size_t)m * m, 0.0); for (int i = 0; i < m; i++) r.P[(size_t)i * m + i] = 1.0 / lam; r.W.assign((size_t)m * NC, 0.0); }
// Fb: b x m (float), y: etiketler
static void rls_block(RLS& r, const float* Fb, const int* y, int b) {
    const int m = r.m; vector<double> Fd((size_t)b * m), G((size_t)b * m, 0.0), C((size_t)b * b), E((size_t)b * NC);
    for (size_t i = 0; i < Fd.size(); i++) Fd[i] = Fb[i];
    const bool par = (double)b * m * m > 4e5;
    // G = F P
    #pragma omp parallel for if (par) schedule(static)
    for (int i = 0; i < b; i++) { double* Gi = &G[(size_t)i * m]; for (int k = 0; k < m; k++) { double a = Fd[(size_t)i * m + k]; const double* Pk = &r.P[(size_t)k * m];
        #pragma omp simd
        for (int j = 0; j < m; j++) Gi[j] += a * Pk[j]; } }
    // C = I + G F^T
    for (int i = 0; i < b; i++) for (int i2 = 0; i2 < b; i2++) { double s = (i == i2) ? 1.0 : 0.0; const double *g = &G[(size_t)i * m], *f = &Fd[(size_t)i2 * m]; for (int j = 0; j < m; j++) s += g[j] * f[j]; C[(size_t)i * b + i2] = s; }
    cholesky(C, b);                                                     // alt ucgende L
    // X = C^-1 G  (b x m)
    vector<double> X = G;
    for (int i = 0; i < b; i++) { double* Xi = &X[(size_t)i * m]; for (int k = 0; k < i; k++) { double l = C[(size_t)i * b + k]; const double* Xk = &X[(size_t)k * m]; for (int j = 0; j < m; j++) Xi[j] -= l * Xk[j]; } double d = C[(size_t)i * b + i]; for (int j = 0; j < m; j++) Xi[j] /= d; }
    for (int i = b - 1; i >= 0; i--) { double* Xi = &X[(size_t)i * m]; for (int k = i + 1; k < b; k++) { double l = C[(size_t)k * b + i]; const double* Xk = &X[(size_t)k * m]; for (int j = 0; j < m; j++) Xi[j] -= l * Xk[j]; } double d = C[(size_t)i * b + i]; for (int j = 0; j < m; j++) Xi[j] /= d; }
    // E = T - F W
    for (int i = 0; i < b; i++) { double z[NC] = {0}; for (int j = 0; j < m; j++) { double f = Fd[(size_t)i * m + j]; const double* w = &r.W[(size_t)j * NC]; for (int c = 0; c < NC; c++) z[c] += f * w[c]; } for (int c = 0; c < NC; c++) E[(size_t)i * NC + c] = (c == y[i] ? 1.0 : 0.0) - z[c]; }
    // W += K E = X^T E ; P -= K F P = X^T G
    #pragma omp parallel for if (par) schedule(static)
    for (int j = 0; j < m; j++) { double* Wj = &r.W[(size_t)j * NC]; double* Pj = &r.P[(size_t)j * m];
        for (int i = 0; i < b; i++) { double a = X[(size_t)i * m + j]; for (int c = 0; c < NC; c++) Wj[c] += a * E[(size_t)i * NC + c]; const double* Gi = &G[(size_t)i * m];
            #pragma omp simd
            for (int j2 = 0; j2 < m; j2++) Pj[j2] -= a * Gi[j2]; } }
}

static void report(const string& mode, int bsz, int seed, int H, int t, int m, const vector<Ev>& tst, double t_feat, double t_upd, double t_extra) {
    vector<int> oldc = cls_of(0, t - 1), newc = TASKS[t - 1], seen = cls_of(0, t), all = cls_of(0, 5);
    Gr to = grp(tst[0], oldc), tn = grp(tst[0], newc), ts = grp(tst[0], seen);
    printf("ADIM|%s|%d|%d|%g|%g|%d|%d|%d|%.6f|%.6f|%.6f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f\n", mode.c_str(), bsz, seed, GAM, LAM, H, t, m, to.mse, tn.mse, ts.mse, to.acc, tn.acc, ts.acc, t_feat, t_upd, t_extra);
    for (int i = 0; i < t; i++) { Gr a = grp(tst[0], TASKS[i]); printf("GOREV|%s|%d|%d|%d|%d|%.6f|%.4f\n", mode.c_str(), bsz, seed, t, i + 1, a.mse, a.acc); }
    fflush(stdout);
}

int main(int argc, char** argv) {
    if (argc < 4) { cerr << "kullanim: tek_hamle_v5 MOD H TOHUM [gamma] [lambda] [blok] [gorev_sayisi] [veri_klasoru]\n"; return 1; }
    string mode = argv[1]; int H = atoi(argv[2]), seed = atoi(argv[3]); if (argc > 4) GAM = atof(argv[4]); if (argc > 5) LAM = atof(argv[5]);
    int bsz = argc > 6 ? atoi(argv[6]) : 64; int T = argc > 7 ? atoi(argv[7]) : 3; string dir = argc > 8 ? argv[8] : "/tmp/mnist_bin";
    Xtr = readbin<float>(dir + "/Xtr.f32", (size_t)NTR * D); Xte = readbin<float>(dir + "/Xte.f32", (size_t)NTE * D);
    ytr = readbin<int>(dir + "/ytr.i32", NTR); yte = readbin<int>(dir + "/yte.i32", NTE);
    byc.assign(NC, {}); for (int i = 0; i < NFIT; i++) byc[ytr[i]].push_back(i);
    auto mkseed = [&](int task, int tag) { return (uint64_t)seed * 100003ULL + task * 131 + tag; };
    Feat f; f.blocks.push_back(make_block(TASKS[0], H, mkseed(1, 0))); rebuild(f); const int m = H + 1;
    printf("V5 mod=%s H=%d tohum=%d gamma=%g lambda=%g blok=%d gorev=%d (%d cekirdek)\n", mode.c_str(), H, seed, GAM, LAM, bsz, T, omp_get_max_threads());
    string tag = mode + "_b" + to_string(mode == "rls" ? bsz : 0) + "_H" + to_string(H) + "_s" + to_string(seed);
    double T0 = now();
    vector<double> S((size_t)m * m, 0.0), R((size_t)m * NC, 0.0), W; RLS rls; if (mode == "rls") rls_init(rls, m, LAM);
    mt19937_64 gsh(seed + 99);
    for (int t = 1; t <= T; t++) {
        vector<int> rows = (mode == "bastan") ? rows_of(cls_of(0, t)) : rows_of(TASKS[t - 1]);          // bastan: gorulen TUM veri tekrar islenir
        shuffle(rows.begin(), rows.end(), gsh);
        double t_feat = 0, t_upd = 0, t_extra = 0;
        if (mode == "bastan") { fill(S.begin(), S.end(), 0.0); fill(R.begin(), R.end(), 0.0); }
        vector<float> Xb, F; vector<int> yb;
        for (size_t s0 = 0; s0 < rows.size(); s0 += CH) {
            int n = min((size_t)CH, rows.size() - s0); Xb.resize((size_t)n * D); yb.resize(n);
            for (int i = 0; i < n; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)rows[s0 + i] * D], D * sizeof(float)); yb[i] = ytr[rows[s0 + i]]; }
            double t0 = now(); features(Xb.data(), n, f, F); t_feat += now() - t0;
            t0 = now();
            if (mode == "rls") { for (int s1 = 0; s1 < n; s1 += bsz) rls_block(rls, &F[(size_t)s1 * m], &yb[s1], min(bsz, n - s1)); }
            else if (mode == "ref64") {                                                    // referans: S,R tamamen double toplamla (V4 syrk float parca-toplamina karsi)
                #pragma omp parallel for schedule(dynamic)
                for (int i = 0; i < m; i++) { double* Si = &S[(size_t)i * m]; for (int k = 0; k < n; k++) { double a = F[(size_t)k * m + i]; const float* row = &F[(size_t)k * m]; for (int j = i; j < m; j++) Si[j] += a * row[j]; } }
                for (int k = 0; k < n; k++) for (int i = 0; i < m; i++) R[(size_t)i * NC + yb[k]] += F[(size_t)k * m + i];
            }
            else { syrk_add(F.data(), n, m, S.data()); rhs_add(F.data(), yb.data(), n, m, R.data()); }
            t_upd += now() - t0;
        }
        double t0 = now();
        if (mode != "rls") { solve_sys(S, R, m, W); t_extra = now() - t0; } else { W = rls.W; }
        FILE* fw = fopen(("/tmp/v5w/" + tag + "_t" + to_string(t) + ".bin").c_str(), "wb"); if (fw) { fwrite(W.data(), sizeof(double), W.size(), fw); fclose(fw); }
        vector<const vector<double>*> Ws = {&W}; auto tst = eval_ws(Xte.data(), yte.data(), nullptr, NTE, f, Ws);
        report(mode, mode == "rls" ? bsz : 0, seed, H, t, m, tst, t_feat, t_upd, t_extra);
    }
    double asym = 0, pm = 0; if (mode == "rls") { for (int i = 0; i < m; i++) for (int j = 0; j < m; j++) { asym = max(asym, fabs(rls.P[(size_t)i * m + j] - rls.P[(size_t)j * m + i])); pm = max(pm, fabs(rls.P[(size_t)i * m + j])); } }
    printf("OZET_ISLEM|%s|b=%d|H=%d|toplam_sn=%.2f|tepe_RAM_MB=%.0f|P_asimetri_goreli=%.3e|cholesky_yeniden=%d\n", mode.c_str(), bsz, H, now() - T0, peak_mb(), pm > 0 ? asym / pm : 0.0, retries);
    return 0;
}
