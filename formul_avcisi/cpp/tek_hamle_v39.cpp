// V34: V33 + OPSIYONEL parcali hesap (V34_CHOL_BLOCKS: blok Cholesky 4002; V34_RHS_GROUPS: sag taraf sutun gruplari; V34_FAST: paralel kopya/Gram/C-kurulumu). Varsayilan (hepsi kapali) = V33 ile ayni.
// Tek hamle V20: V19 CE duzeltmesi + hassas eta + Top-1 analizi + hiz ayristirmasi. V19 kopyasi; V8-V19 dosyalarina dokunulmadi.
// Tek hamle V19: Ridge W0 + TEK CE-gradyan duzeltmesi W1=W0-eta A^-1 F^T G (mevcut Cholesky yeniden kullanilir). V18 kopyasi (kernel/yardimcilar); V8-V18 dosyalarina dokunulmadi.
// NOT: Bu, Ridge tek hamle cozumune EK islemdir; gercek Newton degil, on kosullandirilmis tek CE gradyan adimidir.
// Tek hamle V18: lambda ızgarasi 8 deger {0.001..1000} + sayisal kararlilik/asiri uyum olcumleri. V17 kopyasi; V8-V17 dosyalarina dokunulmadi.
// (V17 notu:) Tek hamle V17: gamma/lambda/T/alpha KALIBRASYON SONRASI dogrulama CE ile secilir (V12 ridge korunur). V16 kopyasi (kernel); V8-V16 dosyalarina dokunulmadi.
// Tek hamle V16: Hadamard olcekleme (s), logaritmik hedef donusumu T'=log(T+eps). V12 kopyasi (kernel); V8-V15 dosyalarina dokunulmadi.
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
#include <functional>
#include <array>
#include <unordered_map>
#include <unordered_map>
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

extern "C" {
void scipy_dpotrf_(const char*, const int*, double*, const int*, int*);
void scipy_cblas_dgemm(int, int, int, int, int, int, double, const double*, int, const double*, int, double, double*, int);
void scipy_cblas_sgemm(int, int, int, int, int, int, float, const float*, int, const float*, int, float, float*, int);
void scipy_cblas_dtrsm(int, int, int, int, int, int, int, double, const double*, int, double*, int);
void scipy_openblas_set_num_threads(int);
int scipy_openblas_get_num_threads(void);
}
static bool g_blas = false, g_fast = false; static int g_rhsg = 1, g_cholblk = 1;
extern "C" int openblas_set_num_threads_local(int);
extern "C" void scipy_cblas_dsyrk(int, int, int, int, int, double, const double*, int, double, double*, int);
static void gemm_f(const float* A, int n, int K, const float* B, int N, float* C, int ldc) {
    if (g_blas) { scipy_cblas_sgemm(101, 111, 111, n, N, K, 1.f, A, K, B, N, 0.f, C, ldc); return; }
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
    if (g_blas && g_cholblk > 1 && m > 1000) {   // V34 blok Cholesky (satir-ana UST ucgen: U^T U = A), sagdan-bakan; diyagonal blok dpotrf, panel dtrsm, artik dsyrk
        int nb = (m + g_cholblk - 1) / g_cholblk; for (int b0 = 0; b0 < m; b0 += nb) { int bs = min(nb, m - b0), info = 0; char u = 'L'; double* dk = &A[(size_t)b0 * m + b0]; scipy_dpotrf_(&u, &bs, dk, &m, &info); if (info) return false; int rest = m - b0 - bs; if (rest > 0) {
            double* pr = &A[(size_t)b0 * m + b0 + bs]; scipy_cblas_dtrsm(101, 141, 121, 112, 131, bs, rest, 1.0, dk, m, pr, m); scipy_cblas_dsyrk(101, 121, 112, rest, bs, -1.0, pr, m, 1.0, &A[(size_t)(b0 + bs) * m + b0 + bs], m); } } return true; }
    if (g_blas) { int n = m, info = 0; char u = 'L'; scipy_dpotrf_(&u, &n, A.data(), &n, &info); return info == 0; }   // C-satir-ana ust ucgeni okur (eski kodla ayni girdi)
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
static void chol_solve_multi_d(const vector<double>& L, int m, const vector<double>& R, int ncols, vector<double>& X) {   // V33: dpotrf sonrasi (satir-ana ust ucgen = L^T): U^T Y = B ; U X = Y
    X = R;
    if (g_rhsg > 1 && ncols >= 64) {   // V34: sutun gruplari bagimsiz; her grup 1 BLAS ipligiyle, gruplar OpenMP ile paralel (kopya yok: ldb=ncols, ofsetli isaretci)
        int G = g_rhsg, ch = (ncols + G - 1) / G;
#pragma omp parallel for schedule(static) num_threads(G)
        for (int g = 0; g < G; g++) { int c0 = g * ch, nc = min(ch, ncols - c0); if (nc <= 0) continue; openblas_set_num_threads_local(1);
            scipy_cblas_dtrsm(101, 141, 121, 112, 131, m, nc, 1.0, L.data(), m, X.data() + c0, ncols); scipy_cblas_dtrsm(101, 141, 121, 111, 131, m, nc, 1.0, L.data(), m, X.data() + c0, ncols); openblas_set_num_threads_local(-1); }
        return; }
    scipy_cblas_dtrsm(101, 141, 121, 112, 131, m, ncols, 1.0, L.data(), m, X.data(), ncols); scipy_cblas_dtrsm(101, 141, 121, 111, 131, m, ncols, 1.0, L.data(), m, X.data(), ncols);
}
static void chol_solve_multi(const vector<double>& L, int m, const vector<double>& R, int V, vector<float>& Wf) {
    if (g_blas) { vector<double> X; chol_solve_multi_d(L, m, R, V, X); Wf.resize((size_t)m * V);
#pragma omp parallel for schedule(static)
        for (size_t q = 0; q < Wf.size(); q++) Wf[q] = (float)X[q]; return; }
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




static void expand_s(Data& d, int c) {            // yogun: [e_{t-2}, e_{t-1}, D]  (V12 'ref')
    const int E = 64, oldK = c * E; int newK = oldK + E; vector<float> X2((size_t)d.n * newK);
    #pragma omp parallel for
    for (int i = 0; i < d.n; i++) { const float* x = &d.X[(size_t)i * oldK]; float* o = &X2[(size_t)i * newK]; memcpy(o, x, oldK * sizeof(float)); int p = oldK; for (int k = 0; k < E; k++) o[p++] = x[E + k] - x[k]; }
    d.X = move(X2); d.sp.assign((size_t)d.n * 3, -1);
}
static const int NT = 14, NA = 10;
static const double TG[NT] = {0.003, 0.005, 0.01, 0.02, 0.03, 0.05, 0.07, 0.1, 0.2, 0.25, 0.5, 1, 2, 4};
static const bool TV8[NT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};       // V8 sicaklik izgarasi
static const bool TV9[NT] = {0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1};       // V9 genisletilmis izgara
static const double AG[NA] = {0, 0.05, 0.1, 0.15, 0.2, 0.25, 0.35, 0.5, 0.75, 1};
struct Cal { double raw = 1e30, cal = 1e30; int ti = 0, ai = 0, rti = 0; double tr = 0, res = -1, piv = 0, cnd = 0; bool fail = false; };
// Z: n x V ham skor, y, P: unigram. ce[T][alpha] = ortalama CE
static void ce_table(const vector<float>& Z, const int* y, int n, const vector<double>& P, double ce[NT][NA]) {
    for (int t = 0; t < NT; t++) for (int a = 0; a < NA; a++) ce[t][a] = 0;
    #pragma omp parallel
    { double loc[NT][NA]; memset(loc, 0, sizeof loc);
      #pragma omp for schedule(static)
      for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]);
          for (int t = 0; t < NT; t++) { double se = 0; for (int v = 0; v < V; v++) se += exp((z[v] - mx) / TG[t]); double py = exp((z[y[i]] - mx) / TG[t]) / se;
              for (int a = 0; a < NA; a++) loc[t][a] += -log((1 - AG[a]) * P[y[i]] + AG[a] * py); } }
      #pragma omp critical
      { for (int t = 0; t < NT; t++) for (int a = 0; a < NA; a++) ce[t][a] += loc[t][a]; } }
    for (int t = 0; t < NT; t++) for (int a = 0; a < NA; a++) ce[t][a] /= n;
}
struct Res { double ce, ppl, top1, top5; };
static Res eval_cal(const vector<float>& Z, const int* y, int n, const vector<double>& P, double al, double T) {
    double sce = 0; long c1 = 0, c5 = 0;
    #pragma omp parallel for reduction(+ : sce, c1, c5) schedule(static)
    for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]); double se = 0; for (int v = 0; v < V; v++) se += exp((z[v] - mx) / T);
        vector<double> p(V); for (int v = 0; v < V; v++) p[v] = (1 - al) * P[v] + al * exp((z[v] - mx) / T) / se; double py = p[y[i]]; sce += -log(py); int rank = 0; for (int v = 0; v < V; v++) if (p[v] > py) rank++; c1 += (rank == 0); c5 += (rank < 5); }
    return {sce / n, exp(sce / n), (double)c1 / n, (double)c5 / n};
}



struct GT { double fwd = 0, soft = 0, tr = 0, bwd = 0, acc = 0, solve = 0; };
// opt=false: V19'un orijinali (softmax icin 2 exp gecisi); opt=true: tek exp gecisi (matematik ayni)
static void ce_grad(const float* F, const int* y, int n, int m, const vector<float>& W0, const vector<double>& P, double al, double T, vector<double>& G, double& ce_sum, GT& tm, bool opt) {
    const int CHn = 4096; vector<float> Z, Ft, Gc;
    for (int s0 = 0; s0 < n; s0 += CHn) { int nn = min(CHn, n - s0); const float* Fb = F + (size_t)s0 * m; double t0 = now(); Z.assign((size_t)nn * V, 0.f); gemm_f(Fb, nn, m, W0.data(), V, Z.data(), V); tm.fwd += now() - t0; t0 = now(); double cs = 0;
        #pragma omp parallel reduction(+ : cs)
        { vector<double> e(V);
          #pragma omp for schedule(static)
          for (int i = 0; i < nn; i++) { float* z = &Z[(size_t)i * V]; int yy = y[s0 + i]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]); double se = 0;
              if (opt) { for (int v = 0; v < V; v++) { e[v] = exp((z[v] - mx) / T); se += e[v]; } } else { for (int v = 0; v < V; v++) se += exp((z[v] - mx) / T); }
              double qy = exp((z[yy] - mx) / T) / se, py = (1 - al) * P[yy] + al * qy, cc = al * qy / (T * py); cs += -log(py);
              if (opt) { double inv = cc / se; for (int v = 0; v < V; v++) z[v] = (float)(inv * e[v]); z[yy] -= (float)cc; }
              else for (int v = 0; v < V; v++) { double q = exp((z[v] - mx) / T) / se; z[v] = (float)(cc * (q - (v == yy ? 1.0 : 0.0))); } } }
        ce_sum += cs; tm.soft += now() - t0; t0 = now(); Ft.assign((size_t)m * nn, 0.f); for (int i = 0; i < nn; i++) for (int j = 0; j < m; j++) Ft[(size_t)j * nn + i] = Fb[(size_t)i * m + j]; tm.tr += now() - t0; t0 = now();
        Gc.assign((size_t)m * V, 0.f); gemm_f(Ft.data(), m, nn, Z.data(), V, Gc.data(), V); tm.bwd += now() - t0; t0 = now(); for (size_t i = 0; i < G.size(); i++) G[i] += Gc[i]; tm.acc += now() - t0; }
}
// satir bazli metrikler (kalibre): ce, top1, top5 ; ham top1 (argmax z)
struct Row { vector<float> ce; vector<char> t1, t5, r1; };
static Row row_metrics(const vector<float>& Z, const int* y, int n, const vector<double>& P, double al, double T) {
    Row R; R.ce.resize(n); R.t1.resize(n); R.t5.resize(n); R.r1.resize(n);
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double mx = -1e30; int am = 0; for (int v = 0; v < V; v++) if (z[v] > mx) { mx = z[v]; am = v; } double se = 0; for (int v = 0; v < V; v++) se += exp((z[v] - mx) / T);
        vector<double> p(V); for (int v = 0; v < V; v++) p[v] = (1 - al) * P[v] + al * exp((z[v] - mx) / T) / se; double py = p[y[i]]; int rank = 0; for (int v = 0; v < V; v++) if (p[v] > py) rank++;
        R.ce[i] = (float)-log(py); R.t1[i] = (rank == 0); R.t5[i] = (rank < 5); R.r1[i] = (am == y[i]); }
    return R;
}



