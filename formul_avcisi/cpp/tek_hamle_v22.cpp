// Tek hamle V22: hedef CE denklemi: CE(W0-eta D) ~ L0 + a eta + b eta^2/2 = CE_hedef ; eta_hesaplanan -> W_hedef = W0 - eta D. V21 kopyasi; onceki dosyalara dokunulmadi.
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

static double now_() { return now(); }
int main(int argc, char** argv) {
    if (argc < 6) { cerr << "kullanim: tek_hamle_v22 c H TOHUM gamma lambda [alpha=0.2] [T=0.01] [veri]\n"; return 1; }
    int c = atoi(argv[1]), H = atoi(argv[2]), seed = atoi(argv[3]); double gam = atof(argv[4]), lam = atof(argv[5]); double al0 = argc > 6 ? atof(argv[6]) : 0.2, T0c = argc > 7 ? atof(argv[7]) : 0.01; string dir = argc > 8 ? argv[8] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); const int m = H + 1;
    { expand_s(tr, c); expand_s(va, c); expand_s(te, c); K = (int)(tr.X.size() / tr.n); KT = K; BASEA = BASEB = BASEP = K; }
    vector<double> Ptr = readbin<double>(dir + "/uni_tr.f64", V);
    printf("V22 c=%d H=%d tohum=%d gamma=%g lambda=%g alpha0=%g T0=%g (%d cekirdek) ; TUM modeller egitim-yalniz (W0,D egitimden); dogrulama kontrol, test bagimsiz\n", c, H, seed, gam, lam, al0, T0c, omp_get_max_threads());
    double Ts = now(); Feat f = make_w1("cift", tr, H, seed); vector<float> Ftr, Fva, Fte; features(tr, f, gam, Ftr); features(va, f, gam, Fva); features(te, f, gam, Fte);
    double t0 = now(); vector<double> S((size_t)m * m, 0.0), R((size_t)m * V, 0.0); syrk_flush256(Ftr.data(), ntr, m, S.data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, R.data());
    vector<double> A = S; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += lam; vector<double> Acopy = A; cholesky(A, m); vector<float> W0; chol_solve_multi(A, m, R, V, W0); double t_ridge = now() - t0;
    GT g1; vector<double> G1((size_t)m * V, 0.0); double cs1 = 0; t0 = now(); ce_grad(Ftr.data(), tr.y.data(), ntr, m, W0, Ptr, al0, T0c, G1, cs1, g1, true); vector<float> dW; chol_solve_multi(A, m, G1, V, dW); double t_corr = now() - t0;
    // sayisal cozum denetimi: ||A D - F^T G|| / ||F^T G|| (cift duyarlik)
    double resn = 0, gn = 0; { vector<double> Dd(dW.begin(), dW.end()); vector<double> rs(omp_get_max_threads(), 0.0), gs(omp_get_max_threads(), 0.0);
#pragma omp parallel for schedule(static)
        for (int i = 0; i < m; i++) { vector<double> row(V, 0.0); for (int k = 0; k < m; k++) { double a = Acopy[(size_t)max(i, k) * m + min(i, k)]; if (a == 0.0) a = Acopy[(size_t)min(i, k) * m + max(i, k)]; const double* dk = &Dd[(size_t)k * V]; for (int v = 0; v < V; v++) row[v] += a * dk[v]; } double r = 0, g = 0; for (int v = 0; v < V; v++) { double e = row[v] - G1[(size_t)i * V + v]; r += e * e; g += G1[(size_t)i * V + v] * G1[(size_t)i * V + v]; } rs[omp_get_thread_num()] += r; gs[omp_get_thread_num()] += g; }
        for (double x : rs) resn += x; for (double x : gs) gn += x; resn = sqrt(resn / gn); }
    printf("RIDGE sure=%.3fs ; CE duzeltme (gradyan+D) sure=%.3fs ; cozum artigi ||A D - F^T G||/||F^T G||=%.3e (A=F^TF+lambda I)\n", t_ridge, t_corr, resn);
    // mantiksal logitler: Z(eta) = Z0 - eta Zd
    auto mk = [&](const vector<float>& F, int n) { vector<float> a((size_t)n * V), b((size_t)n * V); gemm_f(F.data(), n, m, W0.data(), V, a.data(), V); gemm_f(F.data(), n, m, dW.data(), V, b.data(), V); return make_pair(a, b); };
    auto Ptr_ = mk(Ftr, ntr), Pva_ = mk(Fva, nva), Pte_ = mk(Fte, nte);
    auto mix = [&](const pair<vector<float>, vector<float>>& p, int n, double eta) { vector<float> Z((size_t)n * V);
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++) { const float* a = &p.first[(size_t)i * V]; const float* b = &p.second[(size_t)i * V]; float* o = &Z[(size_t)i * V]; for (int v = 0; v < V; v++) o[v] = a[v] - (float)eta * b[v]; } return Z; };
    auto CEf = [&](int set, double eta, double al, double T) { if (set == 0) { auto Z = mix(Ptr_, ntr, eta); return eval_cal(Z, tr.y.data(), ntr, Ptr, al, T); } if (set == 1) { auto Z = mix(Pva_, nva, eta); return eval_cal(Z, va.y.data(), nva, Ptr, al, T); } auto Z = mix(Pte_, nte, eta); return eval_cal(Z, te.y.data(), nte, Ptr, al, T); };
    const char* SN[3] = {"egitim", "dogrulama", "test"};
    // --- yonlu turev (analitik) ve sonlu fark
    double dot = 0; for (size_t i = 0; i < G1.size(); i++) dot += (double)dW[i] * G1[i]; double a_an = -dot / ntr;
    double L0[3]; for (int s = 0; s < 3; s++) L0[s] = CEf(s, 0, al0, T0c).ce;
    printf("L0 (sabit alpha=%g T=%g): egitim=%.5f (PPL %.2f) dogrulama=%.5f (PPL %.2f) test=%.5f (PPL %.2f) ; a_analitik(egitim) = -<D,F^TG>/n = %.6g\n", al0, T0c, L0[0], exp(L0[0]), L0[1], exp(L0[1]), L0[2], exp(L0[2]), a_an);
    const double HH[3] = {0.0005, 0.001, 0.0025}; double aF[3][3], bF[3][3]; // [set][h]
    for (int s = 0; s < 2; s++) for (int hi = 0; hi < 3; hi++) { double lp = CEf(s, HH[hi], al0, T0c).ce, lm = CEf(s, -HH[hi], al0, T0c).ce; aF[s][hi] = (lp - lm) / (2 * HH[hi]); bF[s][hi] = (lp + lm - 2 * L0[s]) / (HH[hi] * HH[hi]); printf("SONLU_FARK %s h=%g a=%.6g b=%.6g\n", SN[s], HH[hi], aF[s][hi], bF[s][hi]); }
    const int HP = 1; double aT = aF[0][HP], bT = bF[0][HP], aV = aF[1][HP], bV = bF[1][HP];
    printf("KATSAYI egitim: L0=%.5f a=%.6g (analitik %.6g) b=%.6g (h=%g) ; en kucuk CE (ikinci derece) = %.5f eta* = %.5f ; dogrulama: L0=%.5f a=%.6g b=%.6g en kucuk CE=%.5f eta*=%.5f\n", L0[0], aT, a_an, bT, HH[HP], L0[0] - aT * aT / (2 * bT), -aT / bT, L0[1], aV, bV, L0[1] - aV * aV / (2 * bV), -aV / bV);
    // --- gercek egri (sabit alpha,T)
    const double EG[] = {0, 0.0005, 0.001, 0.0015, 0.002, 0.003, 0.004, 0.005, 0.006, 0.0075, 0.01, 0.0125, 0.015, 0.02, 0.025, 0.03, 0.04}; const int NG = sizeof(EG) / sizeof(double); vector<array<double, 3>> cur(NG);
    printf("EGRI eta | egitim CE | dogrulama CE | test CE | 2.derece tahmin(egitim)\n");
    for (int g = 0; g < NG; g++) { for (int s = 0; s < 3; s++) cur[g][s] = CEf(s, EG[g], al0, T0c).ce; printf("EGRI %g | %.5f | %.5f | %.5f | %.5f\n", EG[g], cur[g][0], cur[g][1], cur[g][2], L0[0] + aT * EG[g] + 0.5 * bT * EG[g] * EG[g]); fflush(stdout); }
    // --- hedefler
    const double TP[] = {190, 175, 150, 137, 100}; const int NT_ = 5;
    auto solve = [&](double L, double a, double b, double t, double& eta, double& minCE, double& rise, const char*& stt) { minCE = b > 0 ? L - a * a / (2 * b) : NAN; rise = NAN; eta = NAN;
        if (L <= t) { eta = 0; stt = "L0<=hedef: hedef bu veride zaten saglandi (eta=0)"; return; }
        // 0.5 b eta^2 + a eta + (L - t) = 0
        double d = a * a - 2 * b * (L - t); if (d < 0) { stt = "ULASILAMAZ (diskriminant<0)"; return; }
        double r1, r2; if (fabs(b) < 1e-12) { r1 = r2 = -(L - t) / a; } else { r1 = (-a - sqrt(d)) / b; r2 = (-a + sqrt(d)) / b; } double lo = min(r1, r2), hi = max(r1, r2);
        if (lo > 0) { eta = lo; stt = b < 0 ? "kok var (b<0: yerel model asagi acik parabol, minimum YOK)" : "kok var (en kucuk pozitif)"; } else if (hi > 0) { eta = hi; stt = "kok var (yalniz buyuk kok pozitif)"; } else stt = "pozitif kok yok"; };
    for (int var = 0; var < 2; var++) { // 0: egitim denklemi (gorev), 1: dogrulama denklemi (kontrol/tani)
        double L = var == 0 ? L0[0] : L0[1], a = var == 0 ? aT : aV, b = var == 0 ? bT : bV;
        for (int ti = 0; ti < NT_; ti++) { double t = log(TP[ti]); double eta, minCE, rise; const char* stt; solve(L, a, b, t, eta, minCE, rise, stt);
            printf("HEDEF|%s|PPL=%g|CE_hedef=%.5f|L0=%.5f|a=%.6g|b=%.6g|durum=%s|eta_hesap=%.6g|ikinci_derece_min_CE=%.5f (PPL %.2f)|yukselen_kok=%.6g\n", var == 0 ? "EGITIM_DENKLEMI" : "DOGRULAMA_DENKLEMI", TP[ti], t, L, a, b, stt, eta, minCE, exp(minCE), rise);
            // uygulama: ulasilamazsa ikinci derece minimum eta*'da (en iyi olasi)
            double eu = std::isnan(eta) ? -a / b : eta; if (!std::isnan(eta) || b > 0) { Res r0 = CEf(0, eu, al0, T0c), r1 = CEf(1, eu, al0, T0c), r2 = CEf(2, eu, al0, T0c); double pred = L + a * eu + 0.5 * b * eu * eu;
                auto Zv = mix(Pva_, nva, eu); static double ce[NT][NA]; ce_table(Zv, va.y.data(), nva, Ptr, ce); int bt = 0, ba = 0; double bv = 1e30; for (int tt = 0; tt < NT; tt++) if (TV9[tt]) for (int aa = 0; aa < NA; aa++) if (ce[tt][aa] < bv) { bv = ce[tt][aa]; bt = tt; ba = aa; }
                auto Zt = mix(Pte_, nte, eu); Res rc = eval_cal(Zt, te.y.data(), nte, Ptr, AG[ba], TG[bt]);
                printf("UYGULA|%s|PPL=%g|eta_kullanilan=%.6g%s|tahmin_CE(%s)=%.5f|egitim CE=%.5f PPL=%.2f|dogr CE=%.5f PPL=%.2f|test CE=%.5f PPL=%.2f top1=%.4f|tahmin-gercek(%s)=%+.5f|dogr-kalibre(alpha=%g,T=%g): test CE=%.5f PPL=%.2f top1=%.4f top5=%.4f\n", var == 0 ? "EGITIM_DENKLEMI" : "DOGRULAMA_DENKLEMI", TP[ti], eu, std::isnan(eta) ? " (ULASILAMAZ -> ikinci derece minimum)" : "", var == 0 ? "egitim" : "dogr", pred, r0.ce, r0.ppl, r1.ce, r1.ppl, r2.ce, r2.ppl, r2.top1, var == 0 ? "egitim" : "dogr", pred - (var == 0 ? r0.ce : r1.ce), AG[ba], TG[bt], rc.ce, rc.ppl, rc.top1, rc.top5); fflush(stdout); } }
    }
    // --- ikinci derece modelden bagimsiz: gercek egrinin hedefi kesme noktasi (egitim, dogrulama), egri uzerinde dogrusal enterpolasyon
    for (int s = 0; s < 2; s++) for (int ti = 0; ti < NT_; ti++) { double t = log(TP[ti]); double mn = 1e30, emn = 0; for (int g = 0; g < NG; g++) if (cur[g][s] < mn) { mn = cur[g][s]; emn = EG[g]; } double root = NAN; if (cur[0][s] <= t) root = 0; else for (int g = 1; g < NG; g++) if (cur[g][s] <= t) { double x0 = EG[g - 1], x1 = EG[g], y0 = cur[g - 1][s], y1 = cur[g][s]; root = x0 + (y0 - t) / (y0 - y1) * (x1 - x0); break; }
        printf("GERCEK_EGRI_KESISIM|%s|PPL=%g|CE_hedef=%.5f|gercek_egri_min_CE=%.5f (PPL %.2f) eta=%g|hedefi_kesen_eta=%.6g\n", SN[s], TP[ti], t, mn, exp(mn), emn, root); 
        if (s == 0 && !std::isnan(root) && root > 0) { Res r0 = CEf(0, root, al0, T0c), r1 = CEf(1, root, al0, T0c), r2 = CEf(2, root, al0, T0c); printf("UYGULA_GERCEK_KOK|egitim egrisinin kesisimi (yalniz degerlendirme, agirlik guncellemesi yok)|PPL=%g|eta=%.6g|egitim CE=%.5f PPL=%.2f|dogr CE=%.5f PPL=%.2f|test CE=%.5f PPL=%.2f top1=%.4f\n", TP[ti], root, r0.ce, r0.ppl, r1.ce, r1.ppl, r2.ce, r2.ppl, r2.top1); } }
    printf("SURE Ridge=%.3fs CE duzeltme=%.3fs ; katsayi(L0,a,b) icin ek: 1 analitik nokta carpimi + 2 egitim CE degerlendirmesi (+-h) ; TOPLAM=%.1fs tepe_RAM=%.0fMB\n", t_ridge, t_corr, now() - Ts, peak_mb());
    return 0;
}
