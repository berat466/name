// Tek hamle V26c (V25 dogrulamasi, adil okuma): V25: OLCUM/TESHIS: Z=[X,1] (A), Z=F (B), Z=[F,phi(c)] seyrek baglam kimligi (C); ayni Ridge/Cholesky; hedef Y ve Q. V24 kopyasi (kernel/n-gram); onceki dosyalara dokunulmadi.
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
struct FS { int mz = 0; vector<float> tr, va, te; };
struct Sp { int P = 0; vector<int> tr, va, te; };   // satir basina 2 kimlik: [kelime w kimligi, siklik cift (u,w) kimligi veya -1]
int main(int argc, char** argv) {
    if (argc < 6) { cerr << "kullanim: tek_hamle_v25 c H TOHUM gamma lambda [mod=ABC] [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), lam0 = atof(argv[5]); string mod = argc > 6 ? argv[6] : "ABC"; string dir = argc > 7 ? argv[7] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte);
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); KT = K; BASEA = BASEB = BASEP = K; }
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V25 c=%d H=%d tohum=%d gamma=%g lambda_V17=%g mod=%s K(X boyutu)=%d (%d cekirdek); ogretici+agirlik: yalniz egitim; dogrulama secim; test bagimsiz\n", c, H, seed, gam, lam0, mod.c_str(), K, omp_get_max_threads());
    double Ts = now();
    Teach TE; build_teach(TE, tr.ctx, tr.y); double a3[4]; em_weights(TE, va.ctx, va.y, 3, a3);
    NG ngt = ng_eval(TE, te.ctx, te.y, a3, false); print_ng("trigram", "test(egitim sayimlari)", ngt); NG ngv = ng_eval(TE, va.ctx, va.y, a3, false); print_ng("trigram", "dogrulama", ngv);
    vector<double> pi(V); { double tot = 0; for (int v = 0; v < V; v++) tot += TE.c1[v] + 1.0; for (int v = 0; v < V; v++) pi[v] = (TE.c1[v] + 1.0) / tot; }
    // seyrek kimlikler: w (bir onceki kelime) + egitimde en sik NP (u,w) cifti; YALNIZ gecmis baglam kelimelerinden
    const int NP = 2000; unordered_map<long long, int> pidx; { unordered_map<long long, int> cn; for (int i = 0; i < ntr; i++) cn[(long long)tr.ctx[2 * i] * V + tr.ctx[2 * i + 1]]++; vector<pair<int, long long>> v; for (auto& kv : cn) v.push_back({-kv.second, kv.first}); sort(v.begin(), v.end()); for (int k = 0; k < NP && k < (int)v.size(); k++) pidx[v[k].second] = k; printf("SEYREK: egitimde %zu farkli (u,w) cifti; en sik %d tanesi kimlik alir (en az %d tekrar)\n", cn.size(), (int)pidx.size(), v.size() > (size_t)NP ? -v[NP - 1].first : 0); }
    auto mksp = [&](const Data& d, int n, bool pairs) { vector<int> ids((size_t)n * 2, -1); for (int i = 0; i < n; i++) { ids[2 * i] = d.ctx[2 * i + 1]; if (pairs) { auto it = pidx.find((long long)d.ctx[2 * i] * V + d.ctx[2 * i + 1]); if (it != pidx.end()) ids[2 * i + 1] = V + it->second; } } return ids; };
    // ---- genel sistem cozucu
    auto qrowTr = [&](int gi, vector<double>& q, vector<double>& t1, vector<double>& t2) { bool s2, s3; dense_row(TE, tr.ctx[2 * gi], tr.ctx[2 * gi + 1], tr.y[gi], true, a3, q, t1, t2, s2, s3); };   // Q: egitim satirlari LOO
    auto run_system = [&](const string& name, const FS& fs, const Sp* sp, const vector<double>& lams) {
        double t0 = now(); int mz = fs.mz, P = sp ? sp->P : 0, mt = mz + P; size_t MT = mt;
        vector<double> S(MT * MT, 0.0), RY(MT * V, 0.0), RQ(MT * V, 0.0);
        { vector<double> S11((size_t)mz * mz, 0.0); syrk_flush256(fs.tr.data(), ntr, mz, S11.data()); for (int i = 0; i < mz; i++) for (int j = 0; j <= i; j++) { double v = S11[(size_t)i * mz + j]; if (v == 0.0) v = S11[(size_t)j * mz + i]; S[(size_t)i * mt + j] = v; } }
        { vector<double> R1((size_t)mz * V, 0.0); rhs_add(fs.tr.data(), tr.y.data(), ntr, mz, V, R1.data()); for (size_t i = 0; i < R1.size(); i++) RY[i] = R1[i]; }
        // Q'ya dogru: F^T Q (yogun kisim) + seyrek satirlar
        { const int CHn = 4096; vector<float> Qc, Ft, Gc; vector<double> Rq((size_t)mz * V, 0.0);
            for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Qc.assign((size_t)nn * V, 0.f);
#pragma omp parallel
                { vector<double> q(V), t1(V), t2(V);
#pragma omp for schedule(static)
                    for (int i = 0; i < nn; i++) { qrowTr(s0 + i, q, t1, t2); float* o = &Qc[(size_t)i * V]; for (int v = 0; v < V; v++) o[v] = (float)q[v]; } }
                Ft.assign((size_t)mz * nn, 0.f); const float* Fb = fs.tr.data() + (size_t)s0 * mz; for (int i = 0; i < nn; i++) for (int j = 0; j < mz; j++) Ft[(size_t)j * nn + i] = Fb[(size_t)i * mz + j];
                Gc.assign((size_t)mz * V, 0.f); gemm_f(Ft.data(), mz, nn, Qc.data(), V, Gc.data(), V); for (size_t i = 0; i < Rq.size(); i++) Rq[i] += Gc[i];
                if (P) for (int i = 0; i < nn; i++) for (int k = 0; k < 2; k++) { int id = sp->tr[(size_t)(s0 + i) * 2 + k]; if (id < 0) continue; double* dst = &RQ[(size_t)(mz + id) * V]; const float* src = &Qc[(size_t)i * V]; for (int v = 0; v < V; v++) dst[v] += src[v]; } }
            for (size_t i = 0; i < Rq.size(); i++) RQ[i] = Rq[i]; }
        if (P) { for (int i = 0; i < ntr; i++) { int ids[2] = {sp->tr[(size_t)i * 2], sp->tr[(size_t)i * 2 + 1]}; const float* Fr = fs.tr.data() + (size_t)i * mz;
                for (int k = 0; k < 2; k++) if (ids[k] >= 0) { double* row = &S[(size_t)(mz + ids[k]) * mt]; for (int j = 0; j < mz; j++) row[j] += Fr[j]; RY[(size_t)(mz + ids[k]) * V + tr.y[i]] += 1; S[(size_t)(mz + ids[k]) * mt + mz + ids[k]] += 1; }
                if (ids[1] >= 0) S[(size_t)(mz + ids[1]) * mt + mz + ids[0]] += 1; } }
        for (int i = 0; i < mt; i++) for (int j = 0; j < i; j++) S[(size_t)j * mt + i] = S[(size_t)i * mt + j];   // tam simetrik (cholesky hangi ucgeni okursa okusun)
        double t_gram = now() - t0; printf("SISTEM|%s|mz=%d P=%d toplam=%d|gram+RHS suresi=%.2fs\n", name.c_str(), mz, P, mt, t_gram); fflush(stdout);
        auto logits = [&](const vector<float>& F, const vector<int>& ids, int n, const vector<float>& W) { vector<float> Z((size_t)n * V); gemm_f(F.data(), n, mz, W.data(), V, Z.data(), V);
            if (P) {
#pragma omp parallel for schedule(static)
                for (int i = 0; i < n; i++) for (int k = 0; k < 2; k++) { int id = ids[(size_t)i * 2 + k]; if (id < 0) continue; const float* w = &W[(size_t)(mz + id) * V]; float* z = &Z[(size_t)i * V]; for (int v = 0; v < V; v++) z[v] += w[v]; } } return Z; };
        for (double lam : lams) { double t1c = now(); vector<double> A = S; for (int i = 0; i < mt; i++) A[(size_t)i * mt + i] += lam; cholesky(A, mt); vector<float> WY, WQ; chol_solve_multi(A, mt, RY, V, WY); chol_solve_multi(A, mt, RQ, V, WQ); double t_solve = now() - t1c; { double nw = 0, nr = 0, ns = 0; for (float x : WY) nw += (double)x * x; for (double x : RY) nr += x * x; for (double x : S) ns += x * x; printf("DEBUG|%s|lam=%g|||WY||=%.4f |||RY||=%.4f |||S||=%.4g S[0]=%.4g S[1,1]=%.4g S[1,0]=%.4g\n", name.c_str(), lam, sqrt(nw), sqrt(nr), sqrt(ns), S[0], S[(size_t)mt + 1], S[(size_t)mt]); }
            for (int tg = 0; tg < 2; tg++) { const vector<float>& W = tg == 0 ? WY : WQ; auto Zv = logits(fs.va, sp ? sp->va : vector<int>(), nva, W); static double ce[NT][NA]; ce_table(Zv, va.y.data(), nva, Ptr, ce); int bt = 0, ba = 0; double bv = 1e30; for (int t = 0; t < NT; t++) if (TV9[t]) for (int a = 0; a < NA; a++) if (ce[t][a] < bv) { bv = ce[t][a]; bt = t; ba = a; }
                double al = AG[ba], T = TG[bt]; auto Zt = logits(fs.te, sp ? sp->te : vector<int>(), nte, W); auto Zr = logits(fs.tr, sp ? sp->tr : vector<int>(), ntr, W); Res rv = eval_cal(Zv, va.y.data(), nva, Ptr, al, T), rt = eval_cal(Zt, te.y.data(), nte, Ptr, al, T), rr = eval_cal(Zr, tr.y.data(), ntr, Ptr, al, T); Row rm = row_metrics(Zt, te.y.data(), nte, Ptr, al, T);
                // Q'ya yaklasim (karesel hata / acik varyans) ve KL; egitim satirlari LOO
                double rel[3], kl[3], qce[3]; auto dist = [&](const vector<float>& Z, int n, const vector<int>& ctx, const vector<int>& yv, bool loo, int which) { double num = 0, den = 0, klt = 0, qc = 0;
#pragma omp parallel reduction(+ : num, den, klt, qc)
                    { vector<double> p(V), c2(V), c3(V);
#pragma omp for schedule(static)
                        for (int i = 0; i < n; i++) { bool s2, s3; dense_row(TE, ctx[2 * i], ctx[2 * i + 1], yv[i], loo, a3, p, c2, c3, s2, s3); const float* z = &Z[(size_t)i * V]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]); double se = 0; for (int v = 0; v < V; v++) se += exp((z[v] - mx) / T);
                            for (int v = 0; v < V; v++) { double d = z[v] - p[v], e = p[v] - pi[v]; num += d * d; den += e * e; double ps = (1 - al) * Ptr[v] + al * exp((z[v] - mx) / T) / se; if (p[v] > 0) klt += p[v] * (log(p[v]) - log(ps)); } qc += -log(p[yv[i]]); } }
                    rel[which] = num / den; kl[which] = klt / n; qce[which] = qc / n; };
                dist(Zr, ntr, tr.ctx, tr.y, true, 0); dist(Zv, nva, va.ctx, va.y, false, 1); dist(Zt, nte, te.ctx, te.y, false, 2);
                { double cl[NEPS][NAL]; lin_table(Zv, va.y.data(), nva, Ptr, cl); double bl = 1e30; int be = 0, ba2 = 0; for (int e = 0; e < NEPS; e++) for (int a = 0; a < NAL; a++) if (cl[e][a] < bl) { bl = cl[e][a]; be = e; ba2 = a; }
                  static double cf[NTF][NAL]; smf_table(Zv, va.y.data(), nva, Ptr, cf); double bf = 1e30; int bt2 = 0, bb2 = 0; for (int t = 0; t < NTF; t++) for (int a = 0; a < NAL; a++) if (cf[t][a] < bf) { bf = cf[t][a]; bt2 = t; bb2 = a; }
                  for (int kd = 0; kd < 2; kd++) { double a_ = kd ? ALG[ba2] : ALG[bb2], p_ = kd ? EPG[be] : TFG[bt2]; RD q1 = rd_eval(Zr, tr.y.data(), ntr, Ptr, kd, a_, p_), q2 = rd_eval(Zv, va.y.data(), nva, Ptr, kd, a_, p_), q3 = rd_eval(Zt, te.y.data(), nte, Ptr, kd, a_, p_);
                    printf("READ|%s|%s|lambda=%g|%s|alpha=%g|%s=%g|egitim CE=%.4f PPL=%.1f|dogr CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|z<=0 orani=%.4f|y_kirpilan=%.4f|olasilik_toplam_sapma=%.2e|sinir:%s\\n", name.c_str(), tg == 0 ? "Y" : "Q", lam, kd ? "DOGRUSAL" : "SOFTMAX_INCE", a_, kd ? "eps" : "T", p_, q1.ce, q1.ppl, q2.ce, q2.ppl, q2.top1, q2.top5, q3.ce, q3.ppl, q3.top1, q3.top5, q3.negfrac, q3.yclip, q3.maxdev, kd ? ((ba2 == 0 || ba2 == NAL - 1 || be == 0 || be == NEPS - 1) ? "KENAR" : "ic") : ((bb2 == 0 || bb2 == NAL - 1 || bt2 == 0 || bt2 == NTF - 1) ? "KENAR" : "ic")); } }
                double cs[3] = {0, 0, 0}, ct1[3] = {0, 0, 0}; int cc[3] = {0, 0, 0}; for (int i = 0; i < nte; i++) { int k = ngt.cat[i]; cc[k]++; cs[k] += rm.ce[i]; ct1[k] += rm.t1[i]; }
                printf("RES|%s|%s|lambda=%g|alpha=%g|T=%g|egitim CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|dogr CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|dogr-egitim CE=%.4f|rel_hata e/d/t=%.4f/%.4f/%.4f|KL e/d/t=%.4f/%.4f/%.4f|baglam goruldu: CE=%.4f top1=%.4f (n=%d); yalniz bigram: CE=%.4f top1=%.4f (n=%d)|cozum suresi=%.1fs\n", name.c_str(), tg == 0 ? "Y" : "Q", lam, al, T, rr.ce, rr.ppl, rr.top1, rr.top5, rv.ce, rv.ppl, rv.top1, rv.top5, rt.ce, rt.ppl, rt.top1, rt.top5, rv.ce - rr.ce, rel[0], rel[1], rel[2], kl[0], kl[1], kl[2], cs[2] / max(1, cc[2]), ct1[2] / max(1, cc[2]), cc[2], cs[1] / max(1, cc[1]), ct1[1] / max(1, cc[1]), cc[1], t_solve); fflush(stdout); } } };
    // ---- A: Z=[1,X]
    if (mod.find('A') != string::npos) { FS fa; fa.mz = K + 1; auto mkx = [&](const Data& d, int n, vector<float>& o) { o.assign((size_t)n * fa.mz, 1.f); for (int i = 0; i < n; i++) for (int j = 0; j < K; j++) o[(size_t)i * fa.mz + 1 + j] = d.X[(size_t)i * K + j]; }; mkx(tr, ntr, fa.tr); mkx(va, nva, fa.va); mkx(te, nte, fa.te); run_system("A_X_dogrudan(K=" + to_string(K) + ")", fa, nullptr, {1, 10, 100}); }
    // ---- B / C: F
    FS fb; { Feat f = make_w1("cift", tr, H, seed); features(tr, f, gam, fb.tr); features(va, f, gam, fb.va); features(te, f, gam, fb.te); fb.mz = H + 1; }
    if (mod.find('B') != string::npos) run_system("B_F(H=" + to_string(H) + ")", fb, nullptr, {lam0, 100});
    if (mod.find('C') != string::npos) { Sp s1; s1.P = V; s1.tr = mksp(tr, ntr, false); s1.va = mksp(va, nva, false); s1.te = mksp(te, nte, false); run_system("C1_F+w_kimligi(P=" + to_string(s1.P) + ")", fb, &s1, {lam0, 100}); }
    if (mod.find('D') != string::npos) { Sp s2; s2.P = V + NP; s2.tr = mksp(tr, ntr, true); s2.va = mksp(va, nva, true); s2.te = mksp(te, nte, true); run_system("C2_F+w_kimligi+sik_cift(P=" + to_string(s2.P) + ")", fb, &s2, {lam0}); }
    if (mod.find('E') != string::npos) { FS f0; f0.mz = 1; f0.tr.assign(ntr, 1.f); f0.va.assign(nva, 1.f); f0.te.assign(nte, 1.f); Sp s2; s2.P = V + NP; s2.tr = mksp(tr, ntr, true); s2.va = mksp(va, nva, true); s2.te = mksp(te, nte, true); run_system("C0_yalniz_kimlik_(F_yok)(P=" + to_string(s2.P) + ")", f0, &s2, {lam0}); }
    printf("TOPLAM sure=%.1fs tepe_RAM=%.0fMB\n", now() - Ts, peak_mb());
    return 0;
}
