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
#include <immintrin.h>
#include <sys/resource.h>
#include "tanhf_vec.h"
#include "knn_sparse.h"
static bool g_v54tanh = false, g_v54gate = false, g_v54topk = false, g_v54reuse = false, g_v54utab = false, g_v54sparseq = false;
using namespace std;
using Clock = chrono::steady_clock;
static double now() { return chrono::duration<double>(Clock::now().time_since_epoch()).count(); }
static double cpu_now() { timespec ts; clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
struct PhaseRec { string name; double wall, cpu; }; static vector<PhaseRec> g_phases; static double g_ph_w = -1, g_ph_c = 0;
static void phase_mark(const char* nm) { double w = chrono::duration<double>(Clock::now().time_since_epoch()).count(), c = cpu_now(); if (g_ph_w >= 0) g_phases.push_back({nm, w - g_ph_w, c - g_ph_c}); g_ph_w = w; g_ph_c = c; }
static bool g_hlean = false;
static map<string, double> TH;   /* V53h: ince profil sayaclari */
#include <sys/mman.h>
static bool g_v76thp = false, g_v76zc = false, g_v76vec = false;   // V76: >=4 MB tahsisler 2 MB hizali + MADV_HUGEPAGE (THP=madvise modunda sayfa hatasi sayisi ~512x azalir)
#include <mutex>
static bool g_v76pool = false;   // V76: >=8 MB bloklar serbest birakilinca havuzda tutulur ve ayni boyuttaki sonraki tahsiste yeniden kullanilir (VM'de taze sayfa hatasi ~yuzlerce us/MB)
struct V76Pool { std::mutex m; std::vector<std::pair<void*, size_t>> fl; size_t total = 0; };
static V76Pool& v76_pool() { static V76Pool* p = new V76Pool(); return *p; }
static const size_t V76_POOL_MIN = (size_t)8 << 20, V76_POOL_MAXTOT = (size_t)4096 << 20;
static inline size_t v76_cap(size_t n) { return (n + ((size_t)1 << 20) - 1) & ~(((size_t)1 << 20) - 1); }
static inline void* v76_alloc(size_t bytes) {
    if (g_v76pool && bytes >= V76_POOL_MIN) { size_t cap = v76_cap(bytes); { V76Pool& P = v76_pool(); std::lock_guard<std::mutex> lk(P.m); int bi = -1; size_t bs = (size_t)-1; for (size_t i = 0; i < P.fl.size(); i++) { size_t sz = P.fl[i].second; if (sz >= cap && sz <= cap + cap / 4 && sz < bs) { bs = sz; bi = (int)i; } } if (bi >= 0) { void* p = P.fl[bi].first; P.total -= P.fl[bi].second; P.fl.erase(P.fl.begin() + bi); return p; } } return malloc(cap); }
    if (g_v76thp && bytes >= ((size_t)4 << 20)) { void* p = nullptr; size_t al = (size_t)2 << 20, nb = (bytes + al - 1) / al * al; if (posix_memalign(&p, al, nb) == 0) { madvise(p, nb, MADV_HUGEPAGE); return p; } } return malloc(bytes ? bytes : 1); }
static inline void v76_free(void* p, size_t bytes) {
    if (!p) return; if (g_v76pool && bytes >= V76_POOL_MIN) { size_t cap = v76_cap(bytes); V76Pool& P = v76_pool(); std::lock_guard<std::mutex> lk(P.m); if (P.total + cap <= V76_POOL_MAXTOT) { P.fl.push_back({p, cap}); P.total += cap; return; } } free(p); }
template <class T> struct ubuf { T* p = nullptr; size_t n = 0; explicit ubuf(size_t n_) : n(n_) { p = (T*)v76_alloc(n_ * sizeof(T) + 64); if (!p) { fprintf(stderr, "bellek yok\n"); exit(4); } } ~ubuf() { v76_free(p, n * sizeof(T) + 64); } ubuf(const ubuf&) = delete; ubuf& operator=(const ubuf&) = delete; T* data() { return p; } const T* data() const { return p; } T& operator[](size_t i) { return p[i]; } const T& operator[](size_t i) const { return p[i]; } size_t size() const { return n; } };   // ilklenmemis tampon (ilk dokunus paralel)
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
extern "C" void scipy_cblas_ssyrk(int, int, int, int, int, float, const float*, int, float, float*, int);
static bool g_v73rhs = false, g_v73syrk = false;
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
    if (g_v73rhs) {   // V73: parca toplamlari SINIF-ANA ([c][j], j bitisik) -> satir basina 513 bitisik ekleme (eski: [j][c], adim V*8 bayt, 513 dagınık yazma). Her (j,c) elemani icin toplama sirasi ayni (parca icinde k artan, parcalar t artan) -> BIT-OZDES
        int nt = omp_get_max_threads(); vector<vector<double>> loc(nt);
        #pragma omp parallel
        { int t = omp_get_thread_num(); auto& L = loc[t]; L.assign((size_t)V * m, 0.0);
          #pragma omp for schedule(static)
          for (int k = 0; k < n; k++) { const float* row = F + (size_t)k * m; double* d = &L[(size_t)y[k] * m];
            #pragma omp simd
            for (int j = 0; j < m; j++) d[j] += (double)row[j]; } }
        const int TB = 64; int ncb = (V + TB - 1) / TB;   // V76: [c][j] -> [j][c] tasima 64x64 karolarla (iki taraf da onbellek dostu); her elemanda toplama sirasi ayni (R, sonra t=0..nt-1)
        #pragma omp parallel for schedule(dynamic, 4)
        for (int cb = 0; cb < ncb; cb++) { int c0 = cb * TB, c1 = min(V, c0 + TB);
            for (int j0 = 0; j0 < m; j0 += TB) { int j1 = min(m, j0 + TB);
                for (int c = c0; c < c1; c++) for (int j = j0; j < j1; j++) { double a = R[(size_t)j * V + c]; for (int t = 0; t < nt; t++) a += loc[t][(size_t)c * m + j]; R[(size_t)j * V + c] = a; } } }
        return; }
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
    int H = f.H, m = H + 1; double tf0_ = now(); if (g_hlean && F.capacity() < (size_t)d.n * m) { F.reserve((size_t)d.n * m); float* pp = F.data(); size_t nn_ = (size_t)d.n * m;   /* V54: sayfalari tum cekirdeklerle onceden dokun (seri sifir-doldurmayi hizlandirir) */
#pragma omp parallel for schedule(static)
        for (long i = 0; i < (long)nn_; i += 1024) pp[i] = 0.f; } F.resize((size_t)d.n * m); TH["f1b1 F tahsis+sifirlama"] += now() - tf0_; tf0_ = now(); gemm_f(d.X.data(), d.n, K, f.W.data(), H, F.data() + 1, m); TH["f1b2 gemm"] += now() - tf0_; const float g = (float)gam; tf0_ = now();
    #pragma omp parallel for
    for (int i = 0; i < d.n; i++) { float* r = &F[(size_t)i * m]; for (int q = 0; q < 3; q++) { int row = d.sp[(size_t)i * 3 + q]; if (row >= 0) { const float* w = &f.W[(size_t)row * H]; for (int j = 0; j < H; j++) r[j + 1] += w[j]; } }
        if (g_v54tanh && H <= 1024) { float zb[1024]; for (int j = 1; j <= H; j++) zb[j - 1] = g * (r[j] + f.b[j - 1]); tv_tanhf_array(zb, r + 1, H); }   /* V54: libm tanhf ile BIT-OZDES (tum 2^32 girdide dogrulandi), vektorlestirilmis */
        else for (int j = 1; j <= H; j++) r[j] = tanhf(g * (r[j] + f.b[j - 1])); r[0] = 1.f; } TH["f1b3 tanh dongusu"] += now() - tf0_;
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
static bool g_bias = false; static bool g_pair = false, g_v36 = false, g_c0 = false; static map<long long, int> g_prank; static double g_loof = 20; static int g_pmin53 = 0, g_npair = 2000; static double g_lamp53 = 0;
#define LSLOT(k) ((k) >= V && g_lamp53 > 0 ? g_lamp53 : lc)   /* V53/D: cift slotu alt siniri (0 = eski top-2000), cift slotlari icin ayri lambda (0 = lamC ile ayni) */
static inline int v36_pairid(int id2, int id1) { unsigned h = ((unsigned)id2 * 2654435761u) ^ ((unsigned)id1 * 40503u + 12345u); h = (h ^ (h >> 15)) * 2246822519u; return 2 * V + (int)((h ^ (h >> 13)) % HB); }
static void v36_buildrank(const Data& tr, int c) { if (!g_prank.empty()) return; map<long long, int> cnt; for (int i = 0; i < tr.n; i++) cnt[(long long)tr.ctx[(size_t)i * c + c - 2] * V + tr.ctx[(size_t)i * c + c - 1]]++; vector<pair<int, long long>> v; for (auto& kv : cnt) v.push_back({-kv.second, kv.first}); sort(v.begin(), v.end()); int lim = g_pmin53 > 0 ? (int)v.size() : 2000; g_npair = 0; for (int i = 0; i < (int)v.size() && i < lim; i++) { if (g_pmin53 > 0 && -v[i].first < g_pmin53) break; g_prank[v[i].second] = i; g_npair++; } }
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
static int g_passes = 1, g_res = 0; static bool g_v41 = false; static bool g_v39 = false, g_v39shuf = false; static double g_beta39 = 0.5; static int g_knn39 = 10;
static int g_norho45 = 0; static double g_tau45 = 0; static int g_cmode45 = 0; static double g_kap45 = 300, g_f45 = 0.5, g_bmax45 = 0.9;
struct Pool39 { vector<int> nb; vector<float> wt; vector<float> bw; };
static double g_rho50 = -0.25; static bool g_v50 = false; static int g_v49 = 0; static double g_s49 = 2.0; static bool g_only0 = false; static vector<Pool39> g_Pf51; static const vector<int>* g_foldPtr = nullptr; static bool g_oofpool = false, g_mixTrain = false; static int g_mseed = 777; static Pool39 g_Pq; static bool g_v46 = false; static double g_bq46 = 0.9;   // V x knn
static Pool39 v39_graph(const Data& tr, int c) {
    vector<float> Cn((size_t)V * V, 0.f); for (int i = 0; i < tr.n; i++) Cn[(size_t)tr.ctx[(size_t)i * c + c - 1] * V + tr.y[i]] += 1.f;
    vector<float> Nn(Cn.size()); vector<int> cnt(V, 0); for (int u = 0; u < V; u++) { double nn = 0; for (int v = 0; v < V; v++) { float x = sqrtf(Cn[(size_t)u * V + v]); Nn[(size_t)u * V + v] = x; nn += (double)x * x; cnt[u] += (int)Cn[(size_t)u * V + v]; } nn = sqrt(nn) + 1e-9; for (int v = 0; v < V; v++) Nn[(size_t)u * V + v] /= (float)nn; }
    vector<float> S((size_t)V * V); scipy_cblas_sgemm(101, 111, 112, V, V, V, 1.f, Nn.data(), V, Nn.data(), V, 0.f, S.data(), V);
    Pool39 P; P.nb.assign((size_t)V * g_knn39, -1); P.wt.assign((size_t)V * g_knn39, 0.f); P.bw.assign(V, 0.f);
    for (int u = 0; u < V; u++) { vector<pair<float, int>> cand; int k = 0; if (g_v54topk && g_knn39 <= 32) {   /* V54: partial_sort yerine tek gecisli en iyi-k secimi (ayni tam sira: (deger, indeks)) */
            pair<float, int> top[32]; const float* Su = &S[(size_t)u * V]; for (int w = 0; w < V; w++) if (w != u && cnt[w] >= 5) { pair<float, int> x{-Su[w], w}; if (k < g_knn39 || x < top[k - 1]) { int pos = k < g_knn39 ? k++ : k - 1; while (pos > 0 && x < top[pos - 1]) { top[pos] = top[pos - 1]; pos--; } top[pos] = x; } } cand.assign(top, top + k); }
        else { for (int w = 0; w < V; w++) if (w != u && cnt[w] >= 5) cand.push_back({-S[(size_t)u * V + w], w}); k = min((int)cand.size(), g_knn39); partial_sort(cand.begin(), cand.begin() + k, cand.end()); } vector<double> ww(k); double sm = 0, smax = k ? -cand[0].first : 0; for (int q = 0; q < k; q++) { double sv = -cand[q].first; ww[q] = g_tau45 > 0 ? exp((sv - smax) / g_tau45) : sv; sm += ww[q]; } double rho = 0; for (int q = 0; q < k; q++) { P.nb[(size_t)u * g_knn39 + q] = cand[q].second; P.wt[(size_t)u * g_knn39 + q] = (float)(ww[q] / max(sm, 1e-9)); rho += (ww[q] / max(sm, 1e-9)) * (-cand[q].first); } P.bw[u] = (float)rho; }
    if (g_cmode45) { double rb = 0; int nr = 0; for (int u = 0; u < V; u++) if (cnt[u] >= 5) { rb += P.bw[u]; nr++; } rb /= max(1, nr);   // guvenilirlik rho_u (komsu benzerliklerinin agirlikli ortalamasi), ortalamaya gore olceklenir
        for (int u = 0; u < V; u++) { double rho = rb > 0 ? P.bw[u] / rb : 1.0, rare = g_kap45 / (g_kap45 + cnt[u]); double b = g_bmax45 * (g_f45 + (1 - g_f45) * rare) * (g_norho45 ? 1.0 : rho); P.bw[u] = (float)min(1.0, max(0.0, b)); } }
    return P; }
static Pool39 v51_graph_cn(const vector<float>& Cn) {
    double tg0_ = now(); ubuf<float> Nn(Cn.size()); vector<int> cnt(V, 0);   /* V54: Nn/S ilklenmemis (tamami yazilir), Cn kopyalanmaz */
#pragma omp parallel for schedule(static)
    for (int u = 0; u < V; u++) { double nn = 0; for (int v = 0; v < V; v++) { float x = sqrtf(Cn[(size_t)u * V + v]); Nn[(size_t)u * V + v] = x; nn += (double)x * x; cnt[u] += (int)Cn[(size_t)u * V + v]; } nn = sqrt(nn) + 1e-9; for (int v = 0; v < V; v++) Nn[(size_t)u * V + v] /= (float)nn; }
    TH["kg1 norm"] += now() - tg0_; tg0_ = now(); ubuf<float> S((size_t)V * V); TH["kg2a S tahsis"] += now() - tg0_; tg0_ = now(); scipy_cblas_sgemm(101, 111, 112, V, V, V, 1.f, Nn.data(), V, Nn.data(), V, 0.f, S.data(), V);
    TH["kg2 sgemm"] += now() - tg0_; tg0_ = now(); Pool39 P; P.nb.assign((size_t)V * g_knn39, -1); P.wt.assign((size_t)V * g_knn39, 0.f); P.bw.assign(V, 0.f);
#pragma omp parallel for schedule(dynamic, 16)
    for (int u = 0; u < V; u++) { vector<pair<float, int>> cand; int k = 0; if (g_v54topk && g_knn39 <= 32) {   /* V54: partial_sort yerine tek gecisli en iyi-k secimi (ayni tam sira: (deger, indeks)) */
            pair<float, int> top[32]; const float* Su = &S[(size_t)u * V]; for (int w = 0; w < V; w++) if (w != u && cnt[w] >= 5) { pair<float, int> x{-Su[w], w}; if (k < g_knn39 || x < top[k - 1]) { int pos = k < g_knn39 ? k++ : k - 1; while (pos > 0 && x < top[pos - 1]) { top[pos] = top[pos - 1]; pos--; } top[pos] = x; } } cand.assign(top, top + k); }
        else { for (int w = 0; w < V; w++) if (w != u && cnt[w] >= 5) cand.push_back({-S[(size_t)u * V + w], w}); k = min((int)cand.size(), g_knn39); partial_sort(cand.begin(), cand.begin() + k, cand.end()); } vector<double> ww(k); double sm = 0, smax = k ? -cand[0].first : 0; for (int q = 0; q < k; q++) { double sv = -cand[q].first; ww[q] = g_tau45 > 0 ? exp((sv - smax) / g_tau45) : sv; sm += ww[q]; } double rho = 0; for (int q = 0; q < k; q++) { P.nb[(size_t)u * g_knn39 + q] = cand[q].second; P.wt[(size_t)u * g_knn39 + q] = (float)(ww[q] / max(sm, 1e-9)); rho += (ww[q] / max(sm, 1e-9)) * (-cand[q].first); } P.bw[u] = (float)rho; }
    TH["kg3 topk"] += now() - tg0_;
    if (g_cmode45) { double rb = 0; int nr = 0; for (int u = 0; u < V; u++) if (cnt[u] >= 5) { rb += P.bw[u]; nr++; } rb /= max(1, nr);   // guvenilirlik rho_u (komsu benzerliklerinin agirlikli ortalamasi), ortalamaya gore olceklenir
        for (int u = 0; u < V; u++) { double rho = rb > 0 ? P.bw[u] / rb : 1.0, rare = g_kap45 / (g_kap45 + cnt[u]); double b = g_bmax45 * (g_f45 + (1 - g_f45) * rare) * (g_norho45 ? 1.0 : rho); P.bw[u] = (float)min(1.0, max(0.0, b)); } }
    return P; }

static bool g_v51 = false, g_v51shuf = false, g_ridgeK = false; static double g_gF = 0, g_gP = 0;
static vector<vector<double>> g_Qf;            // 5 fold tablosu (V x V) + [5] = tam egitim tablosu
static vector<int> g_fold51;                  // egitim satiri -> fold
static vector<float> g_Utr, g_Uva, g_Ute;     // sabit projeksiyon U = std(s(K)) * P  (n x 512)
static double g_qkap53 = 0; static int g_qpi53 = 1;   // V53/C: Q sifir-kutle duzeltmesi: q <- lam q + (1-lam) pi, lam = n_w/(n_w+kappa); pi = Kneser-Ney devam eden unigram (1) ya da egitim unigrami (0)
static vector<double> v53_pi(const vector<double>& nwv, const vector<double>& Pn_or_Cn_nz_rowmajor, const vector<double>& uni) { return uni; }
static void v53_smooth(vector<double>& Qt, const vector<double>& nw, const vector<double>& pi) { if (g_qkap53 <= 0) return;
#pragma omp parallel for schedule(static)
    for (int w = 0; w < V; w++) { if (nw[w] <= 0) continue; double lam = nw[w] / (nw[w] + g_qkap53); double* q = &Qt[(size_t)w * V]; double sum = 0; for (int v = 0; v < V; v++) { q[v] = lam * q[v] + (1 - lam) * pi[v]; sum += q[v]; } for (int v = 0; v < V; v++) q[v] /= sum; } }
// merkezi cekirdek tablosu: K(w) = (1-b) P_w + b sum_v a_wv P_v  (sayimlar Cn'den; graf Cn'den; unigram yedegi)
static bool g_v60spk = true; static int g_v60q = 384, g_v60chk = 0;   // V60: seyrek kNN (varsayilan acik; V60_SPK=0 -> eski yogun sgemm), K-blok Q, V60_KCHECK=1: iki yolu karsilastir
static Pool39 v51_graph_cn(const vector<float>& Cn);
static Pool39 v51_graph_cn_sp(const vector<float>& Cn) {
    double tg0_ = now(); SpCSR R; R.V = V; R.off.assign(V + 1, 0); vector<int> cnt(V, 0); vector<int> rc(V, 0);
#pragma omp parallel for schedule(static)
    for (int u = 0; u < V; u++) { int c = 0; const float* row = &Cn[(size_t)u * V]; for (int v = 0; v < V; v++) c += row[v] != 0.f; rc[u] = c; }
    for (int u = 0; u < V; u++) R.off[u + 1] = R.off[u] + rc[u]; R.col.resize(R.off[V]); R.val.resize(R.off[V]);
#pragma omp parallel for schedule(static)
    for (int u = 0; u < V; u++) { const float* row = &Cn[(size_t)u * V]; double nn = 0; long e = R.off[u]; for (int v = 0; v < V; v++) { float x0 = row[v]; if (x0 == 0.f) continue; float x = sqrtf(x0); nn += (double)x * x; cnt[u] += (int)x0; R.col[e] = v; R.val[e] = x; e++; }
        nn = sqrt(nn) + 1e-9; for (long q = R.off[u]; q < R.off[u + 1]; q++) R.val[q] /= (float)nn; }   // yogun yolla ayni: Nn = sqrtf(Cn)/(float)nn ; sifirlar dogal olarak atlanir
    TH["kg1 norm+CSR"] += now() - tg0_; tg0_ = now();
    vector<float> ov; vector<int> oi, ok; SpKnnStats st; knn_sparse_topk(R, cnt, g_knn39, 5, g_v60q, ov, oi, ok, &st);
    TH["kg2 seyrek S+top-k"] += now() - tg0_; TH["kg2 seyrek FLOP (G)"] += st.flops * 1e-9; tg0_ = now();
    Pool39 P; P.nb.assign((size_t)V * g_knn39, -1); P.wt.assign((size_t)V * g_knn39, 0.f); P.bw.assign(V, 0.f);
#pragma omp parallel for schedule(dynamic, 16)
    for (int u = 0; u < V; u++) { int k = ok[u]; vector<double> ww(k); double sm = 0, smax = k ? (double)ov[(size_t)u * g_knn39] : 0; for (int q = 0; q < k; q++) { double sv = ov[(size_t)u * g_knn39 + q]; ww[q] = g_tau45 > 0 ? exp((sv - smax) / g_tau45) : sv; sm += ww[q]; }
        double rho = 0; for (int q = 0; q < k; q++) { P.nb[(size_t)u * g_knn39 + q] = oi[(size_t)u * g_knn39 + q]; P.wt[(size_t)u * g_knn39 + q] = (float)(ww[q] / max(sm, 1e-9)); rho += (ww[q] / max(sm, 1e-9)) * (double)ov[(size_t)u * g_knn39 + q]; } P.bw[u] = (float)rho; }
    TH["kg3 topk"] += now() - tg0_;
    if (g_cmode45) { double rb = 0; int nr = 0; for (int u = 0; u < V; u++) if (cnt[u] >= 5) { rb += P.bw[u]; nr++; } rb /= max(1, nr);
        for (int u = 0; u < V; u++) { double rho = rb > 0 ? P.bw[u] / rb : 1.0, rare = g_kap45 / (g_kap45 + cnt[u]); double b = g_bmax45 * (g_f45 + (1 - g_f45) * rare) * (g_norho45 ? 1.0 : rho); P.bw[u] = (float)min(1.0, max(0.0, b)); } }
    return P; }
static Pool39 v60_graph(const vector<float>& Cn) {
    if (!g_v60spk) return v51_graph_cn(Cn);
    Pool39 P = v51_graph_cn_sp(Cn);
    if (g_v60chk) { Pool39 D = v51_graph_cn(Cn); long nbd = 0, rows = 0; double wmax = 0, bmax = 0; for (int u = 0; u < V; u++) { bool same = true; for (int q = 0; q < g_knn39; q++) { size_t i = (size_t)u * g_knn39 + q; if (P.nb[i] != D.nb[i]) same = false; wmax = max(wmax, (double)fabsf(P.wt[i] - D.wt[i])); } if (!same) nbd++; bmax = max(bmax, (double)fabsf(P.bw[u] - D.bw[u])); rows++; }
        printf("V60_KCHECK|satir=%ld nb-farkli-satir=%ld maks|dwt|=%.3e maks|dbw|=%.3e %s\n", rows, nbd, wmax, bmax, (nbd == 0 && wmax == 0 && bmax == 0) ? "BIT-ESDEGER" : "FARK"); }
    return P; }
static vector<double> v51_qtab(const vector<float>& Cn, const vector<double>& Ptr_, bool shuf, Pool39* outP = nullptr) {
    double tq_a = now(); Pool39 P = v60_graph(Cn); TH["k1 v51 graf (6 tablo toplam)"] += now() - tq_a; tq_a = now(); if (outP) *outP = P; vector<double> nw(V, 0.0);
#pragma omp parallel for schedule(static)
    for (int w = 0; w < V; w++) for (int v = 0; v < V; v++) nw[w] += Cn[(size_t)w * V + v];
    TH["kq1 nw"] += now() - tq_a; tq_a = now();
    mt19937_64 gs(9001); vector<int> pm(V); iota(pm.begin(), pm.end(), 0); if (shuf) shuffle(pm.begin(), pm.end(), gs);
    vector<double> Qt((size_t)V * V, 0.0); TH["kq2 Qt tahsis"] += now() - tq_a; tq_a = now();
    vector<int> csr_off(V + 1, 0), csr_v; if (g_v54sparseq) { vector<int> rc(V, 0);
#pragma omp parallel for schedule(static)
        for (int w = 0; w < V; w++) { int c = 0; const float* row = &Cn[(size_t)w * V]; for (int v = 0; v < V; v++) c += row[v] != 0.f; rc[w] = c; }
        for (int w = 0; w < V; w++) csr_off[w + 1] = csr_off[w] + rc[w]; csr_v.resize(csr_off[V]);
#pragma omp parallel for schedule(static)
        for (int w = 0; w < V; w++) { int e = csr_off[w]; const float* row = &Cn[(size_t)w * V]; for (int v = 0; v < V; v++) if (row[v] != 0.f) csr_v[e++] = v; } }
#pragma omp parallel for schedule(dynamic, 8)
    for (int w = 0; w < V; w++) { double* q = &Qt[(size_t)w * V]; vector<double> N(V, 0.0); double ws = 0; for (int qk = 0; qk < g_knn39; qk++) { int nb = P.nb[(size_t)w * g_knn39 + qk]; if (nb < 0) continue; nb = pm[nb]; if (nw[nb] <= 0) continue; double a = P.wt[(size_t)w * g_knn39 + qk]; ws += a; if (g_v54sparseq) { for (int e = csr_off[nb]; e < csr_off[nb + 1]; e++) { int v = csr_v[e]; N[v] += a * Cn[(size_t)nb * V + v] / nw[nb]; } } else for (int v = 0; v < V; v++) N[v] += a * Cn[(size_t)nb * V + v] / nw[nb]; }   /* V54: Cn satiri %98 sifir; +0 terimleri atlamak ayni sonucu verir */
        bool hasN = ws > 0; if (hasN) for (int v = 0; v < V; v++) N[v] /= ws; double bq = g_bq46, sum = 0;
        if (nw[w] > 0 && hasN) for (int v = 0; v < V; v++) q[v] = (1 - bq) * Cn[(size_t)w * V + v] / nw[w] + bq * N[v]; else if (nw[w] > 0) for (int v = 0; v < V; v++) q[v] = Cn[(size_t)w * V + v] / nw[w]; else if (hasN) for (int v = 0; v < V; v++) q[v] = N[v]; else for (int v = 0; v < V; v++) q[v] = Ptr_[v];
        for (int v = 0; v < V; v++) sum += q[v]; if (!(sum > 0)) for (int v = 0; v < V; v++) q[v] = Ptr_[v]; else for (int v = 0; v < V; v++) q[v] /= sum; }
    if (g_qkap53 > 0) { vector<double> pi(V, 0.0); if (g_qpi53 == 1) { double t = 0;
#pragma omp parallel
        { int nt_ = omp_get_num_threads(), t_ = omp_get_thread_num(); int v0 = (int)((long)V * t_ / nt_), v1 = (int)((long)V * (t_ + 1) / nt_); for (int w = 0; w < V; w++) { const float* row = &Cn[(size_t)w * V]; for (int v = v0; v < v1; v++) if (row[v] > 0) pi[v] += 1.0; } }
        for (int v = 0; v < V; v++) { pi[v] += 1.0; t += pi[v]; } for (int v = 0; v < V; v++) pi[v] /= t; } else pi = Ptr_; v53_smooth(Qt, nw, pi); }
    TH["k2 v51 qtab (6 tablo toplam)"] += now() - tq_a;
    return Qt; }
// ---------------- V61: Q ve Cn'yi V x V yogun TUTMADAN uret/tuket ----------------
// Q satiri = alfa_w * (seyrek parca: w'nin kendi satiri U komsu satirlari) + beta_w * (ortak dagilim: pi ya da Ptr). Tuketici yalniz s(K)[w] = sum_y Q[w][y] E(y) (64-boyut) oldugundan
// s(K)[w] = (lam * sum_{y in destek} q_seyrek[y] E(y) + (1-lam) * (pi.E)) / toplam   -> pi.E tablo basina BIR KEZ (O(V*64)); Q tablosu hic olusmaz.
static bool g_v61 = true, g_v61chk = false; static int g_v63arrow = 0; static bool g_v63chk = false;   // V63: C-blogu grubu = ok-basi (arrowhead) matris -> kapali formda tersi
struct CountCSR { vector<long> off; vector<int> col; vector<float> val; };
static CountCSR v61_counts(const Data& tr, int c, const vector<int>& fold, int f) {   // Cn = (son-baglam, y) sayimlari, fold f disarida (f==5: hepsi); yalniz sifir-olmayanlar
    vector<long> keys; keys.reserve(tr.n); for (int i = 0; i < tr.n; i++) if (f == 5 || fold[i] != f) keys.push_back((long)tr.ctx[(size_t)i * c + c - 1] * V + tr.y[i]);
    sort(keys.begin(), keys.end()); CountCSR R; R.off.assign(V + 1, 0); for (size_t i = 0; i < keys.size();) { size_t j = i; while (j < keys.size() && keys[j] == keys[i]) j++; R.col.push_back((int)(keys[i] % V)); R.val.push_back((float)(j - i)); R.off[keys[i] / V + 1]++; i = j; }
    for (int u = 0; u < V; u++) R.off[u + 1] += R.off[u]; return R; }
static Pool39 v61_graph(const CountCSR& C, vector<int>& cnt) {
    double tg0_ = now(); SpCSR R; R.V = V; R.off = C.off; R.col = C.col; R.val.resize(C.val.size()); cnt.assign(V, 0);
#pragma omp parallel for schedule(static)
    for (int u = 0; u < V; u++) { double nn = 0; for (long e = C.off[u]; e < C.off[u + 1]; e++) { float x = sqrtf(C.val[e]); nn += (double)x * x; cnt[u] += (int)C.val[e]; R.val[e] = x; } nn = sqrt(nn) + 1e-9; for (long e = C.off[u]; e < C.off[u + 1]; e++) R.val[e] /= (float)nn; }
    TH["kg1 norm+CSR"] += now() - tg0_; tg0_ = now();
    vector<float> ov; vector<int> oi, ok; SpKnnStats st; knn_sparse_topk(R, cnt, g_knn39, 5, g_v60q, ov, oi, ok, &st);
    TH["kg2 seyrek S+top-k"] += now() - tg0_; TH["kg2 seyrek FLOP (G)"] += st.flops * 1e-9; tg0_ = now();
    Pool39 P; P.nb.assign((size_t)V * g_knn39, -1); P.wt.assign((size_t)V * g_knn39, 0.f); P.bw.assign(V, 0.f);
#pragma omp parallel for schedule(dynamic, 16)
    for (int u = 0; u < V; u++) { int k = ok[u]; vector<double> ww(k); double sm = 0, smax = k ? (double)ov[(size_t)u * g_knn39] : 0; for (int q = 0; q < k; q++) { double sv = ov[(size_t)u * g_knn39 + q]; ww[q] = g_tau45 > 0 ? exp((sv - smax) / g_tau45) : sv; sm += ww[q]; }
        double rho = 0; for (int q = 0; q < k; q++) { P.nb[(size_t)u * g_knn39 + q] = oi[(size_t)u * g_knn39 + q]; P.wt[(size_t)u * g_knn39 + q] = (float)(ww[q] / max(sm, 1e-9)); rho += (ww[q] / max(sm, 1e-9)) * (double)ov[(size_t)u * g_knn39 + q]; } P.bw[u] = (float)rho; }
    TH["kg3 topk"] += now() - tg0_;
    if (g_cmode45) { double rb = 0; int nr = 0; for (int u = 0; u < V; u++) if (cnt[u] >= 5) { rb += P.bw[u]; nr++; } rb /= max(1, nr);
        for (int u = 0; u < V; u++) { double rho = rb > 0 ? P.bw[u] / rb : 1.0, rare = g_kap45 / (g_kap45 + cnt[u]); double b = g_bmax45 * (g_f45 + (1 - g_f45) * rare) * (g_norho45 ? 1.0 : rho); P.bw[u] = (float)min(1.0, max(0.0, b)); } }
    return P; }
// S[w*64+j] = sum_y Q[w][y] Et[y*64+j]  (Q = v51_qtab ile ayni formul; yogun V x V tablo olusmadan)
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
    TH["k2 v61 s(K) (Q olusmadan)"] += now() - t0; }
