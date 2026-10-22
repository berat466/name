// Tek hamle V4: BUYUYEN W1 + KALICI HAFIZA (sinif-artimli MNIST, 5 gorev x 2 sinif). GRADYAN/ADAM/EPOK YOK.
// Tek cozum: W = (S + lambda I)^-1 R (Cholesky). W1 yalniz ilk gorevin verisinden (H0=1024), her yeni gorevde +256 ozellik (yalniz o gorevin verisinden), en fazla 2048.
// Sutun duzeni: 0 = bias, 1..H = ozellikler (buyume sona eklenir -> eski S,R indeksleri degismez).
// Kullanim: ./tek_hamle_v4 AILE TOHUM [gamma=0.125] [lambda=1] [veri_klasoru]
//   AILE: buyume (SIFIR, DONUK, ORNEK20, ORNEK100, TAVAN) | sabit1024 | sabit2048 | v3_1024 | v3_2048
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle_v4 tek_hamle_v4.cpp
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

// bir adimin ciktilari: ADIM + GOREV satirlari
static void report(const string& aile, const string& name, int seed, int t, int m, const vector<Ev>& tst, const vector<Ev>& trn, size_t k, double t_new, double t_m, double t_eval, double lamu, int fail) {
    vector<int> oldc = cls_of(0, t - 1), newc = TASKS[t - 1], seen = cls_of(0, t), all = cls_of(0, 5);
    Gr to = grp(tst[k], oldc), tn = grp(tst[k], newc), ts = grp(tst[k], seen), ta = grp(tst[k], all), ro = grp(trn[k], oldc), rn = grp(trn[k], newc), rs = grp(trn[k], seen);
    printf("ADIM|%s|%s|%d|%g|%g|%d|%d|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f|%.4f|%.4f|%.4f|%.3f|%.3f|%.3f|%g|%d\n", aile.c_str(), name.c_str(), seed, GAM, LAM, t, m, to.mse, tn.mse, ts.mse, ta.mse, ro.mse, rn.mse, rs.mse, to.acc, tn.acc, ts.acc, t_new, t_m, t_eval, lamu, fail);
    for (int i = 0; i < t; i++) { Gr a = grp(tst[k], TASKS[i]), b = grp(trn[k], TASKS[i]); printf("GOREV|%s|%s|%d|%d|%d|%.6f|%.6f|%.4f\n", aile.c_str(), name.c_str(), seed, t, i + 1, a.mse, b.mse, a.acc); }
    fflush(stdout);
}