struct Teach { int n = 0; vector<int> c1; unordered_map<long long, vector<int>> l2, l3; vector<int> ctx, y; };
static void build_teach(Teach& T, const vector<int>& ctx, const vector<int>& y) { int n = (int)y.size(); T.n = n; T.c1.assign(V, 0); T.ctx = ctx; T.y = y; for (int i = 0; i < n; i++) { int u = ctx[2 * i], w = ctx[2 * i + 1], yy = y[i]; T.c1[yy]++; T.l2[w].push_back(yy); T.l3[(long long)u * V + w].push_back(yy); } }
// V23 yumusatma (b): kendisi haric (LOO) interpolasyon
static void teach_row(const Teach& T, int i, double b, vector<double>& p, vector<double>& t1, vector<double>& t2) {
    int u = T.ctx[2 * i], w = T.ctx[2 * i + 1], yy = T.y[i]; double n1 = T.n - 1 + V;
    for (int v = 0; v < V; v++) p[v] = (T.c1[v] + 1.0 - (v == yy ? 1 : 0)) / n1;
    { fill(t1.begin(), t1.end(), 0.0); const auto& L = T.l2.at(w); for (int v : L) t1[v] += 1; t1[yy] -= 1; double tot = (double)L.size() - 1; for (int v = 0; v < V; v++) p[v] = (t1[v] + b * p[v]) / (tot + b); }
    { fill(t2.begin(), t2.end(), 0.0); const auto& L = T.l3.at((long long)u * V + w); for (int v : L) t2[v] += 1; t2[yy] -= 1; double tot = (double)L.size() - 1; for (int v = 0; v < V; v++) p[v] = (t2[v] + b * p[v]) / (tot + b); }
}
// ---- V8 n-gram referansi: dogrusal interpolasyon (ML trigram, ML bigram, unigram(+1), uniform), agirliklar dogrulamada EM ile
static void comp_y(const Teach& T, int u, int w, int y, bool loo, double q[4], bool av[2]) {
    int l = loo ? 1 : 0; q[2] = (T.c1[y] + 1.0 - l) / (T.n + V - l); q[3] = 1.0 / V; q[0] = q[1] = 0; av[0] = av[1] = false;
    auto i2 = T.l2.find(w); if (i2 != T.l2.end()) { int tot = (int)i2->second.size() - l; if (tot > 0) { int cy = 0; for (int v : i2->second) cy += (v == y); cy -= l; q[1] = (double)cy / tot; av[0] = true; } }
    auto i3 = T.l3.find((long long)u * V + w); if (i3 != T.l3.end()) { int tot = (int)i3->second.size() - l; if (tot > 0) { int cy = 0; for (int v : i3->second) cy += (v == y); cy -= l; q[0] = (double)cy / tot; av[1] = true; } }
}
static void dense_row(const Teach& T, int u, int w, int y, bool loo, const double a[4], vector<double>& p, vector<double>& c2, vector<double>& c3, bool& s2, bool& s3) {
    int l = loo ? 1 : 0; double n1 = T.n + V - l, s = a[2] + a[3]; for (int v = 0; v < V; v++) p[v] = a[2] * (T.c1[v] + 1.0 - (loo && v == y ? 1 : 0)) / n1 + a[3] / V; s2 = s3 = false;
    if (a[1] > 0) { auto it = T.l2.find(w); if (it != T.l2.end()) { int tot = (int)it->second.size() - l; if (tot > 0) { s2 = true; fill(c2.begin(), c2.end(), 0.0); for (int v : it->second) c2[v] += 1; if (loo) c2[y] -= 1; for (int v = 0; v < V; v++) p[v] += a[1] * c2[v] / tot; s += a[1]; } } }
    if (a[0] > 0) { auto it = T.l3.find((long long)u * V + w); if (it != T.l3.end()) { int tot = (int)it->second.size() - l; if (tot > 0) { s3 = true; fill(c3.begin(), c3.end(), 0.0); for (int v : it->second) c3[v] += 1; if (loo) c3[y] -= 1; for (int v = 0; v < V; v++) p[v] += a[0] * c3[v] / tot; s += a[0]; } } }
    for (int v = 0; v < V; v++) p[v] /= s;
}

struct NG { double ce = 0, top1 = 0, top5 = 0; vector<float> cer; vector<char> t1, t5, cat; };
// satir kumesi uzerinde n-gram degerlendirmesi: ctx/y = degerlendirilen satirlar; T = sayimlar; loo: satir kendi sayimini cikarir
static NG ng_eval(const Teach& T, const vector<int>& ctx, const vector<int>& y, const double a[4], bool loo) {
    int n = (int)y.size(); NG r; r.cer.resize(n); r.t1.resize(n); r.t5.resize(n); r.cat.resize(n); double sce = 0; long c1 = 0, c5 = 0;
#pragma omp parallel reduction(+ : sce, c1, c5)
    { vector<double> p(V), c2(V), c3(V);
#pragma omp for schedule(static)
        for (int i = 0; i < n; i++) { bool s2, s3; dense_row(T, ctx[2 * i], ctx[2 * i + 1], y[i], loo, a, p, c2, c3, s2, s3); double py = p[y[i]]; int rank = 0; for (int v = 0; v < V; v++) if (p[v] > py) rank++;
            r.cer[i] = (float)-log(py); r.t1[i] = rank == 0; r.t5[i] = rank < 5; r.cat[i] = s3 ? 2 : (s2 ? 1 : 0); sce += -log(py); c1 += rank == 0; c5 += rank < 5; } }
    r.ce = sce / n; r.top1 = (double)c1 / n; r.top5 = (double)c5 / n; return r;
}
static void em_weights(const Teach& T, const vector<int>& ctx, const vector<int>& y, int level, double a[4]) {
    int n = (int)y.size(); vector<array<double, 4>> q(n); vector<array<char, 2>> av(n);
    for (int i = 0; i < n; i++) { double qq[4]; bool aa[2]; comp_y(T, ctx[2 * i], ctx[2 * i + 1], y[i], false, qq, aa); for (int k = 0; k < 4; k++) q[i][k] = qq[k]; av[i] = {(char)aa[0], (char)aa[1]}; }
    double w[4] = {level >= 3 ? 1.0 : 0, level >= 2 ? 1.0 : 0, 1.0, 1.0}; double sw = w[0] + w[1] + w[2] + w[3]; for (int k = 0; k < 4; k++) w[k] /= sw;
    for (int it = 0; it < 200; it++) { double r[4] = {0, 0, 0, 0}; for (int i = 0; i < n; i++) { double cc[4], d = 0; for (int k = 0; k < 4; k++) { bool ok = k == 0 ? (av[i][1] && w[0] > 0) : (k == 1 ? (av[i][0] && w[1] > 0) : true); cc[k] = ok ? w[k] * q[i][k] : 0; d += cc[k]; } for (int k = 0; k < 4; k++) r[k] += cc[k] / d; }
        double tot = r[0] + r[1] + r[2] + r[3]; for (int k = 0; k < 4; k++) w[k] = r[k] / tot; }
    for (int k = 0; k < 4; k++) a[k] = w[k];
}
static void print_ng(const char* name, const char* split, const NG& r) { printf("NG|%s|%s|CE=%.4f|PPL=%.2f|top1=%.4f|top5=%.4f\n", name, split, r.ce, exp(r.ce), r.top1, r.top5); }


// ======== V26 okuma katmani: softmax (kaba V17 izgarasi + ince izgara) ve dogrusal okuma; AYNI Z uzerinde
static const int NAL = 20, NEPS = 8, NTF = 22;
static const double ALG[NAL] = {0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6, 0.7, 0.75, 0.8, 0.85, 0.9, 0.93, 0.95, 0.97, 0.98, 0.99, 1.0};
static const double EPG[NEPS] = {1e-7, 1e-6, 1e-5, 1e-4, 3e-4, 1e-3, 3e-3, 1e-2};
static const double TFG[NTF] = {0.002, 0.003, 0.004, 0.005, 0.0065, 0.008, 0.01, 0.0125, 0.015, 0.02, 0.025, 0.03, 0.04, 0.05, 0.07, 0.1, 0.15, 0.25, 0.5, 1, 2, 4};
struct RD { double ce = 0, ppl = 0, top1 = 0, top5 = 0, negfrac = 0, yclip = 0, maxdev = 0; };
static void lin_table(const vector<float>& Z, const int* y, int n, const vector<double>& P, double ce[NEPS][NAL]) {
    for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) ce[e][a] = 0;
#pragma omp parallel
    { double loc[NEPS][NAL]; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) loc[e][a] = 0;
#pragma omp for schedule(static)
        for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; for (int e = 0; e < NEPS; e++) { double S = 0; for (int v = 0; v < V; v++) S += max((double)z[v], EPG[e]); double zy = max((double)z[y[i]], EPG[e]) / S; for (int a = 0; a < NAL; a++) loc[e][a] += -log((1 - ALG[a]) * P[y[i]] + ALG[a] * zy); } }
#pragma omp critical
        { for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) ce[e][a] += loc[e][a]; } }
    for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) ce[e][a] /= n;
}
static void smf_table(const vector<float>& Z, const int* y, int n, const vector<double>& P, double ce[NTF][NAL]) {
    for (int t = 0; t < NTF; t++) for (int a = 0; a < NAL; a++) ce[t][a] = 0;
#pragma omp parallel
    { double loc[NTF][NAL]; for (int t = 0; t < NTF; t++) for (int a = 0; a < NAL; a++) loc[t][a] = 0;
#pragma omp for schedule(static)
        for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]); for (int t = 0; t < NTF; t++) { double S = 0; for (int v = 0; v < V; v++) S += exp((z[v] - mx) / TFG[t]); double q = exp((z[y[i]] - mx) / TFG[t]) / S; for (int a = 0; a < NAL; a++) loc[t][a] += -log((1 - ALG[a]) * P[y[i]] + ALG[a] * q); } }
#pragma omp critical
        { for (int t = 0; t < NTF; t++) for (int a = 0; a < NAL; a++) ce[t][a] += loc[t][a]; } }
    for (int t = 0; t < NTF; t++) for (int a = 0; a < NAL; a++) ce[t][a] /= n;
}
// kind 0: softmax(z/par); kind 1: dogrusal max(z,par)/sum
static RD rd_eval(const vector<float>& Z, const int* y, int n, const vector<double>& P, int kind, double al, double par) {
    RD r; double sce = 0, dev = 0, neg = 0, yc = 0; long c1 = 0, c5 = 0;
#pragma omp parallel reduction(+ : sce, neg, yc, c1, c5) reduction(max : dev)
    { vector<double> p(V);
#pragma omp for schedule(static)
        for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double S = 0, mx = -1e30; for (int v = 0; v < V; v++) { if (z[v] <= 0) neg += 1; mx = max(mx, (double)z[v]); }
            if (kind == 0) { for (int v = 0; v < V; v++) { p[v] = exp((z[v] - mx) / par); S += p[v]; } } else { for (int v = 0; v < V; v++) { p[v] = max((double)z[v], par); S += p[v]; } if (z[y[i]] <= par) yc += 1; }
            double sum = 0; for (int v = 0; v < V; v++) { p[v] = (1 - al) * P[v] + al * p[v] / S; sum += p[v]; } dev = max(dev, fabs(sum - 1.0)); double py = p[y[i]]; int rank = 0; for (int v = 0; v < V; v++) if (p[v] > py) rank++; sce += -log(py); c1 += rank == 0; c5 += rank < 5; } }
    r.ce = sce / n; r.ppl = exp(r.ce); r.top1 = (double)c1 / n; r.top5 = (double)c5 / n; r.negfrac = neg / ((double)n * V); r.yclip = yc / n; r.maxdev = dev; return r;
}


static void make_dir(const vector<float>& Fin, int n, int hin, const vector<int>& y, int H, int seed, vector<float>& W, vector<float>& b) {
    W.assign((size_t)hin * H, 0.f); b.assign(H, 0.f); mt19937_64 g(seed), gb(seed + 7777); normal_distribution<double> N(0, 1); uniform_int_distribution<int> U(0, n - 1); vector<double> pb(H);
    for (int j = 0; j < H; j++) { int a = U(g), c2; do c2 = U(g); while (c2 == a || y[c2] == y[a]); const float *fa = &Fin[(size_t)a * hin], *fb = &Fin[(size_t)c2 * hin]; double nn = 0; for (int k = 0; k < hin; k++) { double d = fa[k] - fb[k]; nn += d * d; }
        double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0; for (int k = 0; k < hin; k++) { double w = (fa[k] - fb[k]) * inv; W[(size_t)k * H + j] = (float)w; dot += (fa[k] + fb[k]) * w; } pb[j] = -dot / 2; }
    double mu = 0, sdv = 0; for (double v : pb) mu += v; mu /= H; for (double v : pb) sdv += (v - mu) * (v - mu); sdv = sqrt(sdv / H); for (int j = 0; j < H; j++) b[j] = (float)(N(gb) * sdv);
}

