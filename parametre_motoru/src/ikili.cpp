// Ikili kodlama (PMB1) + dogrulama. Kodlama: LEB128 tamsayilar, zigzag isaretli sayilar, hedefler goreli.
#include <algorithm>
#include <cstring>
#include "program.h"

const OpInfo kOpInfo[OP_COUNT] = {
#define X(E, N, C, K) {N, C, K},
  PM_OPS(X)
#undef X
};
const HostInfo kHostInfo[H_COUNT] = {{"putc", 1}, {"puti", 1}, {"putf", 1}, {"getc", 0}};

namespace {

void put_var(std::vector<uint8_t>& o, uint64_t v) {
  while (v >= 0x80) { o.push_back(uint8_t(v) | 0x80); v >>= 7; }
  o.push_back(uint8_t(v));
}
uint64_t zig(int64_t v) { return (uint64_t(v) << 1) ^ uint64_t(v >> 63); }
int64_t unzig(uint64_t v) { return int64_t(v >> 1) ^ -int64_t(v & 1); }

struct Reader {
  const uint8_t* p; size_t n, i = 0;
  uint8_t byte() { if (i >= n) throw PmError("ikili: beklenmedik dosya sonu"); return p[i++]; }
  uint64_t var() {
    uint64_t v = 0; int sh = 0;
    for (;;) {
      uint8_t b = byte();
      if (sh >= 63 && (b & 0x7f) > 1) throw PmError("ikili: sayi tasti");
      v |= uint64_t(b & 0x7f) << sh;
      if (!(b & 0x80)) return v;
      sh += 7;
      if (sh > 63) throw PmError("ikili: sayi cok uzun");
    }
  }
};

bool is_reg_kind(char k) { return k == 'd' || k == 'r' || k == 'f' || k == 's' || k == 'b'; }
int32_t& field(Ins& in, int j) { return j == 0 ? in.a : (j == 1 ? in.b : in.c); }
int32_t field(const Ins& in, int j) { return j == 0 ? in.a : (j == 1 ? in.b : in.c); }

}  // namespace

std::vector<uint8_t> encode_program(const Program& p, bool strip_names) {
  std::vector<uint8_t> o = {'P', 'M', 'B', '1'};
  bool names = p.has_names && !strip_names;
  o.push_back(names ? 1 : 0);
  put_var(o, p.mem_words);
  put_var(o, p.entry);
  put_var(o, p.consts.size());
  for (uint64_t c : p.consts) put_var(o, c);
  put_var(o, p.data.size());
  for (int64_t d : p.data) put_var(o, zig(d));
  put_var(o, p.bss);
  put_var(o, p.funcs.size());
  for (const Func& f : p.funcs) {
    put_var(o, f.nparams); put_var(o, f.nregs); put_var(o, f.code.size());
    for (size_t pc = 0; pc < f.code.size(); pc++) {
      const Ins& in = f.code[pc];
      o.push_back(in.op);
      const OpInfo& oi = kOpInfo[in.op];
      for (int j = 0; j < oi.nops; j++) {
        char k = oi.kinds[j]; int32_t v = field(in, j);
        if (k == 'l') put_var(o, zig(int64_t(v) - int64_t(pc)));
        else if (k == 'i') put_var(o, zig(v));
        else put_var(o, uint64_t(uint32_t(v)));
      }
    }
  }
  if (names)
    for (const Func& f : p.funcs) { put_var(o, f.name.size()); o.insert(o.end(), f.name.begin(), f.name.end()); }
  return o;
}

