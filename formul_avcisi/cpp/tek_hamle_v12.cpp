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


// agirlikli satirlar icin: Fs = sqrt(w) F  (S icin),  Fw = w F (R icin)
static void stats_weighted(const vector<float>& F, const vector<int>& y, const vector<float>& w, int n, int m, vector<double>& S, vector<double>& R) {
    vector<float> Fs, Fw; 
    for (int s0 = 0; s0 < n; s0 += 2048) { int nn = min(2048, n - s0); Fs.assign((size_t)nn * m, 0.f); Fw.assign((size_t)nn * m, 0.f);
        #pragma omp parallel for
        for (int i = 0; i < nn; i++) { float sw = sqrtf(w[s0 + i]), ww = w[s0 + i]; const float* r = &F[(size_t)(s0 + i) * m]; for (int j = 0; j < m; j++) { Fs[(size_t)i * m + j] = r[j] * sw; Fw[(size_t)i * m + j] = r[j] * ww; } }
        syrk_flush256(Fs.data(), nn, m, S.data()); rhs_add(Fw.data(), &y[s0], nn, m, V, R.data()); }
}
int main(int argc, char** argv) {
    if (argc < 6) { cerr << "kullanim: tek_hamle_v12 cift c H TOHUM OZELLIK [FORMULLER=ABC] [veri]\n"; return 1; }
    string mode = argv[1]; int c = atoi(argv[2]), H = atoi(argv[3]), seed = atoi(argv[4]); string oz = argv[5]; string FS = argc > 6 ? argv[6] : "ABC"; string dir = argc > 7 ? argv[7] : "/tmp/v8data";
    int ntr, nva, nte; { ifstream mf(dir + "/meta_c" + to_string(c) + ".txt"); mf >> V >> K >> ntr >> nva >> nte; }
    Data tr = load(dir, c, "train", ntr), va = load(dir, c, "val", nva), te = load(dir, c, "test", nte); const int m = H + 1;
    { int Kd = c * 64 + (c - 1) * 64; BASEA = Kd; BASEB = Kd + V; BASEP = (oz == "pair") ? Kd : (oz == "bag" ? Kd + V : Kd + 2 * V);
      KT = Kd + (oz == "ref" ? 0 : oz == "bag" ? V : oz == "seq" ? 2 * V : oz == "pair" ? HB : 2 * V + HB);
      expand(tr, c, oz); expand(va, c, oz); expand(te, c, oz); K = Kd; }
    printf("V12 oz=%s c=%d H=%d tohum=%d formuller=%s V=%d K=%d egitim=%d dogrulama=%d test=%d (%d cekirdek)\n", oz.c_str(), c, H, seed, FS.c_str(), V, K, ntr, nva, nte, omp_get_max_threads());
    double T0 = now(); double t0 = now(); Feat f = make_w1("cift", tr, H, seed); double t_w1 = now() - t0;
    // B: ornek agirliklari: w_i ~ (token sikligi)^-0.5, ortalama 1'e normalize, [0.25, 4] araliginda sinirli, sonra tekrar ortalama 1
    vector<double> cnt(V, 0.0); for (int i = 0; i < ntr; i++) cnt[tr.y[i]] += 1; double meanc = (double)ntr / V;
    auto wof = [&](int y) { double w = pow(max(cnt[y], 1.0) / meanc, -0.5); return w; };
    double sw = 0; for (int i = 0; i < ntr; i++) sw += wof(tr.y[i]); double nrm = ntr / sw;
    auto wfin = [&](int y) { double w = wof(y) * nrm; return (float)min(4.0, max(0.25, w)); };
    vector<float> wtr(ntr), wva(nva); double smean = 0; for (int i = 0; i < ntr; i++) { wtr[i] = wfin(tr.y[i]); smean += wtr[i]; } for (int i = 0; i < nva; i++) wva[i] = wfin(va.y[i]);
    printf("B ornek agirligi: ort=%.3f min=%.3f maks=%.3f\n", smean / ntr, *min_element(wtr.begin(), wtr.end()), *max_element(wtr.begin(), wtr.end()));
    const double GAMS[4] = {0.5, 1, 2, 4}, LAMS[4] = {1, 10, 100, 1000}, TS[9] = {0.003, 0.005, 0.01, 0.02, 0.03, 0.05, 0.07, 0.1, 0.2};
    struct Best { double ce = 1e30, gam = 0, lam = 0, T = 0; int gi = 0; };
    map<char, Best> best; map<char, vector<vector<double>>> Sall, Rall; map<char, vector<double>> t_stat; vector<vector<double>> varj(4);
    for (char F_ : FS) { Sall[F_].resize(4); Rall[F_].resize(4); t_stat[F_].assign(4, 0.0); }
    vector<float> Ftr, Fva;
    for (int gi = 0; gi < 4; gi++) {
        double gam = GAMS[gi]; features(tr, f, gam, Ftr); features(va, f, gam, Fva);
        map<char, vector<double>> S, R;
        for (char F_ : FS) { if (F_ == 'C') continue; S[F_].assign((size_t)m * m, 0.0); R[F_].assign((size_t)m * V, 0.0); t0 = now();
            if (F_ == 'A') { syrk_flush256(Ftr.data(), ntr, m, S[F_].data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, R[F_].data()); } else stats_weighted(Ftr, tr.y, wtr, ntr, m, S[F_], R[F_]);
            t_stat[F_][gi] = now() - t0; }
        if (FS.find('C') != string::npos && S.find('A') == S.end()) { S['A'].assign((size_t)m * m, 0.0); R['A'].assign((size_t)m * V, 0.0); syrk_flush256(Ftr.data(), ntr, m, S['A'].data()); rhs_add(Ftr.data(), tr.y.data(), ntr, m, V, R['A'].data()); }
        // C: ozellik varyansi (egitim) -> Lambda_jj = lambda * v_j / ortalama(v) ; bias kolonu icin kucuk sabit
        vector<double>& vj = varj[gi]; vj.assign(m, 0.0); { vector<double>& SA = S['A']; double n_ = ntr; double mv = 0; for (int j = 1; j < m; j++) { double mean = SA[j] / n_; vj[j] = max(SA[(size_t)j * m + j] / n_ - mean * mean, 1e-12); mv += vj[j]; } mv /= (m - 1); for (int j = 1; j < m; j++) vj[j] /= mv; vj[0] = 1e-3; }
        for (char F_ : FS) for (int li = 0; li < 4; li++) {
            const vector<double>& Sm = (F_ == 'C') ? S['A'] : S[F_]; const vector<double>& Rm = (F_ == 'C') ? R['A'] : R[F_];
            vector<double> A = Sm; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += (F_ == 'C') ? LAMS[li] * vj[i] : LAMS[li];
            if (!cholesky(A, m)) { printf("  [%c] gamma=%g lambda=%g Cholesky basarisiz\n", F_, gam, LAMS[li]); continue; }
            vector<float> W; chol_solve_multi(A, m, Rm, V, W); double pce = 1e30, pT = 0;
            for (double T : TS) { Metrics M = evaluate(Fva, W, nva, m, va.y.data(), T); if (M.ce < pce) { pce = M.ce; pT = T; } if (M.ce < best[F_].ce) best[F_] = {M.ce, gam, LAMS[li], T, gi}; }
            printf("  [%c] gamma=%g lambda=%g en_iyi_T=%g val_CE=%.4f\n", F_, gam, LAMS[li], pT, pce); fflush(stdout);
        }
        // egitim+dogrulama istatistigi (toplanabilir)
        for (char F_ : FS) { if (F_ == 'C') continue; if (F_ == 'A') { syrk_flush256(Fva.data(), nva, m, S[F_].data()); rhs_add(Fva.data(), va.y.data(), nva, m, V, R[F_].data()); } else stats_weighted(Fva, va.y, wva, nva, m, S[F_], R[F_]); Sall[F_][gi] = move(S[F_]); Rall[F_][gi] = move(R[F_]); }
        if (FS.find('C') != string::npos) { if (FS.find('A') == string::npos) { syrk_flush256(Fva.data(), nva, m, S['A'].data()); rhs_add(Fva.data(), va.y.data(), nva, m, V, R['A'].data()); } Sall['C'][gi] = (FS.find('A') != string::npos) ? Sall['A'][gi] : move(S['A']); Rall['C'][gi] = (FS.find('A') != string::npos) ? Rall['A'][gi] : move(R['A']); }
    }
    double t_search = now() - T0;
    for (char F_ : FS) {
        Best b = best[F_]; double gam = b.gam; vector<float> Ftr2, Fva2, Fte;
        // egitim-yalniz model: dogrulama skorlari (kalibrasyon icin)
        features(tr, f, gam, Ftr2); features(va, f, gam, Fva2);
        { vector<double> S2((size_t)m * m, 0.0), R2((size_t)m * V, 0.0);
          if (F_ == 'B') stats_weighted(Ftr2, tr.y, wtr, ntr, m, S2, R2); else { syrk_flush256(Ftr2.data(), ntr, m, S2.data()); rhs_add(Ftr2.data(), tr.y.data(), ntr, m, V, R2.data()); }
          for (int i = 0; i < m; i++) S2[(size_t)i * m + i] += (F_ == 'C') ? b.lam * varj[b.gi][i] : b.lam; cholesky(S2, m); vector<float> W2; chol_solve_multi(S2, m, R2, V, W2);
          vector<float> Zv((size_t)nva * V); gemm_f(Fva2.data(), nva, m, W2.data(), V, Zv.data(), V);
          string pth = "/tmp/v12data/cift_" + oz + "_" + string(1, F_) + "_c" + to_string(c) + "_H" + to_string(H) + "_s" + to_string(seed) + "_val.f32"; FILE* fo = fopen(pth.c_str(), "wb"); fwrite(Zv.data(), sizeof(float), Zv.size(), fo); fclose(fo); }
        // son model: egitim+dogrulama
        t0 = now(); vector<double> A = Sall[F_][b.gi]; for (int i = 0; i < m; i++) A[(size_t)i * m + i] += (F_ == 'C') ? b.lam * varj[b.gi][i] : b.lam; cholesky(A, m); vector<float> W; chol_solve_multi(A, m, Rall[F_][b.gi], V, W); double t_final = now() - t0;
        features(te, f, gam, Fte); Metrics M = evaluate(Fte, W, nte, m, te.y.data(), b.T);
        { vector<float> Zt((size_t)nte * V); gemm_f(Fte.data(), nte, m, W.data(), V, Zt.data(), V);
          string pth = "/tmp/v12data/cift_" + oz + "_" + string(1, F_) + "_c" + to_string(c) + "_H" + to_string(H) + "_s" + to_string(seed) + "_test.f32"; FILE* fo = fopen(pth.c_str(), "wb"); fwrite(Zt.data(), sizeof(float), Zt.size(), fo); fclose(fo); }
        double ts = t_stat[F_ == 'C' ? 'A' : F_].empty() ? 0.0 : t_stat[F_ == 'C' ? 'A' : F_][b.gi]; double t_train = t_w1 + ts * (double)(ntr + nva) / ntr + t_final;
        printf("SECILEN[%c] gamma=%g lambda=%g T=%g val_CE=%.4f\n", F_, b.gam, b.lam, b.T, b.ce);
        printf("OZET|%c|%s|c=%d|H=%d|tohum=%d|gam=%g|lam=%g|T=%g|valCE=%.4f|mse_ham=%.6f|mse_olas=%.6f|CE=%.4f|PPL=%.2f|top1=%.4f|top5=%.4f|agirlik_uretim_sn=%.2f\n", F_, oz.c_str(), c, H, seed, b.gam, b.lam, b.T, b.ce, M.mse_raw, M.mse_prob, M.ce, M.ppl, M.top1, M.top5, t_train);
    }
    printf("TOPLAM sure=%.1fs (arama dahil) tepe_RAM=%.0fMB\n", now() - T0, peak_mb());
    return 0;
}