static Feat make_w1x(const Data& tr, int H, int seed, double sid) {      // make_w1 + kimlik (seyrek) bloklari sid ile olceklenir
    Feat f; f.H = H; f.W.assign((size_t)KT * H, 0.f); f.b.assign(H, 0.f); mt19937_64 g(seed), gb(seed + 7777); normal_distribution<double> N(0, 1);
    vector<double> pb(H); uniform_int_distribution<int> U(0, tr.n - 1);
    for (int j = 0; j < H; j++) { int a = U(g), b; do b = U(g); while (b == a || tr.y[b] == tr.y[a]);
        const float *xa = &tr.X[(size_t)a * K], *xb = &tr.X[(size_t)b * K]; double nn = 0; for (int k = 0; k < K; k++) { double d = xa[k] - xb[k]; nn += d * d; }
        map<int, double> sd; for (int q = 0; q < 3; q++) { int ra = tr.sp[(size_t)a * 3 + q], rb = tr.sp[(size_t)b * 3 + q]; if (ra >= 0) sd[ra] += 1; if (rb >= 0) sd[rb] -= 1; }
        for (auto& kv : sd) nn += kv.second * sid * kv.second * sid; double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0;
        for (int k = 0; k < K; k++) { double w = (xa[k] - xb[k]) * inv; f.W[(size_t)k * H + j] = (float)w; dot += (xa[k] + xb[k]) * w; }
        for (auto& kv : sd) f.W[(size_t)kv.first * H + j] = (float)(kv.second * sid * inv);
        for (int q = 0; q < 3; q++) { int ra = tr.sp[(size_t)a * 3 + q], rb = tr.sp[(size_t)b * 3 + q]; if (ra >= 0) dot += f.W[(size_t)ra * H + j] * 1.0; if (rb >= 0) dot += f.W[(size_t)rb * H + j]; }
        pb[j] = -dot / 2; }
    double mu = 0, sdv = 0; for (double v : pb) mu += v; mu /= H; for (double v : pb) sdv += (v - mu) * (v - mu); sdv = sqrt(sdv / H); for (int j = 0; j < H; j++) f.b[j] = (float)(N(gb) * sdv); return f;
}
static void make_dir2(const vector<float>& Fin, int n, int hin, const Data& tr, int H, int seed, double sid, vector<float>& W, vector<float>& Wsp, vector<float>& b) {   // yogun girdi + seyrek kimlik (Wsp: (KT-K) x H)
    W.assign((size_t)hin * H, 0.f); Wsp.assign((size_t)(KT - BASEA) * H, 0.f); b.assign(H, 0.f); mt19937_64 g(seed), gb(seed + 7777); normal_distribution<double> N(0, 1); uniform_int_distribution<int> U(0, n - 1); vector<double> pb(H);
    for (int j = 0; j < H; j++) { int a = U(g), c2; do c2 = U(g); while (c2 == a || tr.y[c2] == tr.y[a]); const float *fa = &Fin[(size_t)a * hin], *fb = &Fin[(size_t)c2 * hin]; double nn = 0; for (int k = 0; k < hin; k++) { double d = fa[k] - fb[k]; nn += d * d; }
        map<int, double> sd; for (int q = 0; q < 3; q++) { int ra = tr.sp[(size_t)a * 3 + q], rb = tr.sp[(size_t)c2 * 3 + q]; if (ra >= 0) sd[ra] += 1; if (rb >= 0) sd[rb] -= 1; } for (auto& kv : sd) nn += kv.second * sid * kv.second * sid;
        double inv = 1.0 / (sqrt(nn) + 1e-8), dot = 0; for (int k = 0; k < hin; k++) { double w = (fa[k] - fb[k]) * inv; W[(size_t)k * H + j] = (float)w; dot += (fa[k] + fb[k]) * w; }
        for (auto& kv : sd) Wsp[(size_t)(kv.first - BASEA) * H + j] = (float)(kv.second * sid * inv);
        for (int q = 0; q < 3; q++) { int ra = tr.sp[(size_t)a * 3 + q], rb = tr.sp[(size_t)c2 * 3 + q]; if (ra >= 0) dot += Wsp[(size_t)(ra - BASEA) * H + j]; if (rb >= 0) dot += Wsp[(size_t)(rb - BASEA) * H + j]; } pb[j] = -dot / 2; }
    double mu = 0, sdv = 0; for (double v : pb) mu += v; mu /= H; for (double v : pb) sdv += (v - mu) * (v - mu); sdv = sqrt(sdv / H); for (int j = 0; j < H; j++) b[j] = (float)(N(gb) * sdv);
}
// ---------------- V35: hedef bilgisiyle ic agirliklar (tek hamle, gradyan yok) ----------------
extern "C" void scipy_dsyev_(const char*, const char*, const int*, double*, const int*, double*, double*, const int*, int*);
static int g_nsp35 = 0; static bool g_pca = false; static bool g_v35 = false, g_yshuf = false; static double g_lams = 1000;
static void mgs_cols(vector<double>& A, int D, int l) {   // A: D x l satir-ana; sutunlari ortonormallestir
    for (int j = 0; j < l; j++) { for (int p = 0; p < 2; p++) for (int q = 0; q < j; q++) { double d = 0; for (int i = 0; i < D; i++) d += A[(size_t)i * l + j] * A[(size_t)i * l + q]; for (int i = 0; i < D; i++) A[(size_t)i * l + j] -= d * A[(size_t)i * l + q]; }
        double nn = 0; for (int i = 0; i < D; i++) nn += A[(size_t)i * l + j] * A[(size_t)i * l + j]; nn = sqrt(nn) + 1e-300; for (int i = 0; i < D; i++) A[(size_t)i * l + j] /= nn; } }
static void spd_solve(vector<double> G, int m, vector<double>& R, int nr) {   // G (m x m, SPD) , R (m x nr) yerinde
    for (int j = 0; j < m; j++) { for (int k = 0; k < j; k++) { double s = G[(size_t)j * m + k]; for (int i = 0; i < k; i++) s -= G[(size_t)j * m + i] * G[(size_t)k * m + i]; G[(size_t)j * m + k] = s / G[(size_t)k * m + k]; }
        double s = G[(size_t)j * m + j]; for (int i = 0; i < j; i++) s -= G[(size_t)j * m + i] * G[(size_t)j * m + i]; G[(size_t)j * m + j] = sqrt(s); }
    for (int c = 0; c < nr; c++) { for (int j = 0; j < m; j++) { double s = R[(size_t)j * nr + c]; for (int i = 0; i < j; i++) s -= G[(size_t)j * m + i] * R[(size_t)i * nr + c]; R[(size_t)j * nr + c] = s / G[(size_t)j * m + j]; }
        for (int j = m - 1; j >= 0; j--) { double s = R[(size_t)j * nr + c]; for (int i = j + 1; i < m; i++) s -= G[(size_t)i * m + j] * R[(size_t)i * nr + c]; R[(size_t)j * nr + c] = s / G[(size_t)j * m + j]; } } }
static void resid_chunk(const vector<int>& yl, const vector<float>& Zc, int s0, int nn, vector<float>& R) { R.assign((size_t)nn * V, 0.f); for (int i = 0; i < nn; i++) { float* r = &R[(size_t)i * V]; if (!Zc.empty()) { const float* z = &Zc[(size_t)(s0 + i) * V]; for (int v = 0; v < V; v++) r[v] = -z[v]; } r[yl[s0 + i]] += 1.f; } }
// asama icin cozum: F (n x m) uzerinde ridge W = (F'F + lam I)^-1 F'R ; Zc += F W
static void v35_stagefit(const vector<float>& F, int m, const vector<int>& yl, vector<float>& Zc, int n, double lam) {
    vector<double> G((size_t)m * m, 0.0), RH((size_t)m * V, 0.0); const int CH = 4096; vector<float> R, Gc((size_t)m * m), Rc((size_t)m * V);
    for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); resid_chunk(yl, Zc, s0, nn, R); scipy_cblas_sgemm(101, 112, 111, m, m, nn, 1.f, &F[(size_t)s0 * m], m, &F[(size_t)s0 * m], m, 0.f, Gc.data(), m); for (size_t q = 0; q < G.size(); q++) G[q] += Gc[q];
        scipy_cblas_sgemm(101, 112, 111, m, V, nn, 1.f, &F[(size_t)s0 * m], m, R.data(), V, 0.f, Rc.data(), V); for (size_t q = 0; q < RH.size(); q++) RH[q] += Rc[q]; }
    for (int j = 0; j < m; j++) G[(size_t)j * m + j] += lam; spd_solve(G, m, RH, V); vector<float> Wf(RH.begin(), RH.end());
    for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); vector<float> P((size_t)nn * V); gemm_f(&F[(size_t)s0 * m], nn, m, Wf.data(), V, P.data(), V); for (size_t q = 0; q < P.size(); q++) Zc[(size_t)s0 * V + q] += P[q]; } }
struct V35Diag { int hin, k, l, rankM; double s1, sk, ratio_eig; };
// I: n x hin (ld=hin). seyrek kimlik girdisi degeri sid. Cikis: W (hin x H), Wsp (nsp x H), b (H). Yalniz egitim verisi ve etiket yl kullanilir.
static V35Diag v35_dirs(const float* I, int hin, const Data& tr, double sid, const vector<float>& Zc, const vector<int>& yl, int H, int seed, vector<float>& W, vector<float>& Wsp, vector<float>& b) {
    int n = tr.n, nsp = g_nsp35, D = hin + nsp; vector<double> mu(D, 0.0); vector<double> cs(V, 0.0); vector<double> M((size_t)D * V, 0.0);
    for (int i = 0; i < n; i++) { const float* r = I + (size_t)i * hin; for (int k = 0; k < hin; k++) mu[k] += r[k]; for (int q = 0; q < 3; q++) { int rw = tr.sp[(size_t)i * 3 + q]; if (rw >= 0) mu[hin + rw - BASEA] += sid; } } for (int k = 0; k < D; k++) mu[k] /= n;
    const int CH = 4096; vector<float> R, Gc((size_t)hin * V);
    for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); resid_chunk(yl, Zc, s0, nn, R); for (int i = 0; i < nn; i++) { const float* r = &R[(size_t)i * V]; for (int v = 0; v < V; v++) cs[v] += r[v]; }
        scipy_cblas_sgemm(101, 112, 111, hin, V, nn, 1.f, I + (size_t)s0 * hin, hin, R.data(), V, 0.f, Gc.data(), V); for (size_t q = 0; q < (size_t)hin * V; q++) M[q] += Gc[q];
        for (int i = 0; i < nn; i++) for (int q = 0; q < 3; q++) { int rw = tr.sp[(size_t)(s0 + i) * 3 + q]; if (rw >= 0) { double* mr = &M[(size_t)(hin + rw - BASEA) * V]; const float* r = &R[(size_t)i * V]; for (int v = 0; v < V; v++) mr[v] += sid * r[v]; } } }
    for (int k = 0; k < D; k++) for (int v = 0; v < V; v++) M[(size_t)k * V + v] -= mu[k] * cs[v];   // merkezli girdi: (I-mu)^T R
    V35Diag dg; int k = 0; vector<double> U; int l = min(H + 10, min(D, V)); dg.hin = hin; dg.k = min(H, l); dg.rankM = 0; dg.s1 = dg.sk = dg.ratio_eig = 0; dg.l = 0;
    if (g_pca) {   // V37 C: hedefsiz kontrol -- merkezli girdinin ilk k ana bileseni (Y/residual KULLANILMAZ)
        k = min(H, hin); dg.k = k; vector<double> Sg((size_t)hin * hin, 0.0); vector<float> Gq((size_t)hin * hin);
        for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); scipy_cblas_sgemm(101, 112, 111, hin, hin, nn, 1.f, I + (size_t)s0 * hin, hin, I + (size_t)s0 * hin, hin, 0.f, Gq.data(), hin); for (size_t q = 0; q < Sg.size(); q++) Sg[q] += Gq[q]; }
        for (int a = 0; a < hin; a++) for (int b2 = 0; b2 < hin; b2++) Sg[(size_t)a * hin + b2] = Sg[(size_t)a * hin + b2] / n - mu[a] * mu[b2];
        vector<double> ev2(hin), wk2(64 * hin + 100); int info2 = 0, lw2 = (int)wk2.size(); char jz2 = 'V', up2 = 'U'; scipy_dsyev_(&jz2, &up2, &hin, Sg.data(), &hin, ev2.data(), wk2.data(), &lw2, &info2);
        dg.s1 = sqrt(max(ev2[hin - 1], 0.0)); dg.sk = sqrt(max(ev2[hin - k], 0.0)); dg.rankM = hin; U.assign((size_t)D * k, 0.0); for (int j = 0; j < k; j++) { int col = hin - 1 - j; for (int i = 0; i < hin; i++) U[(size_t)i * k + j] = Sg[(size_t)col * hin + i]; }
    } else { dg.l = l;
    mt19937_64 g(seed + 31337); normal_distribution<double> N(0, 1); vector<double> Om((size_t)V * l); for (auto& x : Om) x = N(g);
    vector<double> Y1((size_t)D * l), Z1((size_t)V * l);
    scipy_cblas_dgemm(101, 111, 111, D, l, V, 1.0, M.data(), V, Om.data(), l, 0.0, Y1.data(), l); mgs_cols(Y1, D, l);
    for (int it = 0; it < 3; it++) { scipy_cblas_dgemm(101, 112, 111, V, l, D, 1.0, M.data(), V, Y1.data(), l, 0.0, Z1.data(), l); mgs_cols(Z1, V, l); scipy_cblas_dgemm(101, 111, 111, D, l, V, 1.0, M.data(), V, Z1.data(), l, 0.0, Y1.data(), l); mgs_cols(Y1, D, l); }
    vector<double> B((size_t)l * V), S((size_t)l * l); scipy_cblas_dgemm(101, 112, 111, l, V, D, 1.0, Y1.data(), l, M.data(), V, 0.0, B.data(), V); scipy_cblas_dgemm(101, 111, 112, l, l, V, 1.0, B.data(), V, B.data(), V, 0.0, S.data(), l);
    vector<double> ev(l), wk(64 * l + 100); int info = 0, lw = (int)wk.size(); char jz = 'V', up = 'U'; scipy_dsyev_(&jz, &up, &l, S.data(), &l, ev.data(), wk.data(), &lw, &info);   // artan sirali
    dg.s1 = sqrt(max(ev[l - 1], 0.0)); dg.sk = sqrt(max(ev[l - dg.k], 0.0)); int rk = 0; for (int j = 0; j < l; j++) if (ev[j] > 1e-12 * ev[l - 1]) rk++; dg.rankM = rk; dg.ratio_eig = 0;
    { double tot = 0, top = 0; for (int j = 0; j < l; j++) tot += max(ev[j], 0.0); for (int j = l - dg.k; j < l; j++) top += max(ev[j], 0.0); dg.ratio_eig = top / tot; }
    k = dg.k; U.assign((size_t)D * k, 0.0);   // U = Q * E_top  (D x k)
    for (int j = 0; j < k; j++) { int col = l - 1 - j; for (int i = 0; i < D; i++) { double s = 0; for (int p = 0; p < l; p++) s += Y1[(size_t)i * l + p] * S[(size_t)col * l + p]; U[(size_t)i * k + j] = s; } }
    }
    vector<float> Ud((size_t)hin * k); for (int i = 0; i < hin; i++) for (int j = 0; j < k; j++) Ud[(size_t)i * k + j] = (float)U[(size_t)i * k + j];
    vector<double> pm(k, 0.0), pq(k, 0.0); vector<float> P;
    for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); P.assign((size_t)nn * k, 0.f); gemm_f(I + (size_t)s0 * hin, nn, hin, Ud.data(), k, P.data(), k);
        for (int i = 0; i < nn; i++) { float* p = &P[(size_t)i * k]; for (int q = 0; q < 3; q++) { int rw = tr.sp[(size_t)(s0 + i) * 3 + q]; if (rw >= 0) for (int j = 0; j < k; j++) p[j] += (float)(sid * U[(size_t)(hin + rw - BASEA) * k + j]); } for (int j = 0; j < k; j++) { pm[j] += p[j]; pq[j] += (double)p[j] * p[j]; } } }
    W.assign((size_t)hin * H, 0.f); Wsp.assign((size_t)nsp * H, 0.f); b.assign(H, 0.f);
    for (int j = 0; j < k; j++) { double m = pm[j] / n, sd = sqrt(max(pq[j] / n - m * m, 1e-12)); for (int i = 0; i < hin; i++) W[(size_t)i * H + j] = (float)(U[(size_t)i * k + j] / sd); for (int r = 0; r < nsp; r++) Wsp[(size_t)r * H + j] = (float)(sid * U[(size_t)(hin + r) * k + j] / sd); b[j] = (float)(-m / sd); }
    return dg; }
