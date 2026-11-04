// Tek hamle V6: ONCEKI CHOLESKY / ONCEKI W'DEN DEVAM (RLS YOK). Gradyan/Adam/epok yok. V4/V5 dosyalarina dokunulmadi (kernel+yardimcilar kopya).
// Modlar:  v4f32   : V4 yolu (S,R float parca-toplam + her gorevde yeniden Cholesky)
//          v4f64   : ayni ama S,R tam double toplam (FP64 referans)
//          v4f32c256: float parca-toplam, 256 satirda bir double'a bosalt
//          v6chol  : onceki Cholesky (U=L^T) uzerine yeni gorev satirlariyla rank-k guncelleme; sayisal sorunda sifirdan Cholesky'ye donus
//          v6dw_ref: W_yeni = W_eski + dW, (S_yeni+lam I) dW = dR - dS W_eski (residual F^T(T-F W_eski)); her gorevde yeniden Cholesky
//          v6dw_pcg: ayni dW denklemi, ama (S_yeni+lam I)'i ONCEKI Cholesky'yi on kosullayici yaparak PCG ile cozer; yakinsamazsa yeniden Cholesky
// Kullanim: ./tek_hamle_v6 MOD H TOHUM [gamma=0.125] [lambda=1] [gorev=5] [zorla_fallback_gorevi=0] [maxit=60] [veri_klasoru]
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle_v6 tek_hamle_v6.cpp
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



