// Program tablosunun duz sayi listesi bicimi: PMB1 <-> liste. Referans/dogrulama araci (pm liste, pm yazliste).
// Parametre tabanli kodlayici ve cozucunun (oz/) bayt-bayt karsilastirildigi C++ referansi. Dogrulama YAPMAZ (yalniz bicim).
// Bicim (bosluk ayirici sayilar, satir sonu '\n'):
//   BAYRAK BELLEK GIRIS
//   NSABIT sabit...            (isaretsiz 64 bit onluk)
//   NVERI veri...              (isaretli 64 bit)
//   BSS
//   NFONK
//   her fonksiyon: NPARAM NYAZMAC NKOMUT, ardindan her komut: KOD islenen... (etiketler MUTLAK hedef, 'i' isaretli, digerleri isaretsiz)
//   bayrak=1 ise her fonksiyon icin: UZUNLUK bayt...
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include "program.h"

std::string program_to_liste(const Program& p) {
  std::string o; char b[40];
  auto num = [&](long long v) { snprintf(b, sizeof b, "%lld", v); o += b; };
  auto unum = [&](unsigned long long v) { snprintf(b, sizeof b, "%llu", v); o += b; };
  num(p.has_names ? 1 : 0); o += ' '; unum(p.mem_words); o += ' '; unum(p.entry); o += '\n';
  unum(p.consts.size()); for (uint64_t c : p.consts) { o += ' '; unum(c); } o += '\n';
  unum(p.data.size()); for (int64_t d : p.data) { o += ' '; num(d); } o += '\n';
  unum(p.bss); o += '\n';
  unum(p.funcs.size()); o += '\n';
  for (const Func& f : p.funcs) {
    unum(f.nparams); o += ' '; unum(f.nregs); o += ' '; unum(f.code.size()); o += '\n';
    for (const Ins& in : f.code) {
      num(in.op); const OpInfo& oi = kOpInfo[in.op];
      for (int j = 0; j < oi.nops; j++) { o += ' '; num(j == 0 ? in.a : j == 1 ? in.b : in.c); }
      o += '\n';
    }
  }
  if (p.has_names) for (const Func& f : p.funcs) { unum(f.name.size()); for (unsigned char ch : f.name) { o += ' '; unum(ch); } o += '\n'; }
  return o;
}

Program liste_to_program(const std::string& text) {
  std::istringstream is(text); Program p; std::string t;
  auto next = [&]() -> const std::string& { if (!(is >> t)) throw PmError("liste: beklenmedik sonu"); return t; };
  auto u = [&]() { const std::string& s = next(); char* e; errno = 0; unsigned long long v = strtoull(s.c_str(), &e, 10); if (*e || errno || s[0] == '-') throw PmError("liste: gecersiz isaretsiz sayi: " + s); return v; };
  auto sg = [&]() { const std::string& s = next(); char* e; errno = 0; long long v = strtoll(s.c_str(), &e, 10); if (*e || errno) throw PmError("liste: gecersiz sayi: " + s); return v; };
  p.has_names = u() != 0; p.mem_words = u(); p.entry = uint32_t(u());
  for (unsigned long long n = u(), i = 0; i < n; i++) p.consts.push_back(u());
  for (unsigned long long n = u(), i = 0; i < n; i++) p.data.push_back(sg());
  p.bss = u();
  unsigned long long nf = u(); p.funcs.resize(nf);
  for (Func& f : p.funcs) {
    f.nparams = uint32_t(u()); f.nregs = uint32_t(u()); unsigned long long nc = u(); f.code.resize(nc);
    for (Ins& in : f.code) {
      unsigned long long op = u(); if (op >= OP_COUNT) throw PmError("liste: bilinmeyen komut kodu");
      in.op = uint8_t(op); in.a = in.b = in.c = 0; const OpInfo& oi = kOpInfo[op];
      for (int j = 0; j < oi.nops; j++) { int32_t v = int32_t(sg()); (j == 0 ? in.a : j == 1 ? in.b : in.c) = v; }
    }
  }
  if (p.has_names) for (Func& f : p.funcs) { for (unsigned long long n = u(), i = 0; i < n; i++) f.name.push_back(char(u())); }
  if (is >> t) throw PmError("liste: sonunda fazla simge");
  return p;
}

