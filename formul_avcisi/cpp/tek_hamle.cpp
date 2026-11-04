// Tek-hamle ogrenme (ELM + Cholesky) vs gradyan inisi (Adam MLP), ayni C++ dongulerle, ayni veri.
// Derleme: g++ -O3 -march=native -fopenmp -o tek_hamle tek_hamle.cpp
// Calistirma: ./tek_hamle digits_train.csv digits_test.csv [Hf=1024] [lambda=1.0]
#include <bits/stdc++.h>
#include <omp.h>
using namespace std;
using Clock = chrono::steady_clock;
static double secs(Clock::time_point a) { return chrono::duration<double>(Clock::now() - a).count(); }

struct Data { int n = 0, d = 0; vector<double> X; vector<int> y; };

Data load(const string& path) {
    ifstream f(path); if (!f) { cerr << "acilamadi: " << path << "\n"; exit(1); }
    Data D; string line; vector<vector<double>> rows;
    while (getline(f, line)) { stringstream ss(line); string t; vector<double> r; while (getline(ss, t, ',')) r.push_back(stod(t)); rows.push_back(r); }
    D.n = rows.size(); D.d = rows[0].size() - 1; D.X.resize((size_t)D.n * D.d); D.y.resize(D.n);
    for (int i = 0; i < D.n; i++) { for (int j = 0; j < D.d; j++) D.X[(size_t)i * D.d + j] = rows[i][j]; D.y[i] = (int)rows[i].back(); }
    return D;
}

// --- ELM -------------------------------------------------------------------------------------------
// ozellik F (n x m), m = Hf+1 (son sutun 1). S = F^T F + lam I ; R = F^T T ; S beta = R  (Cholesky)
double elm(const Data& tr, const Data& te, int Hf, double lam, double& t_fit, double& t_pred) {
    int d = tr.d, C = 10, m = Hf + 1; mt19937_64 g(5); normal_distribution<double> N01(0, 1);
    vector<double> Win((size_t)d * Hf), b(Hf); for (auto& w : Win) w = N01(g) / sqrt((double)d); for (auto& v : b) v = N01(g) * 0.5;
    auto feats = [&](const Data& D) { vector<double> F((size_t)D.n * m);
        #pragma omp parallel for
        for (int i = 0; i < D.n; i++) { for (int h = 0; h < Hf; h++) { double s = b[h]; for (int j = 0; j < d; j++) s += D.X[(size_t)i * d + j] * Win[(size_t)j * Hf + h]; F[(size_t)i * m + h] = tanh(s); } F[(size_t)i * m + Hf] = 1.0; }
        return F; };
    auto t0 = Clock::now();
    vector<double> F = feats(tr), S((size_t)m * m, 0.0), R((size_t)m * C, 0.0);
    #pragma omp parallel for schedule(dynamic)
    for (int a = 0; a < m; a++) { double* Sa = &S[(size_t)a * m];
        for (int i = 0; i < tr.n; i++) { double fa = F[(size_t)i * m + a]; const double* Fi = &F[(size_t)i * m]; for (int c2 = a; c2 < m; c2++) Sa[c2] += fa * Fi[c2]; }
        for (int i = 0; i < tr.n; i++) R[(size_t)a * C + tr.y[i]] += F[(size_t)i * m + a]; }
    for (int a = 0; a < m; a++) { S[(size_t)a * m + a] += lam; for (int c2 = 0; c2 < a; c2++) S[(size_t)a * m + c2] = S[(size_t)c2 * m + a]; }
    // Cholesky: S = L L^T (alt ucgen, yerinde)
    for (int j = 0; j < m; j++) {
        double s = S[(size_t)j * m + j]; for (int k = 0; k < j; k++) s -= S[(size_t)j * m + k] * S[(size_t)j * m + k];
        double Ljj = sqrt(s); S[(size_t)j * m + j] = Ljj;
        #pragma omp parallel for
        for (int i = j + 1; i < m; i++) { double t = S[(size_t)i * m + j]; const double* Li = &S[(size_t)i * m]; const double* Lj = &S[(size_t)j * m]; for (int k = 0; k < j; k++) t -= Li[k] * Lj[k]; S[(size_t)i * m + j] = t / Ljj; }
    }
    vector<double> beta(R); // ileri ve geri yerlestirme (C sag taraf)
    for (int c = 0; c < C; c++) { for (int i = 0; i < m; i++) { double t = beta[(size_t)i * C + c]; for (int k = 0; k < i; k++) t -= S[(size_t)i * m + k] * beta[(size_t)k * C + c]; beta[(size_t)i * C + c] = t / S[(size_t)i * m + i]; }
        for (int i = m - 1; i >= 0; i--) { double t = beta[(size_t)i * C + c]; for (int k = i + 1; k < m; k++) t -= S[(size_t)k * m + i] * beta[(size_t)k * C + c]; beta[(size_t)i * C + c] = t / S[(size_t)i * m + i]; } }
    t_fit = secs(t0); auto t1 = Clock::now();
    vector<double> Ft = feats(te); int ok = 0;
    for (int i = 0; i < te.n; i++) { int best = 0; double bv = -1e300; for (int c = 0; c < C; c++) { double s = 0; for (int k = 0; k < m; k++) s += Ft[(size_t)i * m + k] * beta[(size_t)k * C + c]; if (s > bv) { bv = s; best = c; } } ok += (best == te.y[i]); }
    t_pred = secs(t1); return (double)ok / te.n;
}

