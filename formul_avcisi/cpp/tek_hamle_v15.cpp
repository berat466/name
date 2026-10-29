// Tek hamle V15: ADIM ADIM ILERI + YEREL GERI DUZELTME: dW_t=(F_t^T F_t+lam I)^-1 F_t^T (T_t - F_t W_{t-1}), W_t=W_{t-1}+dW_t. V12 kopyasi (kernel); V8-V14 dosyalarina dokunulmadi.
// Tek hamle V12 (V11 kopyasi; ANA DEGISIKLIK: agirlik formulleri A ridge, B ornek-agirlikli ridge, C ozellik-bazli duzenlilestirme). V8-V11 dosyalarina dokunulmadi.
// Tek hamle V11 (V10'un kopyasi + KELIME KIMLIGI: seyrek one-hot/hash bloklari, yogun matris YOK). V10/V9/V8 dosyalarina dokunulmadi.
// (V10 notu:) Tek hamle V10 (V9'un kopyasi + ILISKI OZELLIKLERI: kelime farki D ve etkilesim I). Giris X' = [e_{t-c..t-1}, D_j = e_{j+1}-e_j (komsu cift), I_j = e_{j+1} (*) e_j]. V9/V8 dosyalarina dokunulmadi.
// (V9 notu:) Tek hamle V9 (V8'in kopyasi + skor dokumu): V8 ile ayni agirliklar uretilir, dogrulama (egitim-yalniz model) ve test (egitim+dogrulama modeli) SKORLARI /tmp/v9data'ya yazilir; kalibrasyon python'da. V8 dosyasina dokunulmadi.
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

