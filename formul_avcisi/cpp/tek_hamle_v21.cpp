// Tek hamle V21: SECICI CE duzeltmesi W=W0-eta (D (.) M); M = egitim kelime sikligina gore (sik/orta/nadir) cikis-sutunu maskesi. V20 kopyasi; onceki dosyalara dokunulmadi.
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
int main(int argc, char** argv) {
    if (argc < 6) { cerr << "kullanim: tek_hamle_v21 c H TOHUM gamma lambda [alpha=0.2] [T=0.01] [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), lam = atof(argv[5]); double al0 = argc > 6 ? atof(argv[6]) : 0.2, T0c = argc > 7 ? atof(argv[7]) : 0.01; string dir = argc > 8 ? argv[8] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); const int m = H + 1;
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); KT = K; BASEA = BASEB = BASEP = K; }
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V), Ptv = readbin<double>(dir + "/uni_trva.f64", V);
    printf("V21 c=%d H=%d tohum=%d gamma=%g lambda=%g alpha0=%g T0=%g (%d cekirdek)\n", c, H, seed, gam, lam, al0, T0c, omp_get_max_threads());
    double Ts = now(); Feat f = make_w1("cift", tr, H, seed); vector<float> Ftr, Fva, Fte; features(tr, f, gam, Ftr); features(va, f, gam, Fva); features(te, f, gam, Fte);
    double t0 = now(); vector<double> S((size_t)m * m, 0.0), R((size_t)m * V, 0.0); syrk_flush256(Ftr.data(), ntr, m, S.data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, R.data());
    vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam; cholesky(A, m); vector<float> W0; chol_solve_multi(A, m, R, V, W0); double t_ridge = now() - t0;
    // tek CE gradyani (egitim) ve tek D = A^-1 F^T G ; maskeler D'ye sutun olcegi olarak uygulanir (yeniden Cholesky/gradyan YOK)
    GT g1; vector<double> G1((size_t)m * V, 0.0); double cs1 = 0; t0 = now(); ce_grad(Ftr.data(), tr.y.data(), ntr, m, W0, Ptr, al0, T0c, G1, cs1, g1, true); vector<float> dW; chol_solve_multi(A, m, G1, V, dW); double t_corr = now() - t0;
    printf("RIDGE sure=%.3fs ; CE duzeltme (gradyan+D, egitim) sure=%.3fs\n", t_ridge, t_corr);
    // kelime siklik gruplari (V20 ile ayni tanim, yalniz egitim hedef sayimlari)
    vector<int> ord(V); iota(ord.begin(), ord.end(), 0); vector<double> cnt(V, 0); for (int i = 0; i < ntr; i++) cnt[tr.y[i]] += 1; sort(ord.begin(), ord.end(), [&](int a, int b) { return cnt[a] > cnt[b]; });
    vector<int> grp(V, 2); { double cum = 0; int ng[3] = {0, 0, 0}; for (int v : ord) { double fr = cum / ntr; grp[v] = fr < 0.5 ? 0 : (fr < 0.8 ? 1 : 2); cum += cnt[v]; ng[grp[v]]++; } printf("KELIME_GRUPLARI (egitim kitlesi): sik=%d tur, orta=%d tur, nadir=%d tur\n", ng[0], ng[1], ng[2]); }
    const int NM = 5; const char* MN[NM] = {"A_V20", "B_sik0", "C_sik.25", "D_sik.5", "E_.25/.75/1"}; const double MK[NM][3] = {{1, 1, 1}, {0, 1, 1}, {0.25, 1, 1}, {0.5, 1, 1}, {0.25, 0.75, 1}};
    #ifdef EXT
    const int NE = 8; const double ETA[NE] = {0, 0.003, 0.005, 0.0075, 0.01, 0.015, 0.02, 0.03};  // EK (gorev disi) genisletilmis izgara: -DEXT
#else
    const int NE = 5; const double ETA[NE] = {0, 0.003, 0.005, 0.0075, 0.01};
