// Tek hamle V23: hedef-kosullu ogretim matrisi Q*(kappa(CE*,Top1*,Top5*)) + Mallows-Cp duzenlilestirmesi Lambda(lambda*(Q*)) -> TEK agirlikli ridge cozumu W=(F^TF+Lambda)^-1 F^T Q*. V22 kopyasi (kernel); onceki dosyalara dokunulmadi.
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
static void build_teach(Teach& T, const Data& d) { T.n = d.n; T.c1.assign(V, 0); T.ctx = d.ctx; T.y = d.y; for (int i = 0; i < d.n; i++) { int u = d.ctx[2 * i], w = d.ctx[2 * i + 1], yy = d.y[i]; T.c1[yy]++; T.l2[w].push_back(yy); T.l3[(long long)u * V + w].push_back(yy); } }
// i. egitim satirinin "kendisi haric" (LOO) interpolasyonlu bigram/trigram dagilimi (b: geri cekilme guclugu)
static void teach_row(const Teach& T, int i, double b, vector<double>& p, vector<double>& t1, vector<double>& t2) {
    int u = T.ctx[2 * i], w = T.ctx[2 * i + 1], yy = T.y[i]; double n1 = T.n - 1 + V;
    for (int v = 0; v < V; v++) p[v] = (T.c1[v] + 1.0 - (v == yy ? 1 : 0)) / n1;
    { fill(t1.begin(), t1.end(), 0.0); const auto& L = T.l2.at(w); for (int v : L) t1[v] += 1; t1[yy] -= 1; double tot = (double)L.size() - 1; for (int v = 0; v < V; v++) p[v] = (t1[v] + b * p[v]) / (tot + b); }
    { fill(t2.begin(), t2.end(), 0.0); const auto& L = T.l3.at((long long)u * V + w); for (int v : L) t2[v] += 1; t2[yy] -= 1; double tot = (double)L.size() - 1; for (int v = 0; v < V; v++) p[v] = (t2[v] + b * p[v]) / (tot + b); }
}
int main(int argc, char** argv) {
    if (argc < 6) { cerr << "kullanim: tek_hamle_v23 c H TOHUM gamma lambda [top1=0.20] [top5=0.40] [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), lam0 = atof(argv[5]); double TOP1 = argc > 6 ? atof(argv[6]) : 0.20, TOP5 = argc > 7 ? atof(argv[7]) : 0.40; string dir = argc > 8 ? argv[8] : "/tmp/v8data"; double BFIX = argc > 9 ? atof(argv[9]) : 0;
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); const int m = H + 1;
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); KT = K; BASEA = BASEB = BASEP = K; }
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V23 c=%d H=%d tohum=%d gamma=%g lambda_V17=%g Top1*=%g Top5*=%g (%d cekirdek); modeller egitim-yalniz; dogrulama secim/kontrol, test bagimsiz\n", c, H, seed, gam, lam0, TOP1, TOP5, omp_get_max_threads());
    double Ts = now(); Feat f = make_w1("cift", tr, H, seed); vector<float> Ftr, Fva, Fte; features(tr, f, gam, Ftr); features(va, f, gam, Fva); features(te, f, gam, Fte);
    double t0 = now(); vector<double> S((size_t)m * m, 0.0), Rhot((size_t)m * V, 0.0); syrk_flush256(Ftr.data(), ntr, m, S.data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, Rhot.data()); double t_gram = now() - t0;
    // ---- 1) OGRETICI (yalniz egitim sayimlari, LOO) ve b secimi (egitim LOO CE)
    t0 = now(); Teach TE; build_teach(TE, tr); double bs[8] = {1, 2, 4, 8, 16, 32, 64, 128}, bestb = 1, bestce = 1e30;
    for (double b : bs) { double sce = 0;
#pragma omp parallel reduction(+ : sce)
        { vector<double> p(V), t1(V), t2(V);
#pragma omp for schedule(static)
            for (int i = 0; i < ntr; i++) { teach_row(TE, i, b, p, t1, t2); sce += -log(p[tr.y[i]]); } }
        printf("OGRETICI b=%g egitim-LOO CE=%.4f (PPL %.1f)\n", b, sce / ntr, exp(sce / ntr)); if (sce / ntr < bestce) { bestce = sce / ntr; bestb = b; } }
    // satir istatistikleri: py (LOO), pi_y (unigram), g1, g5 (y'nin ilk-1/ilk-5 olmasi icin gereken marj)
    if (BFIX > 0) { bestb = BFIX; printf("OGRETICI b sabitlendi (duyarlilik kosusu): b=%g\n", bestb); }
    vector<double> pi(V); { double tot = 0; for (int v = 0; v < V; v++) tot += TE.c1[v] + 1.0; for (int v = 0; v < V; v++) pi[v] = (TE.c1[v] + 1.0) / tot; }
    vector<double> py(ntr), piy(ntr), g1(ntr), g5(ntr);