struct Data { int n = 0; vector<float> X; vector<int> y; vector<int> ctx; vector<int> sp; };    // sp: n x 3 seyrek satir indisi (-1 = yok), deger 1
static int V, K;                                    // K: yogun boyut
static const int HB = 4096;                         // kelime-cifti hash kovasi
static int KT, BASEA, BASEB, BASEP;                 // toplam satir sayisi ve seyrek blok tabanlari
static Data load(const string& dir, int c, const string& spl, int n) { Data d; d.n = n; d.X = readbin<float>(dir + "/c" + to_string(c) + "_" + spl + "_X.f32", (size_t)n * K); d.y = readbin<int>(dir + "/c" + to_string(c) + "_" + spl + "_y.i32", n); d.ctx = readbin<int>(dir + "/c" + to_string(c) + "_" + spl + "_ctx.i32", (size_t)n * c); return d; }
// yogun kisim: [X, D] (V10 'fark'); seyrek kisim: oz'a gore kimlik bloklari (ONE-HOT YOGUN OLUSTURULMAZ)
static void expand(Data& d, int c, const string& oz) {
    const int E = 64, nd = c - 1; int oldK = c * E, newK = oldK + nd * E; vector<float> X2((size_t)d.n * newK);
    #pragma omp parallel for
    for (int i = 0; i < d.n; i++) { const float* x = &d.X[(size_t)i * oldK]; float* o = &X2[(size_t)i * newK]; memcpy(o, x, oldK * sizeof(float)); int p = oldK;
        for (int j = 0; j < nd; j++) for (int k = 0; k < E; k++) o[p++] = x[(j + 1) * E + k] - x[j * E + k]; }
    d.X = move(X2); d.sp.assign((size_t)d.n * 3, -1);
    for (int i = 0; i < d.n; i++) { int id2 = d.ctx[(size_t)i * c + c - 2], id1 = d.ctx[(size_t)i * c + c - 1]; int* s = &d.sp[(size_t)i * 3]; unsigned h = ((unsigned)id2 * 2654435761u) ^ ((unsigned)id1 * 40503u + 12345u); h = (h ^ (h >> 15)) * 2246822519u; int hb = (int)((h ^ (h >> 13)) % HB);
        if (oz == "bag") { s[0] = BASEA + id1; s[1] = BASEA + id2; }                    // sirasiz kimlik torbasi
        else if (oz == "seq") { s[0] = BASEA + id1; s[1] = BASEB + id2; }                // konuma ozel kimlik (t-1 ve t-2 ayri bloklar)
        else if (oz == "pair") { s[0] = BASEP + hb; }                                    // ardisik cift: hash kovasi
        else if (oz == "seqpair") { s[0] = BASEA + id1; s[1] = BASEB + id2; s[2] = BASEP + hb; } }
}
struct Feat { vector<float> W, b; int H; };       // W: KT x H (yogun kisim ilk K satir)
static Feat make_w1(const string& mode, const Data& tr, int H, int seed) {
    Feat f; f.H = H; f.W.assign((size_t)KT * H, 0.f); f.b.assign(H, 0.f); mt19937_64 g(seed), gb(seed + 7777); normal_distribution<double> N(0, 1);
    vector<double> pb(H); vector<double> wd(K); uniform_int_distribution<int> U(0, tr.n - 1);
    for (int j = 0; j < H; j++) {
        int a = U(g), b; do b = U(g); while (b == a || tr.y[b] == tr.y[a]);                  // hedefleri farkli iki egitim satiri
        const float *xa = &tr.X[(size_t)a * K], *xb = &tr.X[(size_t)b * K]; double nn = 0; for (int k = 0; k < K; k++) { double d = xa[k] - xb[k]; nn += d * d; }
        map<int, double> sd; for (int q = 0; q < 3; q++) { int ra = tr.sp[(size_t)a * 3 + q], rb = tr.sp[(size_t)b * 3 + q]; if (ra >= 0) sd[ra] += 1; if (rb >= 0) sd[rb] -= 1; }
        for (auto& kv : sd) nn += kv.second * kv.second;
        double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0;
        for (int k = 0; k < K; k++) { double w = (xa[k] - xb[k]) * inv; f.W[(size_t)k * H + j] = (float)w; dot += (xa[k] + xb[k]) * w; }
        for (auto& kv : sd) f.W[(size_t)kv.first * H + j] = (float)(kv.second * inv);
        for (int q = 0; q < 3; q++) { int ra = tr.sp[(size_t)a * 3 + q], rb = tr.sp[(size_t)b * 3 + q]; if (ra >= 0) dot += f.W[(size_t)ra * H + j]; if (rb >= 0) dot += f.W[(size_t)rb * H + j]; }
        pb[j] = -dot / 2;
    }
    double mu = 0, sdv = 0; for (double v : pb) mu += v; mu /= H; for (double v : pb) sdv += (v - mu) * (v - mu); sdv = sqrt(sdv / H);
    for (int j = 0; j < H; j++) f.b[j] = (float)(N(gb) * sdv);
    return f;
}
// F: n x m, kolon 0 = 1 ; Z = yogun*W + seyrek satir toplami
static void features(const Data& d, const Feat& f, double gam, vector<float>& F) {
    int H = f.H, m = H + 1; F.resize((size_t)d.n * m); gemm_f(d.X.data(), d.n, K, f.W.data(), H, F.data() + 1, m); const float g = (float)gam;
    #pragma omp parallel for
    for (int i = 0; i < d.n; i++) { float* r = &F[(size_t)i * m]; for (int q = 0; q < 3; q++) { int row = d.sp[(size_t)i * 3 + q]; if (row >= 0) { const float* w = &f.W[(size_t)row * H]; for (int j = 0; j < H; j++) r[j + 1] += w[j]; } }
        for (int j = 1; j <= H; j++) r[j] = tanhf(g * (r[j] + f.b[j - 1])); r[0] = 1.f; }
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



static vector<float> Ftr_, Fva_, Fte_; static Data TR, VA, TE; static int H_, M_;
struct StepLog { double val_ce, b1_ce, t_fwd, t_back; };
static double ce_fixed(const vector<float>& Z, const int* y, int n, const vector<double>& P, double alpha, double T) {
    double tot = 0;
    #pragma omp parallel for reduction(+ : tot) schedule(static)
    for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]); double se = 0; for (int v = 0; v < V; v++) se += exp((z[v] - mx) / T);
        double py = exp((z[y[i]] - mx) / T) / se; tot += -log((1 - alpha) * P[y[i]] + alpha * py); }
    return tot / n;
}
// W (m x V) uzerinde tek gecis; rows: F satir kaynagi (Fsrc, labels), blok B ; monitor: her adimda dogrulama/blok-1 CE
static void run_blocks(const vector<vector<float>*>& Fs, const vector<const int*>& ys, const vector<int>& ns, int B, double lam, vector<float>& W, double& t_fwd, double& t_back, vector<StepLog>* log, const vector<double>& P, double al, double Tt) {
    const int m = M_; W.assign((size_t)m * V, 0.f); t_fwd = t_back = 0; int step = 0;
    for (size_t part = 0; part < Fs.size(); part++) for (int s0 = 0; s0 < ns[part]; s0 += B) {
        int nn = min(B, ns[part] - s0); const float* Fb = &(*Fs[part])[(size_t)s0 * m]; const int* yb = ys[part] + s0; double t0 = now();
        vector<float> Z((size_t)nn * V); gemm_f(Fb, nn, m, W.data(), V, Z.data(), V);                      // ileri: F_t W_{t-1}
        #pragma omp parallel for
        for (int i = 0; i < nn; i++) { float* z = &Z[(size_t)i * V]; for (int v = 0; v < V; v++) z[v] = -z[v]; z[yb[i]] += 1.f; }   // E_t = T_t - F_t W
        t_fwd += now() - t0; t0 = now();
        vector<float> Ft((size_t)m * nn); for (int i = 0; i < nn; i++) for (int j = 0; j < m; j++) Ft[(size_t)j * nn + j * 0 + i] = Fb[(size_t)i * m + j];
        vector<float> Gf((size_t)m * V); gemm_f(Ft.data(), m, nn, Z.data(), V, Gf.data(), V); vector<double> G(Gf.begin(), Gf.end());   // F_t^T E_t
        vector<double> S((size_t)m * m, 0.0); syrk_flush256(Fb, nn, m, S.data()); for (int i = 0; i < m; i++) S[(size_t)i * m + i] += lam;
        cholesky(S, m); vector<float> dW; chol_solve_multi(S, m, G, V, dW);                                    // dW_t = (F^T F + lam I)^-1 F^T E
        for (size_t i = 0; i < W.size(); i++) W[i] += dW[i]; t_back += now() - t0; step++;
        if (log) { double tm = now(); vector<float> Zv((size_t)VA.n * V); gemm_f(Fva_.data(), VA.n, m, W.data(), V, Zv.data(), V); StepLog L; L.val_ce = ce_fixed(Zv, VA.y.data(), VA.n, P, al, Tt);
            int nb1 = min(B, TR.n); vector<float> Zb((size_t)nb1 * V); gemm_f(Ftr_.data(), nb1, m, W.data(), V, Zb.data(), V); L.b1_ce = ce_fixed(Zb, TR.y.data(), nb1, P, al, Tt); L.t_fwd = 0; L.t_back = 0; log->push_back(L); t_fwd -= 0; (void)tm; }
    }
}
int main(int argc, char** argv) {
    if (argc < 4) { cerr << "kullanim: tek_hamle_v15 c H TOHUM gamma [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]); string dir = argc > 5 ? argv[5] : "/tmp/v8data"; string oz = "ref"; H_ = H; M_ = H + 1; const int m = M_;
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    TR = load(dir, c, "train", ntr); VA = load(dir, c, "val", nva); TE = load(dir, c, "test", nte);
    { int Kd = c * 64 + (c - 1) * 64; BASEA = Kd; BASEB = Kd + V; BASEP = Kd; KT = Kd; expand(TR, c, oz); expand(VA, c, oz); expand(TE, c, oz); K = Kd; }
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V15 c=%d H=%d tohum=%d gamma=%g V=%d egitim=%d dogrulama=%d test=%d (%d cekirdek)\n", c, H, seed, gam, V, ntr, nva, nte, omp_get_max_threads());
    double T0 = now(); Feat f = make_w1("cift", TR, H, seed); features(TR, f, gam, Ftr_); features(VA, f, gam, Fva_); features(TE, f, gam, Fte_);
    const double LAMS[3] = {10, 100, 1000}; const int BS[3] = {1000, 4000, 8000}; const double TC[4] = {0.005, 0.01, 0.02, 0.05}, AC[4] = {0.15, 0.25, 0.35, 0.5};
    struct Best { double ce = 1e30, lam = 0; int B = 0; } best;
    for (int bi = 0; bi < 3; bi++) for (int li = 0; li < 3; li++) {
        vector<float> W; double tf, tb; run_blocks({&Ftr_}, {TR.y.data()}, {ntr}, BS[bi], LAMS[li], W, tf, tb, nullptr, Ptr, 0, 0);
        vector<float> Zv((size_t)nva * V); gemm_f(Fva_.data(), nva, m, W.data(), V, Zv.data(), V); double pce = 1e30; for (double T : TC) for (double a : AC) pce = min(pce, ce_fixed(Zv, VA.y.data(), nva, Ptr, a, T));
        double raw = ce_fixed(Zv, VA.y.data(), nva, Ptr, 1.0, 0.02);
        printf("  B=%d lambda=%g ileri=%.2fs geri_duzeltme=%.2fs  val_CE(ham a=1,T=0.02)=%.4f  val_CE(mini-kalibre)=%.4f\n", BS[bi], LAMS[li], tf, tb, raw, pce); fflush(stdout);
        if (pce < best.ce) best = {pce, LAMS[li], BS[bi]};
    }
    printf("SECILEN B=%d lambda=%g val_CE(mini-kalibre)=%.4f\n", best.B, best.lam, best.ce);
    // adim adim izleme (secilen ayar, egitim-yalniz): sabit kalibrasyon alpha=0.25, T=0.01 (V12 degerleri)
    vector<StepLog> log; vector<float> W; double tf, tb; run_blocks({&Ftr_}, {TR.y.data()}, {ntr}, best.B, best.lam, W, tf, tb, &log, Ptr, 0.25, 0.01);
    printf("ADIM_IZLEME (sabit kalibrasyon a=0.25,T=0.01): adim | dogrulama CE | blok-1 (ilk blok) egitim CE\n"); for (size_t i = 0; i < log.size(); i++) printf("  adim %zu: val_CE=%.4f blok1_CE=%.4f\n", i + 1, log[i].val_ce, log[i].b1_ce);
    double b1_after1 = log.front().b1_ce, b1_end = log.back().b1_ce; printf("UNUTMA blok-1: adim1 sonrasi %.4f -> son %.4f (fark %+.4f)\n", b1_after1, b1_end, b1_end - b1_after1);
    printf("SURE egitim-yalniz: ileri_hesap_toplam=%.3fs geri_duzeltme_toplam=%.3fs (%zu adim, blok basina ileri %.3fs geri %.3fs)\n", tf, tb, log.size(), tf / log.size(), tb / log.size());
    // dokumler: val (egitim-yalniz son W), test (egitim sonra dogrulama bloklariyla devam eden W)
    { vector<float> Zv((size_t)nva * V); gemm_f(Fva_.data(), nva, m, W.data(), V, Zv.data(), V); FILE* fo = fopen(("/tmp/v15data/cift_loc_c" + to_string(c) + "_H" + to_string(H) + "_s" + to_string(seed) + "_val.f32").c_str(), "wb"); fwrite(Zv.data(), sizeof(float), Zv.size(), fo); fclose(fo); }
    vector<float> W2; double tf2, tb2; run_blocks({&Ftr_, &Fva_}, {TR.y.data(), VA.y.data()}, {ntr, nva}, best.B, best.lam, W2, tf2, tb2, nullptr, Ptr, 0, 0);
    { vector<float> Zt((size_t)nte * V); gemm_f(Fte_.data(), nte, m, W2.data(), V, Zt.data(), V); FILE* fo = fopen(("/tmp/v15data/cift_loc_c" + to_string(c) + "_H" + to_string(H) + "_s" + to_string(seed) + "_test.f32").c_str(), "wb"); fwrite(Zt.data(), sizeof(float), Zt.size(), fo); fclose(fo); }
    printf("TOPLAM sure=%.1fs tepe_RAM=%.0fMB\n", now() - T0, peak_mb());
    return 0;
}
