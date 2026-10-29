// pm: Parametre Motoru komut satiri.  pm asm|run|dump|stats|fuzz
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <sys/resource.h>

#include "program.h"

static bool read_file(const std::string& path, std::string& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  return true;
}
static bool write_file(const std::string& path, const void* d, size_t n) {
  std::ofstream f(path, std::ios::binary);
  if (!f) return false;
  f.write((const char*)d, std::streamsize(n));
  return bool(f);
}

static bool ends_with(const std::string& s, const char* e) { size_t n = strlen(e); return s.size() >= n && s.compare(s.size() - n, n, e) == 0; }
static Program load_any(const std::string& path) {
  if (ends_with(path, ".pmt")) return assemble(sablon_to_pma(path));
  std::string txt;
  if (!read_file(path, txt)) throw PmError("dosya okunamadi: " + path);
  if (is_yapisal(txt)) return assemble(yapisal_to_pma(txt));
  if (txt.size() >= 4 && memcmp(txt.data(), "PMB1", 4) == 0) return decode_program((const uint8_t*)txt.data(), txt.size());
  return assemble(txt);
}

static int usage() {
  fprintf(stderr,
    "kullanim:\n"
    "  pm asm   kaynak.pma -o cikti.pmb [--strip]   yazma bicimini ikili tabloya cevirir\n"
    "  pm run   dosya.(pma|pmb) [--fuel N] [--mem SOZCUK] [--stack S] [--frames F] [--result] [--stats] [--native | --native-strict] [--max-kare N] [--girdi olaylar.txt] [--ekran son.ppm] [--ascii K] [--kare-rapor]   calistirir (stdin -> getc)\n"
    "  pm yapisal tablo.pmy [--pma x.pma] [-o x.pmb] [--strip]   sayisal yapisal tabloyu (PMY1) .pmb/.pma uretir\n"
    "  pm tablo  tablo.pmy                         yapisal tabloyu okunabilir agac olarak basar\n"
    "  pm liste x.pmb / pm yazliste liste.txt -o x.pmb   PMB1 <-> duz sayi listesi (dogrulamasiz; referans araci)\n"
    "  pm listedogrula liste.txt                         liste -> kati dogrulama + analiz (\"1 Z\" / \"0\"; oz/dogr referansi)\n"
    "  pm dump  dosya.(pma|pmb)                    motorun calistirdigi tabloyu okunabilir basar\n"
    "  pm fuzz  dosya.pmb [N] [tohum]               ikiliyi rastgele bozar; yukleyici/VM cokmemeli\n"
    "cikis kodu: 0 basari, 1 hata (dosya/ayristirma/dogrulama), 2 calisma tuzagi (TRAP)\n");
  return 1;
}

