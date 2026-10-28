// Yapisal sayisal parametre tablosu (PMY1) -> yazma bicimi (.pma).
// Girdi: yalnizca sayilar. Metin yok: ad, etiket, atlama ofseti, yazmac numarasi yok. Yapi (nesne sablonlari, davranislar,
// islevler, iliskiler, olaylar, donguler, kosullar) onek bicimli, sayi-sayacli kayitlarla anlatilir.
// Bu bir YORUMLAYICI degil bir OZELLESTIRICI: tablo yuklenirken bir kez .pma'ya, oradan ayni ikili tabloya ve makine koduna cevrilir.
// Uygulamaya ozel hicbir sey bilmez; bicim belgesi YAPISAL.md.
#include <algorithm>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include "program.h"

namespace {

// ---- dugum turleri (sayisal kimlikler; belge ile ayni)
enum EK { E_SAYI = 1, E_ALAN, E_GLOBAL, E_YEREL, E_INDIS, E_KARE, E_AALAN, E_BALAN, E_AINDIS, E_BINDIS, E_TALAN, E_DIZI, E_CAGRI, E_PARAM, E_IKILI, E_TEKLI, E_SEC, E_SON };
enum SK { S_ATA = 1, S_EGER, S_SURDE, S_SAY, S_CAGIR, S_DAVRAN, S_DON, S_YAZI, S_KARAKTER, S_KIR, S_DEVAM, S_SATIR, S_SON };
const char* kEN[] = {"?", "sayi", "alan", "global", "yerel", "indis", "kare", "aalan", "balan", "aindis", "bindis", "talan", "dizi", "cagri", "param", "ikili", "tekli", "sec"};
const char* kSN[] = {"?", "ata", "eger", "surde", "say", "cagir", "davran", "don", "yazi", "karakter", "kir", "devam", "satir"};
const char* kBinS[] = {"?", "+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>", "==", "!=", "<", "<=", ">", ">=", "min", "max", "ve", "veya"};
const char* kBinAsm[] = {"?", "add", "sub", "mul", "div", "mod", "and", "or", "xor", "shl", "sar", "eq", "ne", "lt", "le", "gt", "ge"};
const int kBinMax = 20;
const char* kUnS[] = {"?", "eksi", "bitdegil", "degil"};
enum FG { G_SABIT = 1, G_DOGRUSAL, G_MOD, G_BOL, G_RASTGELE };

struct Expr; typedef std::shared_ptr<Expr> ExprP;
struct Expr { int k = 0; long long v = 0; int a = 0, b = 0; std::vector<ExprP> c; };
struct Stmt; typedef std::shared_ptr<Stmt> StmtP; typedef std::vector<StmtP> Block;
struct Stmt { int k = 0; int a = 0; std::vector<ExprP> e; std::vector<Block> blk; };

struct Fld { int kind = 0; long long p[3] = {0, 0, 0}; bool written = false; };
struct Draw { int shape = 0; std::vector<ExprP> a; };
struct Tm { long long n = 0; std::vector<Fld> f; Block rules; std::vector<Draw> draws; bool stored(const Fld& x) const { return x.written || x.kind == G_RASTGELE; } };
struct Arr { long long size = 0; std::vector<long long> init; };
struct Fn { int np = 0, nl = 0; Block body; };
struct Bh { int np = 0; Block body; };
struct Et { int a = 0, b = 0; ExprP cond; Block body; };
struct Gi { long long code = 0; int t = -1; Block body; };
struct Ag { int g = 0, op = 0, t = 0; ExprP val, cond; };

struct Model {
  long long W = 0, H = 0, bg = 0, frames = 0;
  std::vector<long long> globals; std::vector<Arr> arrs; std::vector<Fn> fns; std::vector<Bh> bhs; std::vector<Tm> tms;
  std::vector<Et> ets; std::vector<Gi> gis; std::vector<Ag> ags;
  bool hasBitis = false; ExprP bitis; Block ilk, son;
};

[[noreturn]] void bad(const std::string& m) { throw PmError("tablo: " + m); }

// ---- okuyucu
struct Rd {
  const uint8_t* p; size_t n, i = 0, cnt = 0;
  [[noreturn]] void fail(const std::string& m) const { bad("sayi #" + std::to_string(cnt + 1) + " (bayt " + std::to_string(i) + "): " + m); }
  unsigned long long u() {
    unsigned long long v = 0; int sh = 0;
    for (;;) {
      if (i >= n) fail("beklenmedik dosya sonu");
      uint8_t b = p[i++];
      if (sh >= 63 && (b & 0x7f) > 1) fail("sayi tasti");
      v |= (unsigned long long)(b & 0x7f) << sh;
      if (!(b & 0x80)) { cnt++; return v; }
      sh += 7; if (sh > 63) fail("sayi cok uzun");
    }
  }
  long long z() { unsigned long long v = u(); return (long long)(v >> 1) ^ -(long long)(v & 1); }
  size_t count(const char* what) { unsigned long long v = u(); if (v > n - i) { cnt--; fail(std::string(what) + " sayisi dosyadan buyuk: " + std::to_string(v)); } return size_t(v); }
  int idx(const char* what, size_t limit) { unsigned long long v = u(); if (v >= limit) { cnt--; fail(std::string(what) + " gecersiz: " + std::to_string(v)); } return int(v); }
};

struct Dec {
  Rd r; Model m;
  ExprP expr(int depth) {
    if (depth > 200) r.fail("ifade cok derin");
    auto e = std::make_shared<Expr>();
    unsigned long long k = r.u();
    if (k < 1 || k >= E_SON) { r.cnt--; r.fail("bilinmeyen ifade kodu " + std::to_string(k) + " (1.." + std::to_string(E_SON - 1) + ")"); }
    e->k = int(k);
    switch (e->k) {
      case E_SAYI: e->v = r.z(); break;
      case E_ALAN: case E_GLOBAL: case E_YEREL: case E_AALAN: case E_BALAN: case E_PARAM: e->a = int(r.idx("indeks", 1u << 20)); break;
      case E_INDIS: case E_KARE: case E_AINDIS: case E_BINDIS: break;
      case E_TALAN: e->a = int(r.idx("sablon", 1u << 20)); e->b = int(r.idx("alan", 1u << 20)); e->c.push_back(expr(depth + 1)); break;
      case E_DIZI: e->a = int(r.idx("dizi", 1u << 20)); e->c.push_back(expr(depth + 1)); break;
      case E_CAGRI: { e->a = int(r.idx("islev", 1u << 20)); size_t n = r.count("arguman"); for (size_t j = 0; j < n; j++) e->c.push_back(expr(depth + 1)); break; }
      case E_IKILI: { unsigned long long o = r.u(); if (o < 1 || o > (unsigned long long)kBinMax) { r.cnt--; r.fail("ikili islem kodu 1.." + std::to_string(kBinMax)); } e->a = int(o); e->c.push_back(expr(depth + 1)); e->c.push_back(expr(depth + 1)); break; }
      case E_TEKLI: { unsigned long long o = r.u(); if (o < 1 || o > 3) { r.cnt--; r.fail("tekli islem kodu 1..3"); } e->a = int(o); e->c.push_back(expr(depth + 1)); break; }
      case E_SEC: for (int j = 0; j < 3; j++) e->c.push_back(expr(depth + 1)); break;
    }
    return e;
  }
  Block block(int depth) {
    if (depth > 100) r.fail("blok cok derin");
    Block b; size_t n = r.count("komut");
    for (size_t j = 0; j < n; j++) {
      auto s = std::make_shared<Stmt>();
      unsigned long long k = r.u();
      if (k < 1 || k >= S_SON) { r.cnt--; r.fail("bilinmeyen komut kodu " + std::to_string(k) + " (1.." + std::to_string(S_SON - 1) + ")"); }
      s->k = int(k);
      switch (s->k) {
        case S_ATA: s->e.push_back(expr(0)); s->e.push_back(expr(0)); break;
        case S_EGER: s->e.push_back(expr(0)); s->blk.push_back(block(depth + 1)); s->blk.push_back(block(depth + 1)); break;
        case S_SURDE: s->e.push_back(expr(0)); s->blk.push_back(block(depth + 1)); break;
        case S_SAY: s->a = int(r.idx("yerel", 256)); s->e.push_back(expr(0)); s->e.push_back(expr(0)); s->blk.push_back(block(depth + 1)); break;
        case S_CAGIR: case S_DAVRAN: { s->a = int(r.idx(s->k == S_CAGIR ? "islev" : "davranis", 1u << 20)); size_t n2 = r.count("arguman"); for (size_t q = 0; q < n2; q++) s->e.push_back(expr(0)); break; }
        case S_DON: case S_YAZI: case S_KARAKTER: s->e.push_back(expr(0)); break;
        case S_KIR: case S_DEVAM: break;
        case S_SATIR: { size_t n2 = r.count("oge"); for (size_t q = 0; q < n2; q++) s->e.push_back(expr(0)); break; }
      }
      b.push_back(s);
    }
    return b;
  }
  void run(const uint8_t* p, size_t n) {
    r.p = p; r.n = n;
    if (n < 4 || memcmp(p, "PMY1", 4) != 0) bad("gecersiz baslik (PMY1 = 80 77 89 49 bekleniyor)");
    r.i = 4;
    m.W = (long long)r.u(); m.H = (long long)r.u(); m.bg = (long long)r.u(); m.frames = (long long)r.u();
    if (m.W < 0 || m.H < 0 || m.W > 4096 || m.H > 4096 || ((m.W == 0) != (m.H == 0))) bad("sahne boyutu gecersiz (ikisi de 0 ya da 1..4096)");
    size_t n1 = r.count("global"); for (size_t j = 0; j < n1; j++) m.globals.push_back(r.z());
    n1 = r.count("dizi");
    for (size_t j = 0; j < n1; j++) {
      Arr a; a.size = (long long)r.u(); if (a.size < 1 || a.size > (1 << 24)) bad("dizi " + std::to_string(j) + ": boyut 1..16777216");
      size_t ni = r.count("baslangic degeri"); if ((long long)ni > a.size) bad("dizi " + std::to_string(j) + ": baslangic degeri sayisi boyuttan buyuk");
      for (size_t q = 0; q < ni; q++) a.init.push_back(r.z());
      m.arrs.push_back(a);
    }
    n1 = r.count("islev");
    for (size_t j = 0; j < n1; j++) {
      Fn f; f.np = int(r.idx("parametre", 256)); f.nl = int(r.idx("yerel", 257)); if (f.nl < f.np) bad("islev " + std::to_string(j) + ": yerel sayisi parametre sayisindan kucuk");
      f.body = block(0); m.fns.push_back(f);
    }
    n1 = r.count("davranis");
    for (size_t j = 0; j < n1; j++) { Bh b; b.np = int(r.idx("parametre", 256)); b.body = block(0); m.bhs.push_back(b); }
    n1 = r.count("sablon");
    for (size_t j = 0; j < n1; j++) {
      Tm t; t.n = (long long)r.u(); if (t.n < 1 || t.n > 10000000) bad("sablon " + std::to_string(j) + ": ornek sayisi 1..10000000");
      size_t nf = r.count("alan");
      for (size_t q = 0; q < nf; q++) {
        Fld f; unsigned long long g = r.u();
        if (g < 1 || g > G_RASTGELE) { r.cnt--; r.fail("alan baslatici kodu 1..5"); }
        f.kind = int(g);
        if (g == G_SABIT) f.p[0] = r.z();
        else if (g == G_DOGRUSAL) { f.p[0] = r.z(); f.p[1] = r.z(); }
        else if (g == G_MOD || g == G_BOL) { f.p[0] = (long long)r.u(); f.p[1] = r.z(); f.p[2] = r.z(); if (f.p[0] < 1) bad("mod/bol: ilk sayi >= 1 olmali"); }
        else { f.p[0] = r.z(); f.p[1] = r.z(); f.p[2] = r.z(); if (f.p[1] < f.p[0]) bad("rastgele: lo <= hi olmali"); }
        t.f.push_back(f);
      }
      t.rules = block(0);
      size_t nd = r.count("cizim");
      for (size_t q = 0; q < nd; q++) {
        Draw d; unsigned long long sh = r.u(); if (sh < 1 || sh > 2) { r.cnt--; r.fail("cizim sekli 1 (dikdortgen) ya da 2 (nokta)"); }
        d.shape = int(sh); int want = sh == 1 ? 5 : 3; for (int w = 0; w < want; w++) d.a.push_back(expr(0));
        t.draws.push_back(d);
      }
      m.tms.push_back(t);
    }
    n1 = r.count("etkilesim");
    for (size_t j = 0; j < n1; j++) { Et e; e.a = int(r.idx("sablon", 1u << 20)); e.b = int(r.idx("sablon", 1u << 20)); e.cond = expr(0); e.body = block(0); m.ets.push_back(e); }
    n1 = r.count("girdi");
    for (size_t j = 0; j < n1; j++) { Gi g; g.code = (long long)r.u(); g.t = int(r.idx("sablon+1", 1u << 20)) - 1; g.body = block(0); m.gis.push_back(g); }
    n1 = r.count("toplama");
    for (size_t j = 0; j < n1; j++) {
      Ag a; a.g = int(r.idx("global", 1u << 20)); unsigned long long op = r.u(); if (op < 1 || op > 4) { r.cnt--; r.fail("toplama islemi 1..4 (toplam, say, enbuyuk, enkucuk)"); }
      a.op = int(op); a.t = int(r.idx("sablon", 1u << 20)); a.val = expr(0); a.cond = expr(0); m.ags.push_back(a);
    }
    unsigned long long hb = r.u(); if (hb > 1) { r.cnt--; r.fail("bitis var-bayragi 0 ya da 1"); }
    if (hb) { m.hasBitis = true; m.bitis = expr(0); }
    m.ilk = block(0); m.son = block(0);
    if (r.i != n) bad("dosya sonunda fazla bayt (" + std::to_string(n - r.i) + ")");
  }
};

// ---- okunabilir dokum
void pexpr(const ExprP& e, std::ostream& o) {
  o << "(" << kEN[e->k];
  switch (e->k) {
    case E_SAYI: o << " " << e->v; break;
    case E_ALAN: case E_GLOBAL: case E_YEREL: case E_AALAN: case E_BALAN: case E_PARAM: o << " " << e->a; break;
    case E_TALAN: o << " " << e->a << " " << e->b; break;
    case E_DIZI: case E_CAGRI: o << " " << e->a; break;
    case E_IKILI: o << " " << kBinS[e->a]; break;
    case E_TEKLI: o << " " << kUnS[e->a]; break;
  }
  for (auto& c : e->c) { o << " "; pexpr(c, o); }
  o << ")";
}
void pblock(const Block& b, std::ostream& o, int ind);
void pstmt(const StmtP& s, std::ostream& o, int ind) {
  std::string pad(size_t(ind) * 2, ' ');
  o << pad << "(" << kSN[s->k];
  if (s->k == S_SAY || s->k == S_CAGIR || s->k == S_DAVRAN) o << " " << s->a;
  for (auto& e : s->e) { o << " "; pexpr(e, o); }
  if (s->blk.empty()) { o << ")\n"; return; }
  o << "\n";
  for (size_t j = 0; j < s->blk.size(); j++) { if (s->k == S_EGER) o << pad << (j == 0 ? "  se:\n" : "  yoksa:\n"); pblock(s->blk[j], o, ind + 2); }
  o << pad << ")\n";
}
void pblock(const Block& b, std::ostream& o, int ind) { for (auto& s : b) pstmt(s, o, ind); }

// ---- baglam
struct Ctx {
  int t = -1, a = -1, b = -1; bool fn = false; int nloc = 16;
  std::vector<std::pair<std::string, std::string>> loops;   // (kir etiketi, devam etiketi)
};

// ---- davranis acilimi (makro): PARAM -> cagri argumani; tablodaki DAVRAN komutlari yerinde acilir
ExprP clone_subst(const ExprP& e, const std::vector<ExprP>* bind) {
  if (e->k == E_PARAM) {
    if (!bind || size_t(e->a) >= bind->size()) bad("param " + std::to_string(e->a) + " davranis disinda ya da arguman sayisini asiyor");
    return clone_subst((*bind)[size_t(e->a)], nullptr);
  }
  auto n = std::make_shared<Expr>(*e);
  for (auto& c : n->c) c = clone_subst(c, bind);
  return n;
}
void expand_block(const Block& in, Block& out, const std::vector<ExprP>* bind, const Model& m, int depth) {
  if (depth > 16) bad("davranis ic ice cagrisi cok derin (ozyinelemeli davranis olamaz)");
  for (const StmtP& s : in) {
    if (s->k == S_DAVRAN) {
      if (size_t(s->a) >= m.bhs.size()) bad("davranis " + std::to_string(s->a) + " yok");
      const Bh& bh = m.bhs[size_t(s->a)];
      if (s->e.size() != size_t(bh.np)) bad("davranis " + std::to_string(s->a) + " " + std::to_string(bh.np) + " arguman ister, " + std::to_string(s->e.size()) + " verildi");
      std::vector<ExprP> args; for (auto& a : s->e) args.push_back(clone_subst(a, bind));
      expand_block(bh.body, out, &args, m, depth + 1);
      continue;
    }
    auto n = std::make_shared<Stmt>(*s);
    for (auto& e : n->e) e = clone_subst(e, bind);
    for (auto& b : n->blk) { Block nb; expand_block(b, nb, bind, m, depth); b = nb; }
    out.push_back(n);
  }
}

struct Gen {
  Model m;
  std::vector<std::string> out; int ntmp = 0, nlab = 0;
  std::string where;
  std::string badLabel; bool needBad = false; std::set<int> usedLoc;
  std::string tmp() { return "v" + std::to_string(ntmp++); }
  std::string lab() { return "L" + std::to_string(nlab++); }
  void emit(const std::string& s) { out.push_back("    " + s); }
  void label(const std::string& l) { out.push_back(l + ":"); }
  static std::string S(long long x) { return std::to_string(x); }
  [[noreturn]] void err(const std::string& msg) { bad(where + ": " + msg); }
  static bool is_lit(const std::string& s) { return !s.empty() && (isdigit((unsigned char)s[0]) || s[0] == '-'); }
  std::string sym(int ti, int fi) { return "s_" + S(ti) + "_" + S(fi); }
  bool stored(int ti, int fi) { return m.tms[size_t(ti)].stored(m.tms[size_t(ti)].f[size_t(fi)]); }

