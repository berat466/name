// Tek hamle V3: (1) W1/b1 kontrol deneyleri (yon vs bias), (2) matematiksel hafiza (S,R istatistikleri, ardisik gorevler).
// GRADYAN YOK, ADAM YOK, EPOK YOK. Tek matematiksel cozum: W = (S + lambda I)^-1 R, Cholesky ile. V2 dosyalarina dokunulmadi.
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle_v3 tek_hamle_v3.cpp
// Kullanim: ./tek_hamle_v3 yon  MOD H tohum        MOD: cift|ayni|ryon_cbias|cyon_rbias|rr|rastgele
//           ./tek_hamle_v3 hafiza OZELLIK H tohum  OZELLIK: rastgele|cift|cift_A  (cift_A: ciftler yalniz 0-4 siniflarindan)
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

// ---------------------------------------------------------------- veri + W1/b1 kurulumu
static vector<float> Xtr, Xte; static vector<int> ytr, yte;

struct W1b { vector<float> W, b; double sig_b = 0, mean_b = 0; };

// w = (xa-xb)/(|xa-xb|+eps), b = -(xa+xb)^T w / 2
static void pair_wb(const float* xa, const float* xb, vector<double>& w, double& b) {
    double nn = 0; for (int k = 0; k < D; k++) { double d = xa[k] - xb[k]; nn += d * d; }
    double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0;
    for (int k = 0; k < D; k++) { w[k] = (xa[k] - xb[k]) * inv; dot += (xa[k] + xb[k]) * w[k]; }
    b = -dot / 2;
}
static double pair_bias_given(const float* xa, const float* xb, const vector<double>& w) {
    double dot = 0; for (int k = 0; k < D; k++) dot += (xa[k] + xb[k]) * w[k]; return -dot / 2;
}

// mode: cift | ayni | ryon_cbias | cyon_rbias | rr | rastgele | cift_A ;  allowed: pair classes (yalniz fit kumesi)
static W1b build_w1(const string& mode, int H, int seed, const vector<int>& allowed) {
    W1b o; o.W.assign((size_t)D * H, 0.f); o.b.assign(H, 0.f);
    if (mode == "rastgele") {                                            // V2 referansi: W~N(0,1/d), b~N(0,0.5^2)
        mt19937_64 g(seed); normal_distribution<double> N(0, 1);
        for (auto& w : o.W) w = (float)(N(g) / sqrt((double)D)); for (auto& b : o.b) b = (float)(N(g) * 0.5); return o;
    }
    vector<vector<int>> byc(NC); for (int i = 0; i < NFIT; i++) byc[ytr[i]].push_back(i);
    int na = allowed.size(); mt19937_64 gp(seed), gd(seed + 1000), gb(seed + 2000);
    normal_distribution<double> N(0, 1); auto pick = [&](int c) { return byc[c][uniform_int_distribution<int>(0, (int)byc[c].size() - 1)(gp)]; };
    vector<int> PA(H), PB(H);
    bool same = (mode == "ayni");
    for (int j = 0; j < H; j++) {
        int ca = allowed[uniform_int_distribution<int>(0, na - 1)(gp)], cb;
        if (same) { cb = ca; } else { do cb = allowed[uniform_int_distribution<int>(0, na - 1)(gp)]; while (cb == ca); }
        int a = pick(ca), b = pick(cb); if (same) { while (b == a) b = pick(cb); }
        PA[j] = a; PB[j] = b;
    }
    // V2 yon+bias (cift ile ayni cekilis): sigma_b icin
    vector<double> w(D), pb(H);
    for (int j = 0; j < H; j++) pair_wb(&Xtr[(size_t)PA[j] * D], &Xtr[(size_t)PB[j] * D], w, pb[j]);
    double mu = 0, sd = 0; for (double v : pb) mu += v; mu /= H; for (double v : pb) sd += (v - mu) * (v - mu); sd = sqrt(sd / H);
    o.sig_b = sd; o.mean_b = mu;
    for (int j = 0; j < H; j++) {
        const float *xa = &Xtr[(size_t)PA[j] * D], *xb = &Xtr[(size_t)PB[j] * D]; double b;
        if (mode == "cift" || mode == "cift_A" || mode == "ayni") { pair_wb(xa, xb, w, b); }
        else if (mode == "ryon_cbias") { double nn = 0; for (int k = 0; k < D; k++) { w[k] = N(gd); nn += w[k] * w[k]; } nn = sqrt(nn); for (int k = 0; k < D; k++) w[k] /= nn; b = pair_bias_given(xa, xb, w); }
        else if (mode == "cyon_rbias") { pair_wb(xa, xb, w, b); b = N(gb) * sd; }
        else if (mode == "rr") { double nn = 0; for (int k = 0; k < D; k++) { w[k] = N(gd); nn += w[k] * w[k]; } nn = sqrt(nn); for (int k = 0; k < D; k++) w[k] /= nn; b = N(gb) * sd; }
        else { cerr << "bilinmeyen mod " << mode << "\n"; exit(1); }
        for (int k = 0; k < D; k++) o.W[(size_t)k * H + j] = (float)w[k]; o.b[j] = (float)b;
    }
    return o;
}

