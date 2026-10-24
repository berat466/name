// Tek hamle V24: OLCUM/TESHIS (n-gram taban cizgileri, bilgi aktarimi Q->F->W, kapasite, hedef->agirlik iliskisi). V23 kopyasi (kernel); onceki dosyalara dokunulmadi.
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
int main(int argc, char** argv) {
    if (argc < 6) { cerr << "kullanim: tek_hamle_v24 c H TOHUM gamma lambda [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), lam0 = atof(argv[5]); string dir = argc > 6 ? argv[6] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); const int m = H + 1;
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); KT = K; BASEA = BASEB = BASEP = K; }
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V24 c=%d H=%d tohum=%d gamma=%g lambda_V17=%g (%d cekirdek); TUM ogrenciler egitim-yalniz; n-gram sayimlari egitim (test_tv: egitim+dogrulama yalniz 137 yeniden uretimi icin)\n", c, H, seed, gam, lam0, omp_get_max_threads());
    double Ts = now();
    // ===== A: n-gram taban cizgileri
    Teach TE; build_teach(TE, tr.ctx, tr.y); vector<int> cat_ctx = tr.ctx, cat_y = tr.y; Teach TV; { vector<int> cx = tr.ctx, yy = tr.y; cx.insert(cx.end(), va.ctx.begin(), va.ctx.end()); yy.insert(yy.end(), va.y.begin(), va.y.end()); build_teach(TV, cx, yy); }
    double a1[4] = {0, 0, 1, 0}, a2[4], a3[4]; em_weights(TE, va.ctx, va.y, 2, a2); em_weights(TE, va.ctx, va.y, 3, a3);
    printf("NG_AGIRLIK (EM, dogrulama, egitim sayimlari) bigram: a2=%.3f a1=%.3f a0=%.4f ; trigram: a3=%.3f a2=%.3f a1=%.3f a0=%.4f\n", a2[1], a2[2], a2[3], a3[0], a3[1], a3[2], a3[3]);
    struct NM { const char* nm; const double* a; } nms[3] = {{"unigram", a1}, {"bigram", a2}, {"trigram", a3}};
    NG ngtest[3];
    for (int k = 0; k < 3; k++) { NG rt = ng_eval(TE, tr.ctx, tr.y, nms[k].a, false), rl = ng_eval(TE, tr.ctx, tr.y, nms[k].a, true), rv = ng_eval(TE, va.ctx, va.y, nms[k].a, false); ngtest[k] = ng_eval(TE, te.ctx, te.y, nms[k].a, false); NG rtv = ng_eval(TV, te.ctx, te.y, nms[k].a, false);
        print_ng(nms[k].nm, "egitim(sayim kendini icerir)", rt); print_ng(nms[k].nm, "egitim(LOO)", rl); print_ng(nms[k].nm, "dogrulama", rv); print_ng(nms[k].nm, "test(egitim sayimlari)", ngtest[k]); print_ng(nms[k].nm, "test(egitim+dogrulama sayimlari; V8 protokolu)", rtv); }
    { // kapsama
        auto cov = [&](const char* nm, const vector<int>& ctx, const vector<int>& y) { int n = (int)y.size(), s2 = 0, s3 = 0, s3y = 0, s2y = 0; for (int i = 0; i < n; i++) { double q[4]; bool av[2]; comp_y(TE, ctx[2 * i], ctx[2 * i + 1], y[i], false, q, av); s2 += av[0]; s3 += av[1]; s3y += q[0] > 0; s2y += q[1] > 0; } printf("KAPSAMA|%s|n=%d|bigram baglami goruldu=%.4f|trigram baglami goruldu=%.4f|bigram (baglam,hedef) goruldu=%.4f|trigram (baglam,hedef) goruldu=%.4f\n", nm, n, (double)s2 / n, (double)s3 / n, (double)s2y / n, (double)s3y / n); };
        cov("dogrulama", va.ctx, va.y); cov("test", te.ctx, te.y); }
    // sik/orta/nadir (V20 tanimi, egitim hedef sayimi)
    vector<int> ord(V); iota(ord.begin(), ord.end(), 0); vector<double> cnt(V, 0); for (int i = 0; i < ntr; i++) cnt[tr.y[i]] += 1; sort(ord.begin(), ord.end(), [&](int a, int b) { return cnt[a] > cnt[b]; });
    vector<int> grp(V, 2); { double cum = 0; for (int v : ord) { double fr = cum / ntr; grp[v] = fr < 0.5 ? 0 : (fr < 0.8 ? 1 : 2); cum += cnt[v]; } }
    auto groups = [&](const char* nm, const vector<float>& cer, const vector<char>& t1, const vector<char>& t5) { const char* gn[3] = {"sik", "orta", "nadir"}; double s[3] = {0, 0, 0}, a[3] = {0, 0, 0}, b[3] = {0, 0, 0}; int cn[3] = {0, 0, 0}; double sc[3] = {0, 0, 0}, ac[3] = {0, 0, 0}; int cc[3] = {0, 0, 0};
        for (int i = 0; i < nte; i++) { int g = grp[te.y[i]]; cn[g]++; s[g] += cer[i]; a[g] += t1[i]; b[g] += t5[i]; int k = ngtest[2].cat[i]; cc[k]++; sc[k] += cer[i]; ac[k] += t1[i]; }
        for (int g = 0; g < 3; g++) printf("GRUP|%s|%s|n=%d|CE=%.4f|top1=%.4f|top5=%.4f\n", nm, gn[g], cn[g], s[g] / cn[g], a[g] / cn[g], b[g] / cn[g]);
        for (int k = 0; k < 3; k++) printf("BAGLAM|%s|trigram baglami %s|n=%d|CE=%.4f|top1=%.4f\n", nm, k == 2 ? "GORULDU" : (k == 1 ? "yalniz bigram goruldu" : "ikisi de gorulmedi"), cc[k], cc[k] ? sc[k] / cc[k] : 0, cc[k] ? ac[k] / cc[k] : 0); };
    for (int k = 0; k < 3; k++) groups(nms[k].nm, ngtest[k].cer, ngtest[k].t1, ngtest[k].t5);
    // ===== ogrenciler: ozellikler, gram
    Feat f = make_w1("cift", tr, H, seed); vector<float> Ftr, Fva, Fte; features(tr, f, gam, Ftr); features(va, f, gam, Fva); features(te, f, gam, Fte);
    double t0 = now(); vector<double> S((size_t)m * m, 0.0), Rhot((size_t)m * V, 0.0); syrk_flush256(Ftr.data(), ntr, m, S.data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, Rhot.data()); double t_gram = now() - t0;
    auto solveW = [&](const vector<double>& R, double lam) { vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam; cholesky(A, m); vector<float> W; chol_solve_multi(A, m, R, V, W); return W; };
    vector<double> pi(V); { double tot = 0; for (int v = 0; v < V; v++) tot += TE.c1[v] + 1.0; for (int v = 0; v < V; v++) pi[v] = (TE.c1[v] + 1.0) / tot; }
    // tam degerlendirme: (alpha,T) dogrulama CE ile; egitim/dogr/test; test satir metrikleri; Q ile distilasyon olcumu (qa != nullptr: trigram agirlik a, qloo: egitim satirlarinda LOO)
    struct SE { double al, T; Res tr, va, te; Row rte; double rel[3], kl[3], qce[3]; };
    auto lg = [&](const vector<float>& F, int n, const vector<float>& W) { vector<float> Z((size_t)n * V); gemm_f(F.data(), n, m, W.data(), V, Z.data(), V); return Z; };
    auto distill = [&](const vector<float>& Z, int n, const vector<int>& ctx, const vector<int>& y, bool loo, const double* qa, double al, double T, double& rel, double& kl, double& qce) { double num = 0, den = 0, klt = 0, qc = 0;
#pragma omp parallel reduction(+ : num, den, klt, qc)
        { vector<double> p(V), c2(V), c3(V);
#pragma omp for schedule(static)
            for (int i = 0; i < n; i++) { bool s2, s3; dense_row(TE, ctx[2 * i], ctx[2 * i + 1], y[i], loo, qa, p, c2, c3, s2, s3); const float* z = &Z[(size_t)i * V]; double mx = -1e30; for (int v = 0; v < V; v++) mx = max(mx, (double)z[v]); double se = 0; for (int v = 0; v < V; v++) se += exp((z[v] - mx) / T);
                for (int v = 0; v < V; v++) { double d = z[v] - p[v], e = p[v] - pi[v]; num += d * d; den += e * e; double ps = (1 - al) * Ptr[v] + al * exp((z[v] - mx) / T) / se; if (p[v] > 0) klt += p[v] * (log(p[v]) - log(ps)); } qc += -log(p[y[i]]); } }
        rel = num / den; kl = klt / n; qce = qc / n; };
    auto full_eval = [&](const vector<float>& W, const double* qa, bool qloo) { SE r; auto Zv = lg(Fva, nva, W); static double ce[NT][NA]; ce_table(Zv, va.y.data(), nva, Ptr, ce); int bt = 0, ba = 0; double bv = 1e30; for (int t = 0; t < NT; t++) if (TV9[t]) for (int a = 0; a < NA; a++) if (ce[t][a] < bv) { bv = ce[t][a]; bt = t; ba = a; }
        r.al = AG[ba]; r.T = TG[bt]; auto Zt = lg(Fte, nte, W); auto Zr = lg(Ftr, ntr, W); r.va = eval_cal(Zv, va.y.data(), nva, Ptr, r.al, r.T); r.te = eval_cal(Zt, te.y.data(), nte, Ptr, r.al, r.T); r.tr = eval_cal(Zr, tr.y.data(), ntr, Ptr, r.al, r.T); r.rte = row_metrics(Zt, te.y.data(), nte, Ptr, r.al, r.T);
        if (qa) { distill(Zr, ntr, tr.ctx, tr.y, qloo, qa, r.al, r.T, r.rel[0], r.kl[0], r.qce[0]); distill(Zv, nva, va.ctx, va.y, false, qa, r.al, r.T, r.rel[1], r.kl[1], r.qce[1]); distill(Zt, nte, te.ctx, te.y, false, qa, r.al, r.T, r.rel[2], r.kl[2], r.qce[2]); } return r; };
    auto pr = [&](const char* nm, double lam, const SE& r) { printf("OGRENCI|%s|lambda=%g|alpha=%g|T=%g|egitim CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|dogr CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|dogr-egitim CE=%.4f\n", nm, lam, r.al, r.T, r.tr.ce, r.tr.ppl, r.tr.top1, r.tr.top5, r.va.ce, r.va.ppl, r.va.top1, r.va.top5, r.te.ce, r.te.ppl, r.te.top1, r.te.top5, r.va.ce - r.tr.ce); fflush(stdout); };
    auto prg = [&](const char* nm, const SE& r) { vector<float> cer(nte); for (int i = 0; i < nte; i++) cer[i] = r.rte.ce[i]; groups(nm, cer, r.rte.t1, r.rte.t5); };
    // --- V17 Ridge (A: gercek etiket)
    vector<float> WA = solveW(Rhot, lam0); SE seA = full_eval(WA, nullptr, false); pr("V17_Ridge_WA", lam0, seA); prg("V17_Ridge", seA);
    // --- V20: tek CE duzeltmesi (egitim-yalniz protokol; eta dogrulama CE ile)
    { vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam0; cholesky(A, m); GT g1; vector<double> G1((size_t)m * V, 0.0); double cs = 0; ce_grad(Ftr.data(), tr.y.data(), ntr, m, WA, Ptr, 0.2, 0.01, G1, cs, g1, true); vector<float> D; chol_solve_multi(A, m, G1, V, D);
      const double ET[5] = {0, 0.003, 0.005, 0.0075, 0.01}; int bi = 0; double bvv = 1e30; auto Zv0 = lg(Fva, nva, WA), Zvd = lg(Fva, nva, D);
      for (int ei = 0; ei < 5; ei++) { vector<float> Z((size_t)nva * V); for (size_t i = 0; i < Z.size(); i++) Z[i] = Zv0[i] - (float)ET[ei] * Zvd[i]; static double ce[NT][NA]; ce_table(Z, va.y.data(), nva, Ptr, ce); double v = 1e30; for (int t = 0; t < NT; t++) if (TV9[t]) for (int a = 0; a < NA; a++) v = min(v, ce[t][a]); printf("V20_ETA|%g|val CE=%.4f\n", ET[ei], v); if (v < bvv) { bvv = v; bi = ei; } }
      vector<float> W20 = WA; for (size_t i = 0; i < W20.size(); i++) W20[i] -= (float)ET[bi] * D[i]; SE se20 = full_eval(W20, nullptr, false); printf("V20_secilen_eta=%g\n", ET[bi]); pr("V20_CE_duzeltme", lam0, se20); prg("V20", se20); }
    // --- V23 hedef-kosullu (CE=ln100; b=8 LOO ogretici; lambda=V17)
    auto buildR = [&](function<void(int, vector<double>&, vector<double>&, vector<double>&, vector<double>&)> rowfill, vector<double>& R, double& qq) { R.assign((size_t)m * V, 0.0); qq = 0; const int CHn = 4096; vector<float> Qc, Ft, Gc; vector<double> qs(omp_get_max_threads(), 0.0);
        for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Qc.assign((size_t)nn * V, 0.f);
#pragma omp parallel
            { vector<double> p(V), t1(V), t2(V), q(V);
#pragma omp for schedule(static)
                for (int i = 0; i < nn; i++) { rowfill(s0 + i, p, t1, t2, q); float* o = &Qc[(size_t)i * V]; double a = 0; for (int v = 0; v < V; v++) { o[v] = (float)q[v]; a += q[v] * q[v]; } qs[omp_get_thread_num()] += a; } }
            Ft.assign((size_t)m * nn, 0.f); const float* Fb = Ftr.data() + (size_t)s0 * m; for (int i = 0; i < nn; i++) for (int j = 0; j < m; j++) Ft[(size_t)j * nn + i] = Fb[(size_t)i * m + j]; Gc.assign((size_t)m * V, 0.f); gemm_f(Ft.data(), m, nn, Qc.data(), V, Gc.data(), V); for (size_t i = 0; i < R.size(); i++) R[i] += Gc[i]; }
        for (double x : qs) qq += x; };
    const double B8 = 8; vector<double> py8(ntr), piy(ntr);
