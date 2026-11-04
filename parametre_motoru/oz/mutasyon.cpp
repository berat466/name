// Dogrulama araci: gecerli bir PMB1 dosyasindan rastgele bozulmus kopyalar uretir. Kullanim: mutasyon girdi.pmb cikdizin adet tohum
#include <cstdio>
#include <fstream>
#include <iterator>
#include <random>
#include <string>
int main(int argc, char** argv) {
  if (argc < 5) return 1;
  std::ifstream f(argv[1], std::ios::binary); std::string raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  int n = atoi(argv[3]); std::mt19937_64 rng(strtoull(argv[4], nullptr, 10));
  for (int it = 0; it < n; it++) {
    std::string m = raw; int k = 1 + int(rng() % 4);
    for (int j = 0; j < k && !m.empty(); j++) {
      size_t pos = rng() % m.size(); int t = int(rng() % 10);
      if (t < 5) m[pos] = char(rng()); else if (t < 7) m[pos] ^= char(1u << (rng() % 8)); else if (t == 7) m.erase(pos, 1); else if (t == 8) m.insert(pos, 1, char(rng())); else m[pos] = char(rng() % 2 ? 0xff : 0x80);
    }
    if (rng() % 12 == 0 && m.size() > 5) m.resize(rng() % m.size());
    char name[512]; snprintf(name, sizeof name, "%s/m%05d.bin", argv[2], it);
    std::ofstream o(name, std::ios::binary); o.write(m.data(), std::streamsize(m.size()));
  }
}