// --- Adam MLP (64-H-10, tam parti) -----------------------------------------------------------------
double adam(const Data& tr, const Data& te, int H, int steps, double lr, double& t_fit) {
    int d = tr.d, C = 10, n = tr.n; mt19937_64 g(7); normal_distribution<double> N01(0, 1);
    vector<double> W1((size_t)d * H), b1(H, 0.0), W2((size_t)H * C), b2(C, 0.0);
    for (auto& w : W1) w = N01(g) / sqrt((double)d); for (auto& w : W2) w = N01(g) / sqrt((double)H);
    vector<vector<double>*> P = {&W1, &b1, &W2, &b2}; vector<vector<double>> M, V; for (auto p : P) { M.emplace_back(p->size(), 0.0); V.emplace_back(p->size(), 0.0); }
    vector<double> A((size_t)n * H), E((size_t)n * C), dA((size_t)n * H), G0(W1.size()), G1(H), G2(W2.size()), G3(C); vector<vector<double>*> G = {&G0, &G1, &G2, &G3};
    auto t0 = Clock::now();
    for (int t = 1; t <= steps; t++) {
        #pragma omp parallel for
        for (int i = 0; i < n; i++) {
            for (int h = 0; h < H; h++) { double s = b1[h]; for (int j = 0; j < d; j++) s += tr.X[(size_t)i * d + j] * W1[(size_t)j * H + h]; A[(size_t)i * H + h] = tanh(s); }
            double z[10], mx = -1e300; for (int c = 0; c < C; c++) { double s = b2[c]; for (int h = 0; h < H; h++) s += A[(size_t)i * H + h] * W2[(size_t)h * C + c]; z[c] = s; mx = max(mx, s); }
            double sum = 0; for (int c = 0; c < C; c++) { z[c] = exp(z[c] - mx); sum += z[c]; }
            for (int c = 0; c < C; c++) E[(size_t)i * C + c] = (z[c] / sum - (tr.y[i] == c)) / n;
            for (int h = 0; h < H; h++) { double s = 0; for (int c = 0; c < C; c++) s += E[(size_t)i * C + c] * W2[(size_t)h * C + c]; double a = A[(size_t)i * H + h]; dA[(size_t)i * H + h] = s * (1 - a * a); }
        }
        fill(G0.begin(), G0.end(), 0.0); fill(G1.begin(), G1.end(), 0.0); fill(G2.begin(), G2.end(), 0.0); fill(G3.begin(), G3.end(), 0.0);
        #pragma omp parallel for
        for (int j = 0; j < d; j++) for (int i = 0; i < n; i++) { double x = tr.X[(size_t)i * d + j]; for (int h = 0; h < H; h++) G0[(size_t)j * H + h] += x * dA[(size_t)i * H + h]; }
        #pragma omp parallel for
        for (int h = 0; h < H; h++) { for (int i = 0; i < n; i++) { G1[h] += dA[(size_t)i * H + h]; double a = A[(size_t)i * H + h]; for (int c = 0; c < C; c++) G2[(size_t)h * C + c] += a * E[(size_t)i * C + c]; } }
        for (int i = 0; i < n; i++) for (int c = 0; c < C; c++) G3[c] += E[(size_t)i * C + c];
        double c1 = 1 - pow(0.9, t), c2 = 1 - pow(0.999, t);
        for (size_t k = 0; k < P.size(); k++) for (size_t q = 0; q < P[k]->size(); q++) { double gq = (*G[k])[q]; M[k][q] = 0.9 * M[k][q] + 0.1 * gq; V[k][q] = 0.999 * V[k][q] + 0.001 * gq * gq; (*P[k])[q] -= lr * (M[k][q] / c1) / (sqrt(V[k][q] / c2) + 1e-8); }
    }
    t_fit = secs(t0); int ok = 0;
    for (int i = 0; i < te.n; i++) { vector<double> a(H); for (int h = 0; h < H; h++) { double s = b1[h]; for (int j = 0; j < d; j++) s += te.X[(size_t)i * d + j] * W1[(size_t)j * H + h]; a[h] = tanh(s); }
        int best = 0; double bv = -1e300; for (int c = 0; c < C; c++) { double s = b2[c]; for (int h = 0; h < H; h++) s += a[h] * W2[(size_t)h * C + c]; if (s > bv) { bv = s; best = c; } } ok += (best == te.y[i]); }
    return (double)ok / te.n;
}