Program decode_program(const uint8_t* data, size_t n) {
  Reader r{data, n};
  if (n < 5 || memcmp(data, "PMB1", 4) != 0) throw PmError("ikili: gecersiz baslik (PMB1 bekleniyor)");
  r.i = 4;
  Program p;
  uint8_t flags = r.byte();
  if (flags > 1) throw PmError("ikili: bilinmeyen bayrak");
  p.has_names = flags & 1;
  p.mem_words = r.var();
  if (p.mem_words == 0 || p.mem_words > Limits::kMaxMemWords) throw PmError("ikili: bellek istegi gecersiz/sinir disi");
  uint64_t e = r.var(); if (e >= Limits::kMaxFuncs) throw PmError("ikili: giris fonksiyonu gecersiz");
  p.entry = uint32_t(e);
  uint64_t nc = r.var(); if (nc > n) throw PmError("ikili: sabit sayisi dosyadan buyuk");
  for (uint64_t i = 0; i < nc; i++) p.consts.push_back(r.var());
  uint64_t nd = r.var(); if (nd > n) throw PmError("ikili: veri sayisi dosyadan buyuk");
  for (uint64_t i = 0; i < nd; i++) p.data.push_back(unzig(r.var()));
  p.bss = r.var(); if (p.bss > p.mem_words) throw PmError("ikili: bss bellekten buyuk");
  uint64_t nf = r.var(); if (nf > Limits::kMaxFuncs || nf > n) throw PmError("ikili: fonksiyon sayisi gecersiz");
  p.funcs.resize(nf);
  for (Func& f : p.funcs) {
    uint64_t np = r.var(), nr = r.var(), ncode = r.var();
    if (nr > Limits::kMaxRegs || np > nr) throw PmError("ikili: yazmac sayisi gecersiz");
    if (ncode > Limits::kMaxCode || ncode > n) throw PmError("ikili: komut sayisi gecersiz");
    f.nparams = uint32_t(np); f.nregs = uint32_t(nr);
    f.code.resize(ncode);
    for (size_t pc = 0; pc < ncode; pc++) {
      Ins& in = f.code[pc];
      uint8_t op = r.byte();
      if (op >= OP_COUNT) throw PmError("ikili: bilinmeyen komut kodu");
      in.op = op; in.a = in.b = in.c = 0;
      const OpInfo& oi = kOpInfo[op];
      for (int j = 0; j < oi.nops; j++) {
        char k = oi.kinds[j]; int64_t v;
        if (k == 'l') v = int64_t(pc) + unzig(r.var());
        else if (k == 'i') v = unzig(r.var());
        else v = int64_t(r.var());
        if (v < INT32_MIN || v > INT32_MAX) throw PmError("ikili: operand sinir disi");
        field(in, j) = int32_t(v);
      }
    }
  }
  if (p.has_names)
    for (Func& f : p.funcs) {
      uint64_t len = r.var(); if (len > 256) throw PmError("ikili: ad cok uzun");
      for (uint64_t i = 0; i < len; i++) f.name.push_back(char(r.byte()));
    }
  if (r.i != n) throw PmError("ikili: dosya sonunda fazla bayt");
  validate_program(p);
  return p;
}

// Dogrulama: gecen her program VM'i cokertemez; kalan hatalar tuzak (trap) olarak bildirilir.
void validate_program(const Program& p) {
  if (p.entry >= p.funcs.size()) throw PmError("dogrulama: giris fonksiyonu yok");
  if (p.funcs[p.entry].nparams != 0) throw PmError("dogrulama: giris fonksiyonu parametre almamali");
  if (p.mem_words < 1 + p.data.size() + p.bss) throw PmError("dogrulama: veri bellege sigmiyor");
  for (size_t fi = 0; fi < p.funcs.size(); fi++) {
    const Func& f = p.funcs[fi];
    auto bad = [&](size_t pc, const char* m) {
      throw PmError("dogrulama: fonksiyon " + std::to_string(fi) + " komut " + std::to_string(pc) + ": " + m);
    };
    if (f.code.empty()) bad(0, "bos fonksiyon");
    uint8_t last = f.code.back().op;
    if (last != OP_JMP && last != OP_RET && last != OP_HALT) bad(f.code.size() - 1, "son komut jmp/ret/halt olmali");
    for (size_t pc = 0; pc < f.code.size(); pc++) {
      const Ins& in = f.code[pc]; const OpInfo& oi = kOpInfo[in.op];
      for (int j = 0; j < oi.nops; j++) {
        char k = oi.kinds[j]; int32_t v = field(in, j);
        if (is_reg_kind(k)) { if (v < 0 || uint32_t(v) >= f.nregs) bad(pc, "yazmac sinir disi"); }
        else if (k == 'l') { if (v < 0 || size_t(v) >= f.code.size()) bad(pc, "atlama hedefi sinir disi"); }
        else if (k == 'F') { if (v < 0 || size_t(v) >= p.funcs.size()) bad(pc, "fonksiyon indeksi gecersiz"); }
        else if (k == 'k') { if (v < 0 || size_t(v) >= p.consts.size()) bad(pc, "sabit indeksi gecersiz"); }
        else if (k == 'h') { if (v < 0 || v >= H_COUNT) bad(pc, "bilinmeyen host islevi"); }
      }
      if (in.op == OP_CALL) {
        uint32_t need = std::max<uint32_t>(1, p.funcs[in.a].nparams);
        if (uint64_t(in.b) + need > f.nregs) bad(pc, "cagri penceresi yazmaclara sigmiyor");
      } else if (in.op == OP_HOST) {
        uint32_t need = std::max(1, kHostInfo[in.a].arity);
        if (uint64_t(in.b) + need > f.nregs) bad(pc, "host penceresi yazmaclara sigmiyor");
      }
    }
  }
}