static void features(const float* X, int n, double gam, const W1b& P, int H, vector<float>& F) {
    int m = H + 1; F.resize((size_t)n * m); gemm_nn(X, n, P.W.data(), H, F.data(), m);
    #pragma omp parallel for
    for (int i = 0; i < n; i++) { float* r = &F[(size_t)i * m]; for (int j = 0; j < H; j++) r[j] = tanhf((float)gam * (r[j] + P.b[j])); r[H] = 1.f; }
}
// satir listesi idx icin istatistik: S (ust ucgen) += F^T F, R += F^T T
static void stats_rows(const vector<int>& idx, double gam, const W1b& P, int H, vector<double>& S, vector<double>& R) {
    int m = H + 1; vector<float> Xb, F; vector<int> yb;
    for (size_t s = 0; s < idx.size(); s += CH) {
        int n = min((size_t)CH, idx.size() - s); Xb.resize((size_t)n * D); yb.resize(n);
        for (int i = 0; i < n; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)idx[s + i] * D], D * sizeof(float)); yb[i] = ytr[idx[s + i]]; }
        features(Xb.data(), n, gam, P, H, F); syrk_add(F.data(), n, m, S.data()); rhs_add(F.data(), yb.data(), n, m, R.data());
    }
}
static void solve_sys(const vector<double>& S, const vector<double>& R, double lam, int m, vector<double>& W, bool& ok) {
    vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam;
    ok = cholesky(A, m); if (ok) chol_solve(A, m, R, W);
}
// F (n x m) ile W (m x NC): sinif basina dogru/toplam
static void eval_counts(const vector<float>& F, const int* y, int n, int m, const vector<double>& W, array<long, NC>& cor, array<long, NC>& tot) {
    cor.fill(0); tot.fill(0); vector<array<long, NC>> lc(omp_get_max_threads()), lt(omp_get_max_threads());
    for (auto& a : lc) a.fill(0); for (auto& a : lt) a.fill(0);
    #pragma omp parallel
    { int t = omp_get_thread_num();
      #pragma omp for
      for (int i = 0; i < n; i++) { double z[NC] = {0}; const float* f = &F[(size_t)i * m]; for (int j = 0; j < m; j++) for (int c = 0; c < NC; c++) z[c] += f[j] * W[(size_t)j * NC + c];
          int bc = 0; for (int c = 1; c < NC; c++) if (z[c] > z[bc]) bc = c; lt[t][y[i]]++; if (bc == y[i]) lc[t][y[i]]++; } }
    for (size_t t = 0; t < lc.size(); t++) for (int c = 0; c < NC; c++) { cor[c] += lc[t][c]; tot[c] += lt[t][c]; }
}
static double total_acc(const array<long, NC>& c, const array<long, NC>& t) { long a = 0, b = 0; for (int i = 0; i < NC; i++) { a += c[i]; b += t[i]; } return (double)a / b; }
static double group_acc(const array<long, NC>& c, const array<long, NC>& t, const vector<int>& cls) { long a = 0, b = 0; for (int k : cls) { a += c[k]; b += t[k]; } return b ? (double)a / b : 0; }

static const double GAMMAS[6] = {0.0625, 0.125, 0.25, 0.5, 1, 2};
static const double LAMS[4] = {1, 10, 100, 1000};