  // ---- islev cerceve yardimcilari
  std::vector<std::string> saved; size_t fnStart = 0;
  void begin_fn(const std::string& name, int np) {
    std::string h = "func " + name + "(";
    for (int j = 0; j < np; j++) h += (j ? ", l" : "l") + S(j);
    out.push_back(h + ") {"); fnStart = out.size();
    badLabel = "bad" + S(nlab++); needBad = false; usedLoc.clear();
  }
  void end_fn(int np) {
    emit("ret 0");
    if (needBad) { label(badLabel); emit("mov zz, -1"); emit("ld zz, zz, 0"); emit("ret 0"); }
    std::vector<std::string> pro;
    for (int l : usedLoc) if (l >= np) pro.push_back("    mov l" + S(l) + ", 0");
    out.insert(out.begin() + long(fnStart), pro.begin(), pro.end());
    out.push_back("}");
  }

  // ---- ifade
  std::string field_read(int ti, int fi, const std::string& idx) {
    const Tm& T = m.tms[size_t(ti)]; const Fld& f = T.f[size_t(fi)];
    if (T.stored(f)) { std::string p = tmp(), v = tmp(); emit("add " + p + ", " + sym(ti, fi) + ", " + idx); emit("ld " + v + ", " + p + ", 0"); return v; }
    std::string v;
    switch (f.kind) {
      case G_SABIT: return S(f.p[0]);
      case G_DOGRUSAL: v = tmp(); emit("mul " + v + ", " + idx + ", " + S(f.p[1])); emit("add " + v + ", " + v + ", " + S(f.p[0])); return v;
      case G_MOD: v = tmp(); emit("mod " + v + ", " + idx + ", " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); return v;
      case G_BOL: v = tmp(); emit("div " + v + ", " + idx + ", " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); return v;
      default: err("dahili hata: sanal rastgele alan");
    }
  }
  void need_t(int ti) { if (ti < 0 || size_t(ti) >= m.tms.size()) err("sablon " + S(ti) + " yok"); }
  void need_f(int ti, int fi) { need_t(ti); if (fi < 0 || size_t(fi) >= m.tms[size_t(ti)].f.size()) err("sablon " + S(ti) + " alan " + S(fi) + " yok"); }
  std::string check_idx(const std::string& v, long long size) {
    if (is_lit(v)) { long long x = atoll(v.c_str()); if (x < 0 || x >= size) err("sabit indeks " + v + " sinir disi (0.." + S(size - 1) + ")"); return v; }
    std::string c = tmp(); needBad = true;
    emit("lt " + c + ", " + v + ", 0"); emit("jnz " + c + ", " + badLabel); emit("ge " + c + ", " + v + ", " + S(size)); emit("jnz " + c + ", " + badLabel);
    return v;
  }
  std::string loc(int l, Ctx& c) { if (l >= c.nloc) err("yerel " + S(l) + " sinir disi (0.." + S(c.nloc - 1) + ")"); usedLoc.insert(l); return "l" + S(l); }
  std::string ex(const ExprP& e, Ctx& c) {
    switch (e->k) {
      case E_SAYI: return S(e->v);
      case E_ALAN: if (c.t < 0) err("'alan' yalniz sablon baglaminda"); need_f(c.t, e->a); return field_read(c.t, e->a, "i");
      case E_INDIS: if (c.t < 0) err("'indis' yalniz sablon baglaminda"); return "i";
      case E_AALAN: if (c.a < 0) err("'aalan' yalniz etkilesimde"); need_f(c.a, e->a); return field_read(c.a, e->a, "ia");
      case E_BALAN: if (c.a < 0) err("'balan' yalniz etkilesimde"); need_f(c.b, e->a); return field_read(c.b, e->a, "ib");
      case E_AINDIS: if (c.a < 0) err("'aindis' yalniz etkilesimde"); return "ia";
      case E_BINDIS: if (c.a < 0) err("'bindis' yalniz etkilesimde"); return "ib";
      case E_GLOBAL: { if (size_t(e->a) >= m.globals.size()) err("global " + S(e->a) + " yok"); std::string v = tmp(); emit("ld " + v + ", g_" + S(e->a) + ", 0"); return v; }
      case E_KARE: { std::string v = tmp(); emit("ld " + v + ", g_kare, 0"); return v; }
      case E_YEREL: return loc(e->a, c);
      case E_TALAN: { need_f(e->a, e->b); std::string ix = check_idx(ex(e->c[0], c), m.tms[size_t(e->a)].n); return field_read(e->a, e->b, ix); }
      case E_DIZI: {
        if (size_t(e->a) >= m.arrs.size()) err("dizi " + S(e->a) + " yok");
        std::string ix = check_idx(ex(e->c[0], c), m.arrs[size_t(e->a)].size), p = tmp(), v = tmp();
        emit("add " + p + ", d_" + S(e->a) + ", " + ix); emit("ld " + v + ", " + p + ", 0"); return v;
      }
      case E_CAGRI: {
        if (size_t(e->a) >= m.fns.size()) err("islev " + S(e->a) + " yok");
        if (e->c.size() != size_t(m.fns[size_t(e->a)].np)) err("islev " + S(e->a) + " " + S(m.fns[size_t(e->a)].np) + " arguman ister");
        std::vector<std::string> av; for (auto& a : e->c) av.push_back(ex(a, c));
        std::string r = tmp(), s = "call " + r + ", f_" + S(e->a); for (auto& a : av) s += ", " + a; emit(s); return r;
      }
      case E_PARAM: err("param davranis disinda kullanilamaz");
      case E_TEKLI: {
        std::string a = ex(e->c[0], c), r = tmp();
        if (e->a == 1) emit("neg " + r + ", " + a); else if (e->a == 2) emit("not " + r + ", " + a); else emit("eq " + r + ", " + a + ", 0");
        return r;
      }
      case E_SEC: {
        std::string r = tmp(), le = lab(), ld = lab(), cnd = ex(e->c[0], c);
        emit("jz " + cnd + ", " + le); std::string a = ex(e->c[1], c); emit("mov " + r + ", " + a); emit("jmp " + ld);
        label(le); std::string b = ex(e->c[2], c); emit("mov " + r + ", " + b); label(ld); return r;
      }
      case E_IKILI: {
        std::string a = ex(e->c[0], c), b = ex(e->c[1], c), r = tmp();
        int o = e->a;
        if (o <= 16) emit(std::string(kBinAsm[o]) + " " + r + ", " + a + ", " + b);
        else if (o == 17 || o == 18) { std::string cc = tmp(), l = lab(); emit("mov " + r + ", " + a); emit((o == 17 ? "lt " : "gt ") + cc + ", " + b + ", " + r); emit("jz " + cc + ", " + l); emit("mov " + r + ", " + b); label(l); }
        else { std::string t1 = tmp(), t2 = tmp(); emit("ne " + t1 + ", " + a + ", 0"); emit("ne " + t2 + ", " + b + ", 0"); emit(std::string(o == 19 ? "and " : "or ") + r + ", " + t1 + ", " + t2); }
        return r;
      }
    }
    err("dahili hata: ifade");
  }