// ---------------- V38: bilgi adresleme = beyazlatilmis girdi + tek tek ozellik yerlestirme + artik sinyalden cikarma (deflation) ----------------
extern "C" { void scipy_cblas_sgemv(int, int, int, int, float, const float*, int, const float*, int, float, float*, int); void scipy_cblas_sger(int, int, int, float, const float*, int, const float*, int, float*, int); }
static bool g_v38 = false; static double g_eps38 = 0.05; static int g_piter = 30;
// Her ozellik j icin: kalan artik R_j ile beyaz girdi Z arasindaki capraz-kovaryans M_j'nin en buyuk tekil cifti (u_j,v_j) = "hangi giris iliskisi -> hangi cikis iliskisi";
// ozellik f_j = tanh(g * Z u_j / s_j); sonra artik, f_j'nin KENDI (dogrusal-olmayan) katkisi cikarilarak guncellenir -> ayni bilgi iki ozellige yerlesmez.
static V35Diag v38_dirs(const float* I, int hin, const Data& tr, const vector<float>& Zc, const vector<int>& yl, int H, double gam, vector<float>& W, vector<float>& b, double& usage_top, double& maxcos, double& meancos) {
    int n = tr.n; const int CH = 4096; vector<double> mu(hin, 0.0); for (int i = 0; i < n; i++) { const float* r = I + (size_t)i * hin; for (int k = 0; k < hin; k++) mu[k] += r[k]; } for (auto& x : mu) x /= n;
    vector<double> Sg((size_t)hin * hin, 0.0); { vector<float> Gq((size_t)hin * hin); for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); scipy_cblas_sgemm(101, 112, 111, hin, hin, nn, 1.f, I + (size_t)s0 * hin, hin, I + (size_t)s0 * hin, hin, 0.f, Gq.data(), hin); for (size_t q = 0; q < Sg.size(); q++) Sg[q] += Gq[q]; } }
    for (int a = 0; a < hin; a++) for (int c2 = 0; c2 < hin; c2++) Sg[(size_t)a * hin + c2] = Sg[(size_t)a * hin + c2] / n - mu[a] * mu[c2];
    vector<double> ev(hin), wk(64 * hin + 100); int info = 0, lw = (int)wk.size(); char jz = 'V', up = 'U'; double tr_ = 0; for (int a = 0; a < hin; a++) tr_ += Sg[(size_t)a * hin + a]; scipy_dsyev_(&jz, &up, &hin, Sg.data(), &hin, ev.data(), wk.data(), &lw, &info);
    vector<double> Wh((size_t)hin * hin, 0.0); { double ridge = g_eps38 * tr_ / hin; for (int k = 0; k < hin; k++) { double sc = 1.0 / sqrt(max(ev[k], 0.0) + ridge); for (int a = 0; a < hin; a++) { double qa = Sg[(size_t)k * hin + a] * sc; for (int c2 = 0; c2 < hin; c2++) Wh[(size_t)a * hin + c2] += qa * Sg[(size_t)k * hin + c2]; } } }   // Wh = Q (L+ridge)^-1/2 Q'
    vector<float> Whf(Wh.begin(), Wh.end()), Z((size_t)n * hin);
    { vector<float> Ic; for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); Ic.assign((size_t)nn * hin, 0.f); for (int i = 0; i < nn; i++) for (int k = 0; k < hin; k++) Ic[(size_t)i * hin + k] = I[(size_t)(s0 + i) * hin + k] - (float)mu[k]; gemm_f(Ic.data(), nn, hin, Whf.data(), hin, &Z[(size_t)s0 * hin], hin); } }
    // M = Z'R (hin x V), R = onehot - Zc ; Z merkezli oldugundan ek duzeltme gerekmez
    vector<double> M((size_t)hin * V, 0.0); { vector<float> R, Gc((size_t)hin * V); for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); resid_chunk(yl, Zc, s0, nn, R); scipy_cblas_sgemm(101, 112, 111, hin, V, nn, 1.f, &Z[(size_t)s0 * hin], hin, R.data(), V, 0.f, Gc.data(), V); for (size_t q = 0; q < M.size(); q++) M[q] += Gc[q]; } }
    // M = U S Vr' (hin satir): MM' ozayristirmasi -> Vr = M' U S^-1  (V x hin) ; R'yi Vr uzerine izdusurmek tam esdeger (deflation M'nin satir uzayinda kalir)
    vector<double> MM((size_t)hin * hin); scipy_cblas_dgemm(101, 111, 112, hin, hin, V, 1.0, M.data(), V, M.data(), V, 0.0, MM.data(), hin);
    vector<double> ev2(hin); scipy_dsyev_(&jz, &up, &hin, MM.data(), &hin, ev2.data(), wk.data(), &lw, &info);
    int r = 0; for (int k = hin - 1; k >= 0; k--) if (ev2[k] > 1e-10 * ev2[hin - 1]) r++; vector<float> Vr((size_t)V * r);   // r = sayisal rank
    { vector<double> Ur((size_t)hin * r), Vd((size_t)V * r); for (int j = 0; j < r; j++) { int col = hin - 1 - j; for (int a = 0; a < hin; a++) Ur[(size_t)a * r + j] = MM[(size_t)col * hin + a]; } scipy_cblas_dgemm(101, 112, 111, V, r, hin, 1.0, M.data(), V, Ur.data(), r, 0.0, Vd.data(), r); for (int j = 0; j < r; j++) { double sg = sqrt(ev2[hin - 1 - j]); for (int v = 0; v < V; v++) Vr[(size_t)v * r + j] = (float)(Vd[(size_t)v * r + j] / sg); } }
    // Rr = R Vr (n x r) ve Mr = Z' Rr (hin x r)
    vector<float> Rr((size_t)n * r), Mr((size_t)hin * r, 0.f); { vector<float> R, P; for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); resid_chunk(yl, Zc, s0, nn, R); P.assign((size_t)nn * r, 0.f); gemm_f(R.data(), nn, V, Vr.data(), r, P.data(), r); memcpy(&Rr[(size_t)s0 * r], P.data(), (size_t)nn * r * sizeof(float)); } }
    scipy_cblas_sgemm(101, 112, 111, hin, r, n, 1.f, Z.data(), hin, Rr.data(), r, 0.f, Mr.data(), r);
    W.assign((size_t)hin * H, 0.f); b.assign(H, 0.f); vector<float> u(hin), tmp(hin), f(n), zu(n), zf(hin), a(r), U((size_t)hin * H, 0.f); vector<float> fall; usage_top = 0; V35Diag dg; dg.hin = hin; dg.k = H; dg.l = r; dg.rankM = r; dg.s1 = dg.sk = 0; dg.ratio_eig = 0;
    double e0 = 0; for (size_t q = 0; q < Rr.size(); q++) e0 += (double)Rr[q] * Rr[q];
    for (int j = 0; j < H; j++) {
        // en buyuk tekil cift: guc yinelemesi (Mr Mr') u
        for (int k = 0; k < hin; k++) u[k] = 1.f / sqrt((float)hin) + 0.01f * ((k * 7919 + j * 104729) % 13 - 6);   // sabit, hedefsiz baslangic
        double sv = 0; for (int it = 0; it < g_piter; it++) { vector<float> t2(r); scipy_cblas_sgemv(101, 112, hin, r, 1.f, Mr.data(), r, u.data(), 1, 0.f, t2.data(), 1); scipy_cblas_sgemv(101, 111, hin, r, 1.f, Mr.data(), r, t2.data(), 1, 0.f, tmp.data(), 1); for (int q = 0; q < j; q++) { double d = 0; for (int k = 0; k < hin; k++) d += (double)tmp[k] * U[(size_t)k * H + q]; for (int k = 0; k < hin; k++) tmp[k] -= (float)(d * U[(size_t)k * H + q]); }   /* onceki yerlestirilmis yonlere dik: ayni iliski iki ozellige gitmez */
 double nn2 = 0; for (float x : tmp) nn2 += (double)x * x; nn2 = sqrt(nn2) + 1e-30; for (int k = 0; k < hin; k++) u[k] = (float)(tmp[k] / nn2); sv = sqrt(nn2); }
        if (j == 0) dg.s1 = sv; dg.sk = sv; if (j == 0) usage_top = sv;
        scipy_cblas_sgemv(101, 111, n, hin, 1.f, Z.data(), hin, u.data(), 1, 0.f, zu.data(), 1);
        double m1 = 0, m2 = 0; for (int i = 0; i < n; i++) { m1 += zu[i]; m2 += (double)zu[i] * zu[i]; } m1 /= n; double sd = sqrt(max(m2 / n - m1 * m1, 1e-12));
        // ozellik: tanh(gam * (Z u - m1)/sd) ; ic agirlik: w = Wh u / sd ; bias = -(mu.w) - m1/sd
        double fm = 0; for (int i = 0; i < n; i++) { f[i] = tanhf((float)(gam * (zu[i] - m1) / sd)); fm += f[i]; } fm /= n; for (int i = 0; i < n; i++) f[i] -= (float)fm;
        double ff = 0; for (int i = 0; i < n; i++) ff += (double)f[i] * f[i];
        for (int a2 = 0; a2 < hin; a2++) { double w = 0; for (int c2 = 0; c2 < hin; c2++) w += Wh[(size_t)a2 * hin + c2] * u[c2]; W[(size_t)a2 * H + j] = (float)(w / sd); U[(size_t)a2 * H + j] = u[a2]; } { double dot = 0; for (int a2 = 0; a2 < hin; a2++) dot += mu[a2] * W[(size_t)a2 * H + j]; b[j] = (float)(-dot - m1 / sd); }
        // deflation: a = Rr' f / ff ; Rr -= f a' ; Mr -= (Z' f) a'
        scipy_cblas_sgemv(101, 112, n, r, 1.f, Rr.data(), r, f.data(), 1, 0.f, a.data(), 1); for (int k = 0; k < r; k++) a[k] = (float)(a[k] / (ff + 1e-12));
        scipy_cblas_sgemv(101, 112, n, hin, 1.f, Z.data(), hin, f.data(), 1, 0.f, zf.data(), 1);
        scipy_cblas_sger(101, n, r, -1.f, f.data(), 1, a.data(), 1, Rr.data(), r); scipy_cblas_sger(101, hin, r, -1.f, zf.data(), 1, a.data(), 1, Mr.data(), r);
    }
    double e1 = 0; for (size_t q = 0; q < Rr.size(); q++) e1 += (double)Rr[q] * Rr[q]; dg.ratio_eig = 1.0 - e1 / max(e0, 1e-30);   // aktarilan artik enerji payi (r-boyutlu izdusumde)
    // tekrar kontrolu: beyaz uzaydaki yonlerin |cos| (u_j . u_k) maks/ortalama
    double mc = 0, sm = 0; long cnt = 0; for (int j = 0; j < H; j++) for (int k = 0; k < j; k++) { double d = 0; for (int a2 = 0; a2 < hin; a2++) d += (double)U[(size_t)a2 * H + j] * U[(size_t)a2 * H + k]; d = fabs(d); mc = max(mc, d); sm += d; cnt++; } maxcos = mc; meancos = sm / max(1L, cnt);
    return dg; }
static void v35_rank(const char* nm, const vector<float>& F, int m, int n, int off) {   // F sutunlari (off'tan) -- merkezli Gram ozdegerleri
    int w = m - off; vector<double> mean(w, 0.0), G((size_t)w * w, 0.0); for (int i = 0; i < n; i++) for (int j = 0; j < w; j++) mean[j] += F[(size_t)i * m + off + j]; for (auto& x : mean) x /= n;
    vector<float> A((size_t)n * w); for (int i = 0; i < n; i++) for (int j = 0; j < w; j++) A[(size_t)i * w + j] = F[(size_t)i * m + off + j] - (float)mean[j]; vector<float> Gc((size_t)w * w); scipy_cblas_sgemm(101, 112, 111, w, w, n, 1.f, A.data(), w, A.data(), w, 0.f, Gc.data(), w); for (size_t q = 0; q < G.size(); q++) G[q] = Gc[q] / n;
    vector<double> ev(w), wk(64 * w + 100); int info = 0, lw = (int)wk.size(); char jz = 'N', up = 'U'; scipy_dsyev_(&jz, &up, &w, G.data(), &w, ev.data(), wk.data(), &lw, &info);
    double mx = ev[w - 1], s1 = 0, s2 = 0; int r6 = 0, r3 = 0; for (int j = 0; j < w; j++) { double e = max(ev[j], 0.0); s1 += e; s2 += e * e; if (e > 1e-6 * mx) r6++; if (e > 1e-3 * mx) r3++; }
    printf("RANK|%s|genislik=%d sayisal_rank(>1e-6*max)=%d rank(>1e-3*max)=%d efektif_rank(PR)=%.1f en_buyuk_ozdeger_payi=%.3f\n", nm, w, r6, r3, s1 * s1 / s2, mx / s1); }