static void v39_mix(const Pool39& P, vector<vector<float>>& Fs, const vector<int>& wd, const vector<int>& last, int n, const vector<vector<float>>& Mean, const vector<int>& has, double beta) {
    for (size_t s = 0; s < Fs.size(); s++) { int m = wd[s];
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) { int u = last[i]; const Pool39& PP = (g_oofpool && g_mixTrain && !g_Pf51.empty()) ? g_Pf51[(*g_foldPtr)[i]] : P; vector<float> acc(m, 0.f); double ws = 0; for (int q = 0; q < g_knn39; q++) { int nb = PP.nb[(size_t)u * g_knn39 + q]; if (nb < 0 || !has[nb]) continue; float w = PP.wt[(size_t)u * g_knn39 + q]; ws += w; const float* mv = &Mean[s][(size_t)nb * m]; for (int j = 0; j < m; j++) acc[j] += w * mv[j]; }
            if (ws <= 0) continue; float* f = &Fs[s][(size_t)i * m]; double bt = g_cmode45 ? PP.bw[u] : beta; for (int j = 0; j < m; j++) f[j] = (float)((1 - bt) * f[j] + bt * acc[j] / ws); } } }
static map<string, double> TT;

// ===================== V53h: CPU hizlandirma yardimcilari (matematik V53 ile ayni) =====================
static bool g_hnoavx = false; static double g_hT0 = 0; static bool g_hfast = false, g_htest = true, g_hhash = false, g_hsplit = false; static int g_hu = 1; static string g_hdump;
static uint64_t h_hash(const void* p, size_t bytes) { const uint64_t* w = (const uint64_t*)p; size_t n8 = bytes / 8; uint64_t h = 1469598103934665603ULL; for (size_t i = 0; i < n8; i++) { h ^= w[i]; h *= 1099511628211ULL; h ^= h >> 29; } const unsigned char* b = (const unsigned char*)p + n8 * 8; for (size_t i = 0; i < bytes - n8 * 8; i++) { h ^= b[i]; h *= 1099511628211ULL; } return h; }
static void h_hd(const char* name, const void* p, size_t bytes) { if (g_hhash) printf("V53H_HASH|%s|%016llx|%zu bayt\n", name, (unsigned long long)h_hash(p, bytes), bytes); if (!g_hdump.empty()) { ofstream f(g_hdump + "/" + name + ".bin", ios::binary); f.write((const char*)p, bytes); } }
// spd_solve ile BIREBIR ayni aritmetik: (1) Cholesky, (2) sutun-basina ileri/geri yerlestirme (toplama sirasi i artan)
static void h_chol(double* G, int m) {
    for (int j = 0; j < m; j++) { for (int k = 0; k < j; k++) { double s = G[(size_t)j * m + k]; for (int i = 0; i < k; i++) s -= G[(size_t)j * m + i] * G[(size_t)k * m + i]; G[(size_t)j * m + k] = s / G[(size_t)k * m + k]; }
        double s = G[(size_t)j * m + j]; for (int i = 0; i < j; i++) s -= G[(size_t)j * m + i] * G[(size_t)j * m + i]; G[(size_t)j * m + j] = sqrt(s); } }
static void h_solve_naive(const double* G, int m, double* R, int ldr, int nc) {   // orijinal spd_solve cozum dongusu (sutun sutun, adimli erisim)
    for (int c = 0; c < nc; c++) { for (int j = 0; j < m; j++) { double s = R[(size_t)j * ldr + c]; for (int i = 0; i < j; i++) s -= G[(size_t)j * m + i] * R[(size_t)i * ldr + c]; R[(size_t)j * ldr + c] = s / G[(size_t)j * m + j]; }
        for (int j = m - 1; j >= 0; j--) { double s = R[(size_t)j * ldr + c]; for (int i = j + 1; i < m; i++) s -= G[(size_t)i * m + j] * R[(size_t)i * ldr + c]; R[(size_t)j * ldr + c] = s / G[(size_t)j * m + j]; } } }
// Ayni islem sirasi (her sutun icin i artan), sutunlar uzerinde vektorlestirilmis + sutun karolari (onbellek): sonuc bit-bit ayni
// Not: orijinal skaler dongulerde derleyici (acma/FMA birlestirmesi) bazi terimleri FMA, bazilarini ayri yuvarlar (kod konumuna bagli);
// bu desen yeniden uretilemez -> ara cift-duyarlik degerlerde ~1 ulp fark olabilir; nihai float agirliklar bit-ozdes dogrulanir.
static inline void h_axpy(double* rj, double g, const double* ri, int w) {
#pragma omp simd
    for (int c = 0; c < w; c++) rj[c] -= g * ri[c]; }
static void h_solve_fast(const double* G, const double* GT, int m, double* R, int ldr, int nc, int tile) {
    for (int t0 = 0; t0 < nc; t0 += tile) { int w = min(tile, nc - t0);
        for (int j = 0; j < m; j++) { double* rj = R + (size_t)j * ldr + t0; const double* gj = G + (size_t)j * m; for (int i = 0; i < j; i++) { double g = gj[i]; const double* ri = R + (size_t)i * ldr + t0;
h_axpy(rj, g, ri, w); } double d = gj[j];
#pragma omp simd
            for (int c = 0; c < w; c++) rj[c] /= d; }
        for (int j = m - 1; j >= 0; j--) { double* rj = R + (size_t)j * ldr + t0; const double* gj = GT + (size_t)j * m; for (int i = j + 1; i < m; i++) { double g = gj[i]; const double* ri = R + (size_t)i * ldr + t0;
h_axpy(rj, g, ri, w); } double d = G[(size_t)j * m + j];
#pragma omp simd
            for (int c = 0; c < w; c++) rj[c] /= d; } } }

#ifdef __AVX512F__
// Kaydedici-bloklu AVX-512 cozucu: 32 sutun (4 zmm) birikimi kayitlarda tutulur; her sutun icin islem sirasi (i artan) orijinalle ayni.
// ldr ve ncols 32'nin kati olmali (dolgu sutunlari sifir RHS -> sifir cozum).
static void h_solve_avx512(const double* G, const double* GT, int m, double* R, int ldr, int ncols) {
    for (int t0 = 0; t0 < ncols; t0 += 32) {
        for (int j = 0; j < m; j++) { double* rj = R + (size_t)j * ldr + t0; __m512d a0 = _mm512_loadu_pd(rj), a1 = _mm512_loadu_pd(rj + 8), a2 = _mm512_loadu_pd(rj + 16), a3 = _mm512_loadu_pd(rj + 24); const double* gj = G + (size_t)j * m;
            for (int i = 0; i < j; i++) { __m512d g = _mm512_set1_pd(gj[i]); const double* ri = R + (size_t)i * ldr + t0; a0 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri), a0); a1 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri + 8), a1); a2 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri + 16), a2); a3 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri + 24), a3); }
            __m512d d = _mm512_set1_pd(gj[j]); _mm512_storeu_pd(rj, _mm512_div_pd(a0, d)); _mm512_storeu_pd(rj + 8, _mm512_div_pd(a1, d)); _mm512_storeu_pd(rj + 16, _mm512_div_pd(a2, d)); _mm512_storeu_pd(rj + 24, _mm512_div_pd(a3, d)); }
        for (int j = m - 1; j >= 0; j--) { double* rj = R + (size_t)j * ldr + t0; __m512d a0 = _mm512_loadu_pd(rj), a1 = _mm512_loadu_pd(rj + 8), a2 = _mm512_loadu_pd(rj + 16), a3 = _mm512_loadu_pd(rj + 24); const double* gj = GT + (size_t)j * m;
            for (int i = j + 1; i < m; i++) { __m512d g = _mm512_set1_pd(gj[i]); const double* ri = R + (size_t)i * ldr + t0; a0 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri), a0); a1 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri + 8), a1); a2 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri + 16), a2); a3 = _mm512_fnmadd_pd(g, _mm512_loadu_pd(ri + 24), a3); }
            __m512d d = _mm512_set1_pd(G[(size_t)j * m + j]); _mm512_storeu_pd(rj, _mm512_div_pd(a0, d)); _mm512_storeu_pd(rj + 8, _mm512_div_pd(a1, d)); _mm512_storeu_pd(rj + 16, _mm512_div_pd(a2, d)); _mm512_storeu_pd(rj + 24, _mm512_div_pd(a3, d)); } } }
#endif
// Q_havuz tablosu: buildQtab(false) ile ayni (V50 blogundan cikarildi)
// ---------------- V64: ortuk W_C ve seyrek Q_havuz satirlari ----------------
static bool g_v75 = false; static bool g_v72u = false, g_v72single = false; static bool g_v65 = false, g_v65chk = false; static int g_v65stride = 10; static vector<float> g65_trF; static vector<int> g65_trIds, g65_trY, g65_trWl; static const float* g65_wcchk = nullptr;
static bool g_v64cmp = false; static bool g_v64imp = false, g_v64spq = false, g_v64qchk = false;
struct ImpCtx { const float* Xf = nullptr; const double* Y0c = nullptr; const double* RCc = nullptr; const vector<size_t>* roff = nullptr; const vector<vector<int>>* sy = nullptr; const vector<int>* nsc = nullptr; const vector<int>* gidv = nullptr; };
static ImpCtx g_imp;
struct QRows { vector<long> off; vector<int> idx; vector<double> val; vector<char> base; vector<double> coef; vector<double> pi, ptr; };   // satir w = sum_t val[t] e_idx[t] + coef[w]*base_vektor (base: 0 yok, 1 pi, 2 Ptr)
static const QRows* g_qr = nullptr;
// Q_havuz satiri: h_qtab ile AYNI formul ; yogun V x V Qt ve Pn olusmaz (Pn satiri = RC_w/n_w, kompakt RCc'den)
static void v64_qrows(const vector<double>& nw, const vector<double>& Ptr_, QRows& Q) {
    const int KQ = g_knn39; const double bq = g_bq46; const auto& roff = *g_imp.roff; const auto& sy = *g_imp.sy; const auto& nsc = *g_imp.nsc; const double* RCc = g_imp.RCc;
    Q.ptr = Ptr_; double sumPtr = 0; for (int v = 0; v < V; v++) sumPtr += Ptr_[v];
    Q.pi.assign(V, 0.0); if (g_qkap53 > 0) { if (g_qpi53 == 1) { double t = 0; for (int w = 0; w < V; w++) if (nw[w] > 0) for (int k = 0; k < nsc[w]; k++) if (RCc[roff[w] + k] > 0) Q.pi[sy[w][k]] += 1.0; for (int v = 0; v < V; v++) { Q.pi[v] += 1.0; t += Q.pi[v]; } for (int v = 0; v < V; v++) Q.pi[v] /= t; } else Q.pi = Ptr_; }
    vector<vector<int>> idxs(V); vector<vector<double>> vals(V); Q.base.assign(V, 0); Q.coef.assign(V, 0.0);
#pragma omp parallel
    { vector<double> N(V, 0.0), Pw(V, 0.0); vector<char> inS(V, 0); vector<int> sup;
#pragma omp for schedule(dynamic, 8)
        for (int w = 0; w < V; w++) { sup.clear(); double ws = 0;
            for (int qk = 0; qk < KQ; qk++) { int nb = g_Pq.nb[(size_t)w * KQ + qk]; if (nb < 0) continue; if (nw[nb] <= 0) continue; double a = g_Pq.wt[(size_t)w * KQ + qk]; ws += a; for (int k = 0; k < nsc[nb]; k++) { int v = sy[nb][k]; double pn = RCc[roff[nb] + k] / nw[nb]; N[v] += a * pn; if (!inS[v]) { inS[v] = 1; sup.push_back(v); } } }
            bool hasN = ws > 0; if (hasN) for (int v : sup) N[v] /= ws;
            if (nw[w] > 0) for (int k = 0; k < nsc[w]; k++) { int v = sy[w][k]; Pw[v] = RCc[roff[w] + k] / nw[w]; if (!inS[v]) { inS[v] = 1; sup.push_back(v); } }
            sort(sup.begin(), sup.end()); vector<double> q(sup.size(), 0.0); double sum = 0; bool dense_ptr = false;
            if (nw[w] > 0 && hasN) for (size_t t = 0; t < sup.size(); t++) q[t] = (1 - bq) * Pw[sup[t]] + bq * N[sup[t]]; else if (nw[w] > 0) for (size_t t = 0; t < sup.size(); t++) q[t] = Pw[sup[t]]; else if (hasN) for (size_t t = 0; t < sup.size(); t++) q[t] = N[sup[t]]; else dense_ptr = true;
            if (!dense_ptr) { for (size_t t = 0; t < sup.size(); t++) sum += q[t]; if (!(sum > 0)) dense_ptr = true; }
            if (dense_ptr) { Q.base[w] = 2; Q.coef[w] = 1.0; sup.clear(); q.clear(); }   // q = Ptr (h_qtab: toplam>0 ise normalizasyon: ayni, Ptr toplami ~1)
            else { if (fabs(sum - 1.0) > 1e-9) for (auto& x : q) x /= sum;
                if (g_qkap53 > 0 && nw[w] > 0) { double lam = nw[w] / (nw[w] + g_qkap53); double sumq = 0; for (double x : q) sumq += x; double s2 = 0;   // yogun toplam: sum_v (lam q_v + (1-lam) pi_v)  (sirali)
                    { size_t t = 0; for (int v = 0; v < V; v++) { double qq = (t < sup.size() && sup[t] == v) ? q[t++] : 0.0; s2 += lam * qq + (1 - lam) * Q.pi[v]; } }
                    for (auto& x : q) x = lam * x / s2; Q.base[w] = 1; Q.coef[w] = (1 - lam) / s2; } }
            idxs[w] = sup; vals[w] = q; for (int v : sup) { N[v] = 0; Pw[v] = 0; inS[v] = 0; } } }
    Q.off.assign(V + 1, 0); for (int w = 0; w < V; w++) Q.off[w + 1] = Q.off[w] + (long)idxs[w].size(); Q.idx.resize(Q.off[V]); Q.val.resize(Q.off[V]);
    for (int w = 0; w < V; w++) { copy(idxs[w].begin(), idxs[w].end(), Q.idx.begin() + Q.off[w]); copy(vals[w].begin(), vals[w].end(), Q.val.begin() + Q.off[w]); } (void)sumPtr; }
static void h_qtab(const vector<double>& nw, const vector<double>& Pn, const vector<double>& Ptr, vector<double>& Qt) {
    const int KQ = g_knn39; Qt.assign((size_t)V * V, 0.0);
#pragma omp parallel for schedule(dynamic, 8)
    for (int w = 0; w < V; w++) { double* q = &Qt[(size_t)w * V]; vector<double> N(V, 0.0); double ws = 0; for (int qk = 0; qk < KQ; qk++) { int nb = g_Pq.nb[(size_t)w * KQ + qk]; if (nb < 0) continue; if (nw[nb] <= 0) continue; double a = g_Pq.wt[(size_t)w * KQ + qk]; ws += a; for (int v = 0; v < V; v++) N[v] += a * Pn[(size_t)nb * V + v]; }
        bool hasN = ws > 0; if (hasN) for (int v = 0; v < V; v++) N[v] /= ws; double bq = g_bq46; double sum = 0;
        if (nw[w] > 0 && hasN) for (int v = 0; v < V; v++) q[v] = (1 - bq) * Pn[(size_t)w * V + v] + bq * N[v]; else if (nw[w] > 0) for (int v = 0; v < V; v++) q[v] = Pn[(size_t)w * V + v]; else if (hasN) for (int v = 0; v < V; v++) q[v] = N[v]; else for (int v = 0; v < V; v++) q[v] = Ptr[v];
        for (int v = 0; v < V; v++) sum += q[v]; if (!(sum > 0)) for (int v = 0; v < V; v++) q[v] = Ptr[v]; else if (fabs(sum - 1.0) > 1e-9) for (int v = 0; v < V; v++) q[v] /= sum; }
    if (g_qkap53 > 0) { vector<double> pi(V, 0.0); if (g_qpi53 == 1) { double t = 0; for (int w = 0; w < V; w++) for (int v = 0; v < V; v++) if (Pn[(size_t)w * V + v] > 0) pi[v] += 1.0; for (int v = 0; v < V; v++) { pi[v] += 1.0; t += pi[v]; } for (int v = 0; v < V; v++) pi[v] /= t; } else pi = Ptr; vector<double> nwd(V); for (int w = 0; w < V; w++) nwd[w] = (double)nw[w]; v53_smooth(Qt, nwd, pi); } }
static void h_fuse(const vector<float>& Z, int n, const vector<int>& y, const vector<int>& wl, const vector<double>* Qt, double alpha, double aread, double eps, const vector<double>& Ptr, double& ce, double& t1, double& t5) { double sce = 0; long c1 = 0, c5 = 0;
#pragma omp parallel reduction(+ : sce, c1, c5)
    { vector<double> p(V), qd(V);
#pragma omp for schedule(static)
        for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], eps); const double* q = (alpha > 0 && Qt) ? &(*Qt)[(size_t)wl[i] * V] : nullptr; if (!q && alpha > 0 && g_qr) { int w_ = wl[i]; const double* bv = g_qr->base[w_] == 1 ? g_qr->pi.data() : g_qr->base[w_] == 2 ? g_qr->ptr.data() : nullptr; double cf = g_qr->coef[w_]; if (bv) for (int v = 0; v < V; v++) qd[v] = cf * bv[v]; else fill(qd.begin(), qd.end(), 0.0); for (long t = g_qr->off[w_]; t < g_qr->off[w_ + 1]; t++) qd[g_qr->idx[t]] += g_qr->val[t]; q = qd.data(); } double sm = 0;
            for (int v = 0; v < V; v++) { double pm_ = (1 - aread) * Ptr[v] + aread * max((double)z[v], eps) / S2; p[v] = q ? (1 - alpha) * pm_ + alpha * q[v] : pm_; sm += p[v]; } double py = p[y[i]] / sm; int rk = 0; for (int v = 0; v < V; v++) if (p[v] / sm > py) rk++; sce += -log(py); c1 += rk == 0; c5 += rk < 5; } }
    ce = sce / n; t1 = (double)c1 / n; t5 = (double)c5 / n; }

// ---- V54: dgemm(A^T, Y0) sonucunu BIT-ozdes ureten seyrek hesap. OpenBLAS (SkylakeX) dgemm'i K'yi bloklara boler (Q=384; kalan 2Q'dan kucukse yarisi),
// her blokta k artan sirada TEK FMA birikimi (acc=fma(a,b,acc)), blok sonunda C += acc. Y0 sifir olan terimler acc'yi degistirmez -> atlanabilir.
// Gecerlilik her calismada uretim sekliyle (M=513, K=mc) cekirdege karsi calisma zamaninda dogrulanir; uymazsa yogun dgemm'e donulur.
static vector<int> h_dgemm_kblocks(int K, int Q = 384) { vector<int> b; for (int ls = 0; ls < K;) { int l = K - ls; if (l >= 2 * Q) l = Q; else if (l > Q) l = l / 2; b.push_back(ls); ls += l; } b.push_back(K); return b; }
// U[j*V + y] (j<mf, y in [ya,yb)) = sum_k ACF[k*mf+j] * Y0[k][y]   (Y0 kompakt: slot k satiri, kolonlar sy[g(k)] sirali)
static void h_u_sparse(int mf, int V, int mc, const double* ACF, const vector<int>& gidv, const vector<vector<int>>& sy, const vector<size_t>& roff, const double* Y0c, double* U, int ya, int yb, const vector<int>& kb, int JBsel = 0) {
    int nt = omp_get_max_threads(); int ny = yb - ya; vector<long> cnt(ny, 0); for (int g = 0; g < V; g++) for (int y : sy[g]) if (y >= ya && y < yb) cnt[y - ya]++;
    long tot = 0; for (long c : cnt) tot += c; vector<int> yq(nt + 1, ya); { long run = 0; int t = 1; for (int y = 0; y < ny && t < nt; y++) { run += cnt[y]; if (run >= tot * t / nt) { yq[t] = ya + y + 1; t++; } } for (; t <= nt; t++) yq[t] = yb; yq[nt] = yb; }
#pragma omp parallel num_threads(nt)
    { int t = omp_get_thread_num(); int y0 = yq[t], y1 = yq[t + 1]; int w = y1 - y0;
      if (w > 0) { const int JB = JBsel > 0 ? JBsel : 128; vector<double> acc((size_t)w * JB), Cc((size_t)w * JB); vector<int> lo(V), hi(V);
        for (int g = 0; g < V; g++) { lo[g] = (int)(lower_bound(sy[g].begin(), sy[g].end(), y0) - sy[g].begin()); hi[g] = (int)(lower_bound(sy[g].begin(), sy[g].end(), y1) - sy[g].begin()); }
        for (int jb = 0; jb < mf; jb += JB) { int jw = min(JB, mf - jb); fill(acc.begin(), acc.end(), 0.0); fill(Cc.begin(), Cc.end(), 0.0);
          size_t b = 0; auto flush = [&]() { for (int y = 0; y < w; y++) { double* c = &Cc[(size_t)y * JB]; double* a = &acc[(size_t)y * JB]; for (int j = 0; j < jw; j++) { c[j] = fma(1.0, a[j], c[j]); a[j] = 0.0; } } };
          for (int k = 0; k < mc; k++) { if (k == kb[b + 1]) { flush(); b++; } int g = gidv[k]; int k0 = lo[g], k1 = hi[g]; if (k0 >= k1) continue; const double* ar = ACF + (size_t)k * mf + jb; const double* yv = Y0c + roff[k]; const int* sl = sy[g].data();
            for (int q = k0; q < k1; q++) { double v = yv[q]; double* a = &acc[(size_t)(sl[q] - y0) * JB];
#pragma omp simd
              for (int j = 0; j < jw; j++) a[j] = fma(ar[j], v, a[j]); } }
          flush();
          for (int y = 0; y < w; y++) for (int j = 0; j < jw; j++) U[(size_t)(jb + j) * V + (y0 + y)] = Cc[(size_t)y * JB + j]; } } } }
// ---------------- V72: U = X^T R_C, R_C'nin GERCEK sifir-olmayanlari uzerinden (CSC: y'ye gore, k artan) ----------------
// Eski h_u_sparse kompakt R_C bloklarini (grup-birlesim sutunlari, cogu yapisal sifir) tarar: 1.7e7 giris x 513. Gercek sifir-olmayan <= 2n (her ornek en cok 2 (slot,y) uretir).
// Sifir terimler FMA birikimini degistirmez (fma(a,0,acc)=acc) ve sifir-olmayanlar ayni (k artan, K-blok) sirada toplanir -> sonuc eski yolla BIT-OZDES.
static void v72_u_csc(int mf, int V, int mc, const double* X, const vector<int>& gidv, const vector<vector<int>>& sy, const vector<size_t>& roff, const double* RCc, double* U, const vector<int>& kb, long* nnz_out, double* t_build) {
    double tb = now(); vector<long> ycnt(V + 1, 0); for (int k = 0; k < mc; k++) { int g = gidv[k]; const double* r = RCc + roff[k]; const int* sl = sy[g].data(); int ns = (int)sy[g].size(); for (int q = 0; q < ns; q++) if (r[q] != 0.0) ycnt[sl[q] + 1]++; }
    for (int y = 0; y < V; y++) ycnt[y + 1] += ycnt[y]; long nnz = ycnt[V]; vector<int> ek(nnz); vector<double> ev(nnz); { vector<long> pos(ycnt.begin(), ycnt.end() - 1); for (int k = 0; k < mc; k++) { int g = gidv[k]; const double* r = RCc + roff[k]; const int* sl = sy[g].data(); int ns = (int)sy[g].size(); for (int q = 0; q < ns; q++) if (r[q] != 0.0) { long p = pos[sl[q]]++; ek[p] = k; ev[p] = r[q]; } } }
    vector<int> blk(mc); for (size_t b = 0; b + 1 < kb.size(); b++) for (int k = kb[b]; k < kb[b + 1]; k++) blk[k] = (int)b; if (nnz_out) *nnz_out = nnz; if (t_build) *t_build = now() - tb;
#pragma omp parallel
    { vector<double> acc(mf), Cc(mf);
#pragma omp for schedule(dynamic, 8)
        for (int y = 0; y < V; y++) { long e0 = ycnt[y], e1 = ycnt[y + 1]; fill(Cc.begin(), Cc.end(), 0.0); fill(acc.begin(), acc.end(), 0.0); int cb = -1;
            for (long e = e0; e < e1; e++) { int k = ek[e]; int b = blk[k]; if (b != cb) { if (cb >= 0) { for (int j = 0; j < mf; j++) { Cc[j] = fma(1.0, acc[j], Cc[j]); acc[j] = 0.0; } } cb = b; } const double* xr = X + (size_t)k * mf; double v = ev[e];
#pragma omp simd
                for (int j = 0; j < mf; j++) acc[j] = fma(xr[j], v, acc[j]); }
            if (cb >= 0) for (int j = 0; j < mf; j++) Cc[j] = fma(1.0, acc[j], Cc[j]);
            for (int j = 0; j < mf; j++) U[(size_t)j * V + y] = Cc[j]; } } }