  // ---- hedef (atanabilir ifade)
  void store(const ExprP& lv, const std::string& val, Ctx& c) {
    switch (lv->k) {
      case E_ALAN: if (c.t < 0) err("'alan' yalniz sablon baglaminda"); need_f(c.t, lv->a); { std::string p = tmp(); emit("add " + p + ", " + sym(c.t, lv->a) + ", i"); emit("st " + val + ", " + p + ", 0"); } return;
      case E_AALAN: if (c.a < 0) err("'aalan' yalniz etkilesimde"); need_f(c.a, lv->a); { std::string p = tmp(); emit("add " + p + ", " + sym(c.a, lv->a) + ", ia"); emit("st " + val + ", " + p + ", 0"); } return;
      case E_BALAN: if (c.a < 0) err("'balan' yalniz etkilesimde"); need_f(c.b, lv->a); { std::string p = tmp(); emit("add " + p + ", " + sym(c.b, lv->a) + ", ib"); emit("st " + val + ", " + p + ", 0"); } return;
      case E_GLOBAL: if (size_t(lv->a) >= m.globals.size()) err("global " + S(lv->a) + " yok"); emit("st " + val + ", g_" + S(lv->a) + ", 0"); return;
      case E_YEREL: emit("mov " + loc(lv->a, c) + ", " + val); return;
      case E_DIZI: {
        if (size_t(lv->a) >= m.arrs.size()) err("dizi " + S(lv->a) + " yok");
        std::string ix = check_idx(ex(lv->c[0], c), m.arrs[size_t(lv->a)].size), p = tmp(); emit("add " + p + ", d_" + S(lv->a) + ", " + ix); emit("st " + val + ", " + p + ", 0"); return;
      }
      case E_TALAN: {
        need_f(lv->a, lv->b); std::string ix = check_idx(ex(lv->c[0], c), m.tms[size_t(lv->a)].n), p = tmp();
        emit("add " + p + ", " + sym(lv->a, lv->b) + ", " + ix); emit("st " + val + ", " + p + ", 0"); return;
      }
      default: err(std::string("atamanin hedefi atanabilir olmali (alan, aalan, balan, global, yerel, dizi, talan); bulunan: ") + kEN[lv->k]);
    }
  }