// ---- Dogrulayici referansi (pm listedogrula): liste bicimini KATI cozer (parse_program'in sinir denetimleriyle ayni), sonra
// validate_program + analyze_program. Cikti: "1 Z\n" (Z = zero_free) ya da "0\n". Parametre tabanli oz/dogr/valid.pmb ile karsilastirilir.
// Simge: [-]rakam+ (bosluk: 9..13, 32). Isaretsiz alan: '-' yok, <= 2^64-1; isaretli alan: [-2^63, 2^63-1] ("-0" gecerli).
std::string liste_dogrula(const std::string& text) {
  try {
    size_t pos = 0; const size_t n = text.size();
    auto isws = [](unsigned char c) { return c == 32 || (c >= 9 && c <= 13); };
    auto tok = [&](bool& neg, unsigned __int128& mag) {   // sonraki simge; yoksa/gecersizse atar
      while (pos < n && isws(text[pos])) pos++;
      if (pos >= n) throw PmError("liste: beklenmedik sonu");
      neg = false; mag = 0; size_t nd = 0;
      if (text[pos] == '-') { neg = true; pos++; }
      for (; pos < n && !isws(text[pos]); pos++) {
        if (text[pos] < '0' || text[pos] > '9') throw PmError("liste: gecersiz simge");
        mag = mag * 10 + (text[pos] - '0'); nd++;
        if (mag > ((unsigned __int128)1 << 64)) throw PmError("liste: sayi cok buyuk");
      }
      if (nd == 0) throw PmError("liste: gecersiz simge");
    };
    auto U = [&](uint64_t lim) -> uint64_t {   // isaretsiz, 0..lim
      bool ng; unsigned __int128 m; tok(ng, m);
      if (ng || m > lim) throw PmError("liste: isaretsiz alan sinir disi");
      return uint64_t(m);
    };
    auto C = [&]() -> uint64_t {   // sabit: isaretsiz 64 bit
      bool ng; unsigned __int128 m; tok(ng, m);
      if (ng || m > UINT64_MAX) throw PmError("liste: sabit sinir disi");
      return uint64_t(m);
    };
    auto S = [&]() -> int64_t {   // isaretli 64 bit
      bool ng; unsigned __int128 m; tok(ng, m);
      if (ng) { if (m > ((unsigned __int128)1 << 63)) throw PmError("liste: isaretli alan sinir disi"); return int64_t(0 - uint64_t(m)); }
      if (m > uint64_t(INT64_MAX)) throw PmError("liste: isaretli alan sinir disi");
      return int64_t(m);
    };
    // Dogrulayicinin sabit sinirlari (524288 simge) bu referansta da uygulanir: simge sayisini bastan say.
    { size_t cnt = 0, q = 0; while (true) { while (q < n && isws(text[q])) q++; if (q >= n) break; cnt++; while (q < n && !isws(text[q])) q++; } if (cnt > 524288) throw PmError("liste: simge sayisi sinirda"); }
    Program p;
    p.has_names = U(1) != 0;
    p.mem_words = U(Limits::kMaxMemWords); if (p.mem_words == 0) throw PmError("liste: bellek 0");
    p.entry = uint32_t(U(Limits::kMaxFuncs - 1));
    for (uint64_t c = U(524288), i = 0; i < c; i++) p.consts.push_back(C());
    for (uint64_t c = U(524288), i = 0; i < c; i++) p.data.push_back(S());
    p.bss = U(Limits::kMaxMemWords); if (p.bss > p.mem_words) throw PmError("liste: bss bellekten buyuk");
    uint64_t nf = U(Limits::kMaxFuncs); p.funcs.resize(nf);
    for (Func& f : p.funcs) {
      uint64_t np = U(Limits::kMaxRegs), nr = U(Limits::kMaxRegs), nc = U(Limits::kMaxCode);
      if (np > nr) throw PmError("liste: np > nr");
      f.nparams = uint32_t(np); f.nregs = uint32_t(nr);
      for (uint64_t i = 0; i < nc; i++) {
        Ins in; in.a = in.b = in.c = 0;
        uint64_t op = U(OP_COUNT - 1); in.op = uint8_t(op); const OpInfo& oi = kOpInfo[op];
        for (int j = 0; j < oi.nops; j++) {
          int64_t v = S(); if (v < INT32_MIN || v > INT32_MAX) throw PmError("liste: operand sinir disi");
          (j == 0 ? in.a : j == 1 ? in.b : in.c) = int32_t(v);
        }
        f.code.push_back(in);
      }
    }
    if (p.has_names) for (Func& f : p.funcs) { for (uint64_t len = U(256), i = 0; i < len; i++) f.name.push_back(char(U(255))); }
    while (pos < n && isws(text[pos])) pos++;
    if (pos < n) throw PmError("liste: sonunda fazla simge");
    validate_program(p);
    analyze_program(p);
    return std::string("1 ") + (p.zero_free ? "1" : "0") + "\n";
  } catch (PmError&) { return "0\n"; }
}