#pragma omp parallel
    { vector<double> p(V), t1(V), t2(V);
#pragma omp for schedule(static)
        for (int i = 0; i < ntr; i++) { teach_row(TE, i, B8, p, t1, t2); py8[i] = p[tr.y[i]]; piy[i] = pi[tr.y[i]]; } }
    double Cloo8 = 0; for (int i = 0; i < ntr; i++) Cloo8 += -log(py8[i]); Cloo8 /= ntr;
    auto ceQ = [&](double k) { double s = 0; for (int i = 0; i < ntr; i++) s += k >= 0 ? -log((1 - k) * py8[i] + k) : -log((1 + k) * py8[i] - k * piy[i]); return s / ntr; };
    auto kappa_ce = [&](double cet) { if (cet < Cloo8) { double lo = 0, hi = 1; for (int it = 0; it < 60; it++) { double mid = (lo + hi) / 2; if (ceQ(mid) > cet) lo = mid; else hi = mid; } return (lo + hi) / 2; } double lo = -1, hi = 0; for (int it = 0; it < 60; it++) { double mid = (lo + hi) / 2; if (ceQ(mid) > cet) lo = mid; else hi = mid; } return (lo + hi) / 2; };
    auto rowQ = [&](double kap) { return [&, kap](int gi, vector<double>& p, vector<double>& t1, vector<double>& t2, vector<double>& q) { teach_row(TE, gi, B8, p, t1, t2); int yy = tr.y[gi]; for (int v = 0; v < V; v++) q[v] = kap >= 0 ? (1 - kap) * p[v] + (v == yy ? kap : 0.0) : (1 + kap) * p[v] - kap * pi[v]; }; };
    // ===== D: hedef 200/150/100
    const double TPP[3] = {200, 150, 100}; vector<vector<float>> WD(3); double kD[3]; vector<SE> seD;
    printf("D_TEMEL ogretici (b=%g, LOO) egitim CE=%.4f (PPL %.1f)\n", B8, Cloo8, exp(Cloo8));
    for (int k = 0; k < 3; k++) { kD[k] = kappa_ce(log(TPP[k])); vector<double> R; double qq; buildR(rowQ(kD[k]), R, qq); WD[k] = solveW(R, lam0); SE s = full_eval(WD[k], nullptr, false); seD.push_back(s); char nm[64]; snprintf(nm, 64, "V23_hedef_PPL%g_kappa=%.6f", TPP[k], kD[k]); pr(nm, lam0, s); if (k == 2) prg("V23", s); }
    // Q* farklari ve W farklari, aktarim gucu ||F dW||/||dQ||
    vector<float> Sf((size_t)m * m); for (size_t i = 0; i < Sf.size(); i++) { size_t r = i / m, cc = i % m; Sf[i] = (float)S[max(r, cc) * m + min(r, cc)]; }
    auto qdiff = [&](double ka, double kb, double& dq, double& qn) { double d = 0, n2 = 0;
#pragma omp parallel reduction(+ : d, n2)
        { vector<double> p(V), t1(V), t2(V);
#pragma omp for schedule(static)
            for (int i = 0; i < ntr; i++) { teach_row(TE, i, B8, p, t1, t2); int yy = tr.y[i]; for (int v = 0; v < V; v++) { double qa_ = ka >= 0 ? (1 - ka) * p[v] + (v == yy ? ka : 0.0) : (1 + ka) * p[v] - ka * pi[v]; double qb_ = kb >= 0 ? (1 - kb) * p[v] + (v == yy ? kb : 0.0) : (1 + kb) * p[v] - kb * pi[v]; d += (qa_ - qb_) * (qa_ - qb_); n2 += qb_ * qb_; } } } dq = sqrt(d); qn = sqrt(n2); };
    for (int a = 0; a < 3; a++) for (int b = a + 1; b < 3; b++) { double dq, qn; qdiff(kD[a], kD[b], dq, qn); double dw = 0, wn = 0; vector<float> dW(WD[a].size()); for (size_t i = 0; i < dW.size(); i++) { dW[i] = WD[a][i] - WD[b][i]; dw += (double)dW[i] * dW[i]; wn += (double)WD[b][i] * WD[b][i]; }
        vector<float> SdW((size_t)m * V); gemm_f(Sf.data(), m, m, dW.data(), V, SdW.data(), V); double q = 0; for (size_t i = 0; i < dW.size(); i++) q += (double)dW[i] * SdW[i]; printf("D_FARK|hedef %g vs %g|kappa %.6f vs %.6f|||dQ||/||Q||=%.5f|||dW||/||W||=%.5f|||F dW||/||dQ||(aktarim)=%.4f|||F dW||/||F W||=%.5f\n", TPP[a], TPP[b], kD[a], kD[b], dq / qn, sqrt(dw / wn), sqrt(q) / dq, sqrt(q / ([&] { vector<float> SW((size_t)m * V); gemm_f(Sf.data(), m, m, WD[b].data(), V, SW.data(), V); double z = 0; for (size_t i = 0; i < WD[b].size(); i++) z += (double)WD[b][i] * SW[i]; return z; })())); }
    // ===== B: Q = trigram (EM agirlikli, egitim sayimlari); A (etiket) ile ayni F ve lambda
    auto rowTri = [&](bool loo) { return [&, loo](int gi, vector<double>& p, vector<double>& t1, vector<double>& t2, vector<double>& q) { bool s2, s3; dense_row(TE, tr.ctx[2 * gi], tr.ctx[2 * gi + 1], tr.y[gi], loo, a3, q, t1, t2, s2, s3); }; };
    vector<double> RB[2]; double qqB[2]; for (int lo = 0; lo < 2; lo++) buildR(rowTri(lo == 1), RB[lo], qqB[lo]);
    const double LB[4] = {0.3, lam0, 30, 300}; 
    for (int li = 0; li < 4; li++) { double lam = LB[li]; vector<float> WA_l = solveW(Rhot, lam); SE sa = full_eval(WA_l, a3, false); char nm[64]; snprintf(nm, 64, "A_etiket_lam%g", lam); pr(nm, lam, sa); printf("DISTIL|%s|Q=trigram egitim(kendini icerir)|rel_hata egitim/dogr/test=%.4f/%.4f/%.4f|KL(Q||ogrenci) %.4f/%.4f/%.4f|Q'nun kendi CE %.4f/%.4f/%.4f\n", nm, sa.rel[0], sa.rel[1], sa.rel[2], sa.kl[0], sa.kl[1], sa.kl[2], sa.qce[0], sa.qce[1], sa.qce[2]);
        for (int lo = 0; lo < 2; lo++) { vector<float> WB = solveW(RB[lo], lam); SE sb = full_eval(WB, a3, lo == 1); snprintf(nm, 64, "B_Q%s_lam%g", lo ? "LOO" : "tam", lam); pr(nm, lam, sb); printf("DISTIL|%s|Q=trigram %s|rel_hata egitim/dogr/test=%.4f/%.4f/%.4f|KL(Q||ogrenci) %.4f/%.4f/%.4f|Q'nun kendi CE %.4f/%.4f/%.4f\n", nm, lo ? "LOO" : "tam", sb.rel[0], sb.rel[1], sb.rel[2], sb.kl[0], sb.kl[1], sb.kl[2], sb.qce[0], sb.qce[1], sb.qce[2]);
            double dw = 0, na = 0, nb = 0; for (size_t i = 0; i < WA_l.size(); i++) { double x = (double)WA_l[i] - WB[i]; dw += x * x; na += (double)WA_l[i] * WA_l[i]; nb += (double)WB[i] * WB[i]; } printf("W_NORM|lam=%g|Q=%s|||WA||=%.4f|||WB||=%.4f|||WA-WB||/||WA||=%.4f\n", lam, lo ? "LOO" : "tam", sqrt(na), sqrt(nb), sqrt(dw / na));
            if (li == 1 && lo == 0) prg("B_Qtam", sb); } }
    printf("SURE gram=%.2fs TOPLAM=%.1fs tepe_RAM=%.0fMB\n", t_gram, now() - Ts, peak_mb());
    return 0;
}