  // ---- komutlar
  void block(const Block& b, Ctx& c) { for (auto& s : b) stmt(s, c); }
  void stmt(const StmtP& s, Ctx& c) {
    ntmp = 0;
    switch (s->k) {
      case S_ATA: { std::string v = ex(s->e[1], c); store(s->e[0], v, c); break; }
      case S_EGER: {
        std::string cnd = ex(s->e[0], c), le = lab(), ld = lab();
        emit("jz " + cnd + ", " + le); block(s->blk[0], c);
        if (!s->blk[1].empty()) { emit("jmp " + ld); label(le); block(s->blk[1], c); label(ld); } else label(le);
        break;
      }
      case S_SURDE: {
        std::string lt = lab(), le = lab(); label(lt); ntmp = 0; std::string cnd = ex(s->e[0], c); emit("jz " + cnd + ", " + le);
        c.loops.push_back({le, lt}); block(s->blk[0], c); c.loops.pop_back(); emit("jmp " + lt); label(le); break;
      }
      case S_SAY: {
        std::string l = loc(s->a, c), end = "e" + S(nlab), lt = lab(), le = lab(), ln = lab();
        std::string f = ex(s->e[0], c); emit("mov " + l + ", " + f); std::string t = ex(s->e[1], c); emit("mov " + end + ", " + t);
        label(lt); std::string cc = tmp(); emit("le " + cc + ", " + l + ", " + end); emit("jz " + cc + ", " + le);
        c.loops.push_back({le, ln}); block(s->blk[0], c); c.loops.pop_back();
        label(ln); emit("add " + l + ", " + l + ", 1"); emit("jmp " + lt); label(le); break;
      }
      case S_CAGIR: {
        if (size_t(s->a) >= m.fns.size()) err("islev " + S(s->a) + " yok");
        if (s->e.size() != size_t(m.fns[size_t(s->a)].np)) err("islev " + S(s->a) + " " + S(m.fns[size_t(s->a)].np) + " arguman ister");
        std::vector<std::string> av; for (auto& a : s->e) av.push_back(ex(a, c));
        std::string z = tmp(), cs = "call " + z + ", f_" + S(s->a); for (auto& a : av) cs += ", " + a; emit(cs); break;
      }
      case S_DON: if (!c.fn) err("'don' yalniz islev icinde"); { std::string v = ex(s->e[0], c); emit("ret " + v); } break;
      case S_YAZI: { std::string v = ex(s->e[0], c); emit("host puti, " + v); break; }
      case S_KARAKTER: { std::string v = ex(s->e[0], c); emit("host putc, " + v); break; }
      case S_SATIR: {
        if (s->e.empty()) err("'satir' en az bir oge ister");
        for (size_t k = 0; k < s->e.size(); k++) { ntmp = 0; std::string v = ex(s->e[k], c); emit("host puti, " + v); emit(std::string("host putc, ") + (k + 1 < s->e.size() ? "32" : "10")); }
        break;
      }
      case S_KIR: if (c.loops.empty()) err("'kir' dongu disinda"); emit("jmp " + c.loops.back().first); break;
      case S_DEVAM: if (c.loops.empty()) err("'devam' dongu disinda"); emit("jmp " + c.loops.back().second); break;
      default: err("dahili hata: komut");
    }
  }