// ---------------------------------------------------------------- DENEY 1+2: W1/b1 kontrolleri
static void run_yon(const string& mode, int H, int seed) {
    const int m = H + 1; double T0 = now();
    vector<int> all(NC); iota(all.begin(), all.end(), 0);
    double tw0 = now(); W1b P = build_w1(mode, H, seed, all); double t_w = now() - tw0;
    printf("YON mod=%s H=%d tohum=%d  sigma_b(cift ciftlerinden)=%.3f ort_b=%.3f\n", mode.c_str(), H, seed, P.sig_b, P.mean_b);
    vector<int> fit(NFIT), val(NTR - NFIT); iota(fit.begin(), fit.end(), 0); iota(val.begin(), val.end(), NFIT);
    vector<float> Fv; const int* yv = &ytr[NFIT];
    struct Best { double val = -1, gam = 0, lam = 0; int gi = 0; } best;
    vector<vector<double>> Sall(6), Rall(6); vector<double> t_stats(6);
    for (int gi = 0; gi < 6; gi++) {
        double gam = GAMMAS[gi], t1 = now();
        vector<double> Sf((size_t)m * m, 0.0), Rf((size_t)m * NC, 0.0), Sv((size_t)m * m, 0.0), Rv((size_t)m * NC, 0.0);
        stats_rows(fit, gam, P, H, Sf, Rf); stats_rows(val, gam, P, H, Sv, Rv); t_stats[gi] = now() - t1;
        features(&Xtr[(size_t)NFIT * D], NTR - NFIT, gam, P, H, Fv);
        for (int li = 0; li < 4; li++) {
            vector<double> W; bool ok; solve_sys(Sf, Rf, LAMS[li], m, W, ok); double va = -1;
            if (ok) { array<long, NC> c, t; eval_counts(Fv, yv, NTR - NFIT, m, W, c, t); va = total_acc(c, t); }
            printf("  gamma=%.4f lambda=%4.0f val=%.4f\n", gam, LAMS[li], va); fflush(stdout);
            if (va > best.val) best = {va, gam, LAMS[li], gi};
        }
        for (size_t i = 0; i < Sf.size(); i++) Sf[i] += Sv[i]; for (size_t i = 0; i < Rf.size(); i++) Rf[i] += Rv[i];
        Sall[gi] = move(Sf); Rall[gi] = move(Rf);
    }
    double t_search = now() - T0;
    double t3 = now(); vector<double> W; bool ok; solve_sys(Sall[best.gi], Rall[best.gi], best.lam, m, W, ok); double t_fs = now() - t3;
    vector<float> Ft; features(Xte.data(), NTE, best.gam, P, H, Ft); array<long, NC> c, t; eval_counts(Ft, yte.data(), NTE, m, W, c, t);
    double test = total_acc(c, t), T1 = now() - T0, t_single = t_w + t_stats[best.gi] + t_fs;
    bool edge = (best.gi == 0 || best.gi == 5);
    printf("SECILEN gamma=%.4f lambda=%.0f val=%.4f%s  TEST=%.4f  tek_model=%.1fs  arama_dahil_toplam=%.1fs  RAM=%.0fMB\n", best.gam, best.lam, best.val, edge ? " [IZGARA KENARI]" : "", test, t_single, T1, peak_mb());
    printf("OZET|%s|%d|%d|%.4f|%.0f|%.4f|%.4f|%.1f|%.1f|%.0f|%d\n", mode.c_str(), H, seed, best.gam, best.lam, best.val, test, t_single, T1, peak_mb(), (int)edge);
}

// ---------------------------------------------------------------- DENEY 3: V3 matematiksel hafiza
struct Group { string name; vector<vector<int>> tasks; };
struct GS { vector<vector<double>> S, R; vector<double> tc; vector<float> Fv, Ft; };

static GS class_stats(double gam, const W1b& P, int H, const vector<vector<int>>& byc_fit) {
    const int m = H + 1; GS g; g.S.assign(NC, vector<double>((size_t)m * m, 0.0)); g.R.assign(NC, vector<double>((size_t)m * NC, 0.0)); g.tc.assign(NC, 0);
    for (int c = 0; c < NC; c++) { double t0 = now(); stats_rows(byc_fit[c], gam, P, H, g.S[c], g.R[c]); g.tc[c] = now() - t0; }
    features(&Xtr[(size_t)NFIT * D], NTR - NFIT, gam, P, H, g.Fv); features(Xte.data(), NTE, gam, P, H, g.Ft);
    return g;
}
static void add_scaled(vector<double>& dst, const vector<double>& src, double a) { for (size_t i = 0; i < dst.size(); i++) dst[i] += a * src[i]; }