// ---------------------------------------------------------------- V6 yardimcilari
static void syrk64(const float* F, int n, int m, double* S) {                 // tam double toplam (referans)
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < m; i++) { double* Si = S + (size_t)i * m; for (int k = 0; k < n; k++) { double a = F[(size_t)k * m + i]; const float* row = F + (size_t)k * m; for (int j = i; j < m; j++) Si[j] += a * row[j]; } }
}
static void syrk_flush(const float* F, int n, int m, double* S, int fl) { for (int s = 0; s < n; s += fl) syrk_add(F + (size_t)s * m, min(fl, n - s), m, S); }
static void lower_to_upper(const vector<double>& L, int m, vector<double>& U) {
    U.assign((size_t)m * m, 0.0);
    #pragma omp parallel for
    for (int i = 0; i < m; i++) for (int j = 0; j <= i; j++) U[(size_t)j * m + i] = L[(size_t)i * m + j];
}
// U^T U W = R  (U ust ucgen satir ana), R: m x NC
static void solveU(const vector<double>& U, int m, const vector<double>& R, vector<double>& W) {
    vector<double> r = R; W.assign((size_t)m * NC, 0.0);
    for (int k = 0; k < m; k++) { double d = U[(size_t)k * m + k]; double y[NC]; for (int c = 0; c < NC; c++) { y[c] = r[(size_t)k * NC + c] / d; r[(size_t)k * NC + c] = y[c]; }
        const double* Uk = &U[(size_t)k * m]; for (int i = k + 1; i < m; i++) { double u = Uk[i]; double* ri = &r[(size_t)i * NC]; for (int c = 0; c < NC; c++) ri[c] -= u * y[c]; } }
    for (int i = m - 1; i >= 0; i--) { double acc[NC]; for (int c = 0; c < NC; c++) acc[c] = r[(size_t)i * NC + c]; const double* Ui = &U[(size_t)i * m];
        for (int k = i + 1; k < m; k++) { double u = Ui[k]; const double* Wk = &W[(size_t)k * NC]; for (int c = 0; c < NC; c++) acc[c] -= u * Wk[c]; }
        double d = Ui[i]; for (int c = 0; c < NC; c++) W[(size_t)i * NC + c] = acc[c] / d; }
}
// A = U^T U uzerine g adet x x^T eklemek (dchud, vektor gruplari kolon kolon). X: g x m, yerinde bozulur. basarisizsa false.
static bool chol_rank_update(vector<double>& U, int m, vector<double>& X, int g) {
    vector<double> c(g), s(g);
    for (int j = 0; j < m; j++) {
        double ujj = U[(size_t)j * m + j];
        for (int v = 0; v < g; v++) { double xj = X[(size_t)v * m + j]; double r = sqrt(ujj * ujj + xj * xj); if (!(r > 0) || !std::isfinite(r)) return false; c[v] = ujj / r; s[v] = xj / r; ujj = r; }
        U[(size_t)j * m + j] = ujj; int len = m - j - 1; if (len <= 0) continue;
        double* ur = &U[(size_t)j * m];
        #pragma omp parallel if ((long)len * g > 20000)
        { int nt = omp_get_num_threads(), tid = omp_get_thread_num(); int lo = j + 1 + (int)((long)len * tid / nt), hi = j + 1 + (int)((long)len * (tid + 1) / nt);
          for (int v = 0; v < g; v++) { double cv = c[v], sv = s[v]; double* xr = &X[(size_t)v * m];
              #pragma omp simd
              for (int i = lo; i < hi; i++) { double u = ur[i], x = xr[i]; ur[i] = cv * u + sv * x; xr[i] = cv * x - sv * u; } } }
    }
    return true;
}
// A = U^T U + D (D: ust ucgen, ayni indeks) -> yeni U ; lambda U icinde zaten var
static bool refactor_from_U_plus(const vector<double>& U, int m, const vector<double>& Dm, vector<double>& Unew) {
    vector<double> A((size_t)m * m, 0.0);
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < m; i++) for (int k = 0; k <= i; k++) { double u = U[(size_t)k * m + i]; const double* Uk = &U[(size_t)k * m]; double* Ai = &A[(size_t)i * m]; for (int j = i; j < m; j++) Ai[j] += u * Uk[j]; }
    for (int i = 0; i < m; i++) for (int j = i; j < m; j++) A[(size_t)i * m + j] += Dm[(size_t)i * m + j];
    if (!cholesky(A, m)) return false; lower_to_upper(A, m, Unew); return true;
}
// rhs += F^T (T - F W)   (F: n x m float, W: m x NC)
static void resid_rhs(const float* F, const int* y, int n, int m, const vector<double>& W, vector<double>& out) {
    int nt = omp_get_max_threads(); vector<vector<double>> loc(nt, vector<double>((size_t)m * NC, 0.0));
    #pragma omp parallel
    { auto& L = loc[omp_get_thread_num()];
      #pragma omp for schedule(static)
      for (int k = 0; k < n; k++) { const float* fr = F + (size_t)k * m; double z[NC] = {0};
          for (int j = 0; j < m; j++) { double f = fr[j]; const double* w = &W[(size_t)j * NC]; for (int c = 0; c < NC; c++) z[c] += f * w[c]; }
          double e[NC]; for (int c = 0; c < NC; c++) e[c] = (c == y[k] ? 1.0 : 0.0) - z[c];
          for (int j = 0; j < m; j++) { double f = fr[j]; double* o = &L[(size_t)j * NC]; for (int c = 0; c < NC; c++) o[c] += f * e[c]; } } }
    for (auto& L : loc) for (size_t i = 0; i < L.size(); i++) out[i] += L[i];
}
// A = S(ust)+lam I -> tam simetrik
static void sym_full(const vector<double>& S, int m, double lam, vector<double>& A) {
    A.assign((size_t)m * m, 0.0);
    #pragma omp parallel for
    for (int i = 0; i < m; i++) for (int j = i; j < m; j++) { double v = S[(size_t)i * m + j]; A[(size_t)i * m + j] = v; A[(size_t)j * m + i] = v; }
    for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam;
}
static void matmulA(const vector<double>& A, int m, const vector<double>& V, vector<double>& O) {
    O.assign((size_t)m * NC, 0.0);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < m; i++) { double z[NC] = {0}; const double* Ai = &A[(size_t)i * m]; for (int j = 0; j < m; j++) { double a = Ai[j]; const double* Vj = &V[(size_t)j * NC]; for (int c = 0; c < NC; c++) z[c] += a * Vj[c]; } for (int c = 0; c < NC; c++) O[(size_t)i * NC + c] = z[c]; }
}
// PCG (on kosullayici: onceki Cholesky U), 10 sag taraf esgudumlu; donus: iterasyon sayisi, <0 basarisiz
static int pcg_solve(const vector<double>& A, const vector<double>& Upre, int m, const vector<double>& b, vector<double>& x, int maxit, double tol, double& relres) {
    x.assign((size_t)m * NC, 0.0); vector<double> r = b, z, p, Ap; solveU(Upre, m, r, z); p = z;
    double bn[NC], rz[NC]; for (int c = 0; c < NC; c++) { bn[c] = 0; rz[c] = 0; } for (int i = 0; i < m; i++) for (int c = 0; c < NC; c++) { bn[c] += b[(size_t)i * NC + c] * b[(size_t)i * NC + c]; rz[c] += r[(size_t)i * NC + c] * z[(size_t)i * NC + c]; }
    for (int c = 0; c < NC; c++) bn[c] = sqrt(bn[c]) + 1e-300;
    for (int it = 1; it <= maxit; it++) {
        matmulA(A, m, p, Ap); double pAp[NC] = {0}; for (int i = 0; i < m; i++) for (int c = 0; c < NC; c++) pAp[c] += p[(size_t)i * NC + c] * Ap[(size_t)i * NC + c];
        double al[NC]; for (int c = 0; c < NC; c++) { if (!(pAp[c] > 0) && rz[c] > 1e-300) return -it; al[c] = (rz[c] > 1e-300) ? rz[c] / pAp[c] : 0.0; }
        for (int i = 0; i < m; i++) for (int c = 0; c < NC; c++) { x[(size_t)i * NC + c] += al[c] * p[(size_t)i * NC + c]; r[(size_t)i * NC + c] -= al[c] * Ap[(size_t)i * NC + c]; }
        double rn[NC] = {0}; for (int i = 0; i < m; i++) for (int c = 0; c < NC; c++) rn[c] += r[(size_t)i * NC + c] * r[(size_t)i * NC + c];
        relres = 0; for (int c = 0; c < NC; c++) relres = max(relres, sqrt(rn[c]) / bn[c]);
        if (relres < tol) return it;
        solveU(Upre, m, r, z); double rzn[NC] = {0}; for (int i = 0; i < m; i++) for (int c = 0; c < NC; c++) rzn[c] += r[(size_t)i * NC + c] * z[(size_t)i * NC + c];
        for (int c = 0; c < NC; c++) { double be = (rz[c] > 1e-300) ? rzn[c] / rz[c] : 0.0; rz[c] = rzn[c]; for (int i = 0; i < m; i++) p[(size_t)i * NC + c] = z[(size_t)i * NC + c] + be * p[(size_t)i * NC + c]; }
    }
    return -maxit;
}