  // ---- yazilan alan analizi
  void mark(const ExprP& lv, const Ctx& c) {
    if (lv->k == E_ALAN && c.t >= 0 && size_t(c.t) < m.tms.size() && size_t(lv->a) < m.tms[size_t(c.t)].f.size()) m.tms[size_t(c.t)].f[size_t(lv->a)].written = true;
    else if (lv->k == E_AALAN && c.a >= 0 && size_t(c.a) < m.tms.size() && size_t(lv->a) < m.tms[size_t(c.a)].f.size()) m.tms[size_t(c.a)].f[size_t(lv->a)].written = true;
    else if (lv->k == E_BALAN && c.b >= 0 && size_t(c.b) < m.tms.size() && size_t(lv->a) < m.tms[size_t(c.b)].f.size()) m.tms[size_t(c.b)].f[size_t(lv->a)].written = true;
    else if (lv->k == E_TALAN && size_t(lv->a) < m.tms.size() && size_t(lv->b) < m.tms[size_t(lv->a)].f.size()) m.tms[size_t(lv->a)].f[size_t(lv->b)].written = true;
  }
  void scan(const Block& b, const Ctx& c) {
    for (auto& s : b) { if (s->k == S_ATA) mark(s->e[0], c); for (auto& bl : s->blk) scan(bl, c); }
  }