static void run_hafiza(const string& feat, int H, int seed) {
    const int m = H + 1; double T0 = now();
    vector<int> allowed; for (int c = 0; c < (feat == "cift_A" ? 5 : NC); c++) allowed.push_back(c);
    string wmode = (feat == "cift_A") ? "cift" : feat;
    W1b P = build_w1(wmode, H, seed, allowed);
    vector<vector<int>> byc(NC); for (int i = 0; i < NFIT; i++) byc[ytr[i]].push_back(i);
    vector<Group> groups;
    groups.push_back({"g5", {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {8, 9}}});
    groups.push_back({"g2", {{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}}});
    if (feat == "cift_A") groups.erase(groups.begin());                      // 5 gorevli bolme bu ozellikle anlamsiz: cift_A yalniz g2
    const double MUS[5] = {0, 0.5, 1, 2, 5};
    struct Sel { double val = -1, gam = 0, lam = 0; int gi = -1; };
    vector<array<Sel, 5>> sel(groups.size());
    const int* yv = &ytr[NFIT]; int nval = NTR - NFIT;
    double tcls_total = 0;
    // ---- FAZ 1: yalniz DOGRULAMA ile (gamma, lambda) secimi, her (grup, mu) icin ayri
    for (int gi = 0; gi < 6; gi++) {
        GS gs = class_stats(GAMMAS[gi], P, H, byc); for (double v : gs.tc) tcls_total += v;
        for (size_t g = 0; g < groups.size(); g++) {
            int T = groups[g].tasks.size(); vector<vector<double>> St(T, vector<double>((size_t)m * m, 0.0)), Rt(T, vector<double>((size_t)m * NC, 0.0));
            for (int t = 0; t < T; t++) for (int c : groups[g].tasks[t]) { add_scaled(St[t], gs.S[c], 1); add_scaled(Rt[t], gs.R[c], 1); }
            for (int mi = 0; mi < 5; mi++) {
                vector<double> Sa((size_t)m * m, 0.0), Ra((size_t)m * NC, 0.0);
                for (int t = 0; t < T; t++) { double w = pow(MUS[mi], T - 1 - t); if (MUS[mi] == 0) w = (t == T - 1) ? 1 : 0; add_scaled(Sa, St[t], w); add_scaled(Ra, Rt[t], w); }   // S_t + mu S_{t-1}  ->  sum mu^(T-1-t) S_t
                for (int li = 0; li < 4; li++) {
                    vector<double> W; bool ok; solve_sys(Sa, Ra, LAMS[li], m, W, ok); double va = -1;
                    if (ok) { array<long, NC> c, t; eval_counts(gs.Fv, yv, nval, m, W, c, t); va = total_acc(c, t); }
                    if (va > sel[g][mi].val) sel[g][mi] = {va, GAMMAS[gi], LAMS[li], gi};
                }
            }
        }
        fprintf(stderr, "  [faz1 gamma=%.4f bitti %.0fs]\n", GAMMAS[gi], now() - T0);
    }
    double t_search = now() - T0;
    // ---- FAZ 2: secilen ayarlarla ardisik ogrenme + TEST (test yalniz burada, secim yapilmaz)
    set<int> gammas_needed; for (auto& a : sel) for (auto& s : a) gammas_needed.insert(s.gi);
    for (int gi : gammas_needed) {
        GS gs = class_stats(GAMMAS[gi], P, H, byc);
        for (size_t g = 0; g < groups.size(); g++) {
            int T = groups[g].tasks.size();
            for (int mi = 0; mi < 5; mi++) {
                Sel& s = sel[g][mi]; if (s.gi != gi) continue;
                vector<double> Sa((size_t)m * m, 0.0), Ra((size_t)m * NC, 0.0); vector<vector<double>> acc(T, vector<double>(T, -1));
                double t_steps = 0; vector<int> seen; array<long, NC> cF, tF; double acc_final_total = 0, acc_old = 0, acc_new = 0;
                for (int t = 0; t < T; t++) {
                    double t0 = now(); vector<double> St((size_t)m * m, 0.0), Rt((size_t)m * NC, 0.0); double tstat = 0;
                    for (int c : groups[g].tasks[t]) { add_scaled(St, gs.S[c], 1); add_scaled(Rt, gs.R[c], 1); tstat += gs.tc[c]; }
                    for (size_t i = 0; i < Sa.size(); i++) Sa[i] = St[i] + MUS[mi] * Sa[i];            // S_acc = S_yeni + mu S_eski
                    for (size_t i = 0; i < Ra.size(); i++) Ra[i] = Rt[i] + MUS[mi] * Ra[i];
                    vector<double> W; bool ok; solve_sys(Sa, Ra, s.lam, m, W, ok); t_steps += tstat + (now() - t0);
                    array<long, NC> c, tt; eval_counts(gs.Ft, yte.data(), NTE, m, W, c, tt);
                    for (int i = 0; i <= t; i++) acc[t][i] = group_acc(c, tt, groups[g].tasks[i]);
                    if (t == T - 1) { cF = c; tF = tt; }
                }
                vector<int> oldc, newc; for (int t = 0; t < T - 1; t++) for (int c : groups[g].tasks[t]) oldc.push_back(c); newc = groups[g].tasks[T - 1];
                acc_old = group_acc(cF, tF, oldc); acc_new = group_acc(cF, tF, newc); acc_final_total = total_acc(cF, tF);
                double forget = 0; for (int i = 0; i < T - 1; i++) forget += acc[i][i] - acc[T - 1][i]; forget /= (T - 1);
                printf("HAFIZA|%s|%d|%d|%s|%.1f|%.4f|%.0f|%.4f|%.4f|%.4f|%.4f|%.4f|%.2f", feat.c_str(), H, seed, groups[g].name.c_str(), MUS[mi], s.gam, s.lam, s.val, acc_old, acc_new, acc_final_total, forget, t_steps / T);
                for (int i = 0; i < T; i++) printf("|%.4f", acc[T - 1][i]); printf("\n"); fflush(stdout);
                // ---- mu=1 denklik kontrolu (g5 veya tek grup): birikimli (tek geciste) V2 ile karsilastir
                if (MUS[mi] == 1 && (groups[g].name == "g5" || feat == "cift_A")) {
                    vector<int> allfit(NFIT); iota(allfit.begin(), allfit.end(), 0); mt19937_64 sh(seed + 77); shuffle(allfit.begin(), allfit.end(), sh);
                    double t0 = now(); vector<double> Sj((size_t)m * m, 0.0), Rj((size_t)m * NC, 0.0); stats_rows(allfit, GAMMAS[gi], P, H, Sj, Rj);
                    vector<double> Wj, Wr; bool ok1, ok2; solve_sys(Sj, Rj, s.lam, m, Wj, ok1); double t_joint = now() - t0;
                    solve_sys(Sa, Ra, s.lam, m, Wr, ok2);
                    double dS = 0, mS = 0; for (int i = 0; i < m; i++) for (int j = i; j < m; j++) { size_t k = (size_t)i * m + j; dS = max(dS, fabs(Sj[k] - Sa[k])); mS = max(mS, fabs(Sj[k])); }
                    double dW = 0, mW = 0; for (size_t k = 0; k < Wj.size(); k++) { dW = max(dW, fabs(Wj[k] - Wr[k])); mW = max(mW, fabs(Wj[k])); }
                    array<long, NC> c1, t1, c2, t2; eval_counts(gs.Ft, yte.data(), NTE, m, Wj, c1, t1); eval_counts(gs.Ft, yte.data(), NTE, m, Wr, c2, t2);
                    printf("DENKLIK|%s|%d|%d|%s|S_goreli_fark=%.3e|W_goreli_fark=%.3e|test_birikimli_V2=%.4f|test_V3_mu1=%.4f|birikimli_yeniden_egitim=%.2fs|artimli_adim_ort=%.2fs\n",
                           feat.c_str(), H, seed, groups[g].name.c_str(), dS / mS, dW / mW, total_acc(c1, t1), total_acc(c2, t2), t_joint, t_steps / T);
                }
            }
        }
    }
    printf("HAFIZA_SURE|%s|%d|%d|arama(faz1)=%.1fs|toplam=%.1fs|RAM=%.0fMB|sinif_istatistik_toplam_6gamma=%.1fs\n", feat.c_str(), H, seed, t_search, now() - T0, peak_mb(), tcls_total);
    for (size_t g = 0; g < groups.size(); g++) for (int mi = 0; mi < 5; mi++) printf("SEC|%s|%d|%d|%s|mu=%.1f|gamma=%.4f|lambda=%.0f|val=%.4f%s\n", feat.c_str(), H, seed, groups[g].name.c_str(), MUS[mi], sel[g][mi].gam, sel[g][mi].lam, sel[g][mi].val, (sel[g][mi].gi == 0 || sel[g][mi].gi == 5) ? " [IZGARA KENARI]" : "");
}

int main(int argc, char** argv) {
    if (argc < 5) { cerr << "kullanim: tek_hamle_v3 yon|hafiza MOD H tohum [veri_klasoru]\n"; return 1; }
    string cmd = argv[1], mod = argv[2]; int H = atoi(argv[3]), seed = atoi(argv[4]); string dir = argc > 5 ? argv[5] : "/tmp/mnist_bin";
    Xtr = readbin<float>(dir + "/Xtr.f32", (size_t)NTR * D); Xte = readbin<float>(dir + "/Xte.f32", (size_t)NTE * D);
    ytr = readbin<int>(dir + "/ytr.i32", NTR); yte = readbin<int>(dir + "/yte.i32", NTE);
    if (cmd == "yon") run_yon(mod, H, seed); else if (cmd == "hafiza") run_hafiza(mod, H, seed); else { cerr << "yon|hafiza\n"; return 1; }
    return 0;
}