// ---------------- V36: kelime + cift kimligi (C blogu) ve OOF residual ----------------
static bool g_bias = false; static bool g_pair = false, g_v36 = false, g_c0 = false; static map<long long, int> g_prank; static double g_loof = 20;
static inline int v36_pairid(int id2, int id1) { unsigned h = ((unsigned)id2 * 2654435761u) ^ ((unsigned)id1 * 40503u + 12345u); h = (h ^ (h >> 15)) * 2246822519u; return 2 * V + (int)((h ^ (h >> 13)) % HB); }
static void v36_buildrank(const Data& tr, int c) { if (!g_prank.empty()) return; map<long long, int> cnt; for (int i = 0; i < tr.n; i++) cnt[(long long)tr.ctx[(size_t)i * c + c - 2] * V + tr.ctx[(size_t)i * c + c - 1]]++; vector<pair<int, long long>> v; for (auto& kv : cnt) v.push_back({-kv.second, kv.first}); sort(v.begin(), v.end()); for (int i = 0; i < (int)v.size() && i < 2000; i++) g_prank[v[i].second] = i; }
static vector<int> v36_ids(const Data& d, int n, int c, int NI) {
    if (g_c0) { vector<int> o((size_t)n * 2); for (int i = 0; i < n; i++) { int id2 = d.ctx[(size_t)i * c + c - 2], id1 = d.ctx[(size_t)i * c + c - 1]; o[2 * (size_t)i] = id1; auto it = g_prank.find((long long)id2 * V + id1); o[2 * (size_t)i + 1] = it == g_prank.end() ? -1 : V + it->second; } return o; } vector<int> o((size_t)n * NI); for (int i = 0; i < n; i++) { int id2 = d.ctx[(size_t)i * c + c - 2], id1 = d.ctx[(size_t)i * c + c - 1]; o[(size_t)NI * i] = id2; o[(size_t)NI * i + 1] = V + id1; if (NI == 3) o[(size_t)NI * i + 2] = v36_pairid(id2, id1); } return o; }
// 2-kat OOF: her yarim, DIGER yarim uzerinde kimlik-ridge ile tahmin edilir (yalniz egitim etiketleri). Cikis: Zc (n x V) = OOF kimlik tahmini
static vector<float> v36_oof(const Data& tr, int c, int NI, double lam, double& tsec) {
    double t0 = now(); int n = tr.n, mc = g_c0 ? V + 2000 : 2 * V + (NI == 3 ? HB : 0); vector<int> ids = v36_ids(tr, n, c, NI); vector<float> Zc((size_t)n * V, 0.f);
    for (int f = 0; f < 2; f++) {
        vector<double> A((size_t)mc * mc, 0.0), RC((size_t)mc * V, 0.0);
        for (int i = 0; i < n; i++) if ((i & 1) != f) for (int p = 0; p < NI; p++) { int a = ids[(size_t)NI * i + p]; if (a < 0) continue; for (int q = 0; q < NI; q++) if (ids[(size_t)NI * i + q] >= 0) A[(size_t)a * mc + ids[(size_t)NI * i + q]] += 1; RC[(size_t)a * V + tr.y[i]] += 1; }
        for (int i = 0; i < mc; i++) A[(size_t)i * mc + i] += lam; cholesky(A, mc); vector<double> Wd; chol_solve_multi_d(A, mc, RC, V, Wd);
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) if ((i & 1) == f) { float* z = &Zc[(size_t)i * V]; for (int p = 0; p < NI; p++) { if (ids[(size_t)NI * i + p] < 0) continue; const double* w = &Wd[(size_t)ids[(size_t)NI * i + p] * V]; for (int v = 0; v < V; v++) z[v] += (float)w[v]; } } }
    tsec = now() - t0; return Zc; }
// ---------------- V39: baglantili ortak havuz (kelime dugumleri; baglanti = egitim bagliminda ortak sonraki-kelime profili) ----------------
static bool g_v39 = false, g_v39shuf = false; static double g_beta39 = 0.5; static int g_knn39 = 10;
struct Pool39 { vector<int> nb; vector<float> wt; };   // V x knn
static Pool39 v39_graph(const Data& tr, int c) {
    vector<float> Cn((size_t)V * V, 0.f); for (int i = 0; i < tr.n; i++) Cn[(size_t)tr.ctx[(size_t)i * c + c - 1] * V + tr.y[i]] += 1.f;
    vector<float> Nn(Cn.size()); vector<int> cnt(V, 0); for (int u = 0; u < V; u++) { double nn = 0; for (int v = 0; v < V; v++) { float x = sqrtf(Cn[(size_t)u * V + v]); Nn[(size_t)u * V + v] = x; nn += (double)x * x; cnt[u] += (int)Cn[(size_t)u * V + v]; } nn = sqrt(nn) + 1e-9; for (int v = 0; v < V; v++) Nn[(size_t)u * V + v] /= (float)nn; }
    vector<float> S((size_t)V * V); scipy_cblas_sgemm(101, 111, 112, V, V, V, 1.f, Nn.data(), V, Nn.data(), V, 0.f, S.data(), V);
    Pool39 P; P.nb.assign((size_t)V * g_knn39, -1); P.wt.assign((size_t)V * g_knn39, 0.f);
    for (int u = 0; u < V; u++) { vector<pair<float, int>> cand; for (int w = 0; w < V; w++) if (w != u && cnt[w] >= 5) cand.push_back({-S[(size_t)u * V + w], w}); int k = min((int)cand.size(), g_knn39); partial_sort(cand.begin(), cand.begin() + k, cand.end()); double sm = 0; for (int q = 0; q < k; q++) sm += -cand[q].first; for (int q = 0; q < k; q++) { P.nb[(size_t)u * g_knn39 + q] = cand[q].second; P.wt[(size_t)u * g_knn39 + q] = (float)(-cand[q].first / max(sm, 1e-9)); } }
    return P; }
static void v39_mix(const Pool39& P, vector<vector<float>>& Fs, const vector<int>& wd, const vector<int>& last, int n, const vector<vector<float>>& Mean, const vector<int>& has, double beta) {
    for (size_t s = 0; s < Fs.size(); s++) { int m = wd[s];
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) { int u = last[i]; vector<float> acc(m, 0.f); double ws = 0; for (int q = 0; q < g_knn39; q++) { int nb = P.nb[(size_t)u * g_knn39 + q]; if (nb < 0 || !has[nb]) continue; float w = P.wt[(size_t)u * g_knn39 + q]; ws += w; const float* mv = &Mean[s][(size_t)nb * m]; for (int j = 0; j < m; j++) acc[j] += w * mv[j]; }
            if (ws <= 0) continue; float* f = &Fs[s][(size_t)i * m]; for (int j = 0; j < m; j++) f[j] = (float)((1 - beta) * f[j] + beta * acc[j] / ws); } } }