int main(int argc, char** argv) {
  if (argc < 3) return usage();
  std::string cmd = argv[1], file = argv[2];
  try {
    if (cmd == "asm") {
      std::string outp; bool strip = false;
      for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) outp = argv[++i];
        else if (!strcmp(argv[i], "--strip")) strip = true;
        else return usage();
      }
      if (outp.empty()) return usage();
      std::string txt; if (!read_file(file, txt)) throw PmError("dosya okunamadi: " + file);
      Program p = assemble(txt);
      auto bin = encode_program(p, strip);
      if (!write_file(outp, bin.data(), bin.size())) throw PmError("yazilamadi: " + outp);
      size_t ncode = 0, nregs = 0; for (auto& f : p.funcs) { ncode += f.code.size(); nregs += f.nregs; }
      printf("%s: %zu bayt (kaynak %zu bayt), %zu fonksiyon, %zu komut, %zu yazmac, %zu sabit, %zu veri sozcugu%s\n",
             outp.c_str(), bin.size(), txt.size(), p.funcs.size(), ncode, nregs, p.consts.size(), p.data.size(), strip ? ", adlar atildi" : "");
      return 0;
    }
    if (cmd == "sablon") {   // pm sablon uygulama.pmt [--pma uretilen.pma] [-o cikti.pmb] [--strip]
      std::string pmaOut, binOut; bool strip = false;
      for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--pma") && i + 1 < argc) pmaOut = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) binOut = argv[++i];
        else if (!strcmp(argv[i], "--strip")) strip = true;
        else return usage();
      }
      std::string pma = sablon_to_pma(file);
      if (!pmaOut.empty() && !write_file(pmaOut, pma.data(), pma.size())) throw PmError("yazilamadi: " + pmaOut);
      Program p = assemble(pma);
      size_t ncode = 0; for (auto& f : p.funcs) ncode += f.code.size();
      auto bin = encode_program(p, strip);
      if (!binOut.empty() && !write_file(binOut, bin.data(), bin.size())) throw PmError("yazilamadi: " + binOut);
      printf("%s: tanim -> %zu satir .pma, %zu bayt parametre dosyasi, %zu fonksiyon, %zu komut, bellek %llu sozcuk\n", file.c_str(), size_t(std::count(pma.begin(), pma.end(), '\n')), bin.size(), p.funcs.size(), ncode, (unsigned long long)p.mem_words);
      return 0;
    }
    if (cmd == "tablo") { std::string txt; if (!read_file(file, txt)) throw PmError("dosya okunamadi: " + file); std::string s = yapisal_dump(txt); fwrite(s.data(), 1, s.size(), stdout); return 0; }
    if (cmd == "yapisal") {   // pm yapisal tablo.pmy [--pma uretilen.pma] [-o cikti.pmb] [--strip]
      std::string pmaOut, binOut; bool strip = false;
      for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--pma") && i + 1 < argc) pmaOut = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) binOut = argv[++i];
        else if (!strcmp(argv[i], "--strip")) strip = true;
        else return usage();
      }
      std::string txt; if (!read_file(file, txt)) throw PmError("dosya okunamadi: " + file);
      std::string pma = yapisal_to_pma(txt);
      if (!pmaOut.empty() && !write_file(pmaOut, pma.data(), pma.size())) throw PmError("yazilamadi: " + pmaOut);
      Program p = assemble(pma);
      size_t ncode = 0; for (auto& f : p.funcs) ncode += f.code.size();
      auto bin = encode_program(p, strip);
      if (!binOut.empty() && !write_file(binOut, bin.data(), bin.size())) throw PmError("yazilamadi: " + binOut);
      printf("%s: tablo %zu bayt -> %zu satir .pma, %zu bayt .pmb, %zu fonksiyon, %zu komut, bellek %llu sozcuk\n", file.c_str(), txt.size(), size_t(std::count(pma.begin(), pma.end(), '\n')), bin.size(), p.funcs.size(), ncode, (unsigned long long)p.mem_words);
      return 0;
    }
    if (cmd == "liste") {   // pm liste x.pmb : bicim cozumu (dogrulamasiz) -> duz sayi listesi
      std::string txt; if (!read_file(file, txt)) throw PmError("dosya okunamadi: " + file);
      std::string s = program_to_liste(parse_program((const uint8_t*)txt.data(), txt.size())); fwrite(s.data(), 1, s.size(), stdout); return 0;
    }
    if (cmd == "listedogrula") {   // pm listedogrula liste.txt : kati cozum + validate_program + analyze_program (parametre tabanli dogrulayicinin referansi)
      std::string txt; if (!read_file(file, txt)) throw PmError("dosya okunamadi: " + file);
      std::string s = liste_dogrula(txt); fwrite(s.data(), 1, s.size(), stdout); return 0;
    }
    if (cmd == "yazliste") {   // pm yazliste liste.txt -o x.pmb : duz sayi listesi -> PMB1 (kodlayici referansi)
      std::string outp; for (int i = 3; i < argc; i++) { if (!strcmp(argv[i], "-o") && i + 1 < argc) outp = argv[++i]; else return usage(); }
      if (outp.empty()) return usage();
      std::string txt; if (!read_file(file, txt)) throw PmError("dosya okunamadi: " + file);
      auto bin = encode_program(liste_to_program(txt), false);
      if (!write_file(outp, bin.data(), bin.size())) throw PmError("yazilamadi: " + outp);
      return 0;
    }
    if (cmd == "dump") { std::string s = disassemble(load_any(file)); fwrite(s.data(), 1, s.size(), stdout); return 0; }
    if (cmd == "run") {
      RunConfig cfg; bool showResult = false, showStats = false, useNative = false, strictNative = false;
      uint64_t maxFrames = 0; std::string inputFile, screenFile; int asciiK = 0; bool frameReport = false;
      for (int i = 3; i < argc; i++) {
        if (!strcmp(argv[i], "--fuel") && i + 1 < argc) cfg.fuel = strtoull(argv[++i], nullptr, 10);
        else if (!strcmp(argv[i], "--mem") && i + 1 < argc) cfg.max_mem_words = strtoull(argv[++i], nullptr, 10);
        else if (!strcmp(argv[i], "--stack") && i + 1 < argc) cfg.stack_words = strtoull(argv[++i], nullptr, 10);
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) cfg.max_frames = strtoull(argv[++i], nullptr, 10);
        else if (!strcmp(argv[i], "--result")) showResult = true;
        else if (!strcmp(argv[i], "--stats")) showStats = true;
        else if (!strcmp(argv[i], "--native-promote") && i + 1 < argc) cfg.native_promote = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--max-kare") && i + 1 < argc) maxFrames = strtoull(argv[++i], nullptr, 10);
        else if (!strcmp(argv[i], "--girdi") && i + 1 < argc) inputFile = argv[++i];
        else if (!strcmp(argv[i], "--ekran") && i + 1 < argc) screenFile = argv[++i];
        else if (!strcmp(argv[i], "--ascii") && i + 1 < argc) asciiK = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--kare-rapor")) frameReport = true;
        else if (!strcmp(argv[i], "--native")) useNative = true;
        else if (!strcmp(argv[i], "--native-strict")) useNative = strictNative = true;
        else return usage();
      }
      Program p = load_any(file);
      IO io; io.in = stdin;
      io.max_frames = maxFrames;
      if (!inputFile.empty()) {   // her satir: "kare kod" (# yorum)
        std::string ev; if (!read_file(inputFile, ev)) throw PmError("girdi dosyasi okunamadi: " + inputFile);
        std::istringstream es(ev); std::string line;
        while (std::getline(es, line)) { if (line.empty() || line[0] == '#') continue; unsigned long long a_, b_; if (sscanf(line.c_str(), "%llu %llu", &a_, &b_) == 2) io.events.push_back({a_, b_}); }
        std::stable_sort(io.events.begin(), io.events.end(), [](const std::pair<uint64_t, uint64_t>& x, const std::pair<uint64_t, uint64_t>& y) { return x.first < y.first; });
      }
      auto t0 = std::chrono::steady_clock::now();
      bool nat = false;
      if (useNative) {
        if (native_available() && native_supported(p)) nat = true;
        else if (strictNative) { fprintf(stderr, "NATIVE: bu program makine koduna cevrilemiyor (desteklenmeyen komut ya da platform)\n"); return 3; }
      }
      RunResult r = nat ? run_program_native(p, io, cfg) : run_program(p, io, cfg);
      double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
      fwrite(io.out.data(), 1, io.out.size(), stdout); fflush(stdout);
      if (frameReport) fprintf(stderr, "ekran: kare=%llu boyut=%ux%u zincir=%016llx son=%016llx\n", (unsigned long long)io.frames, io.fw, io.fh, (unsigned long long)io.chain_hash, (unsigned long long)io.last_hash);
      if (!screenFile.empty() && io.fw) {
        FILE* pf = fopen(screenFile.c_str(), "wb");
        if (!pf) { fprintf(stderr, "HATA: yazilamadi: %s\n", screenFile.c_str()); return 1; }
        fprintf(pf, "P6\n%u %u\n255\n", io.fw, io.fh);
        for (uint32_t px : io.frame) { fputc(int((px >> 16) & 255), pf); fputc(int((px >> 8) & 255), pf); fputc(int(px & 255), pf); }
        fclose(pf);
      }
      if (asciiK > 0 && io.fw) {   // her KxK blogun sol ust pikseli: en parlak kanal >= 0x80 ise '#', degilse '.'
        std::string a;
        for (uint32_t y = 0; y < io.fh; y += uint32_t(asciiK)) {
          for (uint32_t x = 0; x < io.fw; x += uint32_t(asciiK)) {
            uint32_t px = io.frame[size_t(y) * io.fw + x];
            uint32_t m = std::max({(px >> 16) & 255u, (px >> 8) & 255u, px & 255u});
            a.push_back(m >= 0x80 ? '#' : '.');
          }
          a.push_back('\n');
        }
        fwrite(a.data(), 1, a.size(), stdout); fflush(stdout);
      }
      if (r.trap != T_OK) {
        const Func& f = p.funcs[r.func];
        if (nat) { fprintf(stderr, "TRAP: %s (makine kodu, yakit %llu)\n", kTrapName[r.trap], (unsigned long long)r.fuel_used); return 2; }
        fprintf(stderr, "TRAP: %s (fonksiyon %s, komut %u, yakit %llu)\n", kTrapName[r.trap], f.name.empty() ? "?" : f.name.c_str(), r.pc, (unsigned long long)r.fuel_used);
        return 2;
      }
      if (showResult) printf("sonuc: %lld\n", (long long)r.value);
      if (showStats) { struct rusage ru; getrusage(RUSAGE_SELF, &ru); fprintf(stderr, "adim: %llu  sure: %.4f s  hiz: %.1f Madim/s  tepe-bellek: %ld KiB\n", (unsigned long long)r.steps, sec, sec > 0 ? r.steps / sec / 1e6 : 0.0, ru.ru_maxrss); fprintf(stderr, "yakit: %llu\n", (unsigned long long)r.fuel_used); }
      return 0;
    }
    if (cmd == "fuzz") {
      std::string raw; if (!read_file(file, raw)) throw PmError("dosya okunamadi: " + file);
      long n = argc > 3 ? atol(argv[3]) : 10000; unsigned long seed = argc > 4 ? strtoul(argv[4], nullptr, 10) : 1;
      std::mt19937_64 rng(seed);
      long rejected = 0, trapped = 0, ok = 0, skipped = 0, compared = 0, mismatch = 0; bool diff = argc > 5 && !strcmp(argv[5], "--diff");
      RunConfig cfg; cfg.fuel = 200000; cfg.stack_words = 1 << 14; cfg.max_frames = 1 << 10; cfg.max_mem_words = 1 << 20;
      for (long it = 0; it < n; it++) {
        std::string m = raw;
        int k = 1 + int(rng() % 4);
        for (int j = 0; j < k && !m.empty(); j++) {
          size_t pos = rng() % m.size();
          if (rng() % 8 == 0) m[pos] ^= char(1u << (rng() % 8)); else m[pos] = char(rng());
        }
        if (rng() % 16 == 0 && m.size() > 8) m.resize(rng() % m.size());
        Program p;
        try { p = is_yapisal(raw) ? assemble(yapisal_to_pma(m)) : decode_program((const uint8_t*)m.data(), m.size()); } catch (const PmError&) { rejected++; continue; }
        if (p.mem_words > cfg.max_mem_words) { skipped++; continue; }
        IO io;
        RunResult r = run_program(p, io, cfg);
        if (diff && native_available() && native_supported(p)) {
          static const int thr[] = {0, 4, 100000}; cfg.native_promote = thr[rng() % 3];
          IO io2; RunResult r2 = run_program_native(p, io2, cfg);
          compared++;
          if (r2.trap != r.trap || r2.value != r.value || r2.fuel_used != r.fuel_used || io2.out != io.out || io2.frames != io.frames || io2.chain_hash != io.chain_hash) {
            if (const char* dd = getenv("PM_FUZZ_DUMP")) { std::string fn = std::string(dd) + "/fark_" + std::to_string(it) + "_p" + std::to_string(cfg.native_promote) + ".pmb"; write_file(fn, m.data(), m.size()); }
            if (mismatch++ < 5) fprintf(stderr, "FARK (deneme %ld): yorumlayici trap=%s deger=%lld yakit=%llu cikti=%zu bayt | native trap=%s deger=%lld yakit=%llu cikti=%zu bayt\n", it,
              kTrapName[r.trap], (long long)r.value, (unsigned long long)r.fuel_used, io.out.size(), kTrapName[r2.trap], (long long)r2.value, (unsigned long long)r2.fuel_used, io2.out.size());
          }
        }
        if (r.trap == T_OK) ok++; else trapped++;
      }
      printf("fuzz: %ld deneme -> reddedilen %ld, gecerli+calisan %ld, gecerli+tuzak %ld, atlanan(bellek) %ld; cokme yok\n", n, rejected, ok, trapped, skipped);
      printf("gecerlilik orani (reddedilmeyen): %.2f%%\n", n ? 100.0 * (n - rejected) / n : 0.0);
      if (diff) { printf("yorumlayici/native karsilastirma: %ld program karsilastirildi, %ld fark\n", compared, mismatch); if (mismatch) return 4; }
      return 0;
    }
  } catch (const PmError& e) {
    fprintf(stderr, "HATA: %s\n", e.what());
    return 1;
  }
  return usage();
}