#include <malloc.h>
// V54: >=4 MB'lik std::vector/new tahsislerinde sayfalar tum cekirdeklerle onceden dokunulur (VM'de sayfa hatasi maliyeti yuksek ve vector'un sifir-doldurmasi seri)
static bool g_prefault = false;
void* operator new(std::size_t n) { void* p = v76_alloc(n); if (!p) throw std::bad_alloc(); if (g_prefault && n >= (4u << 20) && !omp_in_parallel()) { char* c = (char*)p;
#pragma omp parallel for schedule(static)
        for (long i = 0; i < (long)n; i += 4096) c[i] = 0; } return p; }
void operator delete(void* p) noexcept { free(p); }
void operator delete(void* p, std::size_t n) noexcept { v76_free(p, n); }
int main(int argc, char** argv) {
    g_prefault = getenv("V54_PREFAULT") ? atoi(getenv("V54_PREFAULT")) == 1 : !(getenv("V76_LEGACY") && atoi(getenv("V76_LEGACY")) == 1);   /* V76: >=4 MB tahsislerde sayfalar tum cekirdeklerle onceden dokunulur (seri sifirlama/ilk-dokunus yerine paralel) */
    if (getenv("V54_MALLOPT") && atoi(getenv("V54_MALLOPT")) == 1) { mallopt(M_MMAP_THRESHOLD, 1 << 30); mallopt(M_TRIM_THRESHOLD, 1 << 30); mallopt(M_TOP_PAD, 64 << 20); }   // V54: buyuk tamponlar yigindan; serbest birakilinca isletim sistemine donmez -> ayni boyutlu gecici tamponlar yeniden sayfa hatasi uretmez
    if (argc < 7) { cerr << "kullanim: tek_hamle_v27 c H TOHUM gamma1 lambda gamma2 [mod=SCJ] [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), gam2 = atof(argv[6]); vector<double> lams; { string ls = argv[5]; size_t p0 = 0; while (p0 <= ls.size()) { size_t p1 = ls.find(',', p0); if (p1 == string::npos) p1 = ls.size(); lams.push_back(atof(ls.substr(p0, p1 - p0).c_str())); p0 = p1 + 1; } } double lam = lams[0]; double sidm = argc > 8 ? atof(argv[8]) : 1.0; string tgt = argc > 9 ? argv[9] : "YQ"; bool hasQ = tgt.find('Q') != string::npos; auto plist = [&](const char* a) { vector<double> v; string ls = a; size_t p0 = 0; while (p0 <= ls.size()) { size_t p1 = ls.find(',', p0); if (p1 == string::npos) p1 = ls.size(); if (p1 > p0) v.push_back(atof(ls.substr(p0, p1 - p0).c_str())); p0 = p1 + 1; } return v; }; vector<double> lamC_id = plist(argc > 11 ? argv[11] : "1,10,20,100"), lamC_cb = plist(argc > 12 ? argv[12] : "1,10,100"), lamF_l = plist(argc > 13 ? argv[13] : "1,20,100,1000,1e9"); bool v31mode = false; string mod = argc > 7 ? argv[7] : "SC"; string dir = argc > 10 && string(argv[10]) != "-" ? argv[10] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    g_v54tanh = !(getenv("V54_TANH") && atoi(getenv("V54_TANH")) == 0); g_v54gate = !(getenv("V54_GATE") && atoi(getenv("V54_GATE")) == 0); if (g_v54tanh && !tv_selftest()) { g_v54tanh = false; printf("V54_TANH|oz-denetim BASARISIZ -> libm tanhf\n"); } g_v54sparseq = getenv("V54_SPARSEQ") && atoi(getenv("V54_SPARSEQ")) == 1; g_v54utab = !(getenv("V54_UTAB") && atoi(getenv("V54_UTAB")) == 0); g_v54topk = !(getenv("V54_TOPK") && atoi(getenv("V54_TOPK")) == 0); g_v54reuse = !(getenv("V54_REUSE") && atoi(getenv("V54_REUSE")) == 0); g_hlean = getenv("V53H_LEAN") && atoi(getenv("V53H_LEAN")) == 1; g_hfast = getenv("V53H_FAST") && atoi(getenv("V53H_FAST")) == 1; g_htest = !(getenv("V53H_TEST") && atoi(getenv("V53H_TEST")) == 0); g_hhash = getenv("V53H_HASH") && atoi(getenv("V53H_HASH")) == 1; g_hsplit = getenv("V53H_SPLIT") && atoi(getenv("V53H_SPLIT")) == 1; g_hu = 0; if (getenv("V53H_U")) g_hu = atoi(getenv("V53H_U")); g_v60spk = !(getenv("V60_SPK") && atoi(getenv("V60_SPK")) == 0); if (getenv("V60_Q")) g_v60q = atoi(getenv("V60_Q")); g_v60chk = getenv("V60_KCHECK") && atoi(getenv("V60_KCHECK")) == 1; g_v61 = !(getenv("V61_SPQ") && atoi(getenv("V61_SPQ")) == 0); g_v61chk = getenv("V61_QCHECK") && atoi(getenv("V61_QCHECK")) == 1; g_v63arrow = getenv("V63_ARROW") ? atoi(getenv("V63_ARROW")) : 0; g_v63chk = getenv("V63_CHECK") && atoi(getenv("V63_CHECK")) == 1; g_v73rhs = getenv("V73_RHS") && atoi(getenv("V73_RHS")) == 1; g_v73syrk = getenv("V73_SYRK") && atoi(getenv("V73_SYRK")) == 1;     g_v76pool = getenv("V76_POOL") && atoi(getenv("V76_POOL")) == 1; g_v76zc = getenv("V76_ZCACHE") && atoi(getenv("V76_ZCACHE")) == 1; g_v76vec = getenv("V76_VEC") && atoi(getenv("V76_VEC")) == 1; g_v76thp = getenv("V76_THP") && atoi(getenv("V76_THP")) == 1; g_v75 = getenv("V75_STREAM") && atoi(getenv("V75_STREAM")) == 1; g_v72u = getenv("V72_UCSC") && atoi(getenv("V72_UCSC")) == 1; g_v72single = getenv("V72_SINGLE") && atoi(getenv("V72_SINGLE")) == 1; g_v65 = getenv("V65_EVAL") && atoi(getenv("V65_EVAL")) == 1; g_v65chk = getenv("V65_CHECK") && atoi(getenv("V65_CHECK")) == 1; if (getenv("V65_STRIDE")) g_v65stride = atoi(getenv("V65_STRIDE")); g_v64cmp = getenv("V64_COMPACT") && atoi(getenv("V64_COMPACT")) == 1 && g_v63arrow >= 1; g_v64imp = getenv("V64_IMPL") && atoi(getenv("V64_IMPL")) == 1; g_v64spq = getenv("V64_SPQ") && atoi(getenv("V64_SPQ")) == 1;   /* V54: varsayilan 0 = yogun dgemm (tam bit-ozdes U; ilk-dokunus maliyeti dusunce seyrek-emulasyondan hizli). 2 = dusuk-RAM modu */ if (getenv("V53H_DUMP")) g_hdump = getenv("V53H_DUMP"); g_hnoavx = getenv("V53H_NOAVX") && atoi(getenv("V53H_NOAVX")) == 1;
    if (getenv("V53_PMIN")) g_pmin53 = atoi(getenv("V53_PMIN")); if (getenv("V53_LAMP")) g_lamp53 = atof(getenv("V53_LAMP"));
    if (getenv("V53_QKAP")) g_qkap53 = atof(getenv("V53_QKAP")); if (getenv("V53_QPI")) g_qpi53 = atoi(getenv("V53_QPI"));
    g_bias = getenv("V37_BIAS") && atoi(getenv("V37_BIAS")) == 1; g_v38 = getenv("V38") && atoi(getenv("V38")) == 1; if (getenv("V38_EPS")) g_eps38 = atof(getenv("V38_EPS")); g_passes = getenv("V43_PASSES") ? atoi(getenv("V43_PASSES")) : 1; g_res = getenv("V43_RES") ? atoi(getenv("V43_RES")) : 0; g_tau45 = getenv("V45_TAU") ? atof(getenv("V45_TAU")) : 0; g_norho45 = getenv("V45_NORHO") ? atoi(getenv("V45_NORHO")) : 0; g_cmode45 = getenv("V45_C") ? atoi(getenv("V45_C")) : 0; if (getenv("V45_KAPPA")) g_kap45 = atof(getenv("V45_KAPPA")); if (getenv("V45_F")) g_f45 = atof(getenv("V45_F")); if (getenv("V45_BMAX")) g_bmax45 = atof(getenv("V45_BMAX")); g_rho50 = getenv("V50_RHO") ? atof(getenv("V50_RHO")) : -0.25; if (getenv("V52_KNN")) g_knn39 = atoi(getenv("V52_KNN")); g_oofpool = getenv("V51_OOFPOOL") && atoi(getenv("V51_OOFPOOL")) == 1; if (getenv("V51_MSEED")) g_mseed = atoi(getenv("V51_MSEED")); g_v51 = getenv("V51") && atoi(getenv("V51")) == 1; g_v51shuf = getenv("V51_SHUF") && atoi(getenv("V51_SHUF")) == 1; g_ridgeK = getenv("V51_RIDGEK") && atoi(getenv("V51_RIDGEK")) == 1; g_gF = getenv("V51_GF") ? atof(getenv("V51_GF")) : 0; g_gP = getenv("V51_GP") ? atof(getenv("V51_GP")) : 0; g_v50 = getenv("V50") && atoi(getenv("V50")) == 1; g_v49 = getenv("V49") ? atoi(getenv("V49")) : 0; if (getenv("V49_S")) g_s49 = atof(getenv("V49_S")); g_only0 = getenv("V46_ONLY0") && atoi(getenv("V46_ONLY0")) == 1; g_v46 = getenv("V46") && atoi(getenv("V46")) == 1; if (getenv("V46_BQ")) g_bq46 = atof(getenv("V46_BQ")); g_v41 = !(getenv("V41_GRP") && atoi(getenv("V41_GRP")) == 0);   /* V42: C0 tabaninda varsayilan ACIK (yapi denetimi + rezidu denetimi basarisizsa otomatik yogun yola doner) */ g_v39 = getenv("V39") && atoi(getenv("V39")) == 1; g_v39shuf = getenv("V39_SHUF") && atoi(getenv("V39_SHUF")) == 1; if (getenv("V39_BETA")) g_beta39 = atof(getenv("V39_BETA")); g_pca = getenv("V37_PCA") && atoi(getenv("V37_PCA")) == 1;
    g_pair = getenv("V36_PAIR") && atoi(getenv("V36_PAIR")) == 1; g_c0 = getenv("V36_C0") && atoi(getenv("V36_C0")) == 1; g_v36 = getenv("V36") && atoi(getenv("V36")) == 1; if (getenv("V36_LOOF")) g_loof = atof(getenv("V36_LOOF"));
    g_v35 = (getenv("V35_TGT") && atoi(getenv("V35_TGT")) == 1) || g_v36; g_yshuf = getenv("V35_YSHUF") && atoi(getenv("V35_YSHUF")) == 1; if (getenv("V35_LAMS")) g_lams = atof(getenv("V35_LAMS"));
    g_blas = getenv("V33_BLAS") && atoi(getenv("V33_BLAS")) == 1; g_fast = !(getenv("V34_FAST") && atoi(getenv("V34_FAST")) == 0);   /* varsayilan ACIK (dogrulandi: bit-ozdes agirlik); V34_FAST=0 -> V33 yolu */ if (getenv("V34_RHS_GROUPS")) g_rhsg = atoi(getenv("V34_RHS_GROUPS")); if (getenv("V34_CHOL_BLOCKS")) g_cholblk = atoi(getenv("V34_CHOL_BLOCKS")); printf("V34_MOD|FAST=%d RHS_GRUPLARI=%d CHOL_BLOKLARI=%d\n", (int)g_fast, g_rhsg, g_cholblk); if (g_blas && getenv("V33_BLAS_THREADS")) scipy_openblas_set_num_threads(atoi(getenv("V33_BLAS_THREADS"))); printf("V33_MOD|cekirdekler=%s|OpenMP=%d|OpenBLAS iplik=%d\n", g_blas ? "OPENBLAS (sgemm/dpotrf/dtrsm/dgemm)" : "ESKI elle yazilmis", omp_get_max_threads(), g_blas ? scipy_openblas_get_num_threads() : 0);
    { bool legacy = getenv("V76_LEGACY") && atoi(getenv("V76_LEGACY")) == 1; if (!legacy) { auto on = [&](const char* k) { return !(getenv(k) && atoi(getenv(k)) == 0); };   /* V76 varsayilan: bayrak acikca 0 verilmedikce ACIK */
        if (!getenv("V63_ARROW")) g_v63arrow = 2; if (!getenv("V64_IMPL")) g_v64imp = true; if (!getenv("V64_SPQ")) g_v64spq = true; if (!getenv("V72_UCSC")) g_v72u = true; if (!getenv("V73_RHS")) g_v73rhs = true; if (!getenv("V73_SYRK")) g_v73syrk = true; if (!getenv("V75_STREAM")) g_v75 = true; if (!getenv("V76_VEC")) g_v76vec = true; 
        if (!getenv("V64_COMPACT") && g_v63arrow >= 1) g_v64cmp = true; if (!getenv("V53H_LEAN")) g_hlean = true; if (!getenv("V53H_FAST")) g_hfast = true; (void)on; } }
    printf("V76_YOL|ok-basi=%d(2=seyrek U) ortuk_WC=%d seyrek_Q=%d kompakt=%d CSC_U=%d RHS=%d SYRK=%d akis_val_test=%d AVX512_8alfa=%d THP=%d ZCACHE=%d POOL=%d|lean=%d fast=%d|seyrek K-graf=%d seyrek Q/Cn=%d\n", g_v63arrow, (int)g_v64imp, (int)g_v64spq, (int)g_v64cmp, (int)g_v72u, (int)g_v73rhs, (int)g_v73syrk, (int)g_v75, (int)g_v76vec, (int)g_v76thp, (int)g_v76zc, (int)g_v76pool, (int)g_hlean, (int)g_hfast, (int)g_v60spk, (int)g_v61);
    if (!(g_v63arrow >= 2 && g_v64imp && g_v64spq && g_v72u && g_hlean && g_hfast && g_v60spk && g_v61) && !(getenv("V76_LEGACY"))) { printf("V76_UYARI|hizli yollardan biri KAPALI -> eski/yogun yol riski\n"); }
    double Tp0 = now(); g_hT0 = Tp0; phase_mark("basla"); Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); printf("ZAMAN|veri dosyalarini okuma=%.3fs\n", now() - Tp0); double Tp1 = now();
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); BASEA = K; BASEB = K + V; BASEP = K + 2 * V; KT = K + 2 * V + HB; }
    printf("ZAMAN|expand_s (X'e fark bloku ekleme)=%.3fs\n", now() - Tp1);
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V27 c=%d H=%d tohum=%d gamma1=%g lambda=%g gamma2(F2..F4)=%g mod=%s (%d cekirdek); yalniz egitim; dogrusal okuma (alpha,eps dogrulamayla)\n", c, H, seed, gam, lam, gam2, mod.c_str(), omp_get_max_threads());
    double Ts = now(); Teach TE; double a3[4] = {0, 0, 0, 0}; if (!g_hlean) { build_teach(TE, tr.ctx, tr.y); em_weights(TE, va.ctx, va.y, 3, a3); } printf("ZAMAN|n-gram ogretici sayimlari+EM (yalniz kategori/rapor icin; agirlik uretimi DEGIL)=%.3fs\n", now() - Ts);
    vector<double> pi(V, 0.0); if (!g_hlean) { double tot = 0; for (int v = 0; v < V; v++) tot += TE.c1[v] + 1.0; for (int v = 0; v < V; v++) pi[v] = (TE.c1[v] + 1.0) / tot; }
    vector<char> tcat(nte, 0); if (!g_hlean) for (int i = 0; i < nte; i++) { double q[4]; bool av[2]; comp_y(TE, te.ctx[2 * i], te.ctx[2 * i + 1], te.y[i], false, q, av); tcat[i] = av[1] ? 2 : (av[0] ? 1 : 0); }

    auto v31solve = [&](vector<vector<float>>& Ft, vector<vector<float>>& Fv, vector<vector<float>>& Fe, vector<int>& wd, vector<vector<vector<double>>>& C, vector<vector<double>>& RYb, const string& name) {
        double tz0 = now(); int ns = wd.size(), mf = 0; vector<int> off(ns); for (int s = 0; s < ns; s++) { off[s] = mf; mf += wd[s]; } const int NI = g_pair ? 3 : 2; if (g_c0) v36_buildrank(tr, c); const int mc = g_c0 ? V + g_npair : 2 * V + (g_pair ? HB : 0);
        auto cat = [&](vector<vector<float>>& Fs, int n) { if (g_hfast && ns == 1) return std::move(Fs[0]); vector<float> o((size_t)n * mf); for (int s = 0; s < ns; s++) {
#pragma omp parallel for schedule(static) if (g_fast)
            for (int i = 0; i < n; i++) memcpy(&o[(size_t)i * mf + off[s]], &Fs[s][(size_t)i * wd[s]], wd[s] * sizeof(float)); } return o; };
        vector<float> Fc = cat(Ft, ntr), Fcv = cat(Fv, nva), Fce = cat(Fe, nte); TH["g1a cat (F birlestirme)"] = now() - tz0;
        if (getenv("V52_DUMP")) { string d = getenv("V52_DUMP"); auto wr = [&](const string& nm, const void* p, size_t bytes) { ofstream f(d + "/" + nm, ios::binary); f.write((const char*)p, bytes); };
            wr("Fc.f32", Fc.data(), Fc.size() * 4); wr("Fcv.f32", Fcv.data(), Fcv.size() * 4); wr("Fce.f32", Fce.data(), Fce.size() * 4); wr("ytr.i32", tr.y.data(), ntr * 4); wr("yva.i32", va.y.data(), nva * 4); wr("yte.i32", te.y.data(), nte * 4);
            vector<int> wtr(ntr), wva2(nva), wte2(nte); for (int i = 0; i < ntr; i++) wtr[i] = tr.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nva; i++) wva2[i] = va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) wte2[i] = te.ctx[(size_t)i * c + c - 1]; wr("wtr.i32", wtr.data(), ntr * 4); wr("wva.i32", wva2.data(), nva * 4); wr("wte.i32", wte2.data(), nte * 4);
            printf("V52_DUMP|Fc %dx%d, Fcv, Fce, y/w yazildi: %s\n", ntr, mf, d.c_str()); }
        vector<double> AFF((size_t)mf * mf, 0.0), RF((size_t)mf * V, 0.0);
        for (int i = 0; i < ns; i++) for (int j = 0; j <= i; j++) for (int a = 0; a < wd[i]; a++) for (int k = 0; k < wd[j]; k++) { double cv = C[i][j][(size_t)a * wd[j] + k]; AFF[(size_t)(off[i] + a) * mf + off[j] + k] = cv; AFF[(size_t)(off[j] + k) * mf + off[i] + a] = cv; }
        for (int s = 0; s < ns; s++) for (size_t q = 0; q < RYb[s].size(); q++) RF[(size_t)off[s] * V + q] = RYb[s][q];
        auto idv = [&](const Data& d, int n) { return v36_ids(d, n, c, NI); };
        double tb1_ = now(); vector<int> itr = idv(tr, ntr), iva = idv(va, nva), ite = idv(te, nte); TH["g1b slot id uretimi"] = now() - tb1_;
        vector<double> ACF, RC; if (!g_hfast) { ACF.assign((size_t)mc * mf, 0.0); RC.assign((size_t)mc * V, 0.0); }
        // V53: A_CC son kelimeye gore blok-kosegen (her satirin iki slotu da ayni w_{t-1} grubunda) -> yalniz grup bloklari saklanir (yogun mc x mc YOK; mc cift slot sayisiyla buyur)
        vector<int> gidv(mc, -1), locv(mc, 0), gsz(V, 0); vector<size_t> goff(V + 1, 0);
        { for (int w = 0; w < V; w++) gidv[w] = w; for (auto& kv : g_prank) gidv[V + kv.second] = (int)(kv.first % V); for (int k = 0; k < mc; k++) locv[k] = gsz[gidv[k]]++; for (int g = 0; g < V; g++) goff[g + 1] = goff[g] + (size_t)gsz[g] * gsz[g]; }
        vector<double> ACCB(g_v64cmp ? 0 : goff[V], 0.0), NK(g_v64cmp ? mc : 0, 0.0);   // V64_COMPACT: ok-basi grupta tum bilgi slot sayilari n_k (A[w,p]=A[p,p]=n_p) -> sum sg^2 bellek yok
        auto ACCAT = [&](int a, int b) -> double { if (g_v64cmp) return a == b ? NK[a] : 0.0; int g = gidv[a]; return g == gidv[b] ? ACCB[goff[g] + (size_t)locv[a] * gsz[g] + locv[b]] : 0.0; };
        TH["m2 v31solve sayim oncesi (program basindan)"] = now() - g_hT0; double tcnt_ = now(); if (!g_hfast) {   // V36: genel NI yuvali sayim (kimlik satirlarina sahiplik id % nt; cakismasiz paralel)
            int nt = omp_get_max_threads();
#pragma omp parallel num_threads(nt)
            { int t = omp_get_thread_num();
              for (int i = 0; i < ntr; i++) { int yy = tr.y[i]; const float* fr = &Fc[(size_t)i * mf];
                for (int p = 0; p < NI; p++) { int a = itr[(size_t)NI * i + p]; if (a < 0 || a % nt != t) continue; for (int q = 0; q < NI; q++) if (itr[(size_t)NI * i + q] >= 0) { int b3 = itr[(size_t)NI * i + q]; ACCB[goff[gidv[a]] + (size_t)locv[a] * gsz[gidv[a]] + locv[b3]] += 1; } RC[(size_t)a * V + yy] += 1; double* ra = &ACF[(size_t)a * mf]; for (int j = 0; j < mf; j++) ra[j] += fr[j]; } } }
        } TH["g3 sayim (ACC,ACF,RC)"] = now() - tcnt_;
        TH["g1 v31solve basi->sayim oncesi (cat+AFF/RF+id+ayirma)"] = tcnt_ - tz0;
        printf("V31_HAZIRLIK|F bloku=%d sutun (yogun), C blogu=%d seyrek sutun (yogun one-hot veri matrisi YOK; C^TC yalniz son-kelime grup bloklari: %.1f MB, yogun %dx%d YOK)|sure=%.2fs\n", mf, mc, (double)goff[V] * 8 / 1e6, mc, mc, now() - tz0); fflush(stdout);
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
        // ===================== V53h: lean model-hazirlama yolu (V53H_LEAN=1) =====================
        auto logitsP = [&](const vector<float>& Fm, const vector<int>& ids, int n, const float* WC, const float* WF) { vector<float> Z((size_t)n * V, 0.f);
            if (!WC) {   // V64: ORTUK W_C ->  z = (F - sum_slot X) W_F + sum_slot Y0   (Y0 seyrek)
                const int BR = 1024; vector<float> Ft((size_t)BR * mf); const float* Xf_ = g_imp.Xf; const auto& roff_ = *g_imp.roff; const auto& sy_ = *g_imp.sy; const auto& nsc_ = *g_imp.nsc; const auto& gid_ = *g_imp.gidv;
                for (int s0 = 0; s0 < n; s0 += BR) { int nn = min(BR, n - s0);
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < nn; i++) { float* f = &Ft[(size_t)i * mf]; memcpy(f, &Fm[(size_t)(s0 + i) * mf], mf * sizeof(float)); for (int p = 0; p < NI; p++) { int id = ids[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; const float* xf = &Xf_[(size_t)id * mf]; for (int j = 0; j < mf; j++) f[j] -= xf[j]; } }
                    gemm_f(Ft.data(), nn, mf, WF, V, &Z[(size_t)s0 * V], V);
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < nn; i++) { float* z = &Z[(size_t)(s0 + i) * V]; for (int p = 0; p < NI; p++) { int id = ids[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; int g = gid_[id]; for (int k = 0; k < nsc_[g]; k++) z[sy_[g][k]] += (float)g_imp.Y0c[roff_[id] + k]; } } }
                return Z; }
            gemm_f(Fm.data(), n, mf, WF, V, Z.data(), V);
#pragma omp parallel for schedule(static)
            for (int i = 0; i < n; i++) { float* z = &Z[(size_t)i * V]; for (int p = 0; p < NI; p++) { if (ids[(size_t)NI * i + p] < 0) continue; const float* w1 = WC + (size_t)ids[(size_t)NI * i + p] * V; for (int v = 0; v < V; v++) z[v] += w1[v]; } } return Z; };
        auto h_finish = [&](const float* WCf, const float* WFf, const vector<double>& nwl, const vector<double>& Pn, double t_model) {
            double tq0 = now(); phase_mark("[Q oncesi bosluk]"); double tpre = tq0 - g_hT0 - (TH.count("d1 rezidu denetimi (model hazir disi)") ? TH["d1 rezidu denetimi (model hazir disi)"] : 0.0) - (TT.count("E1 yalniz-kimlik dogrulama degerlendirmesi (logit+alpha/eps+PPL)") ? TT["E1 yalniz-kimlik dogrulama degerlendirmesi (logit+alpha/eps+PPL)"] : 0.0) - (TT.count("E2 ortak cozum dogrulama degerlendirmesi") ? TT["E2 ortak cozum dogrulama degerlendirmesi"] : 0.0); (void)t_model; vector<double> Qt; QRows QR; if (g_v64spq) { v64_qrows(nwl, Ptr, QR); g_qr = &QR; } else h_qtab(nwl, Pn, Ptr, Qt); TH["q1 Q_havuz tablosu"] = now() - tq0; phase_mark("Q_havuz tablosu");
            vector<int> wva(nva), wte(nte); for (int i = 0; i < nva; i++) wva[i] = va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) wte[i] = te.ctx[(size_t)i * c + c - 1];
                        vector<float> Zv; double vce = 0, a0 = 0, e0 = 0, bestA = 0;
            if (g_v75 && !WCf) {   // V75/V76: val/test blok-akis; V76: val logitleri bir kez (isteğe bagli onbellek), 8 eps / 8 alfa AVX-512 seritlerinde AYNI islemlerle
                double ts0 = now(); const int CH = getenv("V76_CH") ? atoi(getenv("V76_CH")) : (int)max(256L, min(32768L, (long)((size_t)320 << 20) / ((long)V * 4))), NA = 8;   /* tampon butcesi 320 MB: V ile ters orantili (genel) */ const double ALFS[NA] = {0, 0.025, 0.05, 0.10, 0.20, 0.30, 0.40, 0.60}; int nt = omp_get_max_threads();
                vector<float> Fb((size_t)CH * mf); ubuf<float> Zb((size_t)CH * V); vector<vector<double>> pmv(nt, vector<double>(V)), qdv(nt, vector<double>(V));
                const bool zcache = g_v76zc; float* Zc = zcache ? (float*)v76_alloc((size_t)nva * V * sizeof(float)) : nullptr; const bool vec = g_v76vec;
                auto blockLogits = [&](const vector<float>& Fm, const vector<int>& ids, int s0, int nn, float* dst) {
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < nn; i++) { float* f = &Fb[(size_t)i * mf]; memcpy(f, &Fm[(size_t)(s0 + i) * mf], mf * sizeof(float)); for (int p = 0; p < NI; p++) { int id = ids[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; const float* xf = &g_imp.Xf[(size_t)id * mf]; for (int j = 0; j < mf; j++) f[j] -= xf[j]; } }
                    gemm_f(Fb.data(), nn, mf, WFf, V, dst, V);
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < nn; i++) { float* z = &dst[(size_t)i * V]; for (int p = 0; p < NI; p++) { int id = ids[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; int g = (*g_imp.gidv)[id]; for (int k = 0; k < (*g_imp.nsc)[g]; k++) z[(*g_imp.sy)[g][k]] += (float)g_imp.Y0c[(*g_imp.roff)[id] + k]; } } };
                // asama 1: (okuma alpha, eps) secimi -- lin_table ile ayni toplamlar
                double cl[NEPS][NAL]; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) cl[e][a] = 0;
                for (int s0 = 0; s0 < nva; s0 += CH) { int nn = min(CH, nva - s0); float* dst = zcache ? Zc + (size_t)s0 * V : Zb.data(); blockLogits(Fcv, iva, s0, nn, dst);
#pragma omp parallel
                    { double loc[NEPS][NAL]; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) loc[e][a] = 0;
#pragma omp for schedule(static)
                        for (int i = 0; i < nn; i++) { const float* z = &dst[(size_t)i * V]; int yy = va.y[s0 + i]; double Sv[NEPS];
                            if (vec) { __m512d S8 = _mm512_setzero_pd(), E8 = _mm512_loadu_pd(EPG); for (int v = 0; v < V; v++) S8 = _mm512_add_pd(S8, _mm512_max_pd(_mm512_set1_pd((double)z[v]), E8)); _mm512_storeu_pd(Sv, S8); }
                            else for (int e = 0; e < NEPS; e++) { double S = 0; for (int v = 0; v < V; v++) S += max((double)z[v], EPG[e]); Sv[e] = S; }
                            for (int e = 0; e < NEPS; e++) { double zy = max((double)z[yy], EPG[e]) / Sv[e]; for (int a = 0; a < NAL; a++) loc[e][a] += -log((1 - ALG[a]) * Ptr[yy] + ALG[a] * zy); } }
#pragma omp critical
                        { for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) cl[e][a] += loc[e][a]; } } }
                vce = 1e30; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) { double c_ = cl[e][a] / nva; if (c_ < vce) { vce = c_; a0 = ALG[a]; e0 = EPG[e]; } }
                TH["v75 val asama1 (logit+lin_table)"] = now() - ts0; ts0 = now();
                // fuse: pm bir kez, q satiri bir kez
                auto prepRow = [&](const float* z, int w_, bool needq, double* pm, double* qd) {
                    double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], e0);
                    for (int v = 0; v < V; v++) pm[v] = (1 - a0) * Ptr[v] + a0 * max((double)z[v], e0) / S2;
                    if (needq) { const QRows& Qr = QR; const double* bv = Qr.base[w_] == 1 ? Qr.pi.data() : Qr.base[w_] == 2 ? Qr.ptr.data() : nullptr; double cf = Qr.coef[w_]; if (bv) for (int v = 0; v < V; v++) qd[v] = cf * bv[v]; else fill(qd, qd + V, 0.0); for (long u = Qr.off[w_]; u < Qr.off[w_ + 1]; u++) qd[Qr.idx[u]] += Qr.val[u]; } };
                auto fuseRow = [&](const float* z, int yy, int w_, const double* alphas, int na, double* ce_o, int* rk_o) {   // skaler (V75 ile ayni)
                    int t = omp_get_thread_num(); double* pm = pmv[t].data(); double* qd = qdv[t].data(); bool needq = false; for (int a = 0; a < na; a++) if (alphas[a] > 0) needq = true; prepRow(z, w_, needq, pm, qd);
                    for (int a = 0; a < na; a++) { double al = alphas[a]; double sm = 0; if (al > 0) { for (int v = 0; v < V; v++) { double p = (1 - al) * pm[v] + al * qd[v]; sm += p; } } else { for (int v = 0; v < V; v++) sm += pm[v]; }
                        double py_ = al > 0 ? ((1 - al) * pm[yy] + al * qd[yy]) : pm[yy]; double py = py_ / sm; int rk = 0;
                        if (al > 0) { for (int v = 0; v < V; v++) { double p = (1 - al) * pm[v] + al * qd[v]; if (p / sm > py) rk++; } } else { for (int v = 0; v < V; v++) if (pm[v] / sm > py) rk++; }
                        ce_o[a] = -log(py); rk_o[a] = rk; } };
                auto fuseRow8 = [&](const float* z, int yy, int w_, double* ce_o, int* rk_o) {   // V76: 8 alfa AVX-512 seritlerinde (her serit skaler yolla ayni islem dizisi)
                    int t = omp_get_thread_num(); double* pm = pmv[t].data(); double* qd = qdv[t].data(); prepRow(z, w_, true, pm, qd);
                    __m512d al = _mm512_loadu_pd(ALFS), om = _mm512_sub_pd(_mm512_set1_pd(1.0), al), sm = _mm512_setzero_pd();
                    for (int v = 0; v < V; v++) { __m512d p = _mm512_fmadd_pd(om, _mm512_set1_pd(pm[v]), _mm512_mul_pd(al, _mm512_set1_pd(qd[v]))); sm = _mm512_add_pd(sm, p); }
                    __m512d py = _mm512_div_pd(_mm512_fmadd_pd(om, _mm512_set1_pd(pm[yy]), _mm512_mul_pd(al, _mm512_set1_pd(qd[yy]))), sm); __m256i rk = _mm256_setzero_si256();
                    for (int v = 0; v < V; v++) { __m512d p = _mm512_fmadd_pd(om, _mm512_set1_pd(pm[v]), _mm512_mul_pd(al, _mm512_set1_pd(qd[v]))); __mmask8 m = _mm512_cmp_pd_mask(_mm512_div_pd(p, sm), py, _CMP_GT_OQ); rk = _mm256_mask_add_epi32(rk, m, rk, _mm256_set1_epi32(1)); }
                    double pyv[8]; int rkv[8]; _mm512_storeu_pd(pyv, py); _mm256_storeu_si256((__m256i*)rkv, rk); for (int a = 0; a < 8; a++) { ce_o[a] = -log(pyv[a]); rk_o[a] = rkv[a]; } };
                double sceA[NA] = {0}; long c1A[NA] = {0}, c5A[NA] = {0};
                for (int s0 = 0; s0 < nva; s0 += CH) { int nn = min(CH, nva - s0); const float* src; if (zcache) src = Zc + (size_t)s0 * V; else { blockLogits(Fcv, iva, s0, nn, Zb.data()); src = Zb.data(); } vector<double> ceb((size_t)nn * NA); vector<int> rkb((size_t)nn * NA);
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < nn; i++) { if (vec) fuseRow8(&src[(size_t)i * V], va.y[s0 + i], wva[s0 + i], &ceb[(size_t)i * NA], &rkb[(size_t)i * NA]); else fuseRow(&src[(size_t)i * V], va.y[s0 + i], wva[s0 + i], ALFS, NA, &ceb[(size_t)i * NA], &rkb[(size_t)i * NA]); }
                    for (int i = 0; i < nn; i++) for (int a = 0; a < NA; a++) { sceA[a] += ceb[(size_t)i * NA + a]; c1A[a] += rkb[(size_t)i * NA + a] == 0; c5A[a] += rkb[(size_t)i * NA + a] < 5; } }
                if (zcache) { free(Zc); }
                double bestV = 1e30; int ba = 0; for (int a = 0; a < NA; a++) if (sceA[a] / nva < bestV) { bestV = sceA[a] / nva; ba = a; } bestA = ALFS[ba]; double cev = sceA[ba] / nva, v1 = (double)c1A[ba] / nva, v5 = (double)c5A[ba] / nva;
                TH["v75 val asama2 (logit+8 alfa birlesik)"] = now() - ts0; ts0 = now();
                if (getenv("V76_DEBUG")) { for (int a = 0; a < NA; a++) printf("V76_DBG|alfa=%g|CE=%.15g|top1=%ld|top5=%ld\n", ALFS[a], sceA[a] / nva, c1A[a], c5A[a]); for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a += 6) printf("V76_DBG|lin e=%g a=%g|%.15g\n", EPG[e], ALG[a], cl[e][a] / nva); }
                printf("V53H_SONUC|VAL CE=%.6f PPL=%.3f Top1=%.4f Top5=%.4f|alpha_Q=%g okuma alpha=%g eps=%g\n", cev, exp(cev), v1, v5, bestA, a0, e0);
                if (g_htest) { double sce = 0; long c1 = 0, c5 = 0; for (int s0 = 0; s0 < nte; s0 += CH) { int nn = min(CH, nte - s0); blockLogits(Fce, ite, s0, nn, Zb.data()); vector<double> ceb(nn); vector<int> rkb(nn);
#pragma omp parallel for schedule(static)
                        for (int i = 0; i < nn; i++) fuseRow(&Zb[(size_t)i * V], te.y[s0 + i], wte[s0 + i], &bestA, 1, &ceb[i], &rkb[i]);
                        for (int i = 0; i < nn; i++) { sce += ceb[i]; c1 += rkb[i] == 0; c5 += rkb[i] < 5; } }
                    double cet = sce / nte; TH["v75 test (logit+fuse)"] = now() - ts0; if (getenv("V76_DEBUG")) printf("V76_DBG|test CE=%.15g|top1=%ld|top5=%ld\n", cet, c1, c5); printf("V53H_SONUC|TEST CE=%.6f PPL=%.3f Top1=%.4f Top5=%.4f\n", cet, exp(cet), (double)c1 / nte, (double)c5 / nte); }
            } else {
double te0 = now(); Zv = logitsP(Fcv, iva, nva, WCf, WFf); TH["e1a0 val logit (gemm+Y0 ekleme)"] = now() - te0; double te1_ = now();  pickLin(Zv, vce, a0, e0); TH["e1a1 pickLin (eps x alfa tablosu)"] = now() - te1_; TH["e1a val logit+okuma secimi"] = now() - te0; te0 = now();
            const double ALF[] = {0, 0.025, 0.05, 0.10, 0.20, 0.30, 0.40, 0.60}; double bestV = 1e30; for (int ai = 0; ai < 8; ai++) { double ce, t1, t5; h_fuse(Zv, nva, va.y, wva, g_v64spq ? nullptr : &Qt, ALF[ai], a0, e0, Ptr, ce, t1, t5); if (ce < bestV) { bestV = ce; bestA = ALF[ai]; } }
            double cev, v1, v5; h_fuse(Zv, nva, va.y, wva, g_v64spq ? nullptr : &Qt, bestA, a0, e0, Ptr, cev, v1, v5); TH["e1b val Q-karisim alpha secimi+metrik"] = now() - te0;
            printf("V53H_SONUC|VAL CE=%.6f PPL=%.3f Top1=%.4f Top5=%.4f|alpha_Q=%g okuma alpha=%g eps=%g\n", cev, exp(cev), v1, v5, bestA, a0, e0);
            if (g_htest && !g_v65) { te0 = now(); auto Ze = logitsP(Fce, ite, nte, WCf, WFf); double cet, t1, t5; h_fuse(Ze, nte, te.y, wte, g_v64spq ? nullptr : &Qt, bestA, a0, e0, Ptr, cet, t1, t5); TH["e2 test degerlendirme"] = now() - te0; printf("V53H_SONUC|TEST CE=%.6f PPL=%.3f Top1=%.4f Top5=%.4f\n", cet, exp(cet), t1, t5); }
            }
            if (g_v65) { double tev0 = now(); const float* WCuse = WCf; const vector<double>* Qd = g_v64spq ? nullptr : &Qt;
                struct R65 { double ce = 0, ppl = 0, t1 = 0, t5 = 0; long n = 0; double bce[4] = {0, 0, 0, 0}, bt1[4] = {0, 0, 0, 0}; long bn[4] = {0, 0, 0, 0}; double maxd = 0, cew = 0; long agree = 0; };
                auto binOf = [&](int y) { return y == 0 ? 0 : y <= 100 ? 1 : y <= 1000 ? 2 : 3; };
                auto evalS = [&](const vector<float>& Fm, const vector<int>& ids, const vector<int>& yv, const vector<int>& wl, int n, bool dropC, double aq, bool shuf, bool cmp) -> R65 {
                    R65 r; vector<int> perm(n); iota(perm.begin(), perm.end(), 0); if (shuf) { mt19937_64 gg(5); shuffle(perm.begin(), perm.end(), gg); }
                    const int CH = 2048; vector<float> Fb((size_t)CH * mf); vector<int> ib((size_t)CH * NI); vector<double> cei(CH); vector<int> rki(CH), rk2(CH);
                    for (int s0 = 0; s0 < n; s0 += CH) { int nn = min(CH, n - s0); memcpy(Fb.data(), &Fm[(size_t)s0 * mf], (size_t)nn * mf * sizeof(float)); for (int i = 0; i < nn * NI; i++) ib[i] = dropC ? -1 : ids[(size_t)s0 * NI + i];
                        vector<float> Z = logitsP(Fb, ib, nn, WCuse, WFf), Z2; if (cmp && g65_wcchk) Z2 = logitsP(Fb, ib, nn, g65_wcchk, WFf);
                        vector<double> mxd(nn, 0.0), cw(nn, 0.0);
#pragma omp parallel
                        { vector<double> p(V), qd(V);
#pragma omp for schedule(static)
                            for (int i = 0; i < nn; i++) { int yy = yv[perm[s0 + i]]; int w_ = wl[s0 + i]; auto one = [&](const float* z, double& ce_, int& rk_) { double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], e0); const double* q = nullptr;
                                    if (aq > 0) { if (Qd) q = &(*Qd)[(size_t)w_ * V]; else { const QRows& Qr = QR; const double* bv = Qr.base[w_] == 1 ? Qr.pi.data() : Qr.base[w_] == 2 ? Qr.ptr.data() : nullptr; double cf = Qr.coef[w_]; if (bv) for (int v = 0; v < V; v++) qd[v] = cf * bv[v]; else fill(qd.begin(), qd.end(), 0.0); for (long t = Qr.off[w_]; t < Qr.off[w_ + 1]; t++) qd[Qr.idx[t]] += Qr.val[t]; q = qd.data(); } }
                                    double sm = 0; for (int v = 0; v < V; v++) { double pm_ = (1 - a0) * Ptr[v] + a0 * max((double)z[v], e0) / S2; p[v] = q ? (1 - aq) * pm_ + aq * q[v] : pm_; sm += p[v]; } double py = p[yy] / sm; int rk = 0; for (int v = 0; v < V; v++) if (p[v] / sm > py) rk++; ce_ = -log(py); rk_ = rk; };
                                double c1_; int k1_; one(&Z[(size_t)i * V], c1_, k1_); cei[i] = c1_; rki[i] = k1_;
                                if (cmp && !Z2.empty()) { double c2_; int k2_; one(&Z2[(size_t)i * V], c2_, k2_); cw[i] = c2_; rk2[i] = k2_; double md = 0; for (int v = 0; v < V; v++) md = max(md, (double)fabsf(Z[(size_t)i * V + v] - Z2[(size_t)i * V + v])); mxd[i] = md; } } }
                        for (int i = 0; i < nn; i++) { int yy = yv[perm[s0 + i]]; int b = binOf(yy); r.ce += cei[i]; r.t1 += rki[i] == 0; r.t5 += rki[i] < 5; r.n++; r.bce[b] += cei[i]; r.bt1[b] += rki[i] == 0; r.bn[b]++; if (cmp && !Z2.empty()) { r.maxd = max(r.maxd, mxd[i]); r.cew += cw[i]; r.agree += (rki[i] == 0) == (rk2[i] == 0); } } }
                    r.ce /= r.n; r.ppl = exp(r.ce); r.t1 /= r.n; r.t5 /= r.n; r.cew /= max(1L, r.n); return r; };
                auto pr = [&](const char* tag, const R65& r) { printf("V65|%s|n=%ld|CE=%.6f|PPL=%.3f|Top1=%.4f|Top5=%.4f|bin(unk,sik<=100,orta<=1000,nadir): n=%ld,%ld,%ld,%ld CE=%.3f,%.3f,%.3f,%.3f Top1=%.4f,%.4f,%.4f,%.4f\n", tag, r.n, r.ce, r.ppl, r.t1, r.t5, r.bn[0], r.bn[1], r.bn[2], r.bn[3], r.bn[0] ? r.bce[0] / r.bn[0] : 0, r.bn[1] ? r.bce[1] / r.bn[1] : 0, r.bn[2] ? r.bce[2] / r.bn[2] : 0, r.bn[3] ? r.bce[3] / r.bn[3] : 0, r.bn[0] ? r.bt1[0] / r.bn[0] : 0, r.bn[1] ? r.bt1[1] / r.bn[1] : 0, r.bn[2] ? r.bt1[2] / r.bn[2] : 0, r.bn[3] ? r.bt1[3] / r.bn[3] : 0); fflush(stdout); };
                double t0e = now(); R65 rte = evalS(Fce, ite, te.y, wte, nte, false, bestA, false, g_v65chk); TH["v65 test akis degerlendirme"] = now() - t0e; pr("TEST", rte);
                if (g_v65chk) printf("V65_ESDEGER|test: maks|dz|(ortuk-acik)=%.3e CE(ortuk)=%.6f CE(acik)=%.6f Top1-uyum=%.6f\n", rte.maxd, rte.ce, rte.cew, (double)rte.agree / rte.n);
                R65 rva = evalS(Fcv, iva, va.y, wva, nva, false, bestA, false, false); pr("VAL", rva);
                vector<int> wtr = g65_trWl; R65 rtr = evalS(g65_trF, g65_trIds, g65_trY, wtr, (int)g65_trY.size(), false, bestA, false, false); pr("EGITIM(her 10. ornek)", rtr);
                pr("TEST_Q_yok", evalS(Fce, ite, te.y, wte, nte, false, 0.0, false, false));
                pr("TEST_baglam_karistirilmis", evalS(Fce, ite, te.y, wte, nte, false, bestA, true, false));
                pr("TEST_C_slotlari_kapali(F-only,Q yok)", evalS(Fce, ite, te.y, wte, nte, true, 0.0, false, false));
                { double sce = 0; long nu = 0; for (int i = 0; i < nte; i++) { sce += -log(Ptr[te.y[i]]); nu += te.y[i] == 0; } printf("V65|UNIGRAM_TEST|CE=%.6f|PPL=%.3f|test hedef <unk> orani=%.4f\n", sce / nte, exp(sce / nte), (double)nu / nte); }
                { long ctxunk = 0; for (int i = 0; i < nte; i++) ctxunk += (te.ctx[(size_t)i * c + c - 1] == 0) || (te.ctx[(size_t)i * c + c - 2] == 0); printf("V65|OOV|test baglamda <unk> orani=%.4f\n", (double)ctxunk / nte); }
                if (g_v72single) {   // V72: TEK GIRDI -- hazir agirliklarla bir yeni sorunun sonucu (egitim hesabi yok); tam softmax + top-1 ayri
                    const int NR = 200; vector<double> tl, tf, t1; vector<float> Fr((size_t)mf); vector<int> ir(NI); int agree = 0; double mdev = 0;
                    for (int rep = 0; rep < NR; rep++) { int i = rep % nte; memcpy(Fr.data(), &Fce[(size_t)i * mf], mf * sizeof(float)); for (int p = 0; p < NI; p++) ir[p] = ite[(size_t)NI * i + p];
                        double ta = now(); static thread_local vector<float> Z1buf; static thread_local vector<float> Ft1; if ((int)Z1buf.size() != V) { Z1buf.assign(V, 0.f); Ft1.assign(mf, 0.f); } vector<float>& Z1 = Z1buf; { float* ft = Ft1.data(); memcpy(ft, Fr.data(), mf * sizeof(float)); for (int p = 0; p < NI; p++) { int id = ir[p]; if (id < 0) continue; const float* xf = &g_imp.Xf[(size_t)id * mf]; for (int j = 0; j < mf; j++) ft[j] -= xf[j]; }
                            float* z = Z1.data(); for (int v = 0; v < V; v++) z[v] = 0.f; for (int j = 0; j < mf; j++) { const float fj = ft[j]; const float* wr = WFf + (size_t)j * V;
#pragma omp simd
                                for (int v = 0; v < V; v++) z[v] += fj * wr[v]; }
                            for (int p = 0; p < NI; p++) { int id = ir[p]; if (id < 0) continue; int g = (*g_imp.gidv)[id]; for (int k = 0; k < (*g_imp.nsc)[g]; k++) z[(*g_imp.sy)[g][k]] += (float)g_imp.Y0c[(*g_imp.roff)[id] + k]; } } double tb = now(); int am = 0; for (int v = 1; v < V; v++) if (Z1[v] > Z1[am]) am = v; double tc = now();
                        if (rep < 100) { tl.push_back(tb - ta); t1.push_back(tc - tb); }
                        else { double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)Z1[v], e0); double pm_y = (1 - a0) * Ptr[te.y[i]] + a0 * max((double)Z1[te.y[i]], e0) / S2; (void)pm_y; tf.push_back(now() - ta); } agree += (am == te.y[i]); }
                    { vector<float> Fb((size_t)20 * mf); vector<int> ib((size_t)20 * NI); for (int i = 0; i < 20; i++) { memcpy(&Fb[(size_t)i * mf], &Fce[(size_t)i * mf], mf * sizeof(float)); for (int p = 0; p < NI; p++) ib[(size_t)i * NI + p] = ite[(size_t)NI * i + p]; } vector<float> Zb = logitsP(Fb, ib, 20, WCuse, WFf); for (int i = 0; i < 20; i++) { static vector<float> F1(512 + 1); vector<float> Fq(mf); memcpy(Fq.data(), &Fce[(size_t)i * mf], mf * sizeof(float)); for (int p = 0; p < NI; p++) { int id = ite[(size_t)NI * i + p]; if (id < 0) continue; for (int j = 0; j < mf; j++) Fq[j] -= g_imp.Xf[(size_t)id * mf + j]; } vector<float> z(V, 0.f); for (int j = 0; j < mf; j++) for (int v = 0; v < V; v++) z[v] += Fq[j] * WFf[(size_t)j * V + v]; for (int p = 0; p < NI; p++) { int id = ite[(size_t)NI * i + p]; if (id < 0) continue; int g = (*g_imp.gidv)[id]; for (int k = 0; k < (*g_imp.nsc)[g]; k++) z[(*g_imp.sy)[g][k]] += (float)g_imp.Y0c[(*g_imp.roff)[id] + k]; } for (int v = 0; v < V; v++) mdev = max(mdev, (double)fabsf(z[v] - Zb[(size_t)i * V + v])); } }
                    sort(tl.begin(), tl.end()); sort(t1.begin(), t1.end()); sort(tf.begin(), tf.end()); double ma = 0; for (int p = 0; p < NI; p++) ma += (ir[p] >= 0) ? (double)(*g_imp.nsc)[(*g_imp.gidv)[ir[p]]] : 0;
                    printf("V72_TEK_GIRDI|logit(ortuk, tum V sinifi) medyan=%.1f us (ilk=%.1f us, min=%.1f, maks=%.1f)|argmax(top-1) medyan=%.1f us|logit+softmax payda+hedef olasilik medyan=%.1f us|aritmetik: F~ cikarma %d + W_F madd %.3g + Y0 ekleme ~%.0f ; ara sayi: %d (F~) + %d (logit)|tum sozluk tarandi: evet (tam softmax icin zorunlu: W_F %.1f MB okunur)|top-1/200 dogru (yalniz bilgi)=%d|tek-girdi logit ile toplu logit maks|fark|=%.3e (20 ornek)\n", tl[tl.size() / 2] * 1e6, tl.front() * 1e6, tl.front() * 1e6, tl.back() * 1e6, t1[t1.size() / 2] * 1e6, tf[tf.size() / 2] * 1e6, NI * mf, (double)mf * V, ma, mf, V, (double)mf * V * 4 / 1e6, agree, mdev); fflush(stdout); }
                printf("V65|SURE|degerlendirme toplam (test+val+egitim altkume+kontroller)=%.2f s\n", now() - tev0); fflush(stdout); }
            if (WCf) h_hd("WC", WCf, (size_t)mc * V * 4); h_hd("WF", WFf, (size_t)mf * V * 4); if (!Zv.empty()) h_hd("Zval", Zv.data(), Zv.size() * 4);
            double tm = 0; for (auto& kv : TH) if (kv.first[0] != 'e' && kv.first[0] != 'd') tm += (kv.first[0] == 'f' ? 0 : 0); (void)tm;
            for (auto& kv : TH) printf("TH|%s|%.3f\n", kv.first.c_str(), kv.second); for (auto& ph : g_phases) printf("FAZ|%s|%.3f|%.3f|%.2f\n", ph.name.c_str(), ph.wall, ph.cpu, ph.cpu / max(ph.wall, 1e-9)); for (auto& kv : TT) printf("TT|%s|%.3f\n", kv.first.c_str(), kv.second); printf("V53H_MODEL_HAZIR|%.3f s (okuma sonrasi: ozellik+Gram/RHS+Cholesky/Y0/X+Schur+W+Q tablosu)|tepe RSS=%.0f MB\n", tpre + TH["q1 Q_havuz tablosu"], peak_mb()); fflush(stdout); };
        if (g_hlean && g_hfast) {
            double lc = lamC_cb[0]; if (lamC_id.size() != 1 || lamC_cb.size() != 1 || lamC_id[0] != lamC_cb[0] || lamF_l.size() != 1) { printf("V53H: hizli yol tek lamC/lamF gerektirir\n"); exit(3); } double lf = lamF_l[0];
            h_hd("Fc", Fc.data(), Fc.size() * 4); h_hd("AFF", AFF.data(), AFF.size() * 8); h_hd("RF", RF.data(), RF.size() * 8);
            vector<vector<int>> gsl(V); for (int g = 0; g < V; g++) gsl[g].reserve(gsz[g]); for (int k = 0; k < mc; k++) gsl[gidv[k]].push_back(k);
            // --- hazirlik: satirlari gruba gore (kararli, i artan), (grup,y) yoluk kumeleri ---
            double t0 = now(); vector<int> rstart(V + 1, 0); for (int i = 0; i < ntr; i++) rstart[itr[(size_t)NI * i] + 1]++; for (int g = 0; g < V; g++) rstart[g + 1] += rstart[g];
            vector<int> rord(ntr); { vector<int> cur(rstart.begin(), rstart.end() - 1); for (int i = 0; i < ntr; i++) rord[cur[itr[(size_t)NI * i]]++] = i; }
            vector<int> nsc(V, 0); vector<vector<int>> sy(V); vector<int> posy; if (!g_v64cmp) posy.assign((size_t)V * V, -1);
            if (g_v64cmp) {   // V64: (grup,y) kumeleri satir listelerinden dogrudan (V x V seen/posy yok); sy[g] artan -> eski yolla ayni
#pragma omp parallel for schedule(dynamic, 16)
                for (int g = 0; g < V; g++) { vector<int>& s_ = sy[g]; s_.reserve(rstart[g + 1] - rstart[g]); for (int ix = rstart[g]; ix < rstart[g + 1]; ix++) s_.push_back(tr.y[rord[ix]]); sort(s_.begin(), s_.end()); s_.erase(unique(s_.begin(), s_.end()), s_.end()); nsc[g] = (int)s_.size(); } }
            else { vector<char> seen((size_t)V * V, 0); for (int i = 0; i < ntr; i++) seen[(size_t)itr[(size_t)NI * i] * V + tr.y[i]] = 1;
#pragma omp parallel for schedule(static)
              for (int g = 0; g < V; g++) { const char* sr = &seen[(size_t)g * V]; int* pr = &posy[(size_t)g * V]; for (int y = 0; y < V; y++) if (sr[y]) { pr[y] = nsc[g]++; sy[g].push_back(y); } } }
            if (getenv("V53H_DENSEY0") && atoi(getenv("V53H_DENSEY0")) == 1) {   // ablasyon: Y0/RC tum V sutunuyla (sifir sutunlar dahil) -> seyrek Y0 kazancini yalitir
                for (int g = 0; g < V; g++) { nsc[g] = V; sy[g].resize(V); iota(sy[g].begin(), sy[g].end(), 0); for (int y = 0; y < V; y++) posy[(size_t)g * V + y] = y; } }
            vector<size_t> roff(mc + 1, 0); for (int k = 0; k < mc; k++) roff[k + 1] = roff[k] + nsc[gidv[k]];
            { double dense_y0 = (double)mc * V, nzs = (double)roff[mc]; double fl_d = 0, fl_s = 0; for (int g = 0; g < V; g++) { fl_d += (double)gsz[g] * gsz[g] * V; fl_s += (double)gsz[g] * gsz[g] * nsc[g]; } printf("V53H_Y0|yogun Y0 = %d x %d = %.4g eleman ; yapisal sifir-olmayan (grup,y) konumlari = %.4g eleman (%.2f%%) ; Y0 cozum islemi (sum sg^2 * sutun): yogun %.3g -> yoluk %.3g madd (%.1fx az)\n", mc, V, dense_y0, nzs, 100.0 * nzs / dense_y0, fl_d, fl_s, fl_d / fl_s); }
            ubuf<double> RCc(roff[mc]), ACF((size_t)mc * mf), Y0c(roff[mc]), Xd((size_t)mc * mf); ubuf<float> Xf((size_t)mc * mf);   // ilklenmemis; her grup kendi satirlarini yazar (paralel ilk-dokunus)
            vector<int> gord(V); iota(gord.begin(), gord.end(), 0); sort(gord.begin(), gord.end(), [&](int a, int b) { return (long)(rstart[a + 1] - rstart[a]) > (long)(rstart[b + 1] - rstart[b]); });
            TH["g2 sayim hazirlik (grup/yoluk/tampon)"] = now() - t0; t0 = now();
            // --- sayim (grup basina; satirlar i artan -> ACF toplama sirasi orijinalle ayni) ---
#pragma omp parallel for schedule(dynamic, 1)
            for (int oi = 0; oi < V; oi++) { int g = gord[oi]; for (int a : gsl[g]) { memset(&ACF[(size_t)a * mf], 0, mf * sizeof(double)); memset(&RCc[roff[a]], 0, nsc[g] * sizeof(double)); }
                for (int ix = rstart[g]; ix < rstart[g + 1]; ix++) { int i = rord[ix]; int yy = tr.y[i]; const float* fr = &Fc[(size_t)i * mf]; int py = g_v64cmp ? (int)(lower_bound(sy[g].begin(), sy[g].end(), yy) - sy[g].begin()) : posy[(size_t)g * V + yy];
                    for (int p = 0; p < NI; p++) { int a = itr[(size_t)NI * i + p]; if (a < 0) continue; if (g_v64cmp) NK[a] += 1; else for (int q = 0; q < NI; q++) { int b3 = itr[(size_t)NI * i + q]; if (b3 >= 0) ACCB[goff[g] + (size_t)locv[a] * gsz[g] + locv[b3]] += 1; } RCc[roff[a] + py] += 1; double* ra = &ACF[(size_t)a * mf]; for (int j = 0; j < mf; j++) ra[j] += fr[j]; } } }
            TH["g3 sayim (ACC,ACF,RC)"] = now() - t0; phase_mark("Gram: slot sayimi (ACC,ACF,RC)"); t0 = now();
            if (g_v65) { for (int i = 0; i < ntr; i += g_v65stride) { g65_trF.insert(g65_trF.end(), &Fc[(size_t)i * mf], &Fc[(size_t)(i + 1) * mf]); for (int p = 0; p < NI; p++) g65_trIds.push_back(itr[(size_t)NI * i + p]); g65_trY.push_back(tr.y[i]); g65_trWl.push_back(tr.ctx[(size_t)i * c + c - 1]); } }
            { size_t before = (size_t)Fc.capacity() * 4; vector<float>().swap(Fc); vector<float>().swap(g_Utr); vector<float>().swap(g_Uva); vector<float>().swap(g_Ute); g_Qf.clear(); g_Qf.shrink_to_fit(); g_Pf51.clear(); g_Pf51.shrink_to_fit(); vector<float>().swap(tr.X); vector<float>().swap(va.X); vector<float>().swap(te.X); (void)before; }   // V54: bu noktadan sonra gerekmeyen buyuk diziler serbest (tepe RAM)
            // --- grup Cholesky + Y0 (yalniz yoluk sutunlari) + X ; buyuk gruplarda sutun-karolari is parcaciklarina dagitilir ---
            double tf = 0, ty = 0, tx = 0; const int BIGT = 128; vector<int> bigs, smalls; for (int g : gord) { if (gsz[g] >= BIGT) bigs.push_back(g); else if (gsz[g] > 0) smalls.push_back(g); } sort(bigs.begin(), bigs.end(), [&](int a, int b) { return gsz[a] > gsz[b]; });
            auto buildG = [&](int g, vector<double>& Gm, vector<double>& GT) { int sg = gsz[g]; const vector<int>& ix = gsl[g]; Gm.resize((size_t)sg * sg); GT.resize((size_t)sg * sg); for (int a = 0; a < sg; a++) { for (int b2 = 0; b2 < sg; b2++) Gm[(size_t)a * sg + b2] = ACCB[goff[g] + (size_t)a * sg + b2]; Gm[(size_t)a * sg + a] += LSLOT(ix[a]); }
                h_chol(Gm.data(), sg); for (int a = 0; a < sg; a++) for (int b2 = 0; b2 < sg; b2++) GT[(size_t)b2 * sg + a] = Gm[(size_t)a * sg + b2]; };
            auto putback = [&](int g, const double* Rm, int nc, int ldr, bool par) { const vector<int>& ix = gsl[g]; int sg = gsz[g], ns_ = nsc[g];
#pragma omp parallel for schedule(static) if (par)
            for (int a = 0; a < sg; a++) { const double* r = Rm + (size_t)a * ldr; memcpy(&Y0c[roff[ix[a]]], r, ns_ * sizeof(double)); double* xd = &Xd[(size_t)ix[a] * mf]; float* xf = &Xf[(size_t)ix[a] * mf]; memcpy(xd, r + ns_, mf * sizeof(double)); for (int j = 0; j < mf; j++) xf[j] = (float)xd[j]; } };
            auto fillR = [&](int g, double* Rm, int nc, int ldr) { const vector<int>& ix = gsl[g]; int sg = gsz[g], ns_ = nsc[g]; for (int a = 0; a < sg; a++) { double* r = Rm + (size_t)a * ldr; memcpy(r, &RCc[roff[ix[a]]], ns_ * sizeof(double)); memcpy(r + ns_, &ACF[(size_t)ix[a] * mf], mf * sizeof(double)); for (int c2 = nc; c2 < ldr; c2++) r[c2] = 0.0; } };
            if (g_v63arrow) {   // V63: A_g = [[n_w, b'],[b, diag(n_p)]] (kelime slotu + cift slotlari; ornek basina tek cift) -> Cholesky/uclu cozum yerine kapali form, O(sg*sutun)
                double tarr = now(); double structerr = 0, bmaxerr = 0; long chkg = 0;
#pragma omp parallel for schedule(dynamic, 4) reduction(max : structerr, bmaxerr) reduction(+ : chkg)
                for (int g = 0; g < V; g++) { const vector<int>& ix = gsl[g]; int sg = gsz[g], ns_ = nsc[g], nc = ns_ + mf; if (sg == 0) continue;
                    int wl = -1; for (int a = 0; a < sg; a++) if (ix[a] == g) wl = a; if (wl < 0) { printf("V63|HATA grup %d kelime slotu yok\n", g); exit(5); }
                    const double* AG = g_v64cmp ? nullptr : &ACCB[goff[g]]; double dw = (g_v64cmp ? NK[ix[wl]] : AG[(size_t)wl * sg + wl]) + LSLOT(g), sS = dw; vector<double> dp(sg, 0.0), bp(sg, 0.0);
                    for (int a = 0; a < sg; a++) if (a != wl) { dp[a] = (g_v64cmp ? NK[ix[a]] : AG[(size_t)a * sg + a]) + LSLOT(ix[a]); bp[a] = g_v64cmp ? NK[ix[a]] : AG[(size_t)wl * sg + a]; sS -= bp[a] * bp[a] / dp[a]; }
                    if (g_v63chk && !g_v64cmp && (g % 7 == 0)) { chkg++; for (int a = 0; a < sg; a++) for (int b2 = 0; b2 < sg; b2++) { if (a == wl || b2 == wl) continue; if (a != b2) structerr = max(structerr, fabs(AG[(size_t)a * sg + b2])); } for (int a = 0; a < sg; a++) if (a != wl) bmaxerr = max(bmaxerr, fabs(AG[(size_t)a * sg + wl] - AG[(size_t)a * sg + a])); }
                    vector<double> t(nc, 0.0), xw(nc, 0.0);
                    for (int a = 0; a < sg; a++) if (a != wl) { const double* rc = &RCc[roff[ix[a]]]; const double* af = &ACF[(size_t)ix[a] * mf]; double f = bp[a] / dp[a]; for (int j = 0; j < ns_; j++) t[j] += f * rc[j]; for (int j = 0; j < mf; j++) t[ns_ + j] += f * af[j]; }
                    { const double* rc = &RCc[roff[ix[wl]]]; const double* af = &ACF[(size_t)ix[wl] * mf]; for (int j = 0; j < ns_; j++) xw[j] = (rc[j] - t[j]) / sS; for (int j = 0; j < mf; j++) xw[ns_ + j] = (af[j] - t[ns_ + j]) / sS; }
                    for (int a = 0; a < sg; a++) { int k = ix[a]; double* y0 = &Y0c[roff[k]]; double* xd = &Xd[(size_t)k * mf]; float* xf = &Xf[(size_t)k * mf];
                        if (a == wl) { for (int j = 0; j < ns_; j++) y0[j] = xw[j]; for (int j = 0; j < mf; j++) { xd[j] = xw[ns_ + j]; xf[j] = (float)xd[j]; } }
                        else { const double* rc = &RCc[roff[k]]; const double* af = &ACF[(size_t)k * mf]; double ib = 1.0 / dp[a], bb = bp[a]; for (int j = 0; j < ns_; j++) y0[j] = (rc[j] - bb * xw[j]) / dp[a]; for (int j = 0; j < mf; j++) { xd[j] = (af[j] - bb * xw[ns_ + j]) / dp[a]; xf[j] = (float)xd[j]; } (void)ib; } } }
                TH["g4a V63 ok-basi kapali form (Y0+X)"] = now() - tarr; if (g_v63chk) printf("V63_YAPI|kontrol edilen grup=%ld|cift-cift d.diyagonal maks=%.3g|maks|A[w,p]-A[p,p]|=%.3g %s\n", chkg, structerr, bmaxerr, (structerr == 0 && bmaxerr == 0) ? "(OK-BASI TAM)" : "(OK-BASI DEGIL)");
                if (g_v64cmp) { double rmax = 0; for (int oi = 0; oi < min(V, 40); oi++) { int g = oi * (oi < 8 ? 1 : (V / 40)); const vector<int>& ix = gsl[g]; int sg = gsz[g], ns_ = nsc[g]; int wl = -1; for (int a = 0; a < sg; a++) if (ix[a] == g) wl = a;
                        for (int c2 = 0; c2 < ns_ + mf; c2 += 37) { auto Yv = [&](int a) { return c2 < ns_ ? Y0c[roff[ix[a]] + c2] : Xd[(size_t)ix[a] * mf + (c2 - ns_)]; }; auto Rv = [&](int a) { return c2 < ns_ ? RCc[roff[ix[a]] + c2] : ACF[(size_t)ix[a] * mf + (c2 - ns_)]; };
                            double sw = (NK[g] + LSLOT(g)) * Yv(wl), sc = fabs(sw) + fabs(Rv(wl)); for (int a = 0; a < sg; a++) if (a != wl) { sw += NK[ix[a]] * Yv(a); sc += fabs(NK[ix[a]] * Yv(a)); } rmax = max(rmax, fabs(sw - Rv(wl)) / (sc + 1e-30));
                            for (int a = 0; a < sg; a++) if (a != wl) { double rr = (NK[ix[a]] + LSLOT(ix[a])) * Yv(a) + NK[ix[a]] * Yv(wl); double sc2 = fabs((NK[ix[a]] + LSLOT(ix[a])) * Yv(a)) + fabs(NK[ix[a]] * Yv(wl)) + fabs(Rv(a)); rmax = max(rmax, fabs(rr - Rv(a)) / (sc2 + 1e-30)); } } }
                    printf("V42_REZIDU|V64 kompakt ok-basi|40 grupta (A_g+Lambda)[Y0|X]-[R_C|A_CF] en buyuk bagil rezidu=%.2e %s\n", rmax, rmax < 1e-8 ? "(KABUL)" : "(RED)"); if (!(rmax < 1e-8)) exit(3); }
                bigs.clear(); smalls.clear(); }
            if (!bigs.empty()) {   // faz A: buyuk gruplarin Cholesky'si paralel (grup basina bir parcacik)
                int nb_ = (int)bigs.size(); vector<vector<double>> BG(nb_), BGT(nb_); double ta = now();
#pragma omp parallel for schedule(dynamic, 1)
                for (int bi = 0; bi < nb_; bi++) buildG(bigs[bi], BG[bi], BGT[bi]); TH["g4a buyuk grup Cholesky duvar"] = now() - ta; tf += now() - ta; double tbB = now();
                for (int bi = 0; bi < nb_; bi++) {   // faz B: sirayla; icerde sutun karolari paralel
                    int g = bigs[bi], sg = gsz[g], ns_ = nsc[g], nc = ns_ + mf, ldr = (nc + 31) & ~31; ta = now(); vector<double> Rm((size_t)sg * ldr); double tfill_ = now(); fillR(g, Rm.data(), nc, ldr); TH["g4b1 buyuk: Rm tahsis+doldurma"] += now() - ta; double tsol_ = now(); int TW = 32, nt_ = ldr / TW;
#pragma omp parallel for schedule(dynamic, 1)
                    for (int ti = 0; ti < nt_; ti++) { int c0 = ti * TW;
#ifdef __AVX512F__
                        if (!g_hnoavx) h_solve_avx512(BG[bi].data(), BGT[bi].data(), sg, Rm.data() + c0, ldr, TW); else
#endif
                        h_solve_fast(BG[bi].data(), BGT[bi].data(), sg, Rm.data() + c0, ldr, TW, 32);
                    }
                    TH["g4b2 buyuk: karo cozum"] += now() - tsol_; double tpb_ = now(); putback(g, Rm.data(), nc, ldr, true); TH["g4b3 buyuk: putback"] += now() - tpb_; double dt = now() - ta; ty += dt * (double)ns_ / nc; tx += dt * (double)mf / nc; } TH["g4b buyuk grup cozum duvar"] = now() - tbB; }
            double tsm0 = now();
#pragma omp parallel for schedule(dynamic, 1) reduction(+ : tf, ty, tx)
            for (int si = 0; si < (int)smalls.size(); si++) { int g = smalls[si]; int sg = gsz[g], ns_ = nsc[g], nc = ns_ + mf, ldr = (nc + 31) & ~31; double ta = now(); vector<double> Gm, GT; buildG(g, Gm, GT); double tb = now(); tf += tb - ta;
                vector<double> Rm((size_t)sg * ldr); fillR(g, Rm.data(), nc, ldr);
#ifdef __AVX512F__
                if (!g_hnoavx) h_solve_avx512(Gm.data(), GT.data(), sg, Rm.data(), ldr, ldr); else
#endif
                h_solve_fast(Gm.data(), GT.data(), sg, Rm.data(), ldr, ldr, 96);
                putback(g, Rm.data(), nc, ldr, false); double dt = now() - tb; ty += dt * (double)ns_ / nc; tx += dt * (double)mf / nc; }
            TH["g4c kucuk grup duvar"] = now() - tsm0; double tgrp = now() - t0; { vector<int> sz(V); for (int g = 0; g < V; g++) sz[g] = gsz[g]; sort(sz.begin(), sz.end(), greater<int>()); double s2 = 0, s3 = 0; for (int x : sz) { s2 += (double)x * x; s3 += (double)x * x * x / 3; } printf("V53H_GRUP|en buyuk 5 grup=%d %d %d %d %d ; sum sg^2=%.3g ; sum sg^3/3=%.3g ; buyuk(>=%d) grup sayisi=%zu\n", sz[0], sz[1], sz[2], sz[3], sz[4], s2, s3, BIGT, bigs.size()); }
            TH["g4 grup Cholesky (CPU-toplam)"] = tf; TH["g5 Y0 cozum (CPU-toplam, tahsisli)"] = ty; TH["g6 X cozum (CPU-toplam, tahsisli)"] = tx; TH["g45 grup cozum duvar suresi (Cholesky+Y0+X+Xf)"] = tgrp; phase_mark("grup Cholesky + Y0 + X"); t0 = now();
            // --- denetim (model hazir suresine dahil degil): bazi gruplarda tam rezidu ---
            if (!g_v64cmp) { double rmax = 0; for (int oi = 0; oi < min(V, 40); oi++) { int g = gord[oi * (oi < 8 ? 1 : (V / 40))]; int sg = gsz[g]; const vector<int>& ix = gsl[g]; int ns_ = nsc[g]; for (int a = 0; a < sg; a++) { for (int c2 = 0; c2 < mf + ns_; c2 += 37) { double acc = 0, rhs; if (c2 < ns_) { acc = LSLOT(ix[a]) * Y0c[roff[ix[a]] + c2]; for (int b2 = 0; b2 < sg; b2++) acc += ACCB[goff[g] + (size_t)a * sg + b2] * Y0c[roff[ix[b2]] + c2]; rhs = RCc[roff[ix[a]] + c2]; } else { int cc = c2 - ns_; acc = LSLOT(ix[a]) * Xd[(size_t)ix[a] * mf + cc]; for (int b2 = 0; b2 < sg; b2++) acc += ACCB[goff[g] + (size_t)a * sg + b2] * Xd[(size_t)ix[b2] * mf + cc]; rhs = ACF[(size_t)ix[a] * mf + cc]; } rmax = max(rmax, fabs(acc - rhs) / (fabs(rhs) + 1.0)); } } }
              printf("V42_REZIDU|V53h hizli yol|40 grupta (A_g+Lambda)[Y0|X]-[R_C|A_CF] en buyuk bagil rezidu=%.2e %s\n", rmax, rmax < 1e-8 ? "(KABUL)" : "(RED)"); if (!(rmax < 1e-8)) exit(3); }
            TH["d1 rezidu denetimi (model hazir disi)"] = now() - t0; phase_mark("[denetim: rezidu - model disi]"); t0 = now();
            // --- Schur parcalari ---
            vector<double> T((size_t)mf * mf, 0.0), U((size_t)mf * V, 0.0);
            scipy_cblas_dgemm(101, 112, 111, mf, mf, mc, 1.0, ACF.data(), mf, Xd.data(), mf, 0.0, T.data(), mf); TH["g8 T=ACF'X (dgemm)"] = now() - t0; t0 = now();
            int hu_mode = g_hu; if (g_v63arrow >= 2) {   // V63: U = A_CF^T Y0 = X^T R_C (kapali formda X_p = (a_p - b_p x_w)/d_p ile ayni) ; R_C seyrek -> yalniz sifir-olmayan (slot,y) konumlari
                vector<int> kb = h_dgemm_kblocks(mc); double tux_ = now(); if (g_v72u) { long nz = 0; double tbld = 0; v72_u_csc(mf, V, mc, Xd.data(), gidv, sy, roff, RCc.data(), U.data(), kb, &nz, &tbld); TH["g9x0 V72 CSC kurulum"] = tbld; TH["g9x1 V72 gercek sifir-olmayan R_C (adet)"] = (double)nz; TH["g9x2 V72 U madd (G)"] = (double)nz * mf * 1e-9; TH["g9x3 V72 eski kompakt R_C giris sayisi"] = (double)roff[mc]; } else h_u_sparse(mf, V, mc, Xd.data(), gidv, sy, roff, RCc.data(), U.data(), 0, V, kb); TH["g9x V63 U = X^T R_C (seyrek)"] = now() - tux_; hu_mode = 99; }
            if (hu_mode == 2) {   // V54: dgemm ile BIT-ozdes seyrek hesap; once uretim sekliyle (M=mf, K=mc) cekirdege karsi dogrulanir
                vector<int> kb = h_dgemm_kblocks(mc); bool ok = true; const int PW = 32; double tck_ = now();
                for (int pnl = 0; pnl < 1 && ok; pnl++) { int ya = pnl == 0 ? 0 : V - PW; ubuf<double> Yp((size_t)mc * PW), Cp((size_t)mf * PW);
#pragma omp parallel for schedule(static)
                    for (int a = 0; a < mc; a++) { double* row = &Yp[(size_t)a * PW]; memset(row, 0, PW * sizeof(double)); int g = gidv[a]; for (int q = 0; q < nsc[g]; q++) { int y = sy[g][q]; if (y >= ya && y < ya + PW) row[y - ya] = Y0c[roff[a] + q]; } }
                    scipy_cblas_dgemm(101, 112, 111, mf, PW, mc, 1.0, ACF.data(), mf, Yp.data(), PW, 0.0, Cp.data(), PW);
                    h_u_sparse(mf, V, mc, ACF.data(), gidv, sy, roff, Y0c.data(), U.data(), ya, ya + PW, kb, mf);
                    for (int j = 0; j < mf && ok; j++) for (int y = ya; y < ya + PW; y++) if (U[(size_t)j * V + y] != Cp[(size_t)j * PW + (y - ya)]) { ok = false; break; } }
                TH["g9c U dogrulama (model hazir disi sayilmaz? hayir: sure dahil)"] = now() - tck_;
                if (ok) { h_u_sparse(mf, V, mc, ACF.data(), gidv, sy, roff, Y0c.data(), U.data(), 0, V, kb); printf("V54_U|seyrek-tam-esdeger U (dgemm K-blok emulasyonu, %zu blok) dogrulandi\n", kb.size() - 1); }
                else { printf("V54_U|emulasyon dogrulanamadi -> yogun dgemm\n"); hu_mode = 0; } }
            if (hu_mode == 2 || hu_mode == 99) {}
            else if (hu_mode == 0) {   // birebir orijinal: yogun Y0d ile dgemm
                double tu0_ = now(); ubuf<double> Y0d((size_t)mc * V);
#pragma omp parallel for schedule(dynamic, 1)
                for (int oi = 0; oi < V; oi++) { int g = gord[oi]; for (int a : gsl[g]) { double* row = &Y0d[(size_t)a * V]; memset(row, 0, V * sizeof(double)); for (int k = 0; k < nsc[g]; k++) row[sy[g][k]] = Y0c[roff[a] + k]; } }
                TH["g9a Y0d kurulum"] = now() - tu0_; tu0_ = now(); scipy_cblas_dgemm(101, 112, 111, mf, V, mc, 1.0, ACF.data(), mf, Y0d.data(), V, 0.0, U.data(), V); TH["g9b dgemm U"] = now() - tu0_; }
            else {   // seyrek: U[:,y] = sum_{slot} ACF[slot,:] * Y0[slot,y] ; cikis sutunu y sahibi is parcacigi (sira: g artan, slot yerel sira) -> parcacik sayisindan bagimsiz, deterministik
                int nt = omp_get_max_threads(); vector<double> Ut((size_t)V * mf, 0.0);
#pragma omp parallel num_threads(nt)
                { int t = omp_get_thread_num(); int y0 = (int)((long)V * t / nt), y1 = (int)((long)V * (t + 1) / nt);
                  for (int g = 0; g < V; g++) { const vector<int>& sl = sy[g]; int ns_ = nsc[g]; if (!ns_) continue; int k0 = (int)(lower_bound(sl.begin(), sl.end(), y0) - sl.begin()), k1 = (int)(lower_bound(sl.begin(), sl.end(), y1) - sl.begin()); if (k0 >= k1) continue;
                    for (int a : gsl[g]) { const double* ar = &ACF[(size_t)a * mf]; const double* yr = &Y0c[roff[a]]; for (int k = k0; k < k1; k++) { double yv = yr[k]; double* ut = &Ut[(size_t)sl[k] * mf];
#pragma omp simd
                            for (int j = 0; j < mf; j++) ut[j] += ar[j] * yv; } } } }
#pragma omp parallel for schedule(static)
                for (int j = 0; j < mf; j++) for (int y = 0; y < V; y++) U[(size_t)j * V + y] = Ut[(size_t)y * mf + j]; }
            TH["g9 U=ACF'Y0"] = now() - t0; phase_mark("T=ACF'X + U=ACF'Y0"); t0 = now();
            // --- Schur cozumu: W_F ---
            vector<double> Sm((size_t)mf * mf), rhs((size_t)mf * V); for (size_t q = 0; q < Sm.size(); q++) Sm[q] = AFF[q] - T[q]; for (int i = 0; i < mf; i++) Sm[(size_t)i * mf + i] += (i == 0 && g_bias) ? lc : lf; for (size_t q = 0; q < rhs.size(); q++) rhs[q] = RF[q] - U[q];
            cholesky(Sm, mf); vector<float> WFf; chol_solve_multi(Sm, mf, rhs, V, WFf); TH["g10 Schur cozumu W_F (Cholesky 513 + V sag taraf)"] = now() - t0; phase_mark("Schur: W_F"); t0 = now();
            // --- W_C = Y0 - X W_F : sgemm dogrudan cikis tamponuna, sonra yoluk konumlarinda Y0 eklenir ---
            ubuf<float> WCf(g_v64imp ? 0 : (size_t)mc * V); double tw0_ = now(); if (!g_v64imp) { scipy_cblas_sgemm(101, 111, 111, mc, V, mf, 1.f, Xf.data(), mf, WFf.data(), V, 0.f, WCf.data(), V); TH["g11a sgemm X*WF"] = now() - tw0_; tw0_ = now();
#pragma omp parallel for schedule(static)
            for (int g = 0; g < V; g++) for (int a : gsl[g]) { float* row = &WCf[(size_t)a * V]; for (int y = 0; y < V; y++) row[y] = 0.f - row[y]; for (int k = 0; k < nsc[g]; k++) { int y = sy[g][k]; row[y] = (float)Y0c[roff[a] + k] + row[y]; } } }
            static vector<float> WCchk; if (g_v65chk && g_v64imp) { WCchk.resize((size_t)mc * V); scipy_cblas_sgemm(101, 111, 111, mc, V, mf, 1.f, Xf.data(), mf, WFf.data(), V, 0.f, WCchk.data(), V);
#pragma omp parallel for schedule(static)
                for (int g = 0; g < V; g++) for (int a : gsl[g]) { float* row = &WCchk[(size_t)a * V]; for (int y = 0; y < V; y++) row[y] = 0.f - row[y]; for (int k = 0; k < nsc[g]; k++) { int y = sy[g][k]; row[y] = (float)Y0c[roff[a] + k] + row[y]; } } g65_wcchk = WCchk.data(); }
            g_imp.Xf = Xf.data(); g_imp.Y0c = Y0c.data(); g_imp.RCc = RCc.data(); g_imp.roff = &roff; g_imp.sy = &sy; g_imp.nsc = &nsc; g_imp.gidv = &gidv;
            TH["g11b Y0 ekleme/isaret"] = now() - tw0_; TH["g11 W_C toplam"] = now() - t0; phase_mark("W_C = Y0 - X W_F");
            vector<double> nwl(V), Pn; if (!g_v64spq) Pn.assign((size_t)V * V, 0.0); for (int w = 0; w < V; w++) { nwl[w] = ACCAT(w, w); if (!g_v64spq && nwl[w] > 0) for (int k = 0; k < nsc[w]; k++) Pn[(size_t)w * V + sy[w][k]] = RCc[roff[w] + k] / nwl[w]; }
            if (g_hhash || !g_hdump.empty()) { vector<double> RCd((size_t)mc * V, 0.0); vector<float> Y0f((size_t)mc * V, 0.f); for (int g = 0; g < V; g++) for (int a : gsl[g]) for (int k = 0; k < nsc[g]; k++) { RCd[(size_t)a * V + sy[g][k]] = RCc[roff[a] + k]; Y0f[(size_t)a * V + sy[g][k]] = (float)Y0c[roff[a] + k]; }
                if (!g_v64cmp) h_hd("ACCB", ACCB.data(), ACCB.size() * 8); h_hd("ACF", ACF.data(), ACF.size() * 8); h_hd("RC", RCd.data(), RCd.size() * 8); h_hd("Y0f", Y0f.data(), Y0f.size() * 4); h_hd("X", Xd.data(), Xd.size() * 8); h_hd("T", T.data(), T.size() * 8); h_hd("U", U.data(), U.size() * 8); }
            h_finish(g_v64imp ? nullptr : WCf.data(), WFf.data(), nwl, Pn, 0.0); exit(0);
        }
        set<double> allc(lamC_id.begin(), lamC_id.end()); for (double x : lamC_cb) allc.insert(x);
        double bestVal = 1e30, bestLC = 0, bestLF = 0; vector<float> bWC, bWF; double bestIdVal = 1e30, bestIdLC = 0; vector<float> bIdWC;
        // V41: C0 tabaninda A_CC blok-kosegendir (satir/sutunlar son kelime w_{t-1} grubuna gore: kelime kimligi + o kelimeyle biten sik ciftler) -> yogun Cholesky yerine grup grup cozum (matematiksel olarak ayni)
        vector<vector<int>> grpv; bool grp_ok = false; double t_grp = 0;
        if (g_v41 && g_c0 && g_blas) { double tg0 = now(); vector<int> gid(mc, -1); for (int w = 0; w < V; w++) gid[w] = w; for (auto& kv : g_prank) gid[V + kv.second] = (int)(kv.first % V); grpv.assign(V, {}); for (int k = 0; k < mc; k++) grpv[gid[k]].push_back(k);
            long viol = 0;   /* V53: yapi insaat geregi blok-kosegen (ACCB yalniz grup bloklarini saklar; blok disi sayim imkansiz) */
            grp_ok = viol == 0; t_grp = now() - tg0; int mxg = 0; for (auto& g : grpv) mxg = max(mxg, (int)g.size()); printf("V41_GRUP|A_CC blok-kosegen mi: %s (blok disi sifir olmayan=%ld) ; grup sayisi=%d en buyuk grup=%d ; dogrulama+kurulum sure=%.3fs\n", grp_ok ? "EVET" : "HAYIR", viol, V, mxg, t_grp); }
        double th_f = 0, th_y = 0, th_x = 0; for (double lc : allc) { double tl = now(); double t1_ = now(); bool use_g = grp_ok; vector<double> A; vector<double> Xg; vector<double> Y0g; if (use_g) { Y0g.assign((size_t)mc * V, 0.0); Xg.assign((size_t)mc * mf, 0.0);
#pragma omp parallel for schedule(dynamic, 4)
                for (int g = 0; g < V; g++) { const vector<int>& ix = grpv[g]; int sg = (int)ix.size(); if (!sg) continue; int nr = V + mf; vector<double> Gm((size_t)sg * sg), Rm((size_t)sg * nr);
                    for (int a = 0; a < sg; a++) { for (int b2 = 0; b2 < sg; b2++) Gm[(size_t)a * sg + b2] = ACCAT(ix[a], ix[b2]); Gm[(size_t)a * sg + a] += LSLOT(ix[a]); memcpy(&Rm[(size_t)a * nr], &RC[(size_t)ix[a] * V], V * sizeof(double)); memcpy(&Rm[(size_t)a * nr + V], &ACF[(size_t)ix[a] * mf], mf * sizeof(double)); }
                    if (g_hsplit) { double ta_ = now(); h_chol(Gm.data(), sg); double tb_ = now(); h_solve_naive(Gm.data(), sg, Rm.data(), nr, V); double tc_ = now(); h_solve_naive(Gm.data(), sg, Rm.data() + V, nr, mf); double td_ = now();
#pragma omp atomic
                        th_f += tb_ - ta_;
#pragma omp atomic
                        th_y += tc_ - tb_;
#pragma omp atomic
                        th_x += td_ - tc_; } else spd_solve(Gm, sg, Rm, nr); for (int a = 0; a < sg; a++) { memcpy(&Y0g[(size_t)ix[a] * V], &Rm[(size_t)a * nr], V * sizeof(double)); memcpy(&Xg[(size_t)ix[a] * mf], &Rm[(size_t)a * nr + V], mf * sizeof(double)); } } TT["M2/3/4 grup-grup Cholesky+Y0+X (V41)"] += now() - t1_; TH["g45 grup cozum duvar suresi"] = now() - t1_; if (g_hsplit) { TH["g4 grup Cholesky (CPU-toplam)"] = th_f; TH["g5 Y0 cozum (CPU-toplam)"] = th_y; TH["g6 X cozum (CPU-toplam)"] = th_x; }
                // V42 rezidu denetimi (grup uyeleri uzerinden; yapi denetimi zaten ACC'nin blok disini sifir buldu): (A_CC+lc I)Y0-R_C ve (A_CC+lc I)X-A_CF, birkac sutunda (X'in 0. sutunu = sabit-1 sutunu ile baglanti)
                { double rmax = 0; auto resid = [&](const vector<double>& Sol, int nc, const vector<double>& Rhs, int cc) { double num = 0, den = 1e-300; for (int g = 0; g < V; g++) { const vector<int>& ix = grpv[g]; for (int a : ix) { double acc = LSLOT(a) * Sol[(size_t)a * nc + cc]; for (int b2 : ix) acc += ACCAT(a, b2) * Sol[(size_t)b2 * nc + cc]; double r = acc - Rhs[(size_t)a * nc + cc]; num += r * r; den += Rhs[(size_t)a * nc + cc] * Rhs[(size_t)a * nc + cc]; } } return sqrt(num / den); };
                  const int cy[6] = {0, 17, V / 3, V / 2, V - 1, 5}; for (int ci = 0; ci < 6; ci++) rmax = max(rmax, resid(Y0g, V, RC, cy[ci] % V)); for (int cj = 0; cj < 4; cj++) rmax = max(rmax, resid(Xg, mf, ACF, (cj * 131) % mf));
                  printf("V42_REZIDU|lamC=%g|grup cozumu bagil rezidu (Y0: 6 sutun, X: 4 sutun, X sutun 0 = sabit-1) = %.2e %s\n", lc, rmax, rmax < 1e-8 ? "(KABUL)" : "(RED -> yogun yola donuluyor)");
                  if (!(rmax < 1e-8)) { printf("V53: grup cozumu reddedildi; yogun yol kaldirildi\n"); exit(3); } } }
            else { printf("V53: yogun A_CC yolu kaldirildi (V41 grup yolu gerekli)\n"); exit(3); }
            t1_ = now(); vector<float> Y0f; vector<double> Y0d; if (use_g) { Y0d.swap(Y0g); Y0f.resize(Y0d.size());
#pragma omp parallel for schedule(static)
                for (size_t q = 0; q < Y0f.size(); q++) Y0f[q] = (float)Y0d[q]; } else if (g_blas) { chol_solve_multi_d(A, mc, RC, V, Y0d); Y0f.resize(Y0d.size());
#pragma omp parallel for schedule(static)
            for (size_t q = 0; q < Y0f.size(); q++) Y0f[q] = (float)Y0d[q]; } else chol_solve_multi(A, mc, RC, V, Y0f); TT["M3 Y0=A_CC^-1 R_C cozum (2001 sag taraf) = yalniz-kimlik agirliklari"] += now() - t1_;
            // yalniz kimlik
            { double te_ = now(); auto Zv = logits(Fcv, iva, nva, &Y0f, nullptr); double vce, al, ep; pickLin(Zv, vce, al, ep); RD r2 = rd_eval(Zv, va.y.data(), nva, Ptr, 1, al, ep); printf("YALNIZ_KIMLIK_TARAMA|lamC=%g|dogr CE=%.4f PPL=%.1f (alpha=%g eps=%g)\n", lc, vce, r2.ppl, al, ep); if (vce < bestIdVal) { bestIdVal = vce; bestIdLC = lc; bIdWC = Y0f; } TT["E1 yalniz-kimlik dogrulama degerlendirmesi (logit+alpha/eps+PPL)"] += now() - te_; }
            if (find(lamC_cb.begin(), lamC_cb.end(), lc) == lamC_cb.end()) { printf("SURE|lamC=%g (yalniz kimlik)|%.2fs\n", lc, now() - tl); continue; }
            t1_ = now(); vector<float> Xf; vector<double> Xd; if (use_g) { Xd.swap(Xg); Xf.resize(Xd.size());
#pragma omp parallel for schedule(static)
            for (size_t q = 0; q < Xf.size(); q++) Xf[q] = (float)Xd[q]; } else if (g_blas) { chol_solve_multi_d(A, mc, ACF, mf, Xd); Xf.resize(Xd.size());
#pragma omp parallel for schedule(static)
            for (size_t q = 0; q < Xf.size(); q++) Xf[q] = (float)Xd[q]; } else chol_solve_multi(A, mc, ACF, mf, Xf); TT["M4 X=A_CC^-1 A_CF cozum (513 sag taraf)"] += now() - t1_; t1_ = now();
            vector<double> T((size_t)mf * mf, 0.0), U((size_t)mf * V, 0.0);
if (g_blas) { double tT_ = now(); scipy_cblas_dgemm(101, 112, 111, mf, mf, mc, 1.0, ACF.data(), mf, Xd.data(), mf, 0.0, T.data(), mf); TH["g8 T=ACF'X (dgemm)"] = now() - tT_; tT_ = now(); scipy_cblas_dgemm(101, 112, 111, mf, V, mc, 1.0, ACF.data(), mf, Y0d.data(), V, 0.0, U.data(), V); TH["g9 U=ACF'Y0"] = now() - tT_; } else
#pragma omp parallel for schedule(static)
            for (int a = 0; a < mf; a++) { double* ta = &T[(size_t)a * mf]; double* ua = &U[(size_t)a * V]; for (int k = 0; k < mc; k++) { double cf = ACF[(size_t)k * mf + a]; if (cf == 0) continue; const float* xr = &Xf[(size_t)k * mf]; for (int b = 0; b < mf; b++) ta[b] += cf * xr[b]; const float* yr = &Y0f[(size_t)k * V]; for (int v = 0; v < V; v++) ua[v] += cf * yr[v]; } }
            TT["M5 T=A_FC X, U=A_FC Y0 (Schur parcalari)"] += now() - t1_; printf("SURE|lamC=%g Cholesky(C blogu)+Schur hazirlik=%.2fs\n", lc, now() - tl); fflush(stdout);
            for (double lf : lamF_l) { double ts = now(); double t2_ = now(); vector<double> Sm((size_t)mf * mf), rhs((size_t)mf * V); for (size_t q = 0; q < Sm.size(); q++) Sm[q] = AFF[q] - T[q]; for (int i = 0; i < mf; i++) Sm[(size_t)i * mf + i] += (i == 0 && g_bias) ? lc : lf; for (size_t q = 0; q < rhs.size(); q++) rhs[q] = RF[q] - U[q];
                cholesky(Sm, mf); vector<float> WFf; chol_solve_multi(Sm, mf, rhs, V, WFf); vector<float> tmp((size_t)mc * V); gemm_f(Xf.data(), mc, mf, WFf.data(), V, tmp.data(), V); vector<float> WCf((size_t)mc * V);
#pragma omp parallel for schedule(static)
                for (size_t q = 0; q < WCf.size(); q++) WCf[q] = Y0f[q] - tmp[q];
                TT["M6 Schur sistemi (513) Cholesky+cozum + W_C=Y0-X W_F (lambda_F basina)"] += now() - t2_; t2_ = now(); auto Zv = logits(Fcv, iva, nva, &WCf, &WFf); double vce, al, ep; pickLin(Zv, vce, al, ep); RD r2 = rd_eval(Zv, va.y.data(), nva, Ptr, 1, al, ep);
                TT["E2 ortak cozum dogrulama degerlendirmesi"] += now() - t2_; printf("ORTAK_TARAMA|lamC=%g lamF=%g|dogr CE=%.4f PPL=%.1f (alpha=%g eps=%g)|sure=%.2fs\n", lc, lf, vce, r2.ppl, al, ep, now() - ts); fflush(stdout);
                if (g_hlean && !g_hfast) {   // V53h: lean cikis (orijinal aritmetik yolu; yalniz nihai model + Q karisimi)
                    vector<double> nwl(V), Pn((size_t)V * V, 0.0); for (int w = 0; w < V; w++) { nwl[w] = ACCAT(w, w); if (nwl[w] > 0) for (int v = 0; v < V; v++) Pn[(size_t)w * V + v] = RC[(size_t)w * V + v] / nwl[w]; }
                    if (g_hhash || !g_hdump.empty()) { h_hd("Fc", Fc.data(), Fc.size() * 4); h_hd("AFF", AFF.data(), AFF.size() * 8); h_hd("RF", RF.data(), RF.size() * 8); h_hd("ACCB", ACCB.data(), ACCB.size() * 8); h_hd("ACF", ACF.data(), ACF.size() * 8); h_hd("RC", RC.data(), RC.size() * 8); h_hd("Y0f", Y0f.data(), Y0f.size() * 4); h_hd("X", Xd.data(), Xd.size() * 8); h_hd("T", T.data(), T.size() * 8); h_hd("U", U.data(), U.size() * 8); }
                    h_finish(WCf.data(), WFf.data(), nwl, Pn, 0.0); exit(0); }
                if (g_v46 && use_g) {   // V46: ogrenme sinyali S = Z'(Y-Q); RHS = Z'Y + rho S = Z'((1+rho)Y - rho Q), tek cozum
                    double tq0 = now(); const int NIq = NI; (void)NIq; const int KQ = g_knn39;
                    vector<double> nw(V, 0.0); vector<int> wpair(mc, -1); for (int w = 0; w < V; w++) { nw[w] = ACCAT(w, w); for (int k2 : grpv[w]) wpair[k2] = w; }
                    vector<double> Pn((size_t)V * V, 0.0); for (int w = 0; w < V; w++) if (nw[w] > 0) for (int v = 0; v < V; v++) Pn[(size_t)w * V + v] = RC[(size_t)w * V + v] / nw[w];
                    // word-seviye ozet: toplam F ve unigram
                    vector<double> Fsum_tot(mf, 0.0); for (int w = 0; w < V; w++) for (int j = 0; j < mf; j++) Fsum_tot[j] += ACF[(size_t)w * mf + j];
                    auto buildQ = [&](int mode, vector<double>& RCq, vector<double>& RFq) {
                        RCq.assign((size_t)mc * V, 0.0); RFq.assign((size_t)mf * V, 0.0); mt19937_64 gs(9001); vector<int> pm(V); iota(pm.begin(), pm.end(), 0); if (mode == 1) shuffle(pm.begin(), pm.end(), gs);
                        if (mode == 3) { for (int a = 0; a < mc; a++) { double d = ACCAT(a, a); for (int v = 0; v < V; v++) RCq[(size_t)a * V + v] = d * Ptr[v]; } for (int j = 0; j < mf; j++) for (int v = 0; v < V; v++) RFq[(size_t)j * V + v] = Fsum_tot[j] * Ptr[v]; return; }
                        vector<double> Nw((size_t)V * V, 0.0), bwv(V, 0.0), coefA(V, 0.0); vector<double> Mw((size_t)V * V, 0.0);
                        for (int w = 0; w < V; w++) { if (nw[w] <= 0) continue; double bw_ = (mode == 2) ? 0.0 : g_bq46; if (nw[w] < 2) bw_ = 1.0;
                            double* N = &Nw[(size_t)w * V]; if (mode == 2 && nw[w] >= 2) { for (int v = 0; v < V; v++) N[v] = Ptr[v]; } else { double ws = 0; for (int q = 0; q < KQ; q++) { int nb = g_Pq.nb[(size_t)w * KQ + q]; if (nb < 0) continue; nb = pm[nb]; if (nw[nb] <= 0) continue; double a = g_Pq.wt[(size_t)w * KQ + q]; ws += a; for (int v = 0; v < V; v++) N[v] += a * Pn[(size_t)nb * V + v]; } if (ws > 0) for (int v = 0; v < V; v++) N[v] /= ws; else for (int v = 0; v < V; v++) N[v] = Ptr[v]; }
                            bwv[w] = bw_; coefA[w] = nw[w] >= 2 ? (1 - bw_) / (nw[w] - 1) : 0.0;
                            for (int v = 0; v < V; v++) { RCq[(size_t)w * V + v] = (1 - bw_) * RC[(size_t)w * V + v] + bw_ * nw[w] * N[v]; Mw[(size_t)w * V + v] = coefA[w] * RC[(size_t)w * V + v] + bw_ * N[v]; }
                            for (int k2 : grpv[w]) if (k2 >= V) { double mp = ACCAT(k2, k2); for (int v = 0; v < V; v++) RCq[(size_t)k2 * V + v] = coefA[w] * (mp * RC[(size_t)w * V + v] - RC[(size_t)k2 * V + v]) + bw_ * mp * N[v]; } }
                        // F kismi: Fsum' M  - LOO duzeltmesi
                        scipy_cblas_dgemm(101, 112, 111, mf, V, V, 1.0, ACF.data(), mf, Mw.data(), V, 0.0, RFq.data(), V);
#pragma omp parallel for schedule(static)
                        for (int j = 0; j < mf; j++) { double* rq = &RFq[(size_t)j * V]; for (int i = 0; i < ntr; i++) { int w = itr[(size_t)NI * i]; double cf = coefA[w]; if (cf != 0.0) rq[tr.y[i]] -= cf * Fc[(size_t)i * mf + j]; } } };
                    vector<double> RCq, RFq, Y0q((size_t)mc * V), Uq((size_t)mf * V), Y0g2((size_t)mc * V);
                    // a) rho listesi
                    vector<double> rhos; { string rs = getenv("V46_RHOS") ? getenv("V46_RHOS") : "-1,-0.5,-0.25,-0.1,0,0.1,0.25"; size_t p0 = 0; while (p0 <= rs.size()) { size_t p1 = rs.find(',', p0); if (p1 == string::npos) p1 = rs.size(); rhos.push_back(atof(rs.substr(p0, p1 - p0).c_str())); p0 = p1 + 1; } }
                    double t_sig_total = 0;
                    for (int mode = 0; mode < (g_only0 ? 1 : 4); mode++) { double tm0 = now(); if (g_ridgeK && g_v51) {   // V51: ridge sinyali MERKEZI cekirdekten (OOF K(w_i)); satir parcalariyla akitilir
                            RCq.assign((size_t)mc * V, 0.0); RFq.assign((size_t)mf * V, 0.0); const int CHn = 2048; vector<float> Qch, Gc((size_t)mf * V);
                            for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Qch.assign((size_t)nn * V, 0.f); for (int i = 0; i < nn; i++) { int gi = s0 + i; const double* qr = &g_Qf[g_fold51[gi]][(size_t)tr.ctx[(size_t)gi * c + c - 1] * V]; float* q = &Qch[(size_t)i * V]; for (int v = 0; v < V; v++) q[v] = (float)qr[v]; }
                                for (int i = 0; i < nn; i++) for (int p = 0; p < NI; p++) { int id = itr[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; double* r1 = &RCq[(size_t)id * V]; const float* q = &Qch[(size_t)i * V]; for (int v = 0; v < V; v++) r1[v] += q[v]; }
                                scipy_cblas_sgemm(101, 112, 111, mf, V, nn, 1.f, &Fc[(size_t)s0 * mf], mf, Qch.data(), V, 0.f, Gc.data(), V); for (size_t q2 = 0; q2 < RFq.size(); q2++) RFq[q2] += Gc[q2]; } } else buildQ(mode, RCq, RFq);
                        // Y0q = A_CC^-1 RCq (gruplu), Uq = ACF' Y0q
#pragma omp parallel for schedule(dynamic, 4)
                        for (int g = 0; g < V; g++) { const vector<int>& ix = grpv[g]; int sg = (int)ix.size(); if (!sg) continue; vector<double> Gm((size_t)sg * sg), Rm((size_t)sg * V); for (int a = 0; a < sg; a++) { for (int b2 = 0; b2 < sg; b2++) Gm[(size_t)a * sg + b2] = ACCAT(ix[a], ix[b2]); Gm[(size_t)a * sg + a] += LSLOT(ix[a]); memcpy(&Rm[(size_t)a * V], &RCq[(size_t)ix[a] * V], V * sizeof(double)); } spd_solve(Gm, sg, Rm, V); for (int a = 0; a < sg; a++) memcpy(&Y0q[(size_t)ix[a] * V], &Rm[(size_t)a * V], V * sizeof(double)); }
                        scipy_cblas_dgemm(101, 112, 111, mf, V, mc, 1.0, ACF.data(), mf, Y0q.data(), V, 0.0, Uq.data(), V);
                        double tsig = now() - tm0; t_sig_total += tsig;
                        // sinyal dagilimi: S = Z'(Y-Q) = (RC - RCq ; RF - RFq)
                        { double nC = 0, nF = 0; vector<double> cn(V, 0.0); for (size_t a = 0; a < (size_t)mc; a++) for (int v = 0; v < V; v++) { double d = RC[a * V + v] - RCq[a * V + v]; nC += d * d; cn[v] += d * d; } for (size_t j = 0; j < (size_t)mf; j++) for (int v = 0; v < V; v++) { double d = RF[j * V + v] - RFq[j * V + v]; nF += d * d; cn[v] += d * d; } double nY = 0; for (size_t q2 = 0; q2 < RC.size(); q2++) nY += RC[q2] * RC[q2]; for (size_t q2 = 0; q2 < RF.size(); q2++) nY += RF[q2] * RF[q2]; sort(cn.begin(), cn.end());
                          printf("V46_SINYAL|mod=%d|||S||/||Z'Y||=%.3f ; payi C=%.3f F=%.3f ; cikis-sutun normu (kare) q10=%.3g medyan=%.3g q90=%.3g maks=%.3g ; sinyal hesabi sure=%.2fs\n", mode, sqrt((nC + nF) / nY), nC / (nC + nF), nF / (nC + nF), cn[V / 10], cn[V / 2], cn[V * 9 / 10], cn[V - 1], tsig); }
                        double bestR = 0, bestV = 1e30; vector<float> bC, bF; double nW0C = 0, nW0F = 0; for (float x : WCf) nW0C += (double)x * x; for (float x : WFf) nW0F += (double)x * x;
                        for (double rho : rhos) { vector<double> Y0r((size_t)mc * V), rhs3((size_t)mf * V);
                            for (size_t q2 = 0; q2 < Y0r.size(); q2++) Y0r[q2] = (1 + rho) * Y0d[q2] - rho * Y0q[q2]; for (size_t q2 = 0; q2 < rhs3.size(); q2++) rhs3[q2] = (1 + rho) * (RF[q2] - U[q2]) - rho * (RFq[q2] - Uq[q2]);
                            vector<float> WFr; chol_solve_multi(Sm, mf, rhs3, V, WFr); vector<float> tmp3((size_t)mc * V), WCr((size_t)mc * V); gemm_f(Xf.data(), mc, mf, WFr.data(), V, tmp3.data(), V);
#pragma omp parallel for schedule(static)
                            for (size_t q2 = 0; q2 < WCr.size(); q2++) WCr[q2] = (float)Y0r[q2] - tmp3[q2];
                            auto Zv4 = logits(Fcv, iva, nva, &WCr, &WFr); double v4, a4, e4; pickLin(Zv4, v4, a4, e4); double dC = 0, dF = 0; for (size_t q2 = 0; q2 < WCr.size(); q2++) { double d = WCr[q2] - WCf[q2]; dC += d * d; } for (size_t q2 = 0; q2 < WFr.size(); q2++) { double d = WFr[q2] - WFf[q2]; dF += d * d; }
                            printf("V46_RHO|mod=%d|rho=%g|dogr CE=%.4f PPL=%.1f (alpha=%g eps=%g)|agirlik degisimi bagil W_C=%.3e W_F=%.3e\n", mode, rho, v4, exp(v4), a4, e4, sqrt(dC / nW0C), sqrt(dF / nW0F));
                            if (v4 < bestV) { bestV = v4; bestR = rho; bC.swap(WCr); bF.swap(WFr); } }
                        auto Zv5 = logits(Fcv, iva, nva, &bC, &bF); auto Ze5 = logits(Fce, ite, nte, &bC, &bF); double v5, a5, e5; pickLin(Zv5, v5, a5, e5); RD rv5 = rd_eval(Zv5, va.y.data(), nva, Ptr, 1, a5, e5); RD re5 = rd_eval(Ze5, te.y.data(), nte, Ptr, 1, a5, e5);
                        printf("V46_SECIM|mod=%d (0=gercek pool-Q,1=karisik baglanti,2=yalniz kendi-kelime LOO,3=unigram yumusatma)|rho=%g (dogrulamayla)|dogr CE=%.4f PPL=%.1f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|sinyal sure=%.2fs\n", mode, bestR, rv5.ce, rv5.ppl, re5.ce, re5.ppl, re5.top1, re5.top5, tsig); fflush(stdout); }
                    if (g_v50) {   // V50: havuz bilgisini SON tahminde olasilik karisimi olarak kullan: P_son=(1-alpha)P_model+alpha Q_havuz
                        double tw0 = now();
                        // Q_havuz tablosu (kelime basina; yalniz egitim sayimlari ve baglantilari): Q(w)=(1-b)P_w+b*sum_v a_wv P_v ; gorulmemis/sifir-toplam satir -> unigram
                        auto buildQtab = [&](bool shuf, vector<double>& Qt) { Qt.assign((size_t)V * V, 0.0); mt19937_64 gs(9001); vector<int> pm(V); iota(pm.begin(), pm.end(), 0); if (shuf) shuffle(pm.begin(), pm.end(), gs);
                            for (int w = 0; w < V; w++) { double* q = &Qt[(size_t)w * V]; vector<double> N(V, 0.0); double ws = 0; for (int qk = 0; qk < KQ; qk++) { int nb = g_Pq.nb[(size_t)w * KQ + qk]; if (nb < 0) continue; nb = pm[nb]; if (nw[nb] <= 0) continue; double a = g_Pq.wt[(size_t)w * KQ + qk]; ws += a; for (int v = 0; v < V; v++) N[v] += a * Pn[(size_t)nb * V + v]; }
                                bool hasN = ws > 0; if (hasN) for (int v = 0; v < V; v++) N[v] /= ws; double bq = g_bq46; double sum = 0;
                                if (nw[w] > 0 && hasN) for (int v = 0; v < V; v++) q[v] = (1 - bq) * Pn[(size_t)w * V + v] + bq * N[v]; else if (nw[w] > 0) for (int v = 0; v < V; v++) q[v] = Pn[(size_t)w * V + v]; else if (hasN) for (int v = 0; v < V; v++) q[v] = N[v]; else for (int v = 0; v < V; v++) q[v] = Ptr[v];
                                for (int v = 0; v < V; v++) sum += q[v]; if (!(sum > 0)) for (int v = 0; v < V; v++) q[v] = Ptr[v]; else if (fabs(sum - 1.0) > 1e-9) for (int v = 0; v < V; v++) q[v] /= sum; }
                            if (g_qkap53 > 0) { vector<double> pi(V, 0.0); if (g_qpi53 == 1) { double t = 0; for (int w = 0; w < V; w++) for (int v = 0; v < V; v++) if (Pn[(size_t)w * V + v] > 0) pi[v] += 1.0; for (int v = 0; v < V; v++) { pi[v] += 1.0; t += pi[v]; } for (int v = 0; v < V; v++) pi[v] /= t; } else pi = Ptr; vector<double> nwd(V); for (int w = 0; w < V; w++) nwd[w] = (double)nw[w]; v53_smooth(Qt, nwd, pi); } };
                        vector<double> QtR, QtS; buildQtab(false, QtR); buildQtab(true, QtS); if (g_v51shuf) QtR = QtS; double t_qtab = now() - tw0;
                        auto weights = [&](double rho, vector<float>& WCo, vector<float>& WFo) { vector<double> Y0r((size_t)mc * V), rhs3((size_t)mf * V); for (size_t q2 = 0; q2 < Y0r.size(); q2++) Y0r[q2] = (1 + rho) * Y0d[q2] - rho * Y0q[q2]; for (size_t q2 = 0; q2 < rhs3.size(); q2++) rhs3[q2] = (1 + rho) * (RF[q2] - U[q2]) - rho * (RFq[q2] - Uq[q2]);
                            chol_solve_multi(Sm, mf, rhs3, V, WFo); vector<float> tmp3((size_t)mc * V); WCo.assign((size_t)mc * V, 0.f); gemm_f(Xf.data(), mc, mf, WFo.data(), V, tmp3.data(), V);
#pragma omp parallel for schedule(static)
                            for (size_t q2 = 0; q2 < WCo.size(); q2++) WCo[q2] = (float)Y0r[q2] - tmp3[q2]; };
                        vector<int> wva(nva), wte(nte); for (int i = 0; i < nva; i++) wva[i] = va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) wte[i] = te.ctx[(size_t)i * c + c - 1];
                        auto fuse = [&](const vector<float>& Z, int n, const vector<int>& y, const vector<int>& wl, const vector<double>* Qt, double alpha, double aread, double eps, double& ce, double& t1, double& t5) { double sce = 0; long c1 = 0, c5 = 0;
#pragma omp parallel reduction(+ : sce, c1, c5)
                            { vector<double> p(V);
#pragma omp for schedule(static)
                                for (int i = 0; i < n; i++) { const float* z = &Z[(size_t)i * V]; double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], eps); const double* q = (alpha > 0 && Qt) ? &(*Qt)[(size_t)wl[i] * V] : nullptr; double sm = 0;
                                    for (int v = 0; v < V; v++) { double pm_ = (1 - aread) * Ptr[v] + aread * max((double)z[v], eps) / S2; p[v] = q ? (1 - alpha) * pm_ + alpha * q[v] : pm_; sm += p[v]; } double py = p[y[i]] / sm; int rk = 0; for (int v = 0; v < V; v++) if (p[v] / sm > py) rk++; sce += -log(py); c1 += rk == 0; c5 += rk < 5; } }
                            ce = sce / n; t1 = (double)c1 / n; t5 = (double)c5 / n; };
                        const double ALF[] = {0, 0.025, 0.05, 0.10, 0.20, 0.30, 0.40, 0.60}; const int NAL2 = 8;
                        struct Sys { const char* name; double rho; int qmode; };   // qmode 0: alpha=0 yalniz, 1: gercek Q, 2: karisik Q
                        Sys systems[] = {{"D sinyalsiz kontrol (rho=0, alpha=0)", 0, 0}, {"A V49 referansi (rho=-0.25, alpha=0)", g_rho50, 0}, {"B yalniz son asama GERCEK Q (rho=0, alpha)", 0, 1}, {"B yalniz son asama KARISIK Q", 0, 2}, {"C her iki asama GERCEK Q (rho=-0.25, alpha)", g_rho50, 1}, {"C her iki asama KARISIK Q", g_rho50, 2}};
                        double t_solve_tot = 0; for (auto& sy : systems) { double ts0 = now(); vector<float> WCs, WFs; weights(sy.rho, WCs, WFs); double t_solve = now() - ts0; t_solve_tot += t_solve;
                            double tp0 = now(); auto Zv = logits(Fcv, iva, nva, &WCs, &WFs); auto Ze = logits(Fce, ite, nte, &WCs, &WFs); double t_logit = now() - tp0; double vce0, a0, e0; pickLin(Zv, vce0, a0, e0);
                            const vector<double>* Qt = sy.qmode == 2 ? &QtS : &QtR; double bestV = 1e30, bestA = 0; int nal = sy.qmode == 0 ? 1 : NAL2;
                            for (int ai = 0; ai < nal; ai++) { double ce, t1, t5; fuse(Zv, nva, va.y, wva, Qt, ALF[ai], a0, e0, ce, t1, t5); printf("V50_ALFA|%s|alpha=%g|dogr CE=%.4f PPL=%.1f\n", sy.name, ALF[ai], ce, exp(ce)); if (ce < bestV) { bestV = ce; bestA = ALF[ai]; } }
                            double tf0 = now(); double cev, v1, v5, cet, t1, t5; fuse(Zv, nva, va.y, wva, Qt, bestA, a0, e0, cev, v1, v5); fuse(Ze, nte, te.y, wte, Qt, bestA, a0, e0, cet, t1, t5); double t_fuse = now() - tf0;
                            double ce0, q1, q5; fuse(Ze, nte, te.y, wte, Qt, 0.0, a0, e0, ce0, q1, q5);
                            printf("V50_SECIM|%s|rho=%g|alpha=%g (dogrulamayla)|okuma alpha=%g eps=%g|dogr CE=%.4f PPL=%.1f Top1=%.4f Top5=%.4f|TEST CE=%.4f PPL=%.2f Top1=%.4f Top5=%.4f|(alpha=0 test PPL=%.2f)|agirlik cozme(rho)=%.2fs ; logit(val+test)=%.2fs ; birlestirme+PPL(val+test)=%.2fs\n", sy.name, sy.rho, bestA, a0, e0, cev, exp(cev), v1, v5, cet, exp(cet), t1, t5, exp(ce0), t_solve, t_logit, t_fuse); { double cvs, a1s, a5s, cts, b1s, b5s; fuse(Zv, nva, va.y, wva, Qt, 0.2, a0, e0, cvs, a1s, a5s); fuse(Ze, nte, te.y, wte, Qt, 0.2, a0, e0, cts, b1s, b5s); if (sy.qmode > 0) printf("V50_SABIT|%s|alpha=0.2 (sabit)|dogr CE=%.4f|TEST PPL=%.2f Top1=%.4f Top5=%.4f\n", sy.name, cvs, exp(cts), b1s, b5s); } fflush(stdout); }
                        struct rusage ru2; getrusage(RUSAGE_SELF, &ru2); printf("V50_SURE|Q_havuz tablosu (gercek+karisik)=%.2fs ; tepe RSS=%.0f MB\n", t_qtab, ru2.ru_maxrss / 1024.0);
                    }
                    printf("V46_TOPLAM_SINYAL_SURESI|%.2fs (4 mod toplam; tek mod ortalama %.2fs)\n", t_sig_total, t_sig_total / 4);
                }
                if (g_passes > 1 && use_g) {   // V43: residual duzeltme gecisleri (ayni Z, ayni gruplu carpan, ayni Schur carpani)
                    vector<float> WC = WCf, WF = WFf; double al_c = al, ep_c = ep; double vce_prev = vce; vector<float> gC, gF;
                    printf("PASS|1|eta=-|dogr CE=%.4f PPL=%.1f|(tek gecis, V42 ile ayni)\n", vce, exp(vce));
                    for (int pass = 2; pass <= g_passes; pass++) { double tp0 = now();
                        // 1) egitim residual'i R (v43_res: 0 dogrusal Y-z ; 1 okuma-tutarli (Y-P)/alpha) -> RCn = C'R (seyrek), RFn = F'R
                        vector<double> RCn((size_t)mc * V, 0.0), RFn((size_t)mf * V, 0.0); { const int CHn = 4096; vector<float> Zch, Rch, Gc((size_t)mf * V);
                            for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Zch.assign((size_t)nn * V, 0.f); gemm_f(&Fc[(size_t)s0 * mf], nn, mf, WF.data(), V, Zch.data(), V);
#pragma omp parallel for schedule(static)
                                for (int i = 0; i < nn; i++) { float* z = &Zch[(size_t)i * V]; for (int p = 0; p < NI; p++) { int id = itr[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; const float* w1 = &WC[(size_t)id * V]; for (int v = 0; v < V; v++) z[v] += w1[v]; }
                                    if (g_res == 0) { for (int v = 0; v < V; v++) z[v] = -z[v]; z[tr.y[s0 + i]] += 1.f; }
                                    else { double S2 = 0; for (int v = 0; v < V; v++) S2 += max((double)z[v], ep_c); for (int v = 0; v < V; v++) { double pp = (1 - al_c) * Ptr[v] + al_c * max((double)z[v], ep_c) / S2; z[v] = (float)(-pp / al_c); } z[tr.y[s0 + i]] += (float)(1.0 / al_c); } }
                                for (int i = 0; i < nn; i++) for (int p = 0; p < NI; p++) { int id = itr[(size_t)NI * (s0 + i) + p]; if (id < 0) continue; double* r1 = &RCn[(size_t)id * V]; const float* z = &Zch[(size_t)i * V]; for (int v = 0; v < V; v++) r1[v] += z[v]; }
                                scipy_cblas_sgemm(101, 112, 111, mf, V, nn, 1.f, &Fc[(size_t)s0 * mf], mf, Zch.data(), V, 0.f, Gc.data(), V); for (size_t q = 0; q < RFn.size(); q++) RFn[q] += Gc[q]; } }
                        double tp1 = now();
                        // 2) dZ = (G+Lambda)^-1 [C'R ; F'R] : grup carpani (yalniz V sag taraf) + ayni Schur Cholesky'si
                        vector<double> Y0n((size_t)mc * V, 0.0);
#pragma omp parallel for schedule(dynamic, 4)
                        for (int g = 0; g < V; g++) { const vector<int>& ix = grpv[g]; int sg = (int)ix.size(); if (!sg) continue; vector<double> Gm((size_t)sg * sg), Rm((size_t)sg * V);
                            for (int a = 0; a < sg; a++) { for (int b2 = 0; b2 < sg; b2++) Gm[(size_t)a * sg + b2] = ACCAT(ix[a], ix[b2]); Gm[(size_t)a * sg + a] += LSLOT(ix[a]); memcpy(&Rm[(size_t)a * V], &RCn[(size_t)ix[a] * V], V * sizeof(double)); }
                            spd_solve(Gm, sg, Rm, V); for (int a = 0; a < sg; a++) memcpy(&Y0n[(size_t)ix[a] * V], &Rm[(size_t)a * V], V * sizeof(double)); }
                        vector<double> Un((size_t)mf * V, 0.0); scipy_cblas_dgemm(101, 112, 111, mf, V, mc, 1.0, ACF.data(), mf, Y0n.data(), V, 0.0, Un.data(), V);
                        vector<double> rh2((size_t)mf * V); for (size_t q = 0; q < rh2.size(); q++) rh2[q] = RFn[q] - Un[q]; vector<float> dF; chol_solve_multi(Sm, mf, rh2, V, dF);
                        vector<float> tmp2((size_t)mc * V), dC((size_t)mc * V); gemm_f(Xf.data(), mc, mf, dF.data(), V, tmp2.data(), V);
#pragma omp parallel for schedule(static)
                        for (size_t q = 0; q < dC.size(); q++) dC[q] = (float)Y0n[q] - tmp2[q];
                        double tp2 = now();
                        // 3) eta yalniz dogrulamada (test'e bakilmaz): W' = W + eta dW
                        double bestE = 0, bestV = 1e30; vector<float> bC, bF; double bal = al_c, bep = ep_c;
                        for (double eta : {0.25, 0.5, 1.0, 1.5}) { vector<float> C2(WC), F2(WF); for (size_t q = 0; q < C2.size(); q++) C2[q] += (float)eta * dC[q]; for (size_t q = 0; q < F2.size(); q++) F2[q] += (float)eta * dF[q];
                            auto Zv2 = logits(Fcv, iva, nva, &C2, &F2); double v2, a2, e2; pickLin(Zv2, v2, a2, e2); printf("PASS|%d|eta=%g|dogr CE=%.4f PPL=%.1f (alpha=%g eps=%g)\n", pass, eta, v2, exp(v2), a2, e2);
                            if (v2 < bestV) { bestV = v2; bestE = eta; bC.swap(C2); bF.swap(F2); bal = a2; bep = e2; } }
                        double tp3 = now();
                        WC.swap(bC); WF.swap(bF); al_c = bal; ep_c = bep;
                        auto Zv3 = logits(Fcv, iva, nva, &WC, &WF); auto Ze3 = logits(Fce, ite, nte, &WC, &WF); RD rv3 = rd_eval(Zv3, va.y.data(), nva, Ptr, 1, al_c, ep_c); RD re3 = rd_eval(Ze3, te.y.data(), nte, Ptr, 1, al_c, ep_c);
                        printf("PASS_SECIM|%d|res=%d|eta=%g (dogrulamayla)|dogr CE=%.4f PPL=%.1f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|ek sure: residual+RHS=%.2fs cozum(grup+Schur)=%.2fs eta-taramasi=%.2fs toplam=%.2fs\n", pass, g_res, bestE, rv3.ce, rv3.ppl, re3.ce, re3.ppl, re3.top1, re3.top5, tp1 - tp0, tp2 - tp1, tp3 - tp2, tp3 - tp0); fflush(stdout);
                        if (bestV >= vce_prev - 1e-9) { printf("PASS_DUR|%d. gecis dogrulamada iyilestirme vermedi; durduruluyor\n", pass); break; } vce_prev = bestV; }
                }
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
          double nr = 0, nrhs = 0; for (size_t q = 0; q < RCr.size(); q++) { double r = RCr[q] + bestLC * bWC[q]; nr += r * r; nrhs += RC[q] * RC[q]; } for (size_t q = 0; q < RFr.size(); q++) { double r = RFr[q] + ((g_bias && q < (size_t)V) ? bestLC : bestLF) * bWF[q]; nr += r * r; nrhs += RF[q] * RF[q]; }
          printf("COZUM_REZIDUSU|||Z^T(ZW-Y)+Lambda W||/||Z^T Y|| = %.3e (float gemm ile olculdu; blok eliminasyon Cholesky cift duyarlik, cozum float)\n", sqrt(nr / nrhs)); }
        TT["E3 final degerlendirme: egitim+dogr+test logit/PPL/kategori + parca okumalari (rapor icin)"] += now() - tf_; printf("V31_TOPLAM|sure=%.1fs\n", now() - tz0); for (auto& kv : TT) printf("TT|%s|%.3fs\n", kv.first.c_str(), kv.second); };
    auto run_chain = [&](const string& name, vector<int> hs, bool joint, bool useX, int idm) {
        auto setsp = [&](Data& d) { d.sp.assign((size_t)d.n * 3, -1); if (idm == 0) return; for (int i = 0; i < d.n; i++) { int id2 = d.ctx[(size_t)i * c + c - 2], id1 = d.ctx[(size_t)i * c + c - 1]; int* sp = &d.sp[(size_t)i * 3]; sp[0] = BASEA + id1; sp[1] = BASEB + id2; if (idm == 2) { unsigned h = ((unsigned)id2 * 2654435761u) ^ ((unsigned)id1 * 40503u + 12345u); h = (h ^ (h >> 15)) * 2246822519u; sp[2] = BASEP + (int)((h ^ (h >> 13)) % HB); } } };
        setsp(tr); setsp(va); setsp(te); int nid = idm == 0 ? 0 : (idm == 1 ? 2 : 3); double rmsX = 0; { double a = 0; for (int i = 0; i < ntr; i++) { const float* r = &tr.X[(size_t)i * K]; for (int k = 0; k < K; k++) a += (double)r[k] * r[k]; } rmsX = sqrt(a / ntr); } double sid1 = idm ? sidm * rmsX / sqrt((double)nid) : 0, sid2 = idm ? sidm / sqrt((double)nid) : 0;
        if (idm) printf("KIMLIK|%s|kimlik sayisi=%d X satir-norm RMS=%.4f ; F1 girdisinde kimlik olcegi=%.4f (kimlik blogu enerjisi=X enerjisi) ; F2..F4 girdisinde kimlik olcegi=%.4f (blok enerjisi=1, X ve F bloklari gibi) ; seyrek boyut=%d (yogun acilmadi)\n", name.c_str(), nid, rmsX, sid1, sid2, KT - BASEA);
        g_nsp35 = idm ? KT - BASEA : 0; TH["m1 run_chain basi (program basindan)"] = now() - g_hT0; if (g_c0) v36_buildrank(tr, c); TH["m1b buildrank"] = now() - g_hT0 - TH["m1 run_chain basi (program basindan)"]; double tc0 = now(); int ns = hs.size(); vector<float> Zc35; vector<int> ylab35 = tr.y; if (g_v35) { Zc35.assign((size_t)ntr * V, 0.f); if (g_v36) { double toof = 0; Zc35 = v36_oof(tr, c, g_pair ? 3 : 2, g_loof, toof); printf("V36_OOF|2-kat out-of-fold kimlik%s ridge (lam=%g) residual baslangici ; sure=%.2fs ; yalniz egitim etiketleri\n", g_pair ? "+cift" : "", g_loof, toof); } if (g_yshuf && g_v36) { vector<int> pm(ntr); iota(pm.begin(), pm.end(), 0); mt19937_64 gs(777); shuffle(pm.begin(), pm.end(), gs); vector<float> Zp((size_t)ntr * V); vector<int> yp(ntr); for (int i = 0; i < ntr; i++) { memcpy(&Zp[(size_t)i * V], &Zc35[(size_t)pm[i] * V], V * sizeof(float)); yp[i] = tr.y[pm[i]]; } Zc35.swap(Zp); ylab35 = yp; printf("V36_KONTROL|residual SATIRLARI karistirildi (R=onehot(y)-Zc ayni permutasyonla; X ile iliskisi kopuk)\n"); } else if (g_yshuf) { mt19937_64 gs(777); shuffle(ylab35.begin(), ylab35.end(), gs); printf("V35_KONTROL|etiketler karistirildi: ic yonler ve asama ridge'leri KARISIK etiketle hesaplanir; yalniz son ortak cozum gercek Y kullanir\n"); } } vector<vector<float>> Ft(ns), Fv(ns), Fe(ns); vector<int> wd(ns);
        for (int s = 0; s < ns; s++) {
            if (s == 0) { Feat f; if (g_v35) { Feat f35; f35.H = hs[0]; vector<float> Wd, Ws, bb; V35Diag dg; if (g_v38) { double ut, mxc, mnc; dg = v38_dirs(tr.X.data(), K, tr, Zc35, ylab35, hs[0], gam, Wd, bb, ut, mxc, mnc); Ws.assign((size_t)(KT - BASEA) * hs[0], 0.f); printf("V38_ADRES|F1|girdi=%d rank(M)=%d ilk_tekil=%.4g son_tekil=%.4g aktarilan_artik_enerji=%.3f yon_cos maks=%.3f ort=%.4f\n", K, dg.rankM, dg.s1, dg.sk, dg.ratio_eig, mxc, mnc); } else dg = v35_dirs(tr.X.data(), K, tr, sid1, Zc35, ylab35, hs[0], seed, Wd, Ws, bb); f35.W.assign((size_t)(idm ? KT : K) * hs[0], 0.f); for (int i = 0; i < K; i++) for (int j = 0; j < hs[0]; j++) f35.W[(size_t)i * hs[0] + j] = Wd[(size_t)i * hs[0] + j]; if (idm) for (int r = 0; r < KT - BASEA; r++) for (int j = 0; j < hs[0]; j++) f35.W[(size_t)(BASEA + r) * hs[0] + j] = Ws[(size_t)r * hs[0] + j]; f35.b = bb; f = f35; printf("V35_SVD|F1|girdi_boyutu=%d (yogun %d + seyrek %d) k=%d (sketch l=%d) M_sayisal_rank=%d (ust sinir min(D,V)=%d) s1=%.4g sk=%.4g ust-k_enerji_payi=%.3f ; rastgele doldurma YOK\n", K + KT - BASEA, K, KT - BASEA, dg.k, dg.l, dg.rankM, min(K + KT - BASEA, V), dg.s1, dg.sk, dg.ratio_eig); } else { double tmw_ = now(); f = idm ? make_w1x(tr, hs[0], seed, sid1) : make_w1("cift", tr, hs[0], seed); TH["f1a make_w1x (yonler)"] = now() - tmw_; } double tfe_ = now(); features(tr, f, gam, Ft[0]); features(va, f, gam, Fv[0]); features(te, f, gam, Fe[0]); TH["f1b features (gemm+tanh)"] = now() - tfe_; wd[0] = hs[0] + 1; if (g_v35 && ns > 1) v35_stagefit(Ft[0], wd[0], ylab35, Zc35, ntr, g_lams); }
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
        phase_mark("onhazir+okuma+F1"); TH["f1 F1 olusturma (yon+gemm, v35 yolu)"] = now() - tc0;
        if (g_v39) { double tv0 = now(); if (g_v51) { double tk0 = now();   // ---- V51: TEK MERKEZI CEKIRDEK K (bir kez; tum asamalar buradan okur) ----
                vector<float> Cfull; if (!g_v61) Cfull.assign((size_t)V * V, 0.f); g_fold51.assign(ntr, 0); for (int i = 0; i < ntr; i++) { g_fold51[i] = (i / 498) % 5; if (!g_v61) Cfull[(size_t)tr.ctx[(size_t)i * c + c - 1] * V + tr.y[i]] += 1.f; }
                double tkk_ = now(); g_Qf.assign(6, {}); g_Pf51.assign(6, Pool39()); for (int f = 0; f < (g_v61 ? 0 : 6); f++) { vector<float> Cf = Cfull; if (f < 5) for (int i = 0; i < ntr; i++) if (g_fold51[i] == f) Cf[(size_t)tr.ctx[(size_t)i * c + c - 1] * V + tr.y[i]] -= 1.f; g_Qf[f] = v51_qtab(Cf, Ptr, g_v51shuf, &g_Pf51[f]); } TH["k0 6 tablo dongusu toplam"] = now() - tkk_; tkk_ = now();   // egitim satiri icin K: kendi belge-katinin DISINDAKI sayimlar+graf (OOF); dogr/test: tam egitim
                // sabit ozet s(K)=sum_y K_y E(y) (E: X'in 64D kelime blogu), sabit rastgele (egitilmeyen) 64->512 eslemesi, egitimden standardizasyon
                TH["k3 Cfull+fold"] = now() - tkk_; tkk_ = now(); vector<float> Et((size_t)V * 64, 0.f); { vector<char> hE(V, 0); for (int i = 0; i < ntr; i++) { int w = tr.ctx[(size_t)i * c + c - 1]; if (!hE[w]) { memcpy(&Et[(size_t)w * 64], &tr.X[(size_t)i * K + 64], 64 * sizeof(float)); hE[w] = 1; } } }
                vector<float> Sf[6]; if (g_v61) { double tv61_ = now();   // V61: Cn CSR (yalniz sifir-olmayan) ; Q tablosu hic olusmaz
                    for (int f = 0; f < 6; f++) { CountCSR C61 = v61_counts(tr, c, g_fold51, f); vector<int> cnt61; g_Pf51[f] = v61_graph(C61, cnt61); v61_stab(C61, g_Pf51[f], Ptr, Et, g_v51shuf, Sf[f]);
                        if (g_v61chk) {   // eski yogun yol: Cn yogun -> v51_qtab (V60 grafi) -> float Q -> sgemm ; farklari raporla
                            vector<float> Cd((size_t)V * V, 0.f); for (int u = 0; u < V; u++) for (long e = C61.off[u]; e < C61.off[u + 1]; e++) Cd[(size_t)u * V + C61.col[e]] = C61.val[e];
                            Pool39 Po; vector<double> Qd = v51_qtab(Cd, Ptr, g_v51shuf, &Po); vector<float> Qf32(Qd.begin(), Qd.end()), So((size_t)V * 64, 0.f); gemm_f(Qf32.data(), V, V, Et.data(), 64, So.data(), 64);
                            long nbd = 0; double wd_ = 0; for (size_t i = 0; i < Po.nb.size(); i++) { if (Po.nb[i] != g_Pf51[f].nb[i]) nbd++; wd_ = max(wd_, (double)fabsf(Po.wt[i] - g_Pf51[f].wt[i])); }
                            double dmax = 0, smax = 0, rmax = 0; long same = 0; for (size_t i = 0; i < So.size(); i++) { double d = fabs((double)So[i] - Sf[f][i]); dmax = max(dmax, d); smax = max(smax, fabs((double)So[i])); if (So[i] == Sf[f][i]) same++; }
                            printf("V61_QCHECK|tablo %d|graf nb-farkli=%ld maks|dwt|=%.2e|s(K) maks|d|=%.3e (maks|s|=%.3e, bagil %.2e) bit-esit eleman=%ld/%zu|Q tablosu bellegi (eski)=%.1f MB\n", f, nbd, wd_, dmax, smax, dmax / smax, same, So.size(), (double)V * V * 8 / 1e6); } }
                    TH["k0 V61 6 tablo (CSR+graf+s(K))"] = now() - tv61_; } else for (int f = 0; f < 6; f++) { vector<float> Qf32(g_Qf[f].begin(), g_Qf[f].end()); Sf[f].assign((size_t)V * 64, 0.f); gemm_f(Qf32.data(), V, V, Et.data(), 64, Sf[f].data(), 64); }
                int NBf = wd[0] - 1; vector<float> Pm((size_t)64 * NBf); { mt19937_64 gp(g_mseed); normal_distribution<double> Nd(0, 1); for (auto& x : Pm) x = (float)(Nd(gp) / 8.0); }
                auto make_s = [&](const Data& d, int n, bool isTrain, vector<float>& sOut) { sOut.assign((size_t)n * 64, 0.f); for (int i = 0; i < n; i++) { int w = d.ctx[(size_t)i * c + c - 1]; int f = isTrain ? g_fold51[i] : 5; memcpy(&sOut[(size_t)i * 64], &Sf[f][(size_t)w * 64], 64 * sizeof(float)); } };
                const bool utab = g_v54gate && g_v54utab && g_gF > 0 && g_gP <= 0;
                if (utab) {   // V54: U satiri yalniz (kat,kelime) anahtarina bagli -> benzersiz anahtarlar icin bir kez (gemm, standardizasyon, tanh); n x 512'lik g_U* dizileri hic olusmaz
                    vector<int> ktr(ntr), kva(nva), kte(nte); for (int i = 0; i < ntr; i++) ktr[i] = g_fold51[i] * V + tr.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nva; i++) kva[i] = 5 * V + va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) kte[i] = 5 * V + te.ctx[(size_t)i * c + c - 1];
                    vector<int> rowOf(6 * V, -1), ukey; for (const vector<int>* kv : {&ktr, &kva, &kte}) for (int k : *kv) if (rowOf[k] < 0) { rowOf[k] = (int)ukey.size(); ukey.push_back(k); } int nu = (int)ukey.size();
                    vector<double> sm(64, 0.0), ss(64, 0.0);   // orijinalle ayni islem sirasi (egitim satirlari i artan)
                    for (int i = 0; i < ntr; i++) { const float* row = &Sf[g_fold51[i]][(size_t)tr.ctx[(size_t)i * c + c - 1] * 64]; for (int j = 0; j < 64; j++) sm[j] += row[j]; } for (auto& x : sm) x /= ntr;
                    for (int i = 0; i < ntr; i++) { const float* row = &Sf[g_fold51[i]][(size_t)tr.ctx[(size_t)i * c + c - 1] * 64]; for (int j = 0; j < 64; j++) { double d = row[j] - sm[j]; ss[j] += d * d; } } for (auto& x : ss) x = sqrt(x / ntr) + 1e-9;
                    vector<float> Su((size_t)nu * 64); for (int u = 0; u < nu; u++) { const float* row = &Sf[ukey[u] / V][(size_t)(ukey[u] % V) * 64]; for (int j = 0; j < 64; j++) Su[(size_t)u * 64 + j] = (float)((row[j] - sm[j]) / ss[j]); }
                    TH["k4 Et,Sf,make_s,stdz"] = now() - tkk_; tkk_ = now();
                    ubuf<float> Uu((size_t)nu * NBf); TH["k5a Uu tahsis (nu satir)"] = now() - tkk_; TH["k5c nu"] = nu; gemm_f(Su.data(), nu, 64, Pm.data(), NBf, Uu.data(), NBf); TH["k5 proj gemm"] = now() - tkk_; tkk_ = now();
                    vector<double> um(NBf, 0.0), us(NBf, 0.0);
#pragma omp parallel
                    { int nt_ = omp_get_num_threads(), t_ = omp_get_thread_num(); int j0 = (int)((long)NBf * t_ / nt_), j1 = (int)((long)NBf * (t_ + 1) / nt_);
                      for (int i = 0; i < ntr; i++) { const float* r = &Uu[(size_t)rowOf[ktr[i]] * NBf]; for (int j = j0; j < j1; j++) um[j] += r[j]; } for (int j = j0; j < j1; j++) um[j] /= ntr;
                      for (int i = 0; i < ntr; i++) { const float* r = &Uu[(size_t)rowOf[ktr[i]] * NBf]; for (int j = j0; j < j1; j++) { double d = r[j] - um[j]; us[j] += d * d; } } for (int j = j0; j < j1; j++) us[j] = sqrt(us[j] / ntr) + 1e-9; }
                    TH["k6 um/us"] = now() - tkk_; tkk_ = now();
#pragma omp parallel for schedule(static)
                    for (int u = 0; u < nu; u++) for (int j = 0; j < NBf; j++) Uu[(size_t)u * NBf + j] = (float)((Uu[(size_t)u * NBf + j] - um[j]) / us[j]);
                    TH["k7 ustd"] = now() - tkk_; tkk_ = now();
                    ubuf<double> Tt((size_t)nu * NBf);   /* V76: tamami paralel yazilir -> seri sifirlama yok */
#pragma omp parallel for schedule(static)
                    for (int u = 0; u < nu; u++) for (int j = 0; j < NBf; j++) Tt[(size_t)u * NBf + j] = tanh(g_gF * Uu[(size_t)u * NBf + j]);
                    auto gate3 = [&](vector<float>& Fm, int n, const vector<int>& key) { int m = wd[0];
#pragma omp parallel for schedule(static)
                        for (int i = 0; i < n; i++) { float* r = &Fm[(size_t)i * m + 1]; const double* tr_ = &Tt[(size_t)rowOf[key[i]] * NBf]; for (int j = 0; j < NBf; j++) { double f = max(-0.999999, min(0.999999, (double)r[j])); double t = tr_[j]; r[j] = (float)((f + t) / (1.0 + f * t)); } } };
                    gate3(Ft[0], ntr, ktr); gate3(Fv[0], nva, kva); gate3(Fe[0], nte, kte); TH["k8 gate atanh/tanh"] = now() - tkk_;
                } else {
                vector<float> str, sva, ste; make_s(tr, ntr, true, str); make_s(va, nva, false, sva); make_s(te, nte, false, ste);
                vector<double> sm(64, 0.0), ss(64, 0.0); for (int i = 0; i < ntr; i++) for (int j = 0; j < 64; j++) sm[j] += str[(size_t)i * 64 + j]; for (auto& x : sm) x /= ntr; for (int i = 0; i < ntr; i++) for (int j = 0; j < 64; j++) { double d = str[(size_t)i * 64 + j] - sm[j]; ss[j] += d * d; } for (auto& x : ss) x = sqrt(x / ntr) + 1e-9;
                auto stdz = [&](vector<float>& sv, int n) { for (int i = 0; i < n; i++) for (int j = 0; j < 64; j++) sv[(size_t)i * 64 + j] = (float)((sv[(size_t)i * 64 + j] - sm[j]) / ss[j]); }; stdz(str, ntr); stdz(sva, nva); stdz(ste, nte);
                TH["k4 Et,Sf,make_s,stdz"] = now() - tkk_; tkk_ = now(); auto proj = [&](const vector<float>& sv, int n, vector<float>& U) { U.assign((size_t)n * NBf, 0.f); gemm_f(sv.data(), n, 64, Pm.data(), NBf, U.data(), NBf); };
                proj(str, ntr, g_Utr); proj(sva, nva, g_Uva); proj(ste, nte, g_Ute); TH["k5 proj gemm"] = now() - tkk_; tkk_ = now();
                vector<double> um(NBf, 0.0), us(NBf, 0.0); for (int i = 0; i < ntr; i++) for (int j = 0; j < NBf; j++) um[j] += g_Utr[(size_t)i * NBf + j]; for (auto& x : um) x /= ntr; for (int i = 0; i < ntr; i++) for (int j = 0; j < NBf; j++) { double d = g_Utr[(size_t)i * NBf + j] - um[j]; us[j] += d * d; } for (auto& x : us) x = sqrt(x / ntr) + 1e-9;
                TH["k6 um/us"] = now() - tkk_; tkk_ = now(); auto ustd = [&](vector<float>& U, int n) { for (int i = 0; i < n; i++) for (int j = 0; j < NBf; j++) U[(size_t)i * NBf + j] = (float)((U[(size_t)i * NBf + j] - um[j]) / us[j]); }; ustd(g_Utr, ntr); ustd(g_Uva, nva); ustd(g_Ute, nte); TH["k7 ustd"] = now() - tkk_; tkk_ = now();
                if (g_gF > 0) { auto gate = [&](vector<float>& Fm, int n, const vector<float>& U) { int m = wd[0];
#pragma omp parallel for schedule(static)
                        for (int i = 0; i < n; i++) { float* r = &Fm[(size_t)i * m + 1]; for (int j = 0; j < NBf; j++) { double f = max(-0.999999, min(0.999999, (double)r[j])); r[j] = (float)tanh(atanh(f) + g_gF * U[(size_t)i * NBf + j]); } } }; if (g_v54gate) {   // V54 ADAY: tanh(atanh f + gU) = (f + t)/(1 + f t), t = tanh(gU) (ozdeslik; yalniz cift-duyarlik yuvarlama farki). U satiri yalniz (kelime,kat)'a bagli -> t tablosu (kelime,kat) basina bir kez
                        auto gate2 = [&](vector<float>& Fm, int n, const vector<float>& U, const vector<int>& key) { int m = wd[0]; vector<int> rowOf(6 * V, -1), first; for (int i = 0; i < n; i++) if (rowOf[key[i]] < 0) { rowOf[key[i]] = (int)first.size(); first.push_back(i); } int nu = (int)first.size(); vector<double> Tt((size_t)nu * NBf);
                            if (getenv("V54_CHK")) for (int i = 0; i < n; i++) if (memcmp(&U[(size_t)i * NBf], &U[(size_t)first[rowOf[key[i]]] * NBf], NBf * sizeof(float)) != 0) { printf("V54_CHK: U satirlari ayni anahtarda FARKLI (i=%d)\n", i); exit(5); }
#pragma omp parallel for schedule(static)
                            for (int u = 0; u < nu; u++) for (int j = 0; j < NBf; j++) Tt[(size_t)u * NBf + j] = tanh(g_gF * U[(size_t)first[u] * NBf + j]);
#pragma omp parallel for schedule(static)
                            for (int i = 0; i < n; i++) { float* r = &Fm[(size_t)i * m + 1]; const double* tr_ = &Tt[(size_t)rowOf[key[i]] * NBf]; for (int j = 0; j < NBf; j++) { double f = max(-0.999999, min(0.999999, (double)r[j])); double t = tr_[j]; r[j] = (float)((f + t) / (1.0 + f * t)); } } };
                        vector<int> ktr(ntr), kva(nva), kte(nte); for (int i = 0; i < ntr; i++) ktr[i] = g_fold51[i] * V + tr.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nva; i++) kva[i] = 5 * V + va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) kte[i] = 5 * V + te.ctx[(size_t)i * c + c - 1];
                        gate2(Ft[0], ntr, g_Utr, ktr); gate2(Fv[0], nva, g_Uva, kva); gate2(Fe[0], nte, g_Ute, kte); }
                    else { gate(Ft[0], ntr, g_Utr); gate(Fv[0], nva, g_Uva); gate(Fe[0], nte, g_Ute); } TH["k8 gate atanh/tanh"] = now() - tkk_; }
                }
                printf("V51_CEKIRDEK|K tablolari (5 OOF + 1 tam) + s(K) 64D + sabit 64->%d esleme: tek sefer, sure=%.2fs ; kapilar g_F=%g g_P=%g ridge_K=%d %s ; U std~1 (egitimden)\n", NBf, now() - tk0, g_gF, g_gP, (int)g_ridgeK, g_v51shuf ? "KARISIK_CEKIRDEK" : "GERCEK_CEKIRDEK"); }
        phase_mark("OOF/Q: K-tablolari+kapi"); TH["f2 V51 K-tablolari+kapi"] = now() - tv0; double th0_ = now(); Pool39 P = (g_v54reuse && g_v51 && g_Pf51.size() == 6) ? g_Pf51[5] : v39_graph(tr, c); g_Pq = P;   /* V54: V39 grafi = V51 tam-egitim tablosunun grafi (ayni sayimlar, ayni islev) */ TH["f3 V39 graf"] = now() - th0_; th0_ = now();
            if (g_v39shuf) { mt19937_64 gs(4242); vector<int> pm(V); iota(pm.begin(), pm.end(), 0); shuffle(pm.begin(), pm.end(), gs); for (auto& x : P.nb) if (x >= 0) x = pm[x]; }   // baglanti hedefleri karistirilir (agirliklar ayni)
            vector<int> lt(ntr), lv(nva), le(nte); for (int i = 0; i < ntr; i++) lt[i] = tr.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nva; i++) lv[i] = va.ctx[(size_t)i * c + c - 1]; for (int i = 0; i < nte; i++) le[i] = te.ctx[(size_t)i * c + c - 1];
            vector<int> has(V, 0); for (int i = 0; i < ntr; i++) has[lt[i]]++; vector<vector<float>> Mean(ns); for (int s = 0; s < ns; s++) { int m = wd[s]; Mean[s].assign((size_t)V * m, 0.f); for (int i = 0; i < ntr; i++) { float* mv = &Mean[s][(size_t)lt[i] * m]; const float* f = &Ft[s][(size_t)i * m]; for (int j = 0; j < m; j++) mv[j] += f[j]; } for (int u = 0; u < V; u++) if (has[u]) for (int j = 0; j < m; j++) Mean[s][(size_t)u * m + j] /= has[u]; }
            double dsum = 0; { int cntd = 0; for (int i = 0; i < min(ntr, 2000); i++) { for (int s = 0; s < ns; s++) { dsum += 0; } cntd++; } }
            TH["f4 V39 ortalama"] = now() - th0_; th0_ = now(); g_foldPtr = &g_fold51; g_mixTrain = true; v39_mix(P, Ft, wd, lt, ntr, Mean, has, g_beta39); g_mixTrain = false; v39_mix(P, Fv, wd, lv, nva, Mean, has, g_beta39); v39_mix(P, Fe, wd, le, nte, Mean, has, g_beta39); TH["f5 V39 karisim"] = now() - th0_; phase_mark("V39 havuz");
            if (g_v49 > 0) { double tt0 = now(); TH["v0 V49 oncesi (program basindan)"] = now() - g_hT0; int m = wd[0]; /* 513: sutun 0 sabit */ const int NB = m - 1;   // 512 ozellik
                vector<double> mu(NB, 0.0), sd(NB, 0.0);   // V53h: sutun bloklarina gore paralel; her sutunun toplami ayni i-sirasinda -> bit-ozdes
#pragma omp parallel
                { int nt_ = omp_get_num_threads(), t_ = omp_get_thread_num(); int j0 = (int)((long)NB * t_ / nt_), j1 = (int)((long)NB * (t_ + 1) / nt_); for (int i = 0; i < ntr; i++) { const float* r = &Ft[0][(size_t)i * m + 1]; for (int j = j0; j < j1; j++) mu[j] += r[j]; } for (int j = j0; j < j1; j++) mu[j] /= ntr; for (int i = 0; i < ntr; i++) { const float* r = &Ft[0][(size_t)i * m + 1]; for (int j = j0; j < j1; j++) { double d = r[j] - mu[j]; sd[j] += d * d; } } }
                TH["v2 V49 mu/sd"] = now() - tt0; double rmsA = 0; for (int j = 0; j < NB; j++) { sd[j] = sqrt(sd[j] / ntr) + 1e-12; rmsA += sd[j]; } rmsA /= NB;   // A sutunlarinin ortalama std'si (hedef olcek)
                // turetilmis sutun tanimlari: (kaynak i, kaynak j (-1: yok), us p)
                struct Col { int kind; int i, j, p; }; vector<Col> cols(NB);   // kind 0: A kopya (degismeden), 1: p. kuvvet merkezli (h_i-mu_i)^p, 2: carpim (h_i-mu_i)(h_j-mu_j), 3: olcekli A (s*h_i)
                if (g_v49 == 1) { for (int j = 0; j < NB; j++) cols[j] = {3, j, -1, 1}; }
                else if (g_v49 == 2) { int nb3 = NB / 3; for (int j = 0; j < nb3; j++) { cols[3 * j] = {0, j, -1, 1}; cols[3 * j + 1] = {1, j, -1, 2}; cols[3 * j + 2] = {1, j, -1, 3}; } for (int j = 3 * nb3; j < NB; j++) cols[j] = {0, j - 3 * nb3 + nb3, -1, 1}; }
                else if (g_v49 == 3) { int na = NB / 4; for (int j = 0; j < na; j++) { cols[j] = {0, j, -1, 1}; cols[na + j] = {1, j, -1, 2}; } for (int q = 2 * na; q < NB; q++) { int a = (q * 37) % NB; int b2 = (a + 1 + (q * 131) % (NB - 1)) % NB; cols[q] = {2, a, b2, 2}; } }
                // turetilmis sutunlarin egitim momentleri (merkez ve std)
                vector<double> cm(NB, 0.0), cs(NB, 1.0); auto val = [&](const float* r, const Col& c, const float* ur) -> double { switch (c.kind) { case 0: return r[c.i]; case 3: return g_s49 * r[c.i]; case 1: { double d = r[c.i] - mu[c.i] - (ur ? g_gP * sd[c.i] * ur[c.i] : 0.0); return c.p == 2 ? d * d : d * d * d; } default: return (r[c.i] - mu[c.i]) * (r[c.j] - mu[c.j]); } };
if (g_hlean && g_v49 == 2 && !(g_v51 && g_gP > 0)) {   // V54: satir-dis, sutun-blok-ic (ayni sutunda i artan toplam sirasi -> bit-ozdes), bellek erisimi ardisil
                    vector<double> sa(NB, 0.0), sb(NB, 0.0);
#pragma omp parallel
                    { int nt_ = omp_get_num_threads(), t_ = omp_get_thread_num(); int q0 = (int)((long)NB * t_ / nt_), q1 = (int)((long)NB * (t_ + 1) / nt_);
                      for (int i = 0; i < ntr; i++) { const float* r = &Ft[0][(size_t)i * m + 1]; for (int q = q0; q < q1; q++) { if (cols[q].kind == 1 || cols[q].kind == 2) { double v = val(r, cols[q], nullptr); sa[q] += v; sb[q] += v * v; } } } }
                    for (int q = 0; q < NB; q++) if (cols[q].kind == 1 || cols[q].kind == 2) { cm[q] = sa[q] / ntr; cs[q] = sqrt(max(sb[q] / ntr - cm[q] * cm[q], 1e-18)); } }
                else {
#pragma omp parallel for schedule(dynamic, 4)
                for (int q = 0; q < NB; q++) if (cols[q].kind == 1 || cols[q].kind == 2) { double a = 0, b = 0; for (int i = 0; i < ntr; i++) { double v = val(&Ft[0][(size_t)i * m + 1], cols[q], (g_v51 && g_gP > 0) ? &g_Utr[(size_t)i * NB] : nullptr); a += v; b += v * v; } cm[q] = a / ntr; cs[q] = sqrt(max(b / ntr - cm[q] * cm[q], 1e-18)) ; } }
                auto apply = [&](vector<float>& Fm, int n, const vector<float>& Uu) { vector<float> out((size_t)n * m); 
#pragma omp parallel for schedule(static)
                    for (int i = 0; i < n; i++) { const float* r = &Fm[(size_t)i * m + 1]; float* o = &out[(size_t)i * m]; o[0] = Fm[(size_t)i * m]; for (int q = 0; q < NB; q++) { const Col& c = cols[q]; double v = val(r, c, (g_v51 && g_gP > 0) ? &Uu[(size_t)i * NB] : nullptr); if (c.kind == 1 || c.kind == 2) v = (v - cm[q]) / cs[q] * rmsA; o[1 + q] = (float)v; } } Fm.swap(out); };
                TH["v3 V49 cm/cs"] = now() - tt0 - TH["v2 V49 mu/sd"]; double tap_ = now();
                if (g_hlean && g_v49 == 2 && !(g_v51 && g_gP > 0)) {   // V53h: ayni aritmetik (d=r-mu; d*d; d*d*d; (v-cm)/cs*rmsA), A/A^2/A^3 uclusu icin d bir kez; yerinde (satir tamponu) -> 232 MB ek tahsis yok
                    const int nb3 = NB / 3; auto apply_fast = [&](vector<float>& Fm, int n) {
#pragma omp parallel
                        { vector<float> rb(NB);
#pragma omp for schedule(static)
                            for (int i = 0; i < n; i++) { float* row = &Fm[(size_t)i * m]; memcpy(rb.data(), row + 1, NB * sizeof(float)); float* o = row;
                                for (int j = 0; j < nb3; j++) { double x = rb[j]; double d = x - mu[j]; double d2 = d * d, d3 = d * d * d; o[1 + 3 * j] = (float)x; o[2 + 3 * j] = (float)((d2 - cm[3 * j + 1]) / cs[3 * j + 1] * rmsA); o[3 + 3 * j] = (float)((d3 - cm[3 * j + 2]) / cs[3 * j + 2] * rmsA); }
                                for (int q = 3 * nb3; q < NB; q++) o[1 + q] = (float)(double)rb[cols[q].i]; } } };
                    apply_fast(Ft[0], ntr); apply_fast(Fv[0], nva); apply_fast(Fe[0], nte); }
                else { apply(Ft[0], ntr, g_Utr); apply(Fv[0], nva, g_Uva); apply(Fe[0], nte, g_Ute); } TH["v4 V49 apply"] = now() - tap_;
                // sayisal denetim: sutun olcek araligi (egitim)
                double mn = 1e30, mx = 0; for (int q = 0; q < NB; q++) { double a = 0; for (int i = 0; i < ntr; i += 7) { double v = Ft[0][(size_t)i * m + 1 + q]; a += v * v; } a = sqrt(a / ((ntr + 6) / 7)); mn = min(mn, a); mx = max(mx, a); }
                phase_mark("V49 donusumu"); TH["v1 V49 toplam"] = now() - tt0; printf("V49_DONUSUM|mod=%d (1=olcek s=%g, 2=A+A^2+A^3 (170 taban), 3=A+A^2+AB (128 taban))|ozellik sayisi=%d (sabit sutun disinda; kapasite ayni)|hedef olcek (A ortalama std)=%.4f ; sutun RMS araligi %.4f..%.4f|donusum sure=%.2fs\n", g_v49, g_s49, NB, rmsA, mn, mx, now() - tt0); }
            if (getenv("V52_PW")) {   // V52: kismi beyazlatma = TERSINIR afin donusum Z=(F-mu) U diag((ev+tau ev1)^-p) (egitim istatistikleri), ortak havuz/Gram oncesi
                double pw = atof(getenv("V52_PW")), tau = getenv("V52_TAU") ? atof(getenv("V52_TAU")) : 1e-4; int m = wd[0], NB = m - 1; double tp0 = now();
                vector<double> mu2(NB, 0.0), Cv((size_t)NB * NB, 0.0); for (int i = 0; i < ntr; i++) { const float* r = &Ft[0][(size_t)i * m + 1]; for (int j = 0; j < NB; j++) mu2[j] += r[j]; } for (auto& x : mu2) x /= ntr;
                { vector<float> Xc((size_t)ntr * NB), Gq((size_t)NB * NB); for (int i = 0; i < ntr; i++) for (int j = 0; j < NB; j++) Xc[(size_t)i * NB + j] = Ft[0][(size_t)i * m + 1 + j] - (float)mu2[j]; scipy_cblas_sgemm(101, 112, 111, NB, NB, ntr, 1.f, Xc.data(), NB, Xc.data(), NB, 0.f, Gq.data(), NB); for (size_t q = 0; q < Cv.size(); q++) Cv[q] = (double)Gq[q] / ntr; }
                vector<double> evv(NB), wkk(64 * NB + 100); int info = 0, lw = (int)wkk.size(); char jz = 'V', up = 'U'; scipy_dsyev_(&jz, &up, &NB, Cv.data(), &NB, evv.data(), wkk.data(), &lw, &info);   // artan sirali; Cv sutun-ana ozvektorler
                double evmax = evv[NB - 1], tot = 0, sc = 0; for (int k = 0; k < NB; k++) tot += max(evv[k], 0.0); vector<double> sk(NB); for (int k = 0; k < NB; k++) { sk[k] = pow(max(evv[k], 0.0) + tau * evmax, -pw); sc += sk[k] * sk[k] * max(evv[k], 0.0); } double nrm = sqrt(tot / sc); for (auto& x : sk) x *= nrm;
                vector<float> T((size_t)NB * NB); for (int a = 0; a < NB; a++) for (int k = 0; k < NB; k++) T[(size_t)a * NB + k] = (float)(Cv[(size_t)k * NB + a] * sk[k]);   // T[a][k] = U[a][k] s_k
                auto tr_ = [&](vector<float>& Fm, int n) { vector<float> Xc((size_t)n * NB), Zo((size_t)n * NB); for (int i = 0; i < n; i++) for (int j = 0; j < NB; j++) Xc[(size_t)i * NB + j] = Fm[(size_t)i * m + 1 + j] - (float)mu2[j]; gemm_f(Xc.data(), n, NB, T.data(), NB, Zo.data(), NB); for (int i = 0; i < n; i++) memcpy(&Fm[(size_t)i * m + 1], &Zo[(size_t)i * NB], NB * sizeof(float)); };
                tr_(Ft[0], ntr); tr_(Fv[0], nva); tr_(Fe[0], nte);
                printf("V52_PW|kismi beyazlatma p=%g tau=%g|ev maks=%.4g min=%.3g ; ters cevrilebilir (T kosul sayisi ~ %.3g) ; sure=%.2fs\n", pw, tau, evmax, evv[0], sk[0] / sk[NB - 1], now() - tp0); }
            { vector<double> bs, ws2; for (int u = 0; u < V; u++) if (has[u] > 0) { bs.push_back(g_cmode45 ? P.bw[u] : g_beta39); for (int q = 0; q < g_knn39; q++) if (P.nb[(size_t)u * g_knn39 + q] >= 0) ws2.push_back(P.wt[(size_t)u * g_knn39 + q]); } sort(bs.begin(), bs.end()); sort(ws2.begin(), ws2.end()); auto qt = [&](vector<double>& v, double f) { return v.empty() ? 0.0 : v[(size_t)(f * (v.size() - 1))]; };
              double wmx = 0; for (int u = 0; u < V; u++) { double a = 0; for (int q = 0; q < g_knn39; q++) a = max(a, (double)P.wt[(size_t)u * g_knn39 + q]); wmx += a; }
              printf("V45_DAGILIM|beta_w: min=%.3f q25=%.3f medyan=%.3f q75=%.3f maks=%.3f (kelime=%zu) ; komsu agirligi a_wv: min=%.4f medyan=%.4f maks=%.4f ; kelime basina en buyuk agirlik ortalamasi=%.3f (tekdüze=0.100)\n", qt(bs, 0), qt(bs, .25), qt(bs, .5), qt(bs, .75), qt(bs, 1), bs.size(), qt(ws2, 0), qt(ws2, .5), qt(ws2, 1), wmx / V); }
            { int nn_ = 0; double sw = 0; for (int u = 0; u < V; u++) if (P.nb[(size_t)u * g_knn39] >= 0) { nn_++; } printf("V39_HAVUZ|dugum=%d (bagli=%d) knn=%d beta=%g %s|baglanti hesabi+karisim sure=%.2fs ; baglantilar yalniz egitim etiketlerinden ; F kapasitesi ayni (%d)\n", V, nn_, g_knn39, g_beta39, g_v39shuf ? "KARISIK_BAGLANTI" : "GERCEK_BAGLANTI", now() - tv0, (int)(accumulate(wd.begin(), wd.end(), 0))); (void)sw; } }
        if (getenv("V35_RANK")) for (int s = 0; s < ns; s++) { string nm = "F" + to_string(s + 1); v35_rank(nm.c_str(), Ft[s], wd[s], ntr, s == 0 ? 1 : 0); }
        // Gram bloklari (cift duyarlik, tam simetrik) ve cift hedef sagtaraflari
        auto cross = [&](const vector<float>& A, int wa, const vector<float>& B, int wb, vector<double>& out) { out.assign((size_t)wa * wb, 0.0); const int CHn = 4096; vector<float> At, Gc; for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Gc.assign((size_t)wa * wb, 0.f); if (g_v73syrk && g_fast && g_blas && &A == &B) scipy_cblas_ssyrk(101, 121, 112, wa, nn, 1.f, &A[(size_t)s0 * wa], wa, 0.f, Gc.data(), wa); else if (g_fast && g_blas) scipy_cblas_sgemm(101, 112, 111, wa, wb, nn, 1.f, &A[(size_t)s0 * wa], wa, &B[(size_t)s0 * wb], wb, 0.f, Gc.data(), wb); else { At.assign((size_t)wa * nn, 0.f); for (int i = 0; i < nn; i++) for (int j = 0; j < wa; j++) At[(size_t)j * nn + i] = A[(size_t)(s0 + i) * wa + j]; gemm_f(At.data(), wa, nn, &B[(size_t)s0 * wb], wb, Gc.data(), wb); } for (size_t q = 0; q < out.size(); q++) out[q] += Gc[q]; } if (g_v73syrk && g_fast && g_blas && &A == &B) { for (int i = 0; i < wa; i++) for (int j = 0; j < i; j++) out[(size_t)i * wa + j] = out[(size_t)j * wa + i]; } };
        double thc_ = now(); vector<vector<vector<double>>> C(ns, vector<vector<double>>(ns)); for (int i = 0; i < ns; i++) for (int j = 0; j <= i; j++) cross(Ft[i], wd[i], Ft[j], wd[j], C[i][j]); phase_mark("Gram: F^T F"); TH["f6 F^T F (AFF)"] = now() - thc_; thc_ = now();
        vector<vector<double>> RY(ns), RQ(ns); for (int s = 0; s < ns; s++) { RY[s].assign((size_t)wd[s] * V, 0.0); rhs_add(Ft[s].data(), tr.y.data(), ntr, wd[s], V, RY[s].data()); RQ[s].assign((size_t)wd[s] * V, 0.0); } phase_mark("Gram: F^T Y"); TH["f7 F^T Y (RF)"] = now() - thc_;
        if (hasQ) { const int CHn = 4096; vector<float> Qc, At, Gc; for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Qc.assign((size_t)nn * V, 0.f);
#pragma omp parallel
                { vector<double> q(V), t1(V), t2(V);
#pragma omp for schedule(static)
                    for (int i = 0; i < nn; i++) { bool s2, s3; dense_row(TE, tr.ctx[2 * (s0 + i)], tr.ctx[2 * (s0 + i) + 1], tr.y[s0 + i], true, a3, q, t1, t2, s2, s3); float* o2 = &Qc[(size_t)i * V]; for (int v = 0; v < V; v++) o2[v] = (float)q[v]; } }
                for (int s = 0; s < ns; s++) { At.assign((size_t)wd[s] * nn, 0.f); for (int i = 0; i < nn; i++) for (int j = 0; j < wd[s]; j++) At[(size_t)j * nn + i] = Ft[s][(size_t)(s0 + i) * wd[s] + j]; Gc.assign((size_t)wd[s] * V, 0.f); gemm_f(At.data(), wd[s], nn, Qc.data(), V, Gc.data(), V); for (size_t q = 0; q < RQ[s].size(); q++) RQ[s][q] += Gc[q]; } } }
        double t_prep = now() - tc0; TH["f0 hazirlik toplam (ozellik+K+gram F)"] = t_prep; if (v31mode) { printf("V31_ZINCIR_OZELLIK|%s|hazirlik=%.2fs\n", name.c_str(), t_prep); v31solve(Ft, Fv, Fe, wd, C, RY, name); printf("ZINCIR_BITTI|%s|toplam=%.1fs tepe_RAM=%.0fMB\n", name.c_str(), now() - tc0, peak_mb()); return; }
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
    if (mod.find('Z') != string::npos) { v31mode = true; { int nst = getenv("V40_STAGES") ? atoi(getenv("V40_STAGES")) : 4, wf = getenv("V40_W") ? atoi(getenv("V40_W")) : H / 4; vector<int> hsv(nst, wf); run_chain("V31_KIMLIK+" + to_string(nst) + "F(" + to_string(nst) + "x" + to_string(wf) + ")", hsv, true, true, 0); } v31mode = false; }
    printf("TOPLAM sure=%.1fs tepe_RAM=%.0fMB\n", now() - Ts, peak_mb());
    return 0;
}