  // ---- ana iskelet
  void lib_screen() {
    long long W = m.W, H = m.H, wh = W * H;
    out.push_back("func putpx(x, y, col) {");
    emit("lt t, x, 0"); emit("jnz t, out"); emit("ge t, x, " + S(W)); emit("jnz t, out");
    emit("lt t, y, 0"); emit("jnz t, out"); emit("ge t, y, " + S(H)); emit("jnz t, out");
    emit("mul k, y, " + S(W)); emit("add k, k, x"); emit("add p, fb, k"); emit("st col, p, 0"); label("out"); emit("ret 0"); out.push_back("}");
    out.push_back("func rect(x, y, w, h, col) {");
    emit("mov yy, 0"); label("ry"); emit("lt c, yy, h"); emit("jz c, done"); emit("mov xx, 0"); label("rx"); emit("lt c, xx, w"); emit("jz c, nexty");
    emit("add px, x, xx"); emit("add py, y, yy"); emit("call z, putpx, px, py, col"); emit("add xx, xx, 1"); emit("jmp rx");
    label("nexty"); emit("add yy, yy, 1"); emit("jmp ry"); label("done"); emit("ret 0"); out.push_back("}");
    out.push_back("func temizle() {");
    emit("mov i, 0"); label("l"); emit("lt c, i, " + S(wh)); emit("jz c, d"); emit("add p, fb, i"); emit("st " + S(m.bg) + ", p, 0"); emit("add i, i, 1"); emit("jmp l"); label("d"); emit("ret 0"); out.push_back("}");
  }