static map<string, double> TT;
int main(int argc, char** argv) {
    if (argc < 7) { cerr << "kullanim: tek_hamle_v27 c H TOHUM gamma1 lambda gamma2 [mod=SCJ] [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), gam2 = atof(argv[6]); vector<double> lams; { string ls = argv[5]; size_t p0 = 0; while (p0 <= ls.size()) { size_t p1 = ls.find(',', p0); if (p1 == string::npos) p1 = ls.size(); lams.push_back(atof(ls.substr(p0, p1 - p0).c_str())); p0 = p1 + 1; } } double lam = lams[0]; double sidm = argc > 8 ? atof(argv[8]) : 1.0; string tgt = argc > 9 ? argv[9] : "YQ"; bool hasQ = tgt.find('Q') != string::npos; auto plist = [&](const char* a) { vector<double> v; string ls = a; size_t p0 = 0; while (p0 <= ls.size()) { size_t p1 = ls.find(',', p0); if (p1 == string::npos) p1 = ls.size(); if (p1 > p0) v.push_back(atof(ls.substr(p0, p1 - p0).c_str())); p0 = p1 + 1; } return v; }; vector<double> lamC_id = plist(argc > 11 ? argv[11] : "1,10,20,100"), lamC_cb = plist(argc > 12 ? argv[12] : "1,10,100"), lamF_l = plist(argc > 13 ? argv[13] : "1,20,100,1000,1e9"); bool v31mode = false; string mod = argc > 7 ? argv[7] : "SC"; string dir = argc > 10 && string(argv[10]) != "-" ? argv[10] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    g_bias = getenv("V37_BIAS") && atoi(getenv("V37_BIAS")) == 1; g_v38 = getenv("V38") && atoi(getenv("V38")) == 1; if (getenv("V38_EPS")) g_eps38 = atof(getenv("V38_EPS")); g_v39 = getenv("V39") && atoi(getenv("V39")) == 1; g_v39shuf = getenv("V39_SHUF") && atoi(getenv("V39_SHUF")) == 1; if (getenv("V39_BETA")) g_beta39 = atof(getenv("V39_BETA")); g_pca = getenv("V37_PCA") && atoi(getenv("V37_PCA")) == 1;
    g_pair = getenv("V36_PAIR") && atoi(getenv("V36_PAIR")) == 1; g_c0 = getenv("V36_C0") && atoi(getenv("V36_C0")) == 1; g_v36 = getenv("V36") && atoi(getenv("V36")) == 1; if (getenv("V36_LOOF")) g_loof = atof(getenv("V36_LOOF"));
    g_v35 = (getenv("V35_TGT") && atoi(getenv("V35_TGT")) == 1) || g_v36; g_yshuf = getenv("V35_YSHUF") && atoi(getenv("V35_YSHUF")) == 1; if (getenv("V35_LAMS")) g_lams = atof(getenv("V35_LAMS"));
    g_blas = getenv("V33_BLAS") && atoi(getenv("V33_BLAS")) == 1; g_fast = !(getenv("V34_FAST") && atoi(getenv("V34_FAST")) == 0);   /* varsayilan ACIK (dogrulandi: bit-ozdes agirlik); V34_FAST=0 -> V33 yolu */ if (getenv("V34_RHS_GROUPS")) g_rhsg = atoi(getenv("V34_RHS_GROUPS")); if (getenv("V34_CHOL_BLOCKS")) g_cholblk = atoi(getenv("V34_CHOL_BLOCKS")); printf("V34_MOD|FAST=%d RHS_GRUPLARI=%d CHOL_BLOKLARI=%d\n", (int)g_fast, g_rhsg, g_cholblk); if (g_blas && getenv("V33_BLAS_THREADS")) scipy_openblas_set_num_threads(atoi(getenv("V33_BLAS_THREADS"))); printf("V33_MOD|cekirdekler=%s|OpenMP=%d|OpenBLAS iplik=%d\n", g_blas ? "OPENBLAS (sgemm/dpotrf/dtrsm/dgemm)" : "ESKI elle yazilmis", omp_get_max_threads(), g_blas ? scipy_openblas_get_num_threads() : 0);
    double Tp0 = now(); Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); printf("ZAMAN|veri dosyalarini okuma=%.3fs\n", now() - Tp0); double Tp1 = now();
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); BASEA = K; BASEB = K + V; BASEP = K + 2 * V; KT = K + 2 * V + HB; }
    printf("ZAMAN|expand_s (X'e fark bloku ekleme)=%.3fs\n", now() - Tp1);
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V27 c=%d H=%d tohum=%d gamma1=%g lambda=%g gamma2(F2..F4)=%g mod=%s (%d cekirdek); yalniz egitim; dogrusal okuma (alpha,eps dogrulamayla)\n", c, H, seed, gam, lam, gam2, mod.c_str(), omp_get_max_threads());
    double Ts = now(); Teach TE; build_teach(TE, tr.ctx, tr.y); double a3[4]; em_weights(TE, va.ctx, va.y, 3, a3); printf("ZAMAN|n-gram ogretici sayimlari+EM (yalniz kategori/rapor icin; agirlik uretimi DEGIL)=%.3fs\n", now() - Ts);
    vector<double> pi(V); { double tot = 0; for (int v = 0; v < V; v++) tot += TE.c1[v] + 1.0; for (int v = 0; v < V; v++) pi[v] = (TE.c1[v] + 1.0) / tot; }
    vector<char> tcat(nte); for (int i = 0; i < nte; i++) { double q[4]; bool av[2]; comp_y(TE, te.ctx[2 * i], te.ctx[2 * i + 1], te.y[i], false, q, av); tcat[i] = av[1] ? 2 : (av[0] ? 1 : 0); }

    auto v31solve = [&](vector<vector<float>>& Ft, vector<vector<float>>& Fv, vector<vector<float>>& Fe, vector<int>& wd, vector<vector<vector<double>>>& C, vector<vector<double>>& RYb, const string& name) {
        double tz0 = now(); int ns = wd.size(), mf = 0; vector<int> off(ns); for (int s = 0; s < ns; s++) { off[s] = mf; mf += wd[s]; } const int NI = g_pair ? 3 : 2; const int mc = g_c0 ? V + 2000 : 2 * V + (g_pair ? HB : 0);
        auto cat = [&](vector<vector<float>>& Fs, int n) { vector<float> o((size_t)n * mf); for (int s = 0; s < ns; s++) {
#pragma omp parallel for schedule(static) if (g_fast)
            for (int i = 0; i < n; i++) memcpy(&o[(size_t)i * mf + off[s]], &Fs[s][(size_t)i * wd[s]], wd[s] * sizeof(float)); } return o; };
        vector<float> Fc = cat(Ft, ntr), Fcv = cat(Fv, nva), Fce = cat(Fe, nte);
        vector<double> AFF((size_t)mf * mf, 0.0), RF((size_t)mf * V, 0.0);
        for (int i = 0; i < ns; i++) for (int j = 0; j <= i; j++) for (int a = 0; a < wd[i]; a++) for (int k = 0; k < wd[j]; k++) { double cv = C[i][j][(size_t)a * wd[j] + k]; AFF[(size_t)(off[i] + a) * mf + off[j] + k] = cv; AFF[(size_t)(off[j] + k) * mf + off[i] + a] = cv; }
        for (int s = 0; s < ns; s++) for (size_t q = 0; q < RYb[s].size(); q++) RF[(size_t)off[s] * V + q] = RYb[s][q];
        auto idv = [&](const Data& d, int n) { return v36_ids(d, n, c, NI); };
        vector<int> itr = idv(tr, ntr), iva = idv(va, nva), ite = idv(te, nte);
        vector<double> ACC((size_t)mc * mc, 0.0), ACF((size_t)mc * mf, 0.0), RC((size_t)mc * V, 0.0);
        {   // V36: genel NI yuvali sayim (kimlik satirlarina sahiplik id % nt; cakismasiz paralel)
            int nt = omp_get_max_threads();
#pragma omp parallel num_threads(nt)
            { int t = omp_get_thread_num();
              for (int i = 0; i < ntr; i++) { int yy = tr.y[i]; const float* fr = &Fc[(size_t)i * mf];
                for (int p = 0; p < NI; p++) { int a = itr[(size_t)NI * i + p]; if (a < 0 || a % nt != t) continue; for (int q = 0; q < NI; q++) if (itr[(size_t)NI * i + q] >= 0) ACC[(size_t)a * mc + itr[(size_t)NI * i + q]] += 1; RC[(size_t)a * V + yy] += 1; double* ra = &ACF[(size_t)a * mf]; for (int j = 0; j < mf; j++) ra[j] += fr[j]; } } }
        }
        printf("V31_HAZIRLIK|F bloku=%d sutun (yogun), C blogu=%d seyrek sutun (yogun one-hot veri matrisi YOK; C^TC yogun %dx%d yalniz Gram, %.0f MB)|sure=%.2fs\n", mf, mc, mc, mc, (double)mc * mc * 8 / 1e6, now() - tz0); fflush(stdout);
        // logit uretimi: bloklar ayri verilebilir
        auto logits = [&](const vector<float>& Fm, const vector<int>& ids, int n, const vector<float>* WC, const vector<float>* WF) { vector<float> Z((size_t)n * V, 0.f); if (WF) gemm_f(Fm.data(), n, mf, WF->data(), V, Z.data(), V);
            if (WC) {
#pragma omp parallel for schedule(static)
                for (int i = 0; i < n; i++) { float* z = &Z[(size_t)i * V]; for (int p = 0; p < NI; p++) { if (ids[(size_t)NI * i + p] < 0) continue; const float* w1 = &(*WC)[(size_t)ids[(size_t)NI * i + p] * V]; for (int v = 0; v < V; v++) z[v] += w1[v]; } } } return Z; };
        auto pickLin = [&](const vector<float>& Zv, double& vce, double& al, double& ep) { static double cl[NEPS][NAL]; lin_table(Zv, va.y.data(), nva, Ptr, cl); vce = 1e30; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) if (cl[e][a] < vce) { vce = cl[e][a]; al = ALG[a]; ep = EPG[e]; } };
        // tam degerlendirme (egitim/dogr/test + baglam kategorileri)
        auto fullEval = [&](const char* tag, const vector<float>* WC, const vector<float>* WF, double lc, double lf) { double tq_ = now(); auto Zv = logits(Fcv, iva, nva, WC, WF); TT["P1 dogrulama logitleri"] += now() - tq_; tq_ = now(); auto Ze = logits(Fce, ite, nte, WC, WF); TT["P2 TEST logitleri (tahmin)"] += now() - tq_; tq_ = now(); auto Zr = logits(Fc, itr, ntr, WC, WF); TT["P3 egitim logitleri (yalniz rapor)"] += now() - tq_; double vce, al, ep; pickLin(Zv, vce, al, ep);
            tq_ = now(); RD r1 = rd_eval(Zr, tr.y.data(), ntr, Ptr, 1, al, ep); TT["P4 egitim PPL/Top-k (yalniz rapor)"] += now() - tq_; RD r2 = rd_eval(Zv, va.y.data(), nva, Ptr, 1, al, ep); tq_ = now(); RD r3 = rd_eval(Ze, te.y.data(), nte, Ptr, 1, al, ep); TT["P5 TEST PPL/Top-1/Top-5 hesabi (tahmin)"] += now() - tq_;
            double cs2[3] = {0, 0, 0}, ct2[3] = {0, 0, 0}; int cc2[3] = {0, 0, 0}; vector<double> pp(V);
            for (int i = 0; i < nte; i++) { const float* z = &Ze[(size_t)i * V]; double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], ep); for (int v = 0; v < V; v++) pp[v] = (1 - al) * Ptr[v] + al * max((double)z[v], ep) / S2; double py = pp[te.y[i]]; int rk = 0; for (int v = 0; v < V; v++) if (pp[v] > py) rk++; int k = tcat[i]; cc2[k]++; cs2[k] += -log(py); ct2[k] += (rk == 0); }
            printf("SONUC|%s|lamC=%g lamF=%g|alpha=%g eps=%g|egitim PPL=%.1f|dogr CE=%.4f PPL=%.1f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|egitim-dogr CE farki=%.4f|baglam GORULDU: PPL=%.1f top1=%.4f (n=%d)|yalniz bigram: PPL=%.1f top1=%.4f (n=%d)|olasilik_toplam_sapma=%.1e\n", tag, lc, lf, al, ep, r1.ppl, r2.ce, r2.ppl, r3.ce, r3.ppl, r3.top1, r3.top5, r2.ce - r1.ce, exp(cs2[2] / cc2[2]), ct2[2] / cc2[2], cc2[2], exp(cs2[1] / max(1, cc2[1])), ct2[1] / max(1, cc2[1]), cc2[1], r3.maxdev); fflush(stdout); return Zr; };
        set<double> allc(lamC_id.begin(), lamC_id.end()); for (double x : lamC_cb) allc.insert(x);
        double bestVal = 1e30, bestLC = 0, bestLF = 0; vector<float> bWC, bWF; double bestIdVal = 1e30, bestIdLC = 0; vector<float> bIdWC;
        for (double lc : allc) { double tl = now(); double t1_ = now(); vector<double> A = (g_blas && allc.size() == 1) ? std::move(ACC) : ACC; for (int i = 0; i < mc; i++) A[(size_t)i * mc + i] += lc; TT["M1 A_CC kopya+lambda_C"] += now() - t1_; t1_ = now(); cholesky(A, mc); TT["M2 Cholesky(A_CC+lC I) 4002x4002"] += now() - t1_; t1_ = now(); vector<float> Y0f; vector<double> Y0d; if (g_blas) { chol_solve_multi_d(A, mc, RC, V, Y0d); Y0f.resize(Y0d.size());
#pragma omp parallel for schedule(static)
            for (size_t q = 0; q < Y0f.size(); q++) Y0f[q] = (float)Y0d[q]; } else chol_solve_multi(A, mc, RC, V, Y0f); TT["M3 Y0=A_CC^-1 R_C cozum (2001 sag taraf) = yalniz-kimlik agirliklari"] += now() - t1_;
            // yalniz kimlik
            { double te_ = now(); auto Zv = logits(Fcv, iva, nva, &Y0f, nullptr); double vce, al, ep; pickLin(Zv, vce, al, ep); RD r2 = rd_eval(Zv, va.y.data(), nva, Ptr, 1, al, ep); printf("YALNIZ_KIMLIK_TARAMA|lamC=%g|dogr CE=%.4f PPL=%.1f (alpha=%g eps=%g)\n", lc, vce, r2.ppl, al, ep); if (vce < bestIdVal) { bestIdVal = vce; bestIdLC = lc; bIdWC = Y0f; } TT["E1 yalniz-kimlik dogrulama degerlendirmesi (logit+alpha/eps+PPL)"] += now() - te_; }
            if (find(lamC_cb.begin(), lamC_cb.end(), lc) == lamC_cb.end()) { printf("SURE|lamC=%g (yalniz kimlik)|%.2fs\n", lc, now() - tl); continue; }
            t1_ = now(); vector<float> Xf; vector<double> Xd; if (g_blas) { chol_solve_multi_d(A, mc, ACF, mf, Xd); Xf.resize(Xd.size());
#pragma omp parallel for schedule(static)
            for (size_t q = 0; q < Xf.size(); q++) Xf[q] = (float)Xd[q]; } else chol_solve_multi(A, mc, ACF, mf, Xf); TT["M4 X=A_CC^-1 A_CF cozum (513 sag taraf)"] += now() - t1_; t1_ = now();
            vector<double> T((size_t)mf * mf, 0.0), U((size_t)mf * V, 0.0);
if (g_blas) { scipy_cblas_dgemm(101, 112, 111, mf, mf, mc, 1.0, ACF.data(), mf, Xd.data(), mf, 0.0, T.data(), mf); scipy_cblas_dgemm(101, 112, 111, mf, V, mc, 1.0, ACF.data(), mf, Y0d.data(), V, 0.0, U.data(), V); } else
#pragma omp parallel for schedule(static)
            for (int a = 0; a < mf; a++) { double* ta = &T[(size_t)a * mf]; double* ua = &U[(size_t)a * V]; for (int k = 0; k < mc; k++) { double cf = ACF[(size_t)k * mf + a]; if (cf == 0) continue; const float* xr = &Xf[(size_t)k * mf]; for (int b = 0; b < mf; b++) ta[b] += cf * xr[b]; const float* yr = &Y0f[(size_t)k * V]; for (int v = 0; v < V; v++) ua[v] += cf * yr[v]; } }
            TT["M5 T=A_FC X, U=A_FC Y0 (Schur parcalari)"] += now() - t1_; printf("SURE|lamC=%g Cholesky(C blogu)+Schur hazirlik=%.2fs\n", lc, now() - tl); fflush(stdout);
            for (double lf : lamF_l) { double ts = now(); double t2_ = now(); vector<double> Sm((size_t)mf * mf), rhs((size_t)mf * V); for (size_t q = 0; q < Sm.size(); q++) Sm[q] = AFF[q] - T[q]; for (int i = 0; i < mf; i++) Sm[(size_t)i * mf + i] += (i == 0 && g_bias) ? lc : lf; for (size_t q = 0; q < rhs.size(); q++) rhs[q] = RF[q] - U[q];
                cholesky(Sm, mf); vector<float> WFf; chol_solve_multi(Sm, mf, rhs, V, WFf); vector<float> tmp((size_t)mc * V); gemm_f(Xf.data(), mc, mf, WFf.data(), V, tmp.data(), V); vector<float> WCf((size_t)mc * V);
#pragma omp parallel for schedule(static)
                for (size_t q = 0; q < WCf.size(); q++) WCf[q] = Y0f[q] - tmp[q];
                TT["M6 Schur sistemi (513) Cholesky+cozum + W_C=Y0-X W_F (lambda_F basina)"] += now() - t2_; t2_ = now(); auto Zv = logits(Fcv, iva, nva, &WCf, &WFf); double vce, al, ep; pickLin(Zv, vce, al, ep); RD r2 = rd_eval(Zv, va.y.data(), nva, Ptr, 1, al, ep);
                TT["E2 ortak cozum dogrulama degerlendirmesi"] += now() - t2_; printf("ORTAK_TARAMA|lamC=%g lamF=%g|dogr CE=%.4f PPL=%.1f (alpha=%g eps=%g)|sure=%.2fs\n", lc, lf, vce, r2.ppl, al, ep, now() - ts); fflush(stdout);
                if ((lf < 1e8 || g_bias) && vce < bestVal) { bestVal = vce; bestLC = lc; bestLF = lf; bWC = WCf; bWF = WFf; }
                if (lf >= 1e8) { printf("KONTROL_F_KAPALI|lamC=%g lamF=1e9: F katkisi kapali -> yalniz kimlik ile karsilastirma: ortak dogr CE=%.4f ; yalniz-kimlik dogr CE (ayni lamC) bkz YALNIZ_KIMLIK_TARAMA\n", lc, vce); } } }
        if (getenv("V33_DUMP")) { string p = getenv("V33_DUMP"); { ofstream f1(p + "_WC.bin", ios::binary); f1.write((const char*)bWC.data(), bWC.size() * sizeof(float)); } { ofstream f2(p + "_WF.bin", ios::binary); f2.write((const char*)bWF.data(), bWF.size() * sizeof(float)); } }
        printf("SECIM|yalniz kimlik: lamC=%g (dogr CE=%.4f)|ortak (kimlik+4F): lamC=%g lamF=%g (dogr CE=%.4f)\n", bestIdLC, bestIdVal, bestLC, bestLF, bestVal); fflush(stdout);
        double tf_ = now(); fullEval("YALNIZ_KIMLIK(secilen)", &bIdWC, nullptr, bestIdLC, 0);
        auto Zrf = fullEval("ORTAK_KIMLIK+4F(secilen)", &bWC, &bWF, bestLC, bestLF);
        // parcali katkilar: ayni W, yalniz C kismi / yalniz F kismi
        fullEval("  ayni W: yalniz C kismi (logit)", &bWC, nullptr, bestLC, bestLF); fullEval("  ayni W: yalniz F kismi (logit)", nullptr, &bWF, bestLC, bestLF);
        // cozum rezidusu (egitim uzayinda): Z^T(Z W - Y) + Lambda W  / ||Z^T Y||
        { vector<float>& D = Zrf; for (int i = 0; i < ntr; i++) D[(size_t)i * V + tr.y[i]] -= 1.f; vector<double> RCr((size_t)mc * V, 0.0); for (int i = 0; i < ntr; i++) { const float* d = &D[(size_t)i * V]; for (int p = 0; p < NI; p++) { if (itr[(size_t)NI * i + p] < 0) continue; double* r1 = &RCr[(size_t)itr[(size_t)NI * i + p] * V]; for (int v = 0; v < V; v++) r1[v] += d[v]; } }
          vector<double> RFr((size_t)mf * V, 0.0); { const int CHn = 4096; vector<float> At, Gc; for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); At.assign((size_t)mf * nn, 0.f); for (int i = 0; i < nn; i++) for (int j = 0; j < mf; j++) At[(size_t)j * nn + i] = Fc[(size_t)(s0 + i) * mf + j]; Gc.assign((size_t)mf * V, 0.f); gemm_f(At.data(), mf, nn, &D[(size_t)s0 * V], V, Gc.data(), V); for (size_t q = 0; q < RFr.size(); q++) RFr[q] += Gc[q]; } }
          double nr = 0, nrhs = 0; for (size_t q = 0; q < RCr.size(); q++) { double r = RCr[q] + bestLC * bWC[q]; nr += r * r; nrhs += RC[q] * RC[q]; } for (size_t q = 0; q < RFr.size(); q++) { double r = RFr[q] + bestLF * bWF[q]; nr += r * r; nrhs += RF[q] * RF[q]; }
          printf("COZUM_REZIDUSU|||Z^T(ZW-Y)+Lambda W||/||Z^T Y|| = %.3e (float gemm ile olculdu; blok eliminasyon Cholesky cift duyarlik, cozum float)\n", sqrt(nr / nrhs)); }
        TT["E3 final degerlendirme: egitim+dogr+test logit/PPL/kategori + parca okumalari (rapor icin)"] += now() - tf_; printf("V31_TOPLAM|sure=%.1fs\n", now() - tz0); for (auto& kv : TT) printf("TT|%s|%.3fs\n", kv.first.c_str(), kv.second); };
    auto run_chain = [&](const string& name, vector<int> hs, bool joint, bool useX, int idm) {
        auto setsp = [&](Data& d) { d.sp.assign((size_t)d.n * 3, -1); if (idm == 0) return; for (int i = 0; i < d.n; i++) { int id2 = d.ctx[(size_t)i * c + c - 2], id1 = d.ctx[(size_t)i * c + c - 1]; int* sp = &d.sp[(size_t)i * 3]; sp[0] = BASEA + id1; sp[1] = BASEB + id2; if (idm == 2) { unsigned h = ((unsigned)id2 * 2654435761u) ^ ((unsigned)id1 * 40503u + 12345u); h = (h ^ (h >> 15)) * 2246822519u; sp[2] = BASEP + (int)((h ^ (h >> 13)) % HB); } } };
        setsp(tr); setsp(va); setsp(te); int nid = idm == 0 ? 0 : (idm == 1 ? 2 : 3); double rmsX = 0; { double a = 0; for (int i = 0; i < ntr; i++) { const float* r = &tr.X[(size_t)i * K]; for (int k = 0; k < K; k++) a += (double)r[k] * r[k]; } rmsX = sqrt(a / ntr); } double sid1 = idm ? sidm * rmsX / sqrt((double)nid) : 0, sid2 = idm ? sidm / sqrt((double)nid) : 0;
        if (idm) printf("KIMLIK|%s|kimlik sayisi=%d X satir-norm RMS=%.4f ; F1 girdisinde kimlik olcegi=%.4f (kimlik blogu enerjisi=X enerjisi) ; F2..F4 girdisinde kimlik olcegi=%.4f (blok enerjisi=1, X ve F bloklari gibi) ; seyrek boyut=%d (yogun acilmadi)\n", name.c_str(), nid, rmsX, sid1, sid2, KT - BASEA);
        g_nsp35 = idm ? KT - BASEA : 0; if (g_c0) v36_buildrank(tr, c); double tc0 = now(); int ns = hs.size(); vector<float> Zc35; vector<int> ylab35 = tr.y; if (g_v35) { Zc35.assign((size_t)ntr * V, 0.f); if (g_v36) { double toof = 0; Zc35 = v36_oof(tr, c, g_pair ? 3 : 2, g_loof, toof); printf("V36_OOF|2-kat out-of-fold kimlik%s ridge (lam=%g) residual baslangici ; sure=%.2fs ; yalniz egitim etiketleri\n", g_pair ? "+cift" : "", g_loof, toof); } if (g_yshuf && g_v36) { vector<int> pm(ntr); iota(pm.begin(), pm.end(), 0); mt19937_64 gs(777); shuffle(pm.begin(), pm.end(), gs); vector<float> Zp((size_t)ntr * V); vector<int> yp(ntr); for (int i = 0; i < ntr; i++) { memcpy(&Zp[(size_t)i * V], &Zc35[(size_t)pm[i] * V], V * sizeof(float)); yp[i] = tr.y[pm[i]]; } Zc35.swap(Zp); ylab35 = yp; printf("V36_KONTROL|residual SATIRLARI karistirildi (R=onehot(y)-Zc ayni permutasyonla; X ile iliskisi kopuk)\n"); } else if (g_yshuf) { mt19937_64 gs(777); shuffle(ylab35.begin(), ylab35.end(), gs); printf("V35_KONTROL|etiketler karistirildi: ic yonler ve asama ridge'leri KARISIK etiketle hesaplanir; yalniz son ortak cozum gercek Y kullanir\n"); } } vector<vector<float>> Ft(ns), Fv(ns), Fe(ns); vector<int> wd(ns);
        for (int s = 0; s < ns; s++) {
            if (s == 0) { Feat f; if (g_v35) { Feat f35; f35.H = hs[0]; vector<float> Wd, Ws, bb; V35Diag dg; if (g_v38) { double ut, mxc, mnc; dg = v38_dirs(tr.X.data(), K, tr, Zc35, ylab35, hs[0], gam, Wd, bb, ut, mxc, mnc); Ws.assign((size_t)(KT - BASEA) * hs[0], 0.f); printf("V38_ADRES|F1|girdi=%d rank(M)=%d ilk_tekil=%.4g son_tekil=%.4g aktarilan_artik_enerji=%.3f yon_cos maks=%.3f ort=%.4f\n", K, dg.rankM, dg.s1, dg.sk, dg.ratio_eig, mxc, mnc); } else dg = v35_dirs(tr.X.data(), K, tr, sid1, Zc35, ylab35, hs[0], seed, Wd, Ws, bb); f35.W.assign((size_t)(idm ? KT : K) * hs[0], 0.f); for (int i = 0; i < K; i++) for (int j = 0; j < hs[0]; j++) f35.W[(size_t)i * hs[0] + j] = Wd[(size_t)i * hs[0] + j]; if (idm) for (int r = 0; r < KT - BASEA; r++) for (int j = 0; j < hs[0]; j++) f35.W[(size_t)(BASEA + r) * hs[0] + j] = Ws[(size_t)r * hs[0] + j]; f35.b = bb; f = f35; printf("V35_SVD|F1|girdi_boyutu=%d (yogun %d + seyrek %d) k=%d (sketch l=%d) M_sayisal_rank=%d (ust sinir min(D,V)=%d) s1=%.4g sk=%.4g ust-k_enerji_payi=%.3f ; rastgele doldurma YOK\n", K + KT - BASEA, K, KT - BASEA, dg.k, dg.l, dg.rankM, min(K + KT - BASEA, V), dg.s1, dg.sk, dg.ratio_eig); } else f = idm ? make_w1x(tr, hs[0], seed, sid1) : make_w1("cift", tr, hs[0], seed); features(tr, f, gam, Ft[0]); features(va, f, gam, Fv[0]); features(te, f, gam, Fe[0]); wd[0] = hs[0] + 1; if (g_v35 && ns > 1) v35_stagefit(Ft[0], wd[0], ylab35, Zc35, ntr, g_lams); }
            else { vector<const vector<float>*> BT, BV, BE; vector<int> bw, bo, bst; vector<string> bn;
                if (useX) { BT.push_back(&tr.X); BV.push_back(&va.X); BE.push_back(&te.X); bw.push_back(K); bo.push_back(0); bst.push_back(K); bn.push_back("X"); }
                for (int j = 0; j < s; j++) if (useX || j == s - 1) { BT.push_back(&Ft[j]); BV.push_back(&Fv[j]); BE.push_back(&Fe[j]); bw.push_back(wd[j] - (j == 0 ? 1 : 0)); bo.push_back(j == 0 ? 1 : 0); bst.push_back(wd[j]); bn.push_back("F" + to_string(j + 1)); }
                int hin = 0; for (int w : bw) hin += w; vector<double> sc(bw.size(), 1.0), rms(bw.size(), 0.0);
                for (size_t q = 0; q < bw.size(); q++) { double acc = 0; for (int i = 0; i < ntr; i++) { double nn = 0; const float* r = &(*BT[q])[(size_t)i * bst[q] + bo[q]]; for (int k = 0; k < bw[q]; k++) nn += (double)r[k] * r[k]; acc += nn; } rms[q] = sqrt(acc / ntr); if (useX) sc[q] = 1.0 / rms[q]; }
                printf("OLCEK|%s|F%d girdisi|", name.c_str(), s + 1); for (size_t q = 0; q < bw.size(); q++) printf("%s(genislik %d): ham satir-norm RMS=%.4f uygulanan olcek=%.4f ; ", bn[q].c_str(), bw[q], rms[q], sc[q]); printf("toplam giris boyutu=%d\n", hin);
                auto build = [&](const vector<const vector<float>*>& B, int n) { vector<float> o2((size_t)n * hin); int off = 0; for (size_t q = 0; q < bw.size(); q++) {
#pragma omp parallel for schedule(static) if (g_fast)
                    for (int i = 0; i < n; i++) { const float* r = &(*B[q])[(size_t)i * bst[q] + bo[q]]; float* d = &o2[(size_t)i * hin + off]; for (int k = 0; k < bw[q]; k++) d[k] = (float)(r[k] * sc[q]); } off += bw[q]; } return o2; };
                vector<float> It = build(BT, ntr), Iv = build(BV, nva), Ie = build(BE, nte); vector<float> W, b, Wsp; if (g_v35) { V35Diag dg; if (g_v38) { double ut, mxc, mnc; dg = v38_dirs(It.data(), hin, tr, Zc35, ylab35, hs[s], gam2, W, b, ut, mxc, mnc); Wsp.assign((size_t)(KT - BASEA) * hs[s], 0.f); printf("V38_ADRES|F%d|girdi=%d rank(M)=%d ilk_tekil=%.4g son_tekil=%.4g aktarilan_artik_enerji=%.3f yon_cos maks=%.3f ort=%.4f\n", s + 1, hin, dg.rankM, dg.s1, dg.sk, dg.ratio_eig, mxc, mnc); } else dg = v35_dirs(It.data(), hin, tr, sid2, Zc35, ylab35, hs[s], seed + 100 * s, W, Wsp, b); printf("V35_SVD|F%d|girdi_boyutu=%d (yogun %d + seyrek %d) k=%d (sketch l=%d) M_sayisal_rank=%d (ust sinir min(D,V)=%d) s1=%.4g sk=%.4g ust-k_enerji_payi=%.3f ; rastgele doldurma YOK\n", s + 1, hin + KT - BASEA, hin, KT - BASEA, dg.k, dg.l, dg.rankM, min(hin + KT - BASEA, V), dg.s1, dg.sk, dg.ratio_eig); } else if (idm) make_dir2(It, ntr, hin, tr, hs[s], seed + 100 * s, sid2, W, Wsp, b); else make_dir(It, ntr, hin, tr.y, hs[s], seed + 100 * s, W, b);
                auto ap = [&](const vector<float>& I, int n, vector<float>& O, const Data& dd) { O.resize((size_t)n * hs[s]); gemm_f(I.data(), n, hin, W.data(), hs[s], O.data(), hs[s]);
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < n; i++) { float* o = &O[(size_t)i * hs[s]]; if (idm) for (int q = 0; q < 3; q++) { int row = dd.sp[(size_t)i * 3 + q]; if (row >= 0) { const float* w = &Wsp[(size_t)(row - BASEA) * hs[s]]; for (int j = 0; j < hs[s]; j++) o[j] += w[j]; } } for (int j = 0; j < hs[s]; j++) o[j] = tanhf((float)gam2 * (o[j] + b[j])); } };
                ap(It, ntr, Ft[s], tr); ap(Iv, nva, Fv[s], va); ap(Ie, nte, Fe[s], te); wd[s] = hs[s]; }
            if (g_v35 && s + 1 < ns) { v35_stagefit(Ft[s], wd[s], ylab35, Zc35, ntr, g_lams); }
        }
        if (g_v39) { double tv0 = now(); Pool39 P = v39_graph(tr, c);
            if (g_v39shuf) { mt19937_64 gs(4242); vector<int> pm(V); iota(pm.begin(), pm.end(), 0); shuffle(pm.begin(), pm.end(), gs); for (auto& x : P.nb) if (x >= 0) x = pm[x]; }   // baglanti hedefleri karistirilir (agirliklar ayni)
            vector<int> lt(ntr), lv(nva), le(nte); for (int i = 0; i < ntr; i++) lt[i] = tr.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nva; i++) lv[i] = va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) le[i] = te.ctx[(size_t)i * c + c - 1];
            vector<int> has(V, 0); for (int i = 0; i < ntr; i++) has[lt[i]]++; vector<vector<float>> Mean(ns); for (int s = 0; s < ns; s++) { int m = wd[s]; Mean[s].assign((size_t)V * m, 0.f); for (int i = 0; i < ntr; i++) { float* mv = &Mean[s][(size_t)lt[i] * m]; const float* f = &Ft[s][(size_t)i * m]; for (int j = 0; j < m; j++) mv[j] += f[j]; } for (int u = 0; u < V; u++) if (has[u]) for (int j = 0; j < m; j++) Mean[s][(size_t)u * m + j] /= has[u]; }
            double dsum = 0; { int cntd = 0; for (int i = 0; i < min(ntr, 2000); i++) { for (int s = 0; s < ns; s++) { dsum += 0; } cntd++; } }
            v39_mix(P, Ft, wd, lt, ntr, Mean, has, g_beta39); v39_mix(P, Fv, wd, lv, nva, Mean, has, g_beta39); v39_mix(P, Fe, wd, le, nte, Mean, has, g_beta39);
            { int nn_ = 0; double sw = 0; for (int u = 0; u < V; u++) if (P.nb[(size_t)u * g_knn39] >= 0) { nn_++; } printf("V39_HAVUZ|dugum=%d (bagli=%d) knn=%d beta=%g %s|baglanti hesabi+karisim sure=%.2fs ; baglantilar yalniz egitim etiketlerinden ; F kapasitesi ayni (%d)\n", V, nn_, g_knn39, g_beta39, g_v39shuf ? "KARISIK_BAGLANTI" : "GERCEK_BAGLANTI", now() - tv0, (int)(wd[0] + wd[1] + wd[2] + wd[3])); (void)sw; } }
        if (getenv("V35_RANK")) for (int s = 0; s < ns; s++) { string nm = "F" + to_string(s + 1); v35_rank(nm.c_str(), Ft[s], wd[s], ntr, s == 0 ? 1 : 0); }
        // Gram bloklari (cift duyarlik, tam simetrik) ve cift hedef sagtaraflari
        auto cross = [&](const vector<float>& A, int wa, const vector<float>& B, int wb, vector<double>& out) { out.assign((size_t)wa * wb, 0.0); const int CHn = 4096; vector<float> At, Gc; for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Gc.assign((size_t)wa * wb, 0.f); if (g_fast && g_blas) scipy_cblas_sgemm(101, 112, 111, wa, wb, nn, 1.f, &A[(size_t)s0 * wa], wa, &B[(size_t)s0 * wb], wb, 0.f, Gc.data(), wb); else { At.assign((size_t)wa * nn, 0.f); for (int i = 0; i < nn; i++) for (int j = 0; j < wa; j++) At[(size_t)j * nn + i] = A[(size_t)(s0 + i) * wa + j]; gemm_f(At.data(), wa, nn, &B[(size_t)s0 * wb], wb, Gc.data(), wb); } for (size_t q = 0; q < out.size(); q++) out[q] += Gc[q]; } };
        vector<vector<vector<double>>> C(ns, vector<vector<double>>(ns)); for (int i = 0; i < ns; i++) for (int j = 0; j <= i; j++) cross(Ft[i], wd[i], Ft[j], wd[j], C[i][j]);
        vector<vector<double>> RY(ns), RQ(ns); for (int s = 0; s < ns; s++) { RY[s].assign((size_t)wd[s] * V, 0.0); rhs_add(Ft[s].data(), tr.y.data(), ntr, wd[s], V, RY[s].data()); RQ[s].assign((size_t)wd[s] * V, 0.0); }
        if (hasQ) { const int CHn = 4096; vector<float> Qc, At, Gc; for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Qc.assign((size_t)nn * V, 0.f);
#pragma omp parallel
                { vector<double> q(V), t1(V), t2(V);
#pragma omp for schedule(static)
                    for (int i = 0; i < nn; i++) { bool s2, s3; dense_row(TE, tr.ctx[2 * (s0 + i)], tr.ctx[2 * (s0 + i) + 1], tr.y[s0 + i], true, a3, q, t1, t2, s2, s3); float* o2 = &Qc[(size_t)i * V]; for (int v = 0; v < V; v++) o2[v] = (float)q[v]; } }
                for (int s = 0; s < ns; s++) { At.assign((size_t)wd[s] * nn, 0.f); for (int i = 0; i < nn; i++) for (int j = 0; j < wd[s]; j++) At[(size_t)j * nn + i] = Ft[s][(size_t)(s0 + i) * wd[s] + j]; Gc.assign((size_t)wd[s] * V, 0.f); gemm_f(At.data(), wd[s], nn, Qc.data(), V, Gc.data(), V); for (size_t q = 0; q < RQ[s].size(); q++) RQ[s][q] += Gc[q]; } } }
        double t_prep = now() - tc0; if (v31mode) { printf("V31_ZINCIR_OZELLIK|%s|hazirlik=%.2fs\n", name.c_str(), t_prep); v31solve(Ft, Fv, Fe, wd, C, RY, name); printf("ZINCIR_BITTI|%s|toplam=%.1fs tepe_RAM=%.0fMB\n", name.c_str(), now() - tc0, peak_mb()); return; }
        printf("ZINCIR|%s|asamalar=%d genislik(her asama; ilk=+bias)=", name.c_str(), ns); for (int s = 0; s < ns; s++) printf("%d ", wd[s]); printf("|hazirlik (ozellik+gram+RHS)=%.2fs\n", t_prep); fflush(stdout);
        auto Cblk = [&](int i, int j) -> const vector<double>& { return C[i][j]; };
        for (int tg = 0; tg < (hasQ ? 2 : 1); tg++) { vector<vector<double>>& RT = tg == 0 ? RY : RQ; vector<vector<float>> W(ns); vector<vector<double>> Wd(ns);
            vector<float> Zr((size_t)ntr * V, 0.f), Zv((size_t)nva * V, 0.f), Ze((size_t)nte * V, 0.f); double prevE[2] = {0, 0};
            auto evalS = [&](int s, double tfit, const char* lab) { double ce[NEPS][NAL]; lin_table(Zv, va.y.data(), nva, Ptr, ce); double bl = 1e30; int be = 0, ba = 0; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) if (ce[e][a] < bl) { bl = ce[e][a]; be = e; ba = a; }
                RD r1 = rd_eval(Zr, tr.y.data(), ntr, Ptr, 1, ALG[ba], EPG[be]), r2 = rd_eval(Zv, va.y.data(), nva, Ptr, 1, ALG[ba], EPG[be]), r3 = rd_eval(Ze, te.y.data(), nte, Ptr, 1, ALG[ba], EPG[be]);
                // kalan regresyon hatasi: Y icin dogrudan; Q icin acik varyans (dogr/test)
                double res[3] = {0, 0, 0}; if (tg == 0) { auto rr = [&](const vector<float>& Z, int n, const vector<int>& y) { double sq = 0, sy = 0;
#pragma omp parallel for reduction(+ : sq, sy) schedule(static)
                        for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double a = 0; for (int v = 0; v < V; v++) a += (double)z[v] * z[v]; sq += a; sy += z[y[i]]; } return (sq - 2 * sy + n) / n; }; res[0] = rr(Zr, ntr, tr.y); res[1] = rr(Zv, nva, va.y); res[2] = rr(Ze, nte, te.y); }
                double ex[2] = {0, 0}; if (tg == 1) { auto dq = [&](const vector<float>& Z, int n, const vector<int>& ctx, const vector<int>& yv) { double num = 0, den = 0;
#pragma omp parallel reduction(+ : num, den)
                        { vector<double> p(V), c2(V), c3(V);
#pragma omp for schedule(static)
                            for (int i = 0; i < n; i++) { bool s2, s3; dense_row(TE, ctx[2 * i], ctx[2 * i + 1], yv[i], false, a3, p, c2, c3, s2, s3); const float* z = &Z[(size_t)i * V]; for (int v = 0; v < V; v++) { double d = z[v] - p[v], e2 = p[v] - pi[v]; num += d * d; den += e2 * e2; } } } return 1 - num / den; }; ex[0] = dq(Zv, nva, va.ctx, va.y); ex[1] = dq(Ze, nte, te.ctx, te.y); }
                { double cs2[3] = {0, 0, 0}, ct2[3] = {0, 0, 0}; int cc2[3] = {0, 0, 0}; double al_ = ALG[ba], ep_ = EPG[be]; vector<double> pp(V);
                  for (int i = 0; i < nte; i++) { const float* z = &Ze[(size_t)i * V]; double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], ep_); for (int v = 0; v < V; v++) pp[v] = (1 - al_) * Ptr[v] + al_ * max((double)z[v], ep_) / S2; double py = pp[te.y[i]]; int rk = 0; for (int v = 0; v < V; v++) if (pp[v] > py) rk++; int k = tcat[i]; cc2[k]++; cs2[k] += -log(py); ct2[k] += (rk == 0); }
                  for (int k = 2; k >= 1; k--) printf("KATEGORI|%s|%s|%s|trigram baglami %s|n=%d|test CE=%.4f PPL=%.1f top1=%.4f\n", name.c_str(), tg == 0 ? "Y" : "Q", lab, k == 2 ? "GORULDU" : "gorulmedi (yalniz bigram)", cc2[k], cs2[k] / cc2[k], exp(cs2[k] / cc2[k]), ct2[k] / cc2[k]); }
                printf("ASAMA|%s|hedef=%s|%s|eklenen=%d kumulatif=%d|kalan hata (Y: ||Y-Yhat||^2/n egit/dogr/test)=%.4f/%.4f/%.4f|Q acik varyans dogr/test=%.4f/%.4f (onceki asamaya gore ek: %+.4f/%+.4f)|dogrusal okuma alpha=%g eps=%g|egitim PPL=%.1f|dogr PPL=%.1f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|egitim-dogr CE=%.4f|asama sure=%.2fs\n", name.c_str(), tg == 0 ? "Y" : "Q", lab, s < 0 ? 0 : wd[s < 0 ? 0 : s], 0, res[0], res[1], res[2], ex[0], ex[1], ex[0] - prevE[0], ex[1] - prevE[1], ALG[ba], EPG[be], r1.ppl, r2.ppl, r3.ce, r3.ppl, r3.top1, r3.top5, r2.ce - r1.ce, tfit); prevE[0] = ex[0]; prevE[1] = ex[1]; fflush(stdout); };
            int cum = 0;
            if (!joint) { for (int s = 0; s < ns; s++) { double tf = now(); vector<double> rhs = RT[s]; for (int j = 0; j < s; j++) { const vector<double>& Cb = Cblk(s, j); for (int a = 0; a < wd[s]; a++) for (int k = 0; k < wd[j]; k++) { double cv = Cb[(size_t)a * wd[j] + k]; if (cv == 0) continue; const double* w = &Wd[j][(size_t)k * V]; double* r = &rhs[(size_t)a * V]; for (int v = 0; v < V; v++) r[v] -= cv * w[v]; } }
                    vector<double> A((size_t)wd[s] * wd[s]); for (int a = 0; a < wd[s]; a++) for (int b2 = 0; b2 < wd[s]; b2++) A[(size_t)a * wd[s] + b2] = C[s][s][(size_t)a * wd[s] + b2]; for (int a = 0; a < wd[s]; a++) A[(size_t)a * wd[s] + a] += lam; cholesky(A, wd[s]); chol_solve_multi(A, wd[s], rhs, V, W[s]); Wd[s].assign(W[s].begin(), W[s].end());
                    auto add = [&](const vector<float>& F, int n, vector<float>& Z) { vector<float> T2((size_t)n * V); gemm_f(F.data(), n, wd[s], W[s].data(), V, T2.data(), V);
#pragma omp parallel for schedule(static)
                        for (size_t q = 0; q < Z.size(); q++) Z[q] += T2[q]; };
                    add(Ft[s], ntr, Zr); add(Fv[s], nva, Zv); add(Fe[s], nte, Ze); cum += wd[s]; double tfit = now() - tf; char lab[32]; snprintf(lab, 32, "F1..F%d(zincir)", s + 1); evalS(s, tfit, lab); } }
            else { int mt = 0; vector<int> off(ns); for (int s = 0; s < ns; s++) { off[s] = mt; mt += wd[s]; } vector<double> A0((size_t)mt * mt, 0.0), rhs((size_t)mt * V, 0.0);
                for (int i = 0; i < ns; i++) for (int j = 0; j <= i; j++) for (int a = 0; a < wd[i]; a++) for (int k = 0; k < wd[j]; k++) { double cv = C[i][j][(size_t)a * wd[j] + k]; A0[(size_t)(off[i] + a) * mt + off[j] + k] = cv; A0[(size_t)(off[j] + k) * mt + off[i] + a] = cv; }
                for (int s = 0; s < ns; s++) for (size_t q = 0; q < RT[s].size(); q++) rhs[(size_t)off[s] * V + q] = RT[s][q];
                for (double lamj : lams) { double tf = now(); vector<double> A = A0; for (int i = 0; i < mt; i++) A[(size_t)i * mt + i] += lamj; cholesky(A, mt); vector<float> Wj; chol_solve_multi(A, mt, rhs, V, Wj); double tsol = now() - tf;
                    fill(Zr.begin(), Zr.end(), 0.f); fill(Zv.begin(), Zv.end(), 0.f); fill(Ze.begin(), Ze.end(), 0.f);
                    for (int s = 0; s < ns; s++) { auto add = [&](const vector<float>& F, int n, vector<float>& Z) { vector<float> T2((size_t)n * V); gemm_f(F.data(), n, wd[s], &Wj[(size_t)off[s] * V], V, T2.data(), V);
#pragma omp parallel for schedule(static)
                            for (size_t q = 0; q < Z.size(); q++) Z[q] += T2[q]; }; add(Ft[s], ntr, Zr); add(Fv[s], nva, Zv); add(Fe[s], nte, Ze); }
                    char lab[96]; snprintf(lab, 96, "F1..F4(ORTAK tek ridge) lam=%g sID=%g", lamj, sidm); printf("CHOL_COZUM|lam=%g|cholesky+cozum suresi=%.3fs\n", lamj, tsol); evalS(ns - 1, now() - tf, lab); } } }
        printf("ZINCIR_BITTI|%s|toplam=%.1fs tepe_RAM=%.0fMB\n", name.c_str(), now() - tc0, peak_mb()); fflush(stdout); };
    if (mod.find('Z') != string::npos) { v31mode = true; run_chain("V31_KIMLIK+4F(4x" + to_string(H / 4) + ")", {H / 4, H / 4, H / 4, H / 4}, true, true, 0); v31mode = false; }
    printf("TOPLAM sure=%.1fs tepe_RAM=%.0fMB\n", now() - Ts, peak_mb());
    return 0;
}
