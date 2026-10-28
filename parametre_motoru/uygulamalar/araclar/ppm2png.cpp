// ppm2png cikti.png olcek1 dosya1.ppm [olcek2 dosya2.ppm ...] : P6 dosyalarini tam sayi olcekle yan yana tek PNG'ye dizer.
// (Sikistirmasiz "stored" deflate bloklari; bagimlilik yok.)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
using std::vector;
static uint32_t crc_table[256];
static void init_crc() { for (uint32_t n = 0; n < 256; n++) { uint32_t c = n; for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; crc_table[n] = c; } }
static uint32_t crc(const uint8_t* p, size_t n, uint32_t c = 0xFFFFFFFFu) { for (size_t i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 255] ^ (c >> 8); return c; }
static void be32(vector<uint8_t>& o, uint32_t v) { for (int i = 3; i >= 0; i--) o.push_back(uint8_t(v >> (8 * i))); }
static void chunk(vector<uint8_t>& o, const char* type, const vector<uint8_t>& data) {
  be32(o, uint32_t(data.size()));
  vector<uint8_t> t(type, type + 4); t.insert(t.end(), data.begin(), data.end());
  o.insert(o.end(), t.begin(), t.end());
  be32(o, ~crc(t.data(), t.size()));
}
struct Img { int w = 0, h = 0; vector<uint8_t> px; };
static bool read_ppm(const char* path, Img& im) {
  FILE* f = fopen(path, "rb"); if (!f) return false;
  int maxv; if (fscanf(f, "P6 %d %d %d", &im.w, &im.h, &maxv) != 3 || maxv != 255) { fclose(f); return false; }
  fgetc(f);
  im.px.resize(size_t(im.w) * im.h * 3);
  bool ok = fread(im.px.data(), 1, im.px.size(), f) == im.px.size(); fclose(f); return ok;
}
int main(int argc, char** argv) {
  if (argc < 4 || (argc - 2) % 2) { fprintf(stderr, "kullanim: ppm2png cikti.png olcek dosya.ppm [olcek dosya.ppm ...]\n"); return 1; }
  init_crc();
  vector<Img> imgs; vector<int> sc;
  for (int i = 2; i + 1 < argc; i += 2) { Img im; sc.push_back(atoi(argv[i])); if (!read_ppm(argv[i + 1], im)) { fprintf(stderr, "okunamadi: %s\n", argv[i + 1]); return 1; } imgs.push_back(im); }
  const int gap = 12; int W = gap, H = 0;
  for (size_t i = 0; i < imgs.size(); i++) { W += imgs[i].w * sc[i] + gap; H = std::max(H, imgs[i].h * sc[i]); }
  H += 2 * gap;
  vector<uint8_t> rgb(size_t(W) * H * 3, 40);
  int x0 = gap;
  for (size_t i = 0; i < imgs.size(); i++) {
    for (int y = 0; y < imgs[i].h * sc[i]; y++) for (int x = 0; x < imgs[i].w * sc[i]; x++)
      for (int c = 0; c < 3; c++) rgb[(size_t(y + gap) * W + x0 + x) * 3 + c] = imgs[i].px[(size_t(y / sc[i]) * imgs[i].w + x / sc[i]) * 3 + c];
    x0 += imgs[i].w * sc[i] + gap;
  }
  vector<uint8_t> raw;
  for (int y = 0; y < H; y++) { raw.push_back(0); raw.insert(raw.end(), rgb.begin() + size_t(y) * W * 3, rgb.begin() + size_t(y + 1) * W * 3); }
  vector<uint8_t> z = {0x78, 0x01};
  for (size_t p = 0; p < raw.size();) {
    size_t n = std::min<size_t>(65535, raw.size() - p); bool last = p + n == raw.size();
    z.push_back(last ? 1 : 0); z.push_back(uint8_t(n)); z.push_back(uint8_t(n >> 8)); z.push_back(uint8_t(~n)); z.push_back(uint8_t(~n >> 8));
    z.insert(z.end(), raw.begin() + p, raw.begin() + p + n); p += n;
  }
  uint32_t a = 1, b = 0; for (uint8_t v : raw) { a = (a + v) % 65521; b = (b + a) % 65521; }
  be32(z, (b << 16) | a);
  vector<uint8_t> out = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  vector<uint8_t> ihdr; be32(ihdr, uint32_t(W)); be32(ihdr, uint32_t(H)); ihdr.push_back(8); ihdr.push_back(2); ihdr.push_back(0); ihdr.push_back(0); ihdr.push_back(0);
  chunk(out, "IHDR", ihdr); chunk(out, "IDAT", z); chunk(out, "IEND", {});
  FILE* f = fopen(argv[1], "wb"); if (!f) return 1; fwrite(out.data(), 1, out.size(), f); fclose(f);
  printf("%s: %dx%d\n", argv[1], W, H);
}
