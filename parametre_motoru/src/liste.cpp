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