#endif
    auto mix = [&](const vector<float>& Z0, const vector<float>& Zd, int n, double eta, int mi) { vector<float> Z((size_t)n * V); vector<float> cm(V); for (int v = 0; v < V; v++) cm[v] = (float)(eta * MK[mi][grp[v]]);
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) { const float* a = &Z0[(size_t)i * V]; const float* b = &Zd[(size_t)i * V]; float* o = &Z[(size_t)i * V]; for (int v = 0; v < V; v++) o[v] = a[v] - cm[v] * b[v]; } return Z; };
    vector<float> Zv0((size_t)nva * V), Zvd((size_t)nva * V); gemm_f(Fva.data(), nva, m, W0.data(), V, Zv0.data(), V); gemm_f(Fva.data(), nva, m, dW.data(), V, Zvd.data(), V);
    struct E { double vce = 1e30, vtop1 = 0, vraw1 = 0, vtop5 = 0; int ti = 0, ai = 0; } e[NM][NE];
    printf("MASKE x ETA TARAMASI (dogrulama): maske | eta | kalibre CE | (alpha,T) | dogr. top1 | top5 | ham top1\n");
    for (int mi = 0; mi < NM; mi++) for (int ei = 0; ei < NE; ei++) {
        if (ei == 0 && mi > 0) { e[mi][0] = e[0][0]; continue; }
        vector<float> Zv = mix(Zv0, Zvd, nva, ETA[ei], mi); static double ce[NT][NA]; ce_table(Zv, va.y.data(), nva, Ptr, ce); E& q = e[mi][ei];
        for (int t = 0; t < NT; t++) if (TV9[t]) for (int a = 0; a < NA; a++) if (ce[t][a] < q.vce) { q.vce = ce[t][a]; q.ti = t; q.ai = a; }
        Row rm = row_metrics(Zv, va.y.data(), nva, Ptr, AG[q.ai], TG[q.ti]); double s1 = 0, s5 = 0, sr = 0; for (int i = 0; i < nva; i++) { s1 += rm.t1[i]; s5 += rm.t5[i]; sr += rm.r1[i]; } q.vtop1 = s1 / nva; q.vtop5 = s5 / nva; q.vraw1 = sr / nva;
    }
    for (int mi = 0; mi < NM; mi++) for (int ei = 0; ei < NE; ei++) { E& q = e[mi][ei]; printf("  %s | eta=%g | %.4f | a=%g T=%g | %.4f | %.4f | %.4f\n", MN[mi], ETA[ei], q.vce, AG[q.ai], TG[q.ti], q.vtop1, q.vtop5, q.vraw1); }
    fflush(stdout);
    double rt1 = e[0][0].vtop1;
    // secim: her maske icin (A: min CE; B: min CE ve val top1 >= ridge-0.002); GLOBAL = tum (maske,eta) ustunde
    int selA[NM], selB[NM]; for (int mi = 0; mi < NM; mi++) { selA[mi] = selB[mi] = 0; for (int ei = 0; ei < NE; ei++) { if (e[mi][ei].vce < e[mi][selA[mi]].vce) selA[mi] = ei; if (e[mi][ei].vtop1 >= rt1 - 0.002 && e[mi][ei].vce < e[mi][selB[mi]].vce) selB[mi] = ei; } }
    int gA[2] = {0, 0}, gB[2] = {0, 0}; for (int mi = 0; mi < NM; mi++) { if (e[mi][selA[mi]].vce < e[gA[0]][gA[1]].vce) { gA[0] = mi; gA[1] = selA[mi]; } if (e[mi][selB[mi]].vce < e[gB[0]][gB[1]].vce) { gB[0] = mi; gB[1] = selB[mi]; } }
    printf("SECIM GLOBAL_KURAL_A: maske=%s eta=%g (val CE %.4f top1 %.4f) | GLOBAL_KURAL_B: maske=%s eta=%g (val CE %.4f top1 %.4f) | ridge val CE %.4f top1 %.4f\n", MN[gA[0]], ETA[gA[1]], e[gA[0]][gA[1]].vce, e[gA[0]][gA[1]].vtop1, MN[gB[0]], ETA[gB[1]], e[gB[0]][gB[1]].vce, e[gB[0]][gB[1]].vtop1, e[0][0].vce, rt1);
    // --- son modeller (egitim+dogrulama); TEST yalniz burada, secimden sonra
    vector<double> S2 = S, R2 = R; syrk_flush256(Fva.data(), nva, m, S2.data()); rhs_add(Fva.data(), va.y.data(), nva, m, V, R2.data()); vector<double> A2 = S2; for (int i = 0; i < m; i++) A2[(size_t)i * m + i] += lam; cholesky(A2, m);
    vector<float> W0f; chol_solve_multi(A2, m, R2, V, W0f); GT g2; vector<double> G2((size_t)m * V, 0.0); double cs2 = 0; t0 = now();
    ce_grad(Ftr.data(), tr.y.data(), ntr, m, W0f, Ptv, al0, T0c, G2, cs2, g2, true); ce_grad(Fva.data(), va.y.data(), nva, m, W0f, Ptv, al0, T0c, G2, cs2, g2, true); vector<float> d2; chol_solve_multi(A2, m, G2, V, d2); double t_corr_final = now() - t0;
    vector<float> Zt0((size_t)nte * V), Ztd((size_t)nte * V); gemm_f(Fte.data(), nte, m, W0f.data(), V, Zt0.data(), V); gemm_f(Fte.data(), nte, m, d2.data(), V, Ztd.data(), V);
    // egitim-dogrulama farki icin egitim-yalniz modelin egitim logitleri
    vector<float> Ztr0((size_t)ntr * V), Ztrd((size_t)ntr * V); gemm_f(Ftr.data(), ntr, m, W0.data(), V, Ztr0.data(), V); gemm_f(Ftr.data(), ntr, m, dW.data(), V, Ztrd.data(), V);
    Row Rr = row_metrics(Zt0, te.y.data(), nte, Ptv, AG[e[0][0].ai], TG[e[0][0].ti]);
    double ridge_trce; { Res rr = eval_cal(Ztr0, tr.y.data(), ntr, Ptr, AG[e[0][0].ai], TG[e[0][0].ti]); ridge_trce = rr.ce; }
    { Res r = eval_cal(Zt0, te.y.data(), nte, Ptv, AG[e[0][0].ai], TG[e[0][0].ti]); printf("OZET|RIDGE|-|eta=0|alpha=%g|T=%g|val_CE=%.4f|val_top1=%.4f|test_CE=%.4f|PPL=%.2f|top1=%.4f|top5=%.4f|train_CE=%.4f|gap=%.4f\n", AG[e[0][0].ai], TG[e[0][0].ti], e[0][0].vce, rt1, r.ce, r.ppl, r.top1, r.top5, ridge_trce, e[0][0].vce - ridge_trce); }
    auto report = [&](int mi, int ei, const char* nm) {
        E& q = e[mi][ei]; vector<float> Zt = mix(Zt0, Ztd, nte, ETA[ei], mi); Row Rn = row_metrics(Zt, te.y.data(), nte, Ptv, AG[q.ai], TG[q.ti]); Res r = eval_cal(Zt, te.y.data(), nte, Ptv, AG[q.ai], TG[q.ti]);
        vector<float> Zr = mix(Ztr0, Ztrd, ntr, ETA[ei], mi); Res rtr = eval_cal(Zr, tr.y.data(), ntr, Ptr, AG[q.ai], TG[q.ti]);
        int c2w = 0, w2c = 0, rc2w = 0, rw2c = 0; double sc[3] = {0, 0, 0}, so[3] = {0, 0, 0}, t1n[3] = {0, 0, 0}, t1o[3] = {0, 0, 0}, t5n[3] = {0, 0, 0}, t5o[3] = {0, 0, 0}; int cn[3] = {0, 0, 0}; double rawn = 0;
        for (int i = 0; i < nte; i++) { int g = grp[te.y[i]]; cn[g]++; if (Rr.t1[i] && !Rn.t1[i]) c2w++; if (!Rr.t1[i] && Rn.t1[i]) w2c++; if (Rr.r1[i] && !Rn.r1[i]) rc2w++; if (!Rr.r1[i] && Rn.r1[i]) rw2c++; sc[g] += Rn.ce[i]; so[g] += Rr.ce[i]; t1n[g] += Rn.t1[i]; t1o[g] += Rr.t1[i]; t5n[g] += Rn.t5[i]; t5o[g] += Rr.t5[i]; rawn += Rn.r1[i]; }
        printf("OZET|%s|%s|eta=%g|alpha=%g|T=%g|val_CE=%.4f|val_top1=%.4f|test_CE=%.4f|PPL=%.2f|top1=%.4f|top5=%.4f|train_CE=%.4f|gap=%.4f\n", nm, MN[mi], ETA[ei], AG[q.ai], TG[q.ti], q.vce, q.vtop1, r.ce, r.ppl, r.top1, r.top5, rtr.ce, q.vce - rtr.ce);
        printf("GECIS|%s|%s|kalibre top1: dogru->yanlis=%d yanlis->dogru=%d net=%+d | ham top1: dogru->yanlis=%d yanlis->dogru=%d net=%+d\n", nm, MN[mi], c2w, w2c, w2c - c2w, rc2w, rw2c, rw2c - rc2w);
        const char* gn_[3] = {"sik", "orta", "nadir"}; for (int g = 0; g < 3; g++) printf("GRUP|%s|%s|%s|n=%d|CE ridge=%.4f yeni=%.4f (fark %+.4f)|top1 ridge=%.4f yeni=%.4f|top5 ridge=%.4f yeni=%.4f\n", nm, MN[mi], gn_[g], cn[g], so[g] / cn[g], sc[g] / cn[g], (sc[g] - so[g]) / cn[g], t1o[g] / cn[g], t1n[g] / cn[g], t5o[g] / cn[g], t5n[g] / cn[g]); fflush(stdout); };
    for (int mi = 0; mi < NM; mi++) { report(mi, selA[mi], "MASKE_KURAL_A"); report(mi, selB[mi], "MASKE_KURAL_B"); }
    printf("GLOBAL_A_isaret maske=%s eta=%g ; GLOBAL_B_isaret maske=%s eta=%g\n", MN[gA[0]], ETA[gA[1]], MN[gB[0]], ETA[gB[1]]);
    printf("SURE son-model CE duzeltme (gradyan egitim+dogrulama + D)=%.3fs ; maske uygulama (tum tarama) ihmal edilebilir (yalniz Z0-eta*m*Zd)\n", t_corr_final);
    printf("TOPLAM sure=%.1fs tepe_RAM=%.0fMB\n", now() - Ts, peak_mb());
    return 0;
}