int main(int argc, char** argv) {
    if (argc < 3) { cerr << "kullanim: tek_hamle_v4 AILE TOHUM [gamma] [lambda] [veri_klasoru]\n"; return 1; }
    string aile = argv[1]; int seed = atoi(argv[2]); if (argc > 3) GAM = atof(argv[3]); if (argc > 4) LAM = atof(argv[4]); string dir = argc > 5 ? argv[5] : "/tmp/mnist_bin";
    Xtr = readbin<float>(dir + "/Xtr.f32", (size_t)NTR * D); Xte = readbin<float>(dir + "/Xte.f32", (size_t)NTE * D);
    ytr = readbin<int>(dir + "/ytr.i32", NTR); yte = readbin<int>(dir + "/yte.i32", NTE);
    byc.assign(NC, {}); for (int i = 0; i < NFIT; i++) byc[ytr[i]].push_back(i);
    double T0 = now(); const int T = 5;
    printf("V4 aile=%s tohum=%d gamma=%g lambda=%g (%d cekirdek)\n", aile.c_str(), seed, GAM, LAM, omp_get_max_threads());
    auto mkseed = [&](int task, int tag) { return (uint64_t)seed * 100003ULL + task * 131 + tag; };

    if (aile.rfind("sabit", 0) == 0 || aile.rfind("v3_", 0) == 0) {
        // ------- sabit ozellik + birikimli hafiza (mu=1). sabit*: W1 yalniz gorev-1 verisinden; v3_*: W1 tum siniflardan (V3 referansi, gelecegi gormus)
        int H = atoi(aile.substr(aile.find_first_of("0123456789")).c_str()); bool v3 = aile.rfind("v3_", 0) == 0;
        Feat f; f.blocks.push_back(make_block(v3 ? cls_of(0, 5) : TASKS[0], H, mkseed(1, 0))); rebuild(f); int m = H + 1;
        vector<double> S((size_t)m * m, 0.0), R((size_t)m * NC, 0.0), W; string name = (v3 ? "V3_" : "SABIT_") + to_string(H);
        for (int t = 1; t <= T; t++) {
            double t0 = now(); stats_rows(rows_of(TASKS[t - 1]), f, S, R); double tn = now() - t0; t0 = now(); solve_sys(S, R, m, W); double tm = now() - t0;
            double e0 = now(); vector<int> seenrows = rows_of(cls_of(0, t)); vector<const vector<double>*> Ws = {&W};
            auto trn = eval_ws(Xtr.data(), ytr.data(), &seenrows, seenrows.size(), f, Ws); auto tst = eval_ws(Xte.data(), yte.data(), nullptr, NTE, f, Ws);
            report(aile, name, seed, t, m, tst, trn, 0, tn, tm, now() - e0, lam_used, last_fail);
        }
    } else if (aile == "buyume") {
        const int NMET = 7; const char* names[NMET] = {"BUYUME_SIFIR", "BUYUME_DONUK", "BUYUME_ORNEK20", "BUYUME_ORNEK100", "BUYUME_TAVAN", "BUYUME_TAMPON20", "BUYUME_TAMPON100"};
        const int KB[2] = {20, 100};
        Feat f; vector<MethodState> M(NMET); for (int k = 0; k < NMET; k++) M[k].name = names[k];
        // ornek hafizasi: her gorev geldiginde siniflardan K satir (yalniz o gorevin verisi); agirlik = sinif fit sayisi / K
        vector<vector<int>> bufrows(2); vector<vector<double>> bufw(2); mt19937_64 gbuf(seed + 424242);
        int mold = 0;
        for (int t = 1; t <= T; t++) {
            int dh = (t == 1) ? 1024 : 256; f.blocks.push_back(make_block(TASKS[t - 1], dh, mkseed(t, 1))); rebuild(f); int m = f.H + 1;
            vector<int> trows = rows_of(TASKS[t - 1]);
            // yeni gorevin KESIN istatistigi (tum m ozellik uzerinde)
            double t0 = now(); vector<double> Sn((size_t)m * m, 0.0), Rn((size_t)m * NC, 0.0); stats_rows(trows, f, Sn, Rn); double tnew = now() - t0;
            // ---- SIFIR: eski S,R sifir dolgulu + yeni gorev; ortak cozum
            t0 = now(); { vector<double> S((size_t)m * m, 0.0), R((size_t)m * NC, 0.0); if (t > 1) { pad_add(M[0].S, mold, S, m); padR_add(M[0].R, mold, R); } for (size_t i = 0; i < S.size(); i++) S[i] += Sn[i]; for (size_t i = 0; i < R.size(); i++) R[i] += Rn[i];
                M[0].S = move(S); M[0].R = move(R); M[0].m = m; solve_sys(M[0].S, M[0].R, m, M[0].W); M[0].lamu = lam_used; M[0].fail = last_fail; } M[0].t_method = now() - t0;
            // ---- DONUK: eski W donuk, yalniz yeni blok yeni gorev istatistigiyle (kalinti) cozulur
            t0 = now(); { last_fail = 0; lam_used = LAM; if (t == 1) { solve_sys(Sn, Rn, m, M[1].W); } else {
                int dn = m - mold; vector<double> A((size_t)dn * dn, 0.0), rhs((size_t)dn * NC, 0.0), Wn;
                for (int i = 0; i < dn; i++) for (int j = i; j < dn; j++) A[(size_t)i * dn + j] = Sn[(size_t)(mold + i) * m + mold + j];
                for (int i = 0; i < dn; i++) { for (int c = 0; c < NC; c++) { double s = Rn[(size_t)(mold + i) * NC + c]; for (int k = 0; k < mold; k++) s -= Sn[(size_t)k * m + mold + i] * M[1].W[(size_t)k * NC + c]; rhs[(size_t)i * NC + c] = s; } }
                double lam = LAM; bool ok = false; for (int tr = 0; tr < 6 && !ok; tr++) { vector<double> B = A; for (int i = 0; i < dn; i++) B[(size_t)i * dn + i] += lam; if (cholesky(B, dn)) { chol_solve(B, dn, rhs, Wn); ok = true; } else { retries++; lam *= 10; } } lam_used = lam; last_fail = ok ? 0 : 1; if (!ok) Wn.assign((size_t)dn * NC, 0.0);
                M[1].W.resize((size_t)m * NC, 0.0); for (int i = 0; i < dn; i++) for (int c = 0; c < NC; c++) M[1].W[(size_t)(mold + i) * NC + c] = Wn[(size_t)i * NC + c]; } } M[1].lamu = lam_used; M[1].fail = last_fail; M[1].t_method = now() - t0;
            // ---- ORNEK20/100: sifir dolgu + (eski veri icin eksik bloklar tamponun agirlikli ornegiyle TAHMIN)
            for (int q = 0; q < 2; q++) { MethodState& ms = M[2 + q]; MethodState& mt = M[5 + q]; t0 = now();
                vector<double> S((size_t)m * m, 0.0), R((size_t)m * NC, 0.0), St((size_t)m * m, 0.0), Rt((size_t)m * NC, 0.0);
                if (t > 1) {
                    pad_add(ms.S, mold, S, m); padR_add(ms.R, mold, R);
                    int nb = bufrows[q].size(); vector<float> Xb((size_t)nb * D), F; vector<int> yb(nb);
                    for (int i = 0; i < nb; i++) { memcpy(&Xb[(size_t)i * D], &Xtr[(size_t)bufrows[q][i] * D], D * sizeof(float)); yb[i] = ytr[bufrows[q][i]]; }
                    features(Xb.data(), nb, f, F); vector<float> Fs = F, Fw = F;
                    for (int i = 0; i < nb; i++) { float sw = (float)sqrt(bufw[q][i]), ww = (float)bufw[q][i]; for (int j = 0; j < m; j++) { Fs[(size_t)i * m + j] *= sw; Fw[(size_t)i * m + j] *= ww; } }
                    vector<double> Se((size_t)m * m, 0.0), Re((size_t)m * NC, 0.0); syrk_add(Fs.data(), nb, m, Se.data()); rhs_add(Fw.data(), yb.data(), nb, m, Re.data());
                    for (int i = 0; i < m; i++) for (int j = max(i, mold); j < m; j++) S[(size_t)i * m + j] += Se[(size_t)i * m + j];     // KARMA: kesin eski blok + yalniz yeni sutun iceren bloklar tampondan
                    for (int i = mold; i < m; i++) for (int c = 0; c < NC; c++) R[(size_t)i * NC + c] += Re[(size_t)i * NC + c];
                    St = Se; Rt = Re;                                                                                                         // TAMPON: eski verinin TUM istatistigi tampondan (PSD)
                }
                for (size_t i = 0; i < S.size(); i++) { S[i] += Sn[i]; St[i] += Sn[i]; } for (size_t i = 0; i < R.size(); i++) { R[i] += Rn[i]; Rt[i] += Rn[i]; }
                ms.S = move(S); ms.R = move(R); ms.m = m; solve_sys(ms.S, ms.R, m, ms.W); ms.lamu = lam_used; ms.fail = last_fail; ms.t_method = now() - t0;
                t0 = now(); mt.m = m; solve_sys(St, Rt, m, mt.W); mt.lamu = lam_used; mt.fail = last_fail; mt.t_method = now() - t0 + (ms.t_method > 0 ? 0 : 0);
                for (int c : TASKS[t - 1]) { vector<int> rr = byc[c]; shuffle(rr.begin(), rr.end(), gbuf); int K = KB[q]; for (int i = 0; i < K; i++) { bufrows[q].push_back(rr[i]); bufw[q].push_back((double)byc[c].size() / K); } }   // bu gorevin ornekleri tampona
            }
            // ---- TAVAN (bilgi icin, uygulanamaz): gorulen tum gorevlerin verisiyle simdiki ozelliklerin kesin ortak cozumu
            t0 = now(); { vector<double> S((size_t)m * m, 0.0), R((size_t)m * NC, 0.0); stats_rows(rows_of(cls_of(0, t)), f, S, R); solve_sys(S, R, m, M[4].W); M[4].lamu = lam_used; M[4].fail = last_fail; } M[4].t_method = now() - t0;
            // ---- degerlendirme (hepsi ayni ozellik kumesi)
            double e0 = now(); vector<int> seenrows = rows_of(cls_of(0, t)); vector<const vector<double>*> Ws; for (int k = 0; k < NMET; k++) Ws.push_back(&M[k].W);
            auto trn = eval_ws(Xtr.data(), ytr.data(), &seenrows, seenrows.size(), f, Ws); auto tst = eval_ws(Xte.data(), yte.data(), nullptr, NTE, f, Ws); double te = now() - e0;
            for (int k = 0; k < NMET; k++) report(aile, M[k].name, seed, t, m, tst, trn, k, tnew, M[k].t_method, te, M[k].lamu, M[k].fail);
            mold = m;
        }
    } else { cerr << "bilinmeyen aile\n"; return 1; }
    printf("OZET_ISLEM|%s|%d|toplam_sn=%.1f|tepe_RAM_MB=%.0f|cholesky_yeniden=%d\n", aile.c_str(), seed, now() - T0, peak_mb(), retries);
    return 0;
}
