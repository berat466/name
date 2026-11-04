// th_gen_data <data_src_dizini> <cikis_dizini>
// V59 veri kumesini (10.001 kelime) kaynak-verisinden BIREBIR uretir:
//   ids_{train,val,test}.i32 : [blok x 500] int32 (kelime kimlikleri; id>=V -> 0 zaten uygulanmis)
//   E_table.f32              : [10001 x 64] float32 (egitimden PPMI-SVD(64) gomme tablosu, V'den bagimsiz)
// uretilen: c2_{split}_ctx.i32 [n x 2], c2_{split}_y.i32 [n], c2_{split}_X.f32 [n x 128] (= [E[ctx0], E[ctx1]]), uni_tr.f64, uni_trva.f64 (kaynak dizinden kopya), meta_c2.txt
// Yeni sayi/ogrenme uretmez; hesap yalnizca indeks-bakma ve kopyadir.
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
using namespace std;
template <class T> static vector<T> rd(const string& p) { FILE* f = fopen(p.c_str(), "rb"); if (!f) { fprintf(stderr, "acilamadi: %s\n", p.c_str()); exit(1); } fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); vector<T> v(n / sizeof(T)); if (fread(v.data(), 1, n, f) != (size_t)n) { fprintf(stderr, "okuma hatasi %s\n", p.c_str()); exit(1); } fclose(f); return v; }
template <class T> static void wr(const string& p, const vector<T>& v) { FILE* f = fopen(p.c_str(), "wb"); if (!f) { fprintf(stderr, "yazilamadi: %s\n", p.c_str()); exit(1); } fwrite(v.data(), sizeof(T), v.size(), f); fclose(f); }
int main(int argc, char** argv) {
    if (argc < 3) { fprintf(stderr, "kullanim: th_gen_data <data_src> <out>\n"); return 2; }
    string in = argv[1], out = argv[2]; const int V = 10001, E = 64, BL = 500; vector<float> Et = rd<float>(in + "/E_table.f32"); if ((int)Et.size() != V * E) { fprintf(stderr, "E_table boyutu yanlis\n"); return 3; }
    long n[3]; const char* sp[3] = {"train", "val", "test"};
    for (int s = 0; s < 3; s++) { vector<int32_t> ids = rd<int32_t>(in + "/ids_" + sp[s] + ".i32"); long nb = (long)ids.size() / BL; long m = nb * (BL - 2); n[s] = m;
        vector<int32_t> ctx(m * 2), y(m); vector<float> X((size_t)m * 2 * E); long k = 0;
        for (long b = 0; b < nb; b++) for (int t = 2; t < BL; t++, k++) { int a = ids[b * BL + t - 2], c = ids[b * BL + t - 1]; ctx[2 * k] = a; ctx[2 * k + 1] = c; y[k] = ids[b * BL + t]; memcpy(&X[(size_t)k * 2 * E], &Et[(size_t)a * E], E * 4); memcpy(&X[(size_t)k * 2 * E + E], &Et[(size_t)c * E], E * 4); }
        wr(out + "/c2_" + sp[s] + "_ctx.i32", ctx); wr(out + "/c2_" + sp[s] + "_y.i32", y); wr(out + "/c2_" + sp[s] + "_X.f32", X); }
    for (const char* f : {"uni_tr.f64", "uni_trva.f64"}) { vector<char> b = rd<char>(in + "/" + f); wr(out + "/" + f, b); }
    FILE* mf = fopen((out + "/meta_c2.txt").c_str(), "wb"); fprintf(mf, "%d %d %ld %ld %ld\n", V, 2 * E, n[0], n[1], n[2]); fclose(mf);
    printf("tamam: n_train=%ld n_val=%ld n_test=%ld\n", n[0], n[1], n[2]); return 0;
}