  std::string generate() {
    if (m.tms.size() > 1000 || m.fns.size() > 5000) bad("cok fazla sablon/islev");
    bool hasScreen = m.W > 0;
    // 1) davranislari ac
    auto exp = [&](Block& b) { Block nb; expand_block(b, nb, nullptr, m, 0); b = nb; };
    for (auto& t : m.tms) exp(t.rules);
    for (auto& e : m.ets) exp(e.body);
    for (auto& g : m.gis) exp(g.body);
    for (auto& f : m.fns) exp(f.body);
    exp(m.ilk); exp(m.son);
    // 2) dogrulamalar ve yazilan alan analizi
    for (auto& e : m.ets) { if (size_t(e.a) >= m.tms.size() || size_t(e.b) >= m.tms.size()) bad("etkilesim: sablon yok"); if (e.a == e.b) bad("etkilesim: iki sablon farkli olmali"); }
    for (auto& g : m.gis) if (g.t >= 0 && size_t(g.t) >= m.tms.size()) bad("girdi: sablon yok");
    for (auto& a : m.ags) { if (size_t(a.g) >= m.globals.size()) bad("toplama: global yok"); if (size_t(a.t) >= m.tms.size()) bad("toplama: sablon yok"); }
    for (size_t ti = 0; ti < m.tms.size(); ti++) { Ctx c; c.t = int(ti); scan(m.tms[ti].rules, c); }
    for (auto& e : m.ets) { Ctx c; c.a = e.a; c.b = e.b; scan(e.body, c); }
    for (auto& g : m.gis) { Ctx c; c.t = g.t; scan(g.body, c); }
    for (auto& f : m.fns) { Ctx c; scan(f.body, c); }
    { Ctx c; scan(m.ilk, c); scan(m.son, c); }
    bool anyFrame = !m.ets.empty() || !m.gis.empty() || !m.ags.empty() || m.hasBitis;
    for (auto& t : m.tms) if (!t.rules.empty() || !t.draws.empty()) anyFrame = true;
    bool loop = hasScreen || m.frames > 0 || m.hasBitis;
    if (anyFrame && !loop) bad("kare dongusu icin 'sahne' ya da kare sayisi (>0) ya da 'bitis' gerekli");
    if (loop && !hasScreen && m.frames == 0 && !m.hasBitis) bad("kare dongusu sonsuz: kare sayisi >0 ya da bitis gerekli");
    for (auto& t : m.tms) if (!t.draws.empty() && !hasScreen) bad("cizim var ama sahne yok");
    // 3) bellek ve veri
    long long words = 64 + (long long)m.globals.size() + 1 + (hasScreen ? m.W * m.H : 0);
    for (auto& a : m.arrs) words += a.size;
    long long nvirt = 0, nstored = 0, wstored = 0;
    for (auto& t : m.tms) for (auto& f : t.f) { if (t.stored(f)) { words += t.n; nstored++; wstored += t.n; } else nvirt += t.n; }
    out.push_back("; PMY1 tablosundan uretildi (yapisal katman) -- elle duzenlenmez");
    out.push_back("; ozet sanal_alan_sozcugu=" + S(nvirt) + " kayitli_alan=" + S(nstored) + " kayitli_sozcuk=" + S(wstored) + " cerceve_sozcugu=" + S(hasScreen ? m.W * m.H : 0));
    out.push_back("memory " + S(std::max<long long>(4096, words + 1024)));
    for (size_t g = 0; g < m.globals.size(); g++) out.push_back("data g_" + S(long(g)) + " = " + S(m.globals[g]));
    out.push_back("data g_kare = 0");
    for (size_t a = 0; a < m.arrs.size(); a++) out.push_back("space d_" + S(long(a)) + " " + S(m.arrs[a].size));
    for (size_t ti = 0; ti < m.tms.size(); ti++) for (size_t fi = 0; fi < m.tms[ti].f.size(); fi++) if (stored(int(ti), int(fi))) out.push_back("space " + sym(int(ti), int(fi)) + " " + S(m.tms[ti].n));
    if (hasScreen) { out.push_back("space fb " + S(m.W * m.H)); lib_screen(); }
    // 4) baslat: alan uretecleri + dizi baslangic degerleri
    out.push_back("func baslat() {");
    for (size_t a = 0; a < m.arrs.size(); a++) for (size_t k = 0; k < m.arrs[a].init.size(); k++) if (m.arrs[a].init[k] != 0) emit("st " + S(m.arrs[a].init[k]) + ", d_" + S(long(a)) + ", " + S(long(k)));
    for (size_t ti = 0; ti < m.tms.size(); ti++) {
      Tm& T = m.tms[ti]; bool any = false; for (size_t fi = 0; fi < T.f.size(); fi++) if (stored(int(ti), int(fi))) any = true;
      if (!any) continue;
      std::string l = lab(), d = lab();
      emit("mov i, 0"); label(l); emit("lt c, i, " + S(T.n)); emit("jz c, " + d);
      for (size_t fi = 0; fi < T.f.size(); fi++) {
        const Fld& f = T.f[fi]; if (!stored(int(ti), int(fi))) continue;
        ntmp = 0; std::string v = tmp();
        switch (f.kind) {
          case G_SABIT: emit("mov " + v + ", " + S(f.p[0])); break;
          case G_DOGRUSAL: emit("mul " + v + ", i, " + S(f.p[1])); emit("add " + v + ", " + v + ", " + S(f.p[0])); break;
          case G_MOD: emit("mod " + v + ", i, " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); break;
          case G_BOL: emit("div " + v + ", i, " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); break;
          default:   // rastgele: h1=(i*1664525 + tohum*1013904223 + 12345) & 0xFFFFFF ; h2=(h1*1664525 + 1013904223) & 0xFFFFFF ; lo + (h2>>4) % (hi-lo+1)
            emit("mul " + v + ", i, 1664525"); emit("add " + v + ", " + v + ", " + S(f.p[2] * 1013904223LL + 12345)); emit("and " + v + ", " + v + ", 16777215");
            emit("mul " + v + ", " + v + ", 1664525"); emit("add " + v + ", " + v + ", 1013904223"); emit("and " + v + ", " + v + ", 16777215");
            emit("shr " + v + ", " + v + ", 4"); emit("mod " + v + ", " + v + ", " + S(f.p[1] - f.p[0] + 1)); emit("add " + v + ", " + v + ", " + S(f.p[0])); break;
        }
        std::string p = tmp(); emit("add " + p + ", " + sym(int(ti), int(fi)) + ", i"); emit("st " + v + ", " + p + ", 0");
      }
      emit("add i, i, 1"); emit("jmp " + l); label(d);
    }
    emit("ret 0"); out.push_back("}");
    // 5) islevler
    for (size_t k = 0; k < m.fns.size(); k++) {
      where = "islev " + S(long(k)); Fn& f = m.fns[k]; Ctx c; c.fn = true; c.nloc = f.nl;
      begin_fn("f_" + S(long(k)), f.np); block(f.body, c); end_fn(f.np);
    }
    // 6) sablon kurallari / etkilesim / girdi / toplama / cizim
    for (size_t ti = 0; ti < m.tms.size(); ti++) {
      Tm& T = m.tms[ti]; if (T.rules.empty()) continue;
      where = "sablon " + S(long(ti)) + " kurallari"; Ctx c; c.t = int(ti);
      begin_fn("r_" + S(long(ti)), 0); std::string l = lab(), nx = lab(), d = lab();
      c.loops.clear(); emit("mov i, 0"); label(l); emit("lt c, i, " + S(T.n)); emit("jz c, " + d);
      c.loops.push_back({nx, nx}); block(T.rules, c); c.loops.pop_back();
      label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d); end_fn(0);
    }
    for (size_t k = 0; k < m.ets.size(); k++) {
      Et& e = m.ets[k]; where = "etkilesim " + S(long(k)); Ctx c; c.a = e.a; c.b = e.b; const Tm& A = m.tms[size_t(e.a)]; const Tm& B = m.tms[size_t(e.b)];
      begin_fn("e_" + S(long(k)), 0);
      std::string la = lab(), lb = lab(), nxa = lab(), nxb = lab(), d = lab();
      emit("mov ia, 0"); label(la); emit("lt c, ia, " + S(A.n)); emit("jz c, " + d);
      emit("mov ib, 0"); label(lb); emit("lt c, ib, " + S(B.n)); emit("jz c, " + nxa);
      ntmp = 0; std::string cnd = ex(e.cond, c); emit("jz " + cnd + ", " + nxb);
      c.loops.push_back({nxb, nxb}); block(e.body, c); c.loops.pop_back();
      label(nxb); emit("add ib, ib, 1"); emit("jmp " + lb); label(nxa); emit("add ia, ia, 1"); emit("jmp " + la); label(d); end_fn(0);
    }
    for (size_t k = 0; k < m.gis.size(); k++) {
      Gi& g = m.gis[k]; where = "girdi " + S(long(k)); Ctx c; c.t = g.t;
      begin_fn("gi_" + S(long(k)), 0);
      if (g.t < 0) block(g.body, c);
      else {
        std::string l = lab(), nx = lab(), d = lab(); emit("mov i, 0"); label(l); emit("lt c, i, " + S(m.tms[size_t(g.t)].n)); emit("jz c, " + d);
        c.loops.push_back({nx, nx}); block(g.body, c); c.loops.pop_back();
        label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d);
      }
      end_fn(0);
    }
    for (size_t k = 0; k < m.ags.size(); k++) {
      Ag& a = m.ags[k]; const Tm& T = m.tms[size_t(a.t)]; where = "toplama " + S(long(k)); Ctx c; c.t = a.t;
      begin_fn("a_" + S(long(k)), 0);
      long long init = a.op == 3 ? -(1LL << 62) : a.op == 4 ? (1LL << 62) : 0;
      emit("mov acc, " + S(init));
      std::string l = lab(), nx = lab(), d = lab(); emit("mov i, 0"); label(l); emit("lt c, i, " + S(T.n)); emit("jz c, " + d);
      ntmp = 0; std::string cnd = ex(a.cond, c); emit("jz " + cnd + ", " + nx);
      std::string v = ex(a.val, c);
      if (a.op == 1) emit("add acc, acc, " + v);
      else if (a.op == 2) emit("add acc, acc, 1");
      else { std::string sk = lab(), cc = tmp(); emit((a.op == 3 ? "gt " : "lt ") + cc + ", " + v + ", acc"); emit("jz " + cc + ", " + sk); emit("mov acc, " + v); label(sk); }
      label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d);
      emit("st acc, g_" + S(a.g) + ", 0"); end_fn(0);
    }
    if (hasScreen) for (size_t ti = 0; ti < m.tms.size(); ti++) {
      Tm& T = m.tms[ti]; if (T.draws.empty()) continue; where = "sablon " + S(long(ti)) + " cizimi"; Ctx c; c.t = int(ti);
      begin_fn("c_" + S(long(ti)), 0); std::string l = lab(), nx = lab(), d = lab();
      emit("mov i, 0"); label(l); emit("lt c, i, " + S(T.n)); emit("jz c, " + d);
      for (auto& dr : T.draws) {
        ntmp = 0; std::vector<std::string> v; for (auto& a : dr.a) v.push_back(ex(a, c));
        if (dr.shape == 1) emit("call z, rect, " + v[0] + ", " + v[1] + ", " + v[2] + ", " + v[3] + ", " + v[4]); else emit("call z, putpx, " + v[0] + ", " + v[1] + ", " + v[2]);
      }
      label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d); end_fn(0);
    }
    { where = "ilk blok"; Ctx c; begin_fn("ilk", 0); block(m.ilk, c); end_fn(0); }
    { where = "son blok"; Ctx c; begin_fn("son", 0); block(m.son, c); end_fn(0); }
    // 7) ana dongu
    out.push_back("func main() {");
    emit("call z, baslat"); emit("call z, ilk");
    if (loop) {
      emit("mov f, 0"); label("fl");
      if (m.frames > 0) { emit("lt c, f, " + S(m.frames)); emit("jz c, fin"); }
      if (!m.gis.empty()) {
        label("ev"); emit("host e, poll"); emit("jz e, evd"); emit("shl key, e, 32"); emit("shr key, key, 32");
        for (size_t k = 0; k < m.gis.size(); k++) { std::string nk = "nk" + S(long(k)); emit("eq t, key, " + S(m.gis[k].code)); emit("jz t, " + nk); emit("call z, gi_" + S(long(k))); label(nk); }
        emit("jmp ev"); label("evd");
      }
      for (size_t ti = 0; ti < m.tms.size(); ti++) if (!m.tms[ti].rules.empty()) emit("call z, r_" + S(long(ti)));
      for (size_t k = 0; k < m.ets.size(); k++) emit("call z, e_" + S(long(k)));
      for (size_t k = 0; k < m.ags.size(); k++) emit("call z, a_" + S(long(k)));
      if (m.hasBitis) { where = "bitis"; Ctx c; ntmp = 0; std::string cnd = ex(m.bitis, c); emit("jz " + cnd + ", nobit"); emit("jmp fin"); label("nobit"); }
      if (hasScreen) {
        emit("call z, temizle");
        for (size_t ti = 0; ti < m.tms.size(); ti++) if (!m.tms[ti].draws.empty()) emit("call z, c_" + S(long(ti)));
        emit("host r, present, fb, " + S(m.W) + ", " + S(m.H)); emit("jnz r, fin");
      }
      emit("ld fk, g_kare, 0"); emit("add fk, fk, 1"); emit("st fk, g_kare, 0");
      emit("add f, f, 1"); emit("jmp fl"); label("fin");
    }
    emit("call z, son"); emit("ret 0"); out.push_back("}");
    std::string res; for (auto& l : out) res += l + "\n";
    return res;
  }
};