static void report(const string& mode, int seed, int H, int t, int m, const vector<Ev>& tst, double t_feat, double t_stat, double t_fac, double t_sol, const string& extra) {
    vector<int> oldc = cls_of(0, t - 1), newc = TASKS[t - 1], seen = cls_of(0, t);
    Gr to = grp(tst[0], oldc), tn = grp(tst[0], newc), ts = grp(tst[0], seen);
    printf("ADIM|%s|%d|%g|%g|%d|%d|%d|%.6f|%.6f|%.6f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%.4f|%s\n", mode.c_str(), seed, GAM, LAM, H, t, m, to.mse, tn.mse, ts.mse, to.acc, tn.acc, ts.acc, t_feat, t_stat, t_fac, t_sol, t_stat + t_fac + t_sol, extra.c_str());
    for (int i = 0; i < t; i++) { Gr a = grp(tst[0], TASKS[i]); printf("GOREV|%s|%d|%d|%d|%d|%.6f|%.4f\n", mode.c_str(), seed, H, t, i + 1, a.mse, a.acc); }
    fflush(stdout);
}

int main(int argc, char** argv) {
    if (argc < 4) { cerr << "kullanim: tek_hamle_v6 MOD H TOHUM [gamma] [lambda] [gorev] [zorla_fb] [maxit] [veri_klasoru]\n"; return 1; }
    string mode = argv[1]; int H = atoi(argv[2]), seed = atoi(argv[3]); if (argc > 4) GAM = atof(argv[4]); if (argc > 5) LAM = atof(argv[5]);
    int T = argc > 6 ? atoi(argv[6]) : 5; int forcefb = argc > 7 ? atoi(argv[7]) : 0; int maxit = argc > 8 ? atoi(argv[8]) : 60; string dir = argc > 9 ? argv[9] : "/tmp/mnist_bin";
    Xtr = readbin<float>(dir + "/Xtr.f32", (size_t)NTR * D); Xte = readbin<float>(dir + "/Xte.f32", (size_t)NTE * D);
    ytr = readbin<int>(dir + "/ytr.i32", NTR); yte = readbin<int>(dir + "/yte.i32", NTE);
    byc.assign(NC, {}); for (int i = 0; i < NFIT; i++) byc[ytr[i]].push_back(i);
    auto mkseed = [&](int task, int tag) { return (uint64_t)seed * 100003ULL + task * 131 + tag; };
    Feat f; f.blocks.push_back(make_block(TASKS[0], H, mkseed(1, 0))); rebuild(f); const int m = H + 1;
    printf("V6 mod=%s H=%d tohum=%d gamma=%g lambda=%g gorev=%d zorla_fb=%d maxit=%d (%d cekirdek)\n", mode.c_str(), H, seed, GAM, LAM, T, forcefb, maxit, omp_get_max_threads());

    if (mode == "mikro") {
        // MIKRO KIYAS: k yeni satir geldiginde  (a) rank-k Cholesky guncelleme + cozum   vs   (b) V4 yolu: dS=F^T F (syrk) + S'e ekle + yeniden Cholesky + cozum.
        // Eski durum: gorev-1 verisinden S0/U0. Yeni satirlar: gorev-2'den k adet. Her k icin 5 tekrar, medyan sure.
        mt19937_64 gsh(seed + 99); vector<int> r1 = rows_of(TASKS[0]); vector<float> Xb, F; vector<int> yb; vector<double> S0((size_t)m * m, 0.0), R0((size_t)m * NC, 0.0);
        { int n = r1.size(); Xb.resize((size_t)n * D); yb.resize(n); for (int i = 0; i < n; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)r1[i] * D], D * sizeof(float)); yb[i] = ytr[r1[i]]; }
          for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); features(&Xb[(size_t)s0 * D], nn, f, F); syrk_add(F.data(), nn, m, S0.data()); rhs_add(F.data(), &yb[s0], nn, m, R0.data()); } }
        vector<double> A0 = S0; for (int i = 0; i < m; i++) A0[(size_t)i * m + i] += LAM; cholesky(A0, m); vector<double> U0; lower_to_upper(A0, m, U0);
        vector<int> r2 = rows_of(TASKS[1]); shuffle(r2.begin(), r2.end(), gsh);
        int ks[] = {4, 16, 64, 256, 1024, 4096}; auto med5 = [](vector<double> v) { sort(v.begin(), v.end()); return v[v.size() / 2]; };
        printf("MIKRO|H=%d|k|rank_k_guncelleme_sn|V4_dS+Cholesky_sn|oran(rank_k/V4)|W_goreli_fark(rank_k vs V4)\n", H);
        for (int k : ks) {
            Xb.resize((size_t)k * D); yb.resize(k); for (int i = 0; i < k; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)r2[i] * D], D * sizeof(float)); yb[i] = ytr[r2[i]]; }
            features(Xb.data(), k, f, F); vector<double> ta, tb, Wa, Wb;
            for (int rep = 0; rep < 5; rep++) {
                double t0 = now(); vector<double> U = U0, R = R0; rhs_add(F.data(), yb.data(), k, m, R.data());
                for (int g0 = 0; g0 < k; g0 += 256) { int g = min(256, k - g0); vector<double> Xg((size_t)g * m); for (int v = 0; v < g; v++) for (int j = 0; j < m; j++) Xg[(size_t)v * m + j] = F[(size_t)(g0 + v) * m + j]; chol_rank_update(U, m, Xg, g); }
                solveU(U, m, R, Wa); ta.push_back(now() - t0);
                t0 = now(); vector<double> S = S0, R2 = R0; syrk_add(F.data(), k, m, S.data()); rhs_add(F.data(), yb.data(), k, m, R2.data()); for (int i = 0; i < m; i++) S[(size_t)i * m + i] += LAM; cholesky(S, m); chol_solve(S, m, R2, Wb); tb.push_back(now() - t0);
            }
            double num = 0, den = 0; for (size_t i = 0; i < Wa.size(); i++) { num += (Wa[i] - Wb[i]) * (Wa[i] - Wb[i]); den += Wb[i] * Wb[i]; }
            printf("MIKRO|%d|%d|%.5f|%.5f|%.2f|%.2e\n", H, k, med5(ta), med5(tb), med5(ta) / med5(tb), sqrt(num / den));
        }
        return 0;
    }
    string tag = mode + "_H" + to_string(H) + "_s" + to_string(seed);
    double T0 = now(); mt19937_64 gsh(seed + 99);
    const bool is_v4 = (mode == "v4f32" || mode == "v4f64" || mode == "v4f32c256"), is_chol = (mode == "v6chol"), is_dw = (mode == "v6dw_ref" || mode == "v6dw_pcg");
    if (!(is_v4 || is_chol || is_dw)) { cerr << "bilinmeyen mod\n"; return 1; }
    vector<double> S((size_t)m * m, 0.0), R((size_t)m * NC, 0.0), W((size_t)m * NC, 0.0), U, Upre, A;
    int total_fb = 0;
    for (int t = 1; t <= T; t++) {
        vector<int> rows = rows_of(TASKS[t - 1]); shuffle(rows.begin(), rows.end(), gsh);
        double t_feat = 0, t_stat = 0, t_fac = 0, t_sol = 0; string extra = "-"; int fb = 0;
        vector<double> dR((size_t)m * NC, 0.0), dS;                       // dS yalniz fallback icin
        vector<double> Uold; if (is_chol && t > 1) Uold = U;              // fallback icin yedek (tek m^2 kopya)
        bool chol_fail = false; vector<float> Xb, F; vector<int> yb; vector<double> Xg;
        for (size_t s0 = 0; s0 < rows.size(); s0 += CH) {
            int n = min((size_t)CH, rows.size() - s0); Xb.resize((size_t)n * D); yb.resize(n);
            for (int i = 0; i < n; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)rows[s0 + i] * D], D * sizeof(float)); yb[i] = ytr[rows[s0 + i]]; }
            double t0 = now(); features(Xb.data(), n, f, F); t_feat += now() - t0; t0 = now();
            if (is_v4) { if (mode == "v4f64") syrk64(F.data(), n, m, S.data()); else if (mode == "v4f32c256") syrk_flush(F.data(), n, m, S.data(), 256); else syrk_add(F.data(), n, m, S.data()); rhs_add(F.data(), yb.data(), n, m, R.data()); }
            else if (is_chol) {
                rhs_add(F.data(), yb.data(), n, m, R.data());
                if (t == 1) syrk_add(F.data(), n, m, S.data());                               // ilk gorev: S kurulur, sonra Cholesky
                else if (!chol_fail) { const int G = 256; Xg.resize((size_t)G * m);
                    for (int g0 = 0; g0 < n && !chol_fail; g0 += G) { int g = min(G, n - g0); for (int v = 0; v < g; v++) for (int j = 0; j < m; j++) Xg[(size_t)v * m + j] = F[(size_t)(g0 + v) * m + j];
                        if (!chol_rank_update(U, m, Xg, g)) chol_fail = true; } }
            } else {                                                                            // dW modlari: S float birikir, residual rhs
                syrk_add(F.data(), n, m, S.data()); resid_rhs(F.data(), yb.data(), n, m, W, dR);
            }
            t_stat += now() - t0;
        }
        double t0 = now();
        if (is_v4) { bool ok; vector<double> Aa = S; for (int i = 0; i < m; i++) Aa[(size_t)i * m + i] += LAM; ok = cholesky(Aa, m); t_fac = now() - t0; t0 = now(); if (ok) chol_solve(Aa, m, R, W); else { fprintf(stderr, "Cholesky basarisiz\n"); fb = 1; } t_sol = now() - t0; }
        else if (is_chol) {
            if (t == 1) { vector<double> Aa = S; for (int i = 0; i < m; i++) Aa[(size_t)i * m + i] += LAM; bool ok = cholesky(Aa, m); if (!ok) { fprintf(stderr, "t1 Cholesky basarisiz\n"); return 2; } lower_to_upper(Aa, m, U); t_fac = now() - t0; }
            else {
                if (forcefb == t) chol_fail = true;
                if (chol_fail) {                                                                // GUVENLI DONUS: yedek U + yeni gorev dS ile sifirdan Cholesky
                    fb = 1; total_fb++; double tf = now(); dS.assign((size_t)m * m, 0.0); vector<double> dRtmp((size_t)m * NC, 0.0);
                    for (size_t s0 = 0; s0 < rows.size(); s0 += CH) { int n = min((size_t)CH, rows.size() - s0); Xb.resize((size_t)n * D); yb.resize(n);
                        for (int i = 0; i < n; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)rows[s0 + i] * D], D * sizeof(float)); yb[i] = ytr[rows[s0 + i]]; }
                        features(Xb.data(), n, f, F); syrk_add(F.data(), n, m, dS.data()); }
                    vector<double> Un; if (refactor_from_U_plus(Uold, m, dS, Un)) U = move(Un); else { fprintf(stderr, "fallback Cholesky basarisiz\n"); return 3; }
                    t_fac = now() - tf; extra = "FALLBACK";
                } else t_fac = 0;                                                               // rank-k guncelleme zaten t_stat icinde
            }
            t0 = now(); solveU(U, m, R, W); t_sol = now() - t0;
        } else {                                                                                // v6dw_*
            if (mode == "v6dw_ref" || t == 1) {
                vector<double> Aa = S; for (int i = 0; i < m; i++) Aa[(size_t)i * m + i] += LAM; bool ok = cholesky(Aa, m); t_fac = now() - t0; t0 = now();
                vector<double> dW; if (ok) chol_solve(Aa, m, dR, dW); else { fprintf(stderr, "Cholesky basarisiz\n"); fb = 1; dW.assign((size_t)m * NC, 0.0); }
                for (size_t i = 0; i < W.size(); i++) W[i] += dW[i]; t_sol = now() - t0;
                if (mode == "v6dw_pcg") { lower_to_upper(Aa, m, Upre); t_fac = now() - t0 + 0; }
            } else {
                sym_full(S, m, LAM, A); vector<double> dW; double rr = 0; int it = pcg_solve(A, Upre, m, dR, dW, maxit, 1e-9, rr); t_sol = now() - t0;
                char buf[80]; if (it > 0) { for (size_t i = 0; i < W.size(); i++) W[i] += dW[i]; snprintf(buf, sizeof buf, "pcg_it=%d relres=%.1e", it, rr); extra = buf; }
                else {                                                                          // GUVENLI DONUS: yeniden Cholesky
                    fb = 1; total_fb++; double tf = now(); vector<double> Aa = S; for (int i = 0; i < m; i++) Aa[(size_t)i * m + i] += LAM; bool ok = cholesky(Aa, m);
                    vector<double> dW2; if (ok) chol_solve(Aa, m, dR, dW2); for (size_t i = 0; i < W.size(); i++) W[i] += dW2[i]; lower_to_upper(Aa, m, Upre); t_fac = now() - tf;
                    snprintf(buf, sizeof buf, "FALLBACK pcg_it=%d relres=%.1e", -it, rr); extra = buf; }
            }
        }
        FILE* fw = fopen(("/tmp/v6w/" + tag + "_t" + to_string(t) + ".bin").c_str(), "wb"); if (fw) { fwrite(W.data(), sizeof(double), W.size(), fw); fclose(fw); }
        vector<const vector<double>*> Ws = {&W}; auto tst = eval_ws(Xte.data(), yte.data(), nullptr, NTE, f, Ws);
        report(mode, seed, H, t, m, tst, t_feat, t_stat, t_fac, t_sol, extra);
    }
    printf("OZET_ISLEM|%s|H=%d|toplam_sn=%.2f|tepe_RAM_MB=%.0f|fallback_sayisi=%d\n", mode.c_str(), H, now() - T0, peak_mb(), total_fb);
    return 0;
}