int main(int argc, char** argv) {
    if (argc < 3) { cerr << "kullanim: " << argv[0] << " train.csv test.csv [Hf] [lambda]\n"; return 1; }
    Data tr = load(argv[1]), te = load(argv[2]); int Hf = argc > 3 ? atoi(argv[3]) : 1024; double lam = argc > 4 ? atof(argv[4]) : 1.0;
    // standartlastir (egitim istatistigiyle)
    for (int j = 0; j < tr.d; j++) { double mu = 0, s2 = 0; for (int i = 0; i < tr.n; i++) mu += tr.X[(size_t)i * tr.d + j]; mu /= tr.n; for (int i = 0; i < tr.n; i++) { double v = tr.X[(size_t)i * tr.d + j] - mu; s2 += v * v; } double sd = sqrt(s2 / tr.n) + 1e-6;
        for (int i = 0; i < tr.n; i++) tr.X[(size_t)i * tr.d + j] = (tr.X[(size_t)i * tr.d + j] - mu) / sd; for (int i = 0; i < te.n; i++) te.X[(size_t)i * te.d + j] = (te.X[(size_t)i * te.d + j] - mu) / sd; }
    printf("egitim %d, test %d, girdi %d, %d is parcacigi\n\n", tr.n, te.n, tr.d, omp_get_max_threads());
    double tf, tp; double a1 = adam(tr, te, 64, 300, 0.01, tf);
    printf("%-34s dogruluk %5.1f%%   ogrenme suresi %7.3f sn\n", "Adam MLP 64-64-10 (300 adim)", a1 * 100, tf);
    for (int hf : {256, Hf}) { double a2 = elm(tr, te, hf, lam, tf, tp);
        printf("%-34s dogruluk %5.1f%%   ogrenme suresi %7.3f sn   (tahmin %.3f sn)\n", ("ELM tek hamle Hf=" + to_string(hf)).c_str(), a2 * 100, tf, tp); }
    return 0;
}