Model decode_model(const std::string& bytes) { Dec d; d.run((const uint8_t*)bytes.data(), bytes.size()); return d.m; }

}  // namespace

bool is_yapisal(const std::string& bytes) { return bytes.size() >= 4 && memcmp(bytes.data(), "PMY1", 4) == 0; }

std::string yapisal_to_pma(const std::string& bytes) {
  Gen g; g.m = decode_model(bytes);
  return g.generate();
}

std::string yapisal_dump(const std::string& bytes) {
  Model m = decode_model(bytes); std::ostringstream o;
  o << "sahne " << m.W << "x" << m.H << " arkaplan " << m.bg << " kare " << m.frames << "\n";
  o << "globaller:"; for (size_t i = 0; i < m.globals.size(); i++) o << " [" << i << "]=" << m.globals[i]; o << "\n";
  for (size_t i = 0; i < m.arrs.size(); i++) o << "dizi " << i << " boyut " << m.arrs[i].size << " baslangic " << m.arrs[i].init.size() << " deger\n";
  for (size_t i = 0; i < m.fns.size(); i++) { o << "islev " << i << " parametre=" << m.fns[i].np << " yerel=" << m.fns[i].nl << "\n"; pblock(m.fns[i].body, o, 1); }
  for (size_t i = 0; i < m.bhs.size(); i++) { o << "davranis " << i << " parametre=" << m.bhs[i].np << "\n"; pblock(m.bhs[i].body, o, 1); }
  for (size_t i = 0; i < m.tms.size(); i++) {
    const Tm& t = m.tms[i]; o << "sablon " << i << " adet=" << t.n << "\n";
    static const char* gn[] = {"?", "sabit", "dogrusal", "mod", "bol", "rastgele"};
    for (size_t j = 0; j < t.f.size(); j++) { o << "  alan " << j << " " << gn[t.f[j].kind] << " " << t.f[j].p[0] << " " << t.f[j].p[1] << " " << t.f[j].p[2] << "\n"; }
    o << "  kurallar:\n"; pblock(t.rules, o, 2);
    for (auto& d : t.draws) { o << "  ciz " << (d.shape == 1 ? "dikdortgen" : "nokta"); for (auto& a : d.a) { o << " "; pexpr(a, o); } o << "\n"; }
  }
  for (size_t i = 0; i < m.ets.size(); i++) { o << "etkilesim " << i << " a=" << m.ets[i].a << " b=" << m.ets[i].b << " kosul "; pexpr(m.ets[i].cond, o); o << "\n"; pblock(m.ets[i].body, o, 1); }
  for (size_t i = 0; i < m.gis.size(); i++) { o << "girdi " << i << " tus=" << m.gis[i].code << " sablon=" << m.gis[i].t << "\n"; pblock(m.gis[i].body, o, 1); }
  for (size_t i = 0; i < m.ags.size(); i++) { o << "toplama " << i << " global=" << m.ags[i].g << " islem=" << m.ags[i].op << " sablon=" << m.ags[i].t << " deger "; pexpr(m.ags[i].val, o); o << " kosul "; pexpr(m.ags[i].cond, o); o << "\n"; }
  if (m.hasBitis) { o << "bitis "; pexpr(m.bitis, o); o << "\n"; }
  o << "ilk:\n"; pblock(m.ilk, o, 1); o << "son:\n"; pblock(m.son, o, 1);
  return o.str();
}