#pragma omp parallel
    { vector<double> p(V), t1(V), t2(V);
#pragma omp for schedule(static)
        for (int i = 0; i < ntr; i++) { teach_row(TE, i, bestb, p, t1, t2); int yy = tr.y[i]; py[i] = p[yy]; piy[i] = pi[yy]; double top[5] = {-1e30, -1e30, -1e30, -1e30, -1e30};
            for (int v = 0; v < V; v++) if (v != yy) { double d = p[v] - p[yy]; if (d > top[4]) { top[4] = d; for (int k = 4; k > 0 && top[k] > top[k - 1]; k--) swap(top[k], top[k - 1]); } }
            g1[i] = top[0]; g5[i] = top[4]; } }
    double Cloo = 0, Cuni = 0; for (int i = 0; i < ntr; i++) { Cloo += -log(py[i]); Cuni += -log(piy[i]); } Cloo /= ntr; Cuni /= ntr;
    auto frac_lt = [&](const vector<double>& g, double r) { long k = 0; for (double x : g) k += (x < r); return (double)k / g.size(); };
    printf("OGRETICI SECILEN b=%g ; kappa=0 ogretici (saf sayim, LOO): egitim CE=%.4f (PPL %.1f) Top-1=%.4f Top-5=%.4f ; unigram CE=%.4f (PPL %.1f) ; ogretici hazirlama sure=%.2fs\n", bestb, Cloo, exp(Cloo), frac_lt(g1, 0), frac_lt(g5, 0), Cuni, exp(Cuni), now() - t0);
    auto ceQ = [&](double k) { double s = 0; for (int i = 0; i < ntr; i++) s += k >= 0 ? -log((1 - k) * py[i] + k) : -log((1 + k) * py[i] - k * piy[i]); return s / ntr; };
    // kappa(CE*): isaretli: k>0 etiket karisimi (sharpen), k<0 unigram karisimi (flatten); egitim CE(Q*) = CE* olacak sekilde
    auto kappa_ce = [&](double cet, bool& clipped) { clipped = false; if (fabs(cet - Cloo) < 1e-12) return 0.0; if (cet < Cloo) { double lo = 0, hi = 1; for (int it = 0; it < 60; it++) { double mid = (lo + hi) / 2; if (ceQ(mid) > cet) lo = mid; else hi = mid; } return (lo + hi) / 2; }
        if (ceQ(-1) < cet) { clipped = true; return -1.0; } double lo = -1, hi = 0; for (int it = 0; it < 60; it++) { double mid = (lo + hi) / 2; if (ceQ(mid) > cet) lo = mid; else hi = mid; } return (lo + hi) / 2; };
    auto kappa_top = [&](const vector<double>& g, double frac) { vector<double> s = g; sort(s.begin(), s.end()); size_t k = (size_t)ceil(frac * s.size()); if (k >= s.size()) k = s.size() - 1; double r = s[k]; if (frac_lt(g, 0) >= frac) return 0.0; return r <= 0 ? 0.0 : r / (1 + r); };
    // ---- 2) tek-hamle cozucu: R = F^T Q*(kappa) ; Cp ile lambda ; W
    const double LG[] = {1, 3, 10, 30, 100, 300, 1000}; const int NL = 7; double tcp_total = 0;
    struct Cfg { string name; double kappa, lam, tCE, t1, t5, qq, cp_min; vector<float> W; Res rt, rv, rtest; double al, T; double secs; };
    vector<Cfg> cfgs;
    auto solve_cfg = [&](const string& name, bool hot, double kap, double lamfix /* >0: sabit lambda; 0: Cp */) {
        Cfg cf; cf.name = name; cf.kappa = kap; double t0c = now(); vector<double> R((size_t)m * V, 0.0), qqv; double qq = 0; double tE = 0;
        if (hot) { R = Rhot; qq = ntr; cf.tCE = NAN; cf.t1 = NAN; cf.t5 = NAN; }
        else { double r = kap >= 0 ? kap / (1 - kap + 1e-300) : 0; cf.tCE = ceQ(kap); cf.t1 = kap >= 0 ? frac_lt(g1, r) : NAN; cf.t5 = kap >= 0 ? frac_lt(g5, r) : NAN;
            const int CHn = 4096; vector<float> Qc, Ft, Gc; vector<double> qs(omp_get_max_threads(), 0.0);
            for (int s0 = 0; s0 < ntr; s0 += CHn) { int nn = min(CHn, ntr - s0); Qc.assign((size_t)nn * V, 0.f);
#pragma omp parallel
                { vector<double> p(V), t1(V), t2(V);
#pragma omp for schedule(static)
                    for (int i = 0; i < nn; i++) { int gi = s0 + i, yy = tr.y[gi]; teach_row(TE, gi, bestb, p, t1, t2); float* q = &Qc[(size_t)i * V]; double a = 0;
                        for (int v = 0; v < V; v++) { double x = kap >= 0 ? (1 - kap) * p[v] + (v == yy ? kap : 0.0) : (1 + kap) * p[v] - kap * pi[v]; q[v] = (float)x; a += x * x; } qs[omp_get_thread_num()] += a; } }
                Ft.assign((size_t)m * nn, 0.f); const float* Fb = Ftr.data() + (size_t)s0 * m; for (int i = 0; i < nn; i++) for (int j = 0; j < m; j++) Ft[(size_t)j * nn + i] = Fb[(size_t)i * m + j];
                Gc.assign((size_t)m * V, 0.f); gemm_f(Ft.data(), m, nn, Qc.data(), V, Gc.data(), V); for (size_t i = 0; i < R.size(); i++) R[i] += Gc[i]; }
            for (double x : qs) qq += x; }
        cf.qq = qq; vector<double> I((size_t)m * m, 0.0); for (int i = 0; i < m; i++) I[(size_t)i * m + i] = 1.0;
        double lamsel = lamfix, cpmin = NAN;
        if (lamfix <= 0) { double sig2 = 0, bestcp = 1e300; vector<double> cps(NL), sses(NL), dfs(NL);
            for (int li = 0; li < NL; li++) { vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += LG[li]; cholesky(A, m); vector<float> W; chol_solve_multi(A, m, R, V, W); vector<float> Ai; chol_solve_multi(A, m, I, m, Ai);
                double wr = 0, ww = 0; for (size_t i = 0; i < W.size(); i++) { wr += (double)W[i] * R[i]; ww += (double)W[i] * W[i]; } double tr_ = 0; for (int i = 0; i < m; i++) tr_ += Ai[(size_t)i * m + i];
                sses[li] = qq - wr - LG[li] * ww; dfs[li] = m - LG[li] * tr_; if (li == 0) sig2 = sses[0] / ((double)ntr * V - dfs[0] * V); }
            for (int li = 0; li < NL; li++) { cps[li] = sses[li] + 2 * sig2 * dfs[li] * V; if (cps[li] < bestcp) { bestcp = cps[li]; lamsel = LG[li]; } }
            cpmin = bestcp; printf("CP|%s|", name.c_str()); for (int li = 0; li < NL; li++) printf("lam=%g:SSE=%.1f,df=%.1f,Cp=%.1f ", LG[li], sses[li], dfs[li], cps[li]); printf("-> lambda*=%g\n", lamsel); }
        cf.lam = lamsel; cf.cp_min = cpmin; { vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lamsel; cholesky(A, m); chol_solve_multi(A, m, R, V, cf.W); }
        // degerlendirme: dogrulama (alpha,T secimi) -> egitim/dogrulama/test (ayni alpha,T)
        auto lg = [&](const vector<float>& F, int n) { vector<float> Z((size_t)n * V); gemm_f(F.data(), n, m, cf.W.data(), V, Z.data(), V); return Z; };
        auto Zv = lg(Fva, nva); static double ce[NT][NA]; ce_table(Zv, va.y.data(), nva, Ptr, ce); int bt = 0, ba = 0; double bv = 1e30; for (int t = 0; t < NT; t++) if (TV9[t]) for (int a = 0; a < NA; a++) if (ce[t][a] < bv) { bv = ce[t][a]; bt = t; ba = a; }
        cf.al = AG[ba]; cf.T = TG[bt]; cf.rv = eval_cal(Zv, va.y.data(), nva, Ptr, cf.al, cf.T); { auto Zt = lg(Fte, nte); cf.rtest = eval_cal(Zt, te.y.data(), nte, Ptr, cf.al, cf.T); } { auto Zr = lg(Ftr, ntr); cf.rt = eval_cal(Zr, tr.y.data(), ntr, Ptr, cf.al, cf.T); }
        cf.secs = now() - t0c; printf("CFG|%s|kappa=%.5f|lambda=%g|ogretici_egitim_CE=%.4f|ogretici_Top1=%.4f|ogretici_Top5=%.4f|alpha=%g|T=%g|egitim CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|dogr CE=%.4f PPL=%.1f top1=%.4f top5=%.4f|test CE=%.4f PPL=%.2f top1=%.4f top5=%.4f|genelleme(dogr-egitim CE)=%.4f|sure=%.2fs\n", name.c_str(), kap, lamsel, cf.tCE, cf.t1, cf.t5, cf.al, cf.T, cf.rt.ce, cf.rt.ppl, cf.rt.top1, cf.rt.top5, cf.rv.ce, cf.rv.ppl, cf.rv.top1, cf.rv.top5, cf.rtest.ce, cf.rtest.ppl, cf.rtest.top1, cf.rtest.top5, cf.rv.ce - cf.rt.ce, cf.secs); fflush(stdout); cfgs.push_back(std::move(cf)); };
    solve_cfg("ridge_hot_lamV17", true, 0, lam0);
    solve_cfg("ridge_hot_Cp", true, 0, 0);
    const double TPP[3] = {200, 150, 100}; for (int k = 0; k < 3; k++) { bool cl; double ce_t = log(TPP[k]); double kp = kappa_ce(ce_t, cl); printf("HEDEF_KAPPA|PPL=%g|CE*=%.5f|kappa_CE=%.5f%s\n", TPP[k], ce_t, kp, cl ? " (unigram karisimi sinirinda: hedef bu yonde ulasilamaz)" : ""); solve_cfg("A_PPL" + to_string((int)TPP[k]), false, kp, 0); }
    { bool cl; double kce = kappa_ce(log(100.0), cl), k1 = kappa_top(g1, TOP1), k5 = kappa_top(g5, TOP5); double kb = max(kce, max(k1, k5)); printf("HEDEF_KAPPA|CE*=ln100: kappa_CE=%.5f kappa_Top1(>=%g)=%.5f kappa_Top5(>=%g)=%.5f -> kappa=max=%.5f\n", kce, TOP1, k1, TOP5, k5, kb); solve_cfg("B_PPL100_Top1_Top5", false, kb, 0); }
    solve_cfg("K0_saf_sayim_ogretici", false, 0, 0);
    { bool cl; double k100 = kappa_ce(log(100.0), cl), kb = max(k100, max(kappa_top(g1, TOP1), kappa_top(g5, TOP5)));
      solve_cfg("A_PPL100_lamV17", false, k100, lam0); solve_cfg("B_PPL100_Top1_Top5_lamV17", false, kb, lam0); solve_cfg("K0_lamV17", false, 0, lam0); }
    // ---- agirlik degisimi
    printf("AGIRLIK_FARKI (||Wa-Wb||_F/||Wb||_F)\n"); for (size_t a = 0; a < cfgs.size(); a++) for (size_t b2 = 0; b2 < cfgs.size(); b2++) if (a < b2) { double d = 0, n = 0; for (size_t i = 0; i < cfgs[a].W.size(); i++) { double x = (double)cfgs[a].W[i] - cfgs[b2].W[i]; d += x * x; n += (double)cfgs[b2].W[i] * cfgs[b2].W[i]; } printf("WDIFF|%s|%s|%.4f\n", cfgs[a].name.c_str(), cfgs[b2].name.c_str(), sqrt(d / n)); }
    printf("SURE gram(F^TF,F^TY)=%.2fs TOPLAM=%.1fs tepe_RAM=%.0fMB\n", t_gram, now() - Ts, peak_mb());
    return 0;
}
