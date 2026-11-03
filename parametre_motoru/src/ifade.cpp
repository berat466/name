// Bolum 02 -- Ifadeler, operatorler, sabitler.  "pm ifade": C++ ifade/operator alt kumesi on yuzu.
//   metin -> lexer -> ayristirici -> AST -> PMX1 (kanonik SAYISAL AST, sayi listesi) -> [PMX1 cozulur] -> anlam + kod uretimi -> PMA metni -> assemble -> PMB1.
//   Derleme asamasi her zaman PMX1 sayi listesinden calisir (metin AST'den degil): "pm ifade --pmx1" sayilari yazar, "--pmx1-oku" sayilardan derler.
// Not [Kesin]: bu dosya C++ referans/gelistirme aracidir (motorun kendisi gibi); uretilen cikti sayisal PMB1'dir ve ELF/svm/pm uzerinde ayni calisir.
#include <quadmath.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include "program.h"
using std::string; using std::vector;

namespace {
struct IErr { string code, msg; int line; };
[[noreturn]] void fail(const string& code, const string& msg, int line = 0) { throw IErr{code, msg, line}; }

// ============================== LEXER (02.026-030) ==============================
enum TokK { TK_EOF, TK_ID, TK_INT, TK_FLT, TK_CHR, TK_STR, TK_P };
struct Tok {
  TokK k = TK_EOF; string s;            // kimlik / noktalama / dize baytlari
  uint64_t u = 0; double d = 0; long double ld = 0; __float128 q128 = 0;   // tamsayi degeri / ondalik deger / karakter degeri / tam kesinlikli (x87 80 bit) deger
  int tcode = 0;                        // sabit turu (TY_* kodu, asagida)
  string udl;                           // kullanici tanimli sonek (_km gibi), bos = yok
  bool decimal = true; int line = 0;
};
// tur kodlari (AST'de de kullanilir)
enum TyK { Y_VOID, Y_BOOL, Y_CHAR, Y_SCHAR, Y_UCHAR, Y_SHORT, Y_USHORT, Y_INT, Y_UINT, Y_LONG, Y_ULONG, Y_LLONG, Y_ULLONG, Y_FLOAT, Y_DOUBLE, Y_LDOUBLE,
           Y_NULLPTR, Y_PTR, Y_REF, Y_RREF, Y_ARR, Y_CLASS, Y_AUTO, Y_FUNC, Y_FLOAT128 };

struct Lexer {
  const string& src; size_t p = 0; int line = 1; vector<Tok> out;
  explicit Lexer(const string& s) : src(s) {}
  static bool idStart(char c) { return isalpha((unsigned char)c) || c == '_'; }
  static bool idChar(char c) { return isalnum((unsigned char)c) || c == '_'; }
  char at(size_t i) const { return i < src.size() ? src[i] : 0; }
  // kacis dizisi (02.026): \n \t \\ \' \" \? \a \b \f \r \v \0 \ooo \xhh
  int64_t escape(size_t& i) {   // src[i] == '\\' sonrasi karakterde
    char c = at(i++);
    switch (c) {
      case 'n': return 10; case 't': return 9; case 'r': return 13; case 'a': return 7; case 'b': return 8; case 'f': return 12; case 'v': return 11;
      case '\\': return '\\'; case '\'': return '\''; case '"': return '"'; case '?': return '?';
      case 'x': { if (!isxdigit((unsigned char)at(i))) fail("E_LEX", "\\x sonrasi onaltilik rakam yok", line); uint64_t v = 0; while (isxdigit((unsigned char)at(i))) { v = v * 16 + (isdigit((unsigned char)at(i)) ? at(i) - '0' : (tolower(at(i)) - 'a' + 10)); i++; } if (v > 255) fail("E_LEX", "\\x degeri char sinirini asiyor", line); return int64_t(v); }
      default:
        if (c >= '0' && c <= '7') { int v = c - '0', n = 1; while (n < 3 && at(i) >= '0' && at(i) <= '7') { v = v * 8 + (at(i) - '0'); i++; n++; } if (v > 255) fail("E_LEX", "sekizlik kacis degeri char sinirini asiyor", line); return v; }
        fail("E_LEX", string("bilinmeyen kacis dizisi: \\") + c, line);
    }
  }
  string suffix(size_t& i) { size_t b = i; while (idChar(at(i))) i++; return src.substr(b, i - b); }
  void number() {
    Tok t; t.line = line; t.k = TK_INT; size_t i = p; string digits; int base = 10; bool isf = false;
    if (at(i) == '0' && (at(i + 1) == 'x' || at(i + 1) == 'X')) { base = 16; i += 2; }
    else if (at(i) == '0' && (at(i + 1) == 'b' || at(i + 1) == 'B')) { base = 2; i += 2; }
    auto dig = [&](char c) { return base == 16 ? isxdigit((unsigned char)c) != 0 : isdigit((unsigned char)c) != 0; };
    size_t start = i;
    // rakam ayiraci (02.030): ' yalniz iki rakam arasinda
    auto scan = [&]() { while (true) { char c = at(i); if (dig(c)) { digits += c; i++; } else if (c == '\'' && !digits.empty() && dig(at(i + 1)) && dig(at(i - 1))) i++; else break; } };
    scan();
    if (digits.empty() && base != 10 && !(base == 16 && at(i) == '.')) fail("E_LEX", "sayi on ekinden sonra rakam yok", line);
    if (base == 10 && (at(i) == '.' || at(i) == 'e' || at(i) == 'E')) isf = true;
    if (base == 16 && (at(i) == '.' || at(i) == 'p' || at(i) == 'P')) isf = true;
    if (isf) {
      string f = digits;
      if (at(i) == '.') { f += '.'; i++; string frac; size_t save = digits.size(); digits.clear(); scan(); frac = digits; digits.clear(); (void)save; f += frac; }
      if ((base == 10 && (at(i) == 'e' || at(i) == 'E')) || (base == 16 && (at(i) == 'p' || at(i) == 'P'))) {
        f += at(i++); if (at(i) == '+' || at(i) == '-') f += at(i++);
        if (!isdigit((unsigned char)at(i))) fail("E_LEX", "us kismi bos", line);
        digits.clear(); scan(); f += digits;
      } else if (base == 16) fail("E_LEX", "onaltilik ondalik sabitte p ussu gerekli", line);
      string sf = suffix(i); t.k = TK_FLT; t.tcode = Y_DOUBLE;
      if (sf == "f" || sf == "F") t.tcode = Y_FLOAT; else if (sf == "l" || sf == "L") t.tcode = Y_LDOUBLE; else if (sf == "q" || sf == "Q") t.tcode = Y_FLOAT128; else if (!sf.empty()) { if (sf[0] != '_') fail("E_LEX", "gecersiz ondalik sabit soneki: " + sf, line); t.udl = sf; }
      if (base == 16) f = "0x" + f;
      t.d = strtod(f.c_str(), nullptr); t.ld = strtold(f.c_str(), nullptr); t.q128 = strtoflt128(f.c_str(), nullptr); p = i; out.push_back(t); return;
    }
    // tamsayi
    uint64_t v = 0; bool oct = false;
    if (base == 10 && digits.size() > 1 && digits[0] == '0') { base = 8; oct = true; }
    for (char c : digits) {
      int dv = isdigit((unsigned char)c) ? c - '0' : tolower(c) - 'a' + 10;
      if (dv >= base) fail("E_LEX", string("gecersiz rakam '") + c + "' " + (oct ? "sekizlik" : base == 2 ? "ikilik" : "") + " sabitte", line);
      if (v > (UINT64_MAX - dv) / base) fail("E_LEX", "tamsayi sabiti 64 bite sigmiyor", line);
      v = v * base + dv;
    }
    (void)start;
    string sf = suffix(i); bool u = false; int l = 0; string udl;
    {   // standart soneki ayir: u/U ve l/L/ll/LL (herhangi sirada, buyuk/kucuk harf uyumlu), kalan _ ile baslayan = UDL
      size_t k = 0; bool ok = true;
      while (k < sf.size() && ok) {
        char c = sf[k];
        if ((c == 'u' || c == 'U') && !u) { u = true; k++; }
        else if ((c == 'l' || c == 'L') && l == 0) { if (k + 1 < sf.size() && sf[k + 1] == c) { l = 2; k += 2; } else { l = 1; k++; } }
        else ok = false;
      }
      if (k < sf.size()) { if (sf[0] != '_' ) fail("E_LEX", "gecersiz tamsayi sabit soneki: " + sf, line); udl = sf; u = false; l = 0; }
    }
    t.decimal = base == 10; t.u = v; t.udl = udl;
    // tur secimi (C++ [lex.icon] tablosu)
    if (!udl.empty()) t.tcode = Y_ULLONG;
    else {
      bool dec = t.decimal; int c = -1;
      // adaylar sirasi
      vector<int> cand;
      if (!u && l == 0) cand = dec ? vector<int>{Y_INT, Y_LONG, Y_LLONG} : vector<int>{Y_INT, Y_UINT, Y_LONG, Y_ULONG, Y_LLONG, Y_ULLONG};
      else if (u && l == 0) cand = {Y_UINT, Y_ULONG, Y_ULLONG};
      else if (!u && l == 1) cand = dec ? vector<int>{Y_LONG, Y_LLONG} : vector<int>{Y_LONG, Y_ULONG, Y_LLONG, Y_ULLONG};
      else if (u && l == 1) cand = {Y_ULONG, Y_ULLONG};
      else if (!u && l == 2) cand = dec ? vector<int>{Y_LLONG} : vector<int>{Y_LLONG, Y_ULLONG};
      else cand = {Y_ULLONG};
      for (int y : cand) {
        bool sg = (y == Y_INT || y == Y_LONG || y == Y_LLONG); int bits = (y == Y_INT || y == Y_UINT) ? 32 : 64;
        uint64_t mx = sg ? ((1ull << (bits - 1)) - 1) : (bits == 64 ? UINT64_MAX : ((1ull << bits) - 1));
        if (v <= mx) { c = y; break; }
      }
      if (c < 0) fail("E_LEX", "tamsayi sabiti hicbir tura sigmiyor (isaretli ondalik sabit icin u soneki gerekir)", line);
      t.tcode = c;
    }
    p = i; out.push_back(t);
  }
  void run() {
    while (p < src.size()) {
      char c = src[p];
      if (c == '\n') { line++; p++; continue; }
      if (isspace((unsigned char)c)) { p++; continue; }
      if (c == '/' && at(p + 1) == '/') { while (p < src.size() && src[p] != '\n') p++; continue; }
      if (c == '/' && at(p + 1) == '*') { p += 2; while (p < src.size() && !(src[p] == '*' && at(p + 1) == '/')) { if (src[p] == '\n') line++; p++; } if (p >= src.size()) fail("E_LEX", "kapanmamis yorum", line); p += 2; continue; }
      if (c == '#') { while (p < src.size() && src[p] != '\n') p++; continue; }   // on-isleyici satirlari (#include) yok sayilir
      if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)at(p + 1)))) {
        if (c == '.') { Tok t; /* .5 gibi */ size_t i = p + 1; string f = "0."; while (isdigit((unsigned char)at(i))) f += at(i++); if (at(i) == 'e' || at(i) == 'E') { f += at(i++); if (at(i) == '+' || at(i) == '-') f += at(i++); while (isdigit((unsigned char)at(i))) f += at(i++); } t.k = TK_FLT; t.line = line; t.tcode = Y_DOUBLE; string sf = suffix(i); if (sf == "f" || sf == "F") t.tcode = Y_FLOAT; else if (sf == "l" || sf == "L") t.tcode = Y_LDOUBLE; else if (sf == "q" || sf == "Q") t.tcode = Y_FLOAT128; else if (!sf.empty()) { if (sf[0] != '_') fail("E_LEX", "gecersiz ondalik sonek", line); t.udl = sf; } t.d = strtod(f.c_str(), nullptr); t.ld = strtold(f.c_str(), nullptr); t.q128 = strtoflt128(f.c_str(), nullptr); p = i; out.push_back(t); continue; }
        number(); continue;
      }
      // dize (ham dize dahil)
      if (c == 'R' && at(p + 1) == '"') { rawString(); continue; }
      if (c == '"') { str(); continue; }
      if (c == '\'') { chr(); continue; }
      if (idStart(c)) { size_t b = p; while (idChar(at(p))) p++; Tok t; t.k = TK_ID; t.s = src.substr(b, p - b); t.line = line; out.push_back(t); continue; }
      // noktalama (en uzun eslesme)
      static const char* P3[] = {"<=>", "<<=", ">>=", "...", "->*"};
      static const char* P2[] = {"->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "::", ".*"};
      Tok t; t.k = TK_P; t.line = line; bool f = false;
      for (auto s : P3) if (src.compare(p, 3, s) == 0) { t.s = s; p += 3; f = true; break; }
      if (!f) for (auto s : P2) if (src.compare(p, 2, s) == 0) { t.s = s; p += 2; f = true; break; }
      if (!f) { if (!strchr("+-*/%&|^~!<>=?:;,.()[]{}", c)) fail("E_LEX", string("gecersiz karakter '") + c + "'", line); t.s = string(1, c); p++; }
      out.push_back(t);
    }
    Tok e; e.k = TK_EOF; e.line = line; out.push_back(e);
  }
  // dize: bitisik dizeler birlestirilir (02.028); UDL soneki (varsa) hepsi icin ortak
  void str() {
    Tok t; t.k = TK_STR; t.line = line; p++;
    while (true) {
      if (p >= src.size() || src[p] == '\n') fail("E_LEX", "kapanmamis dize sabiti", line);
      char c = src[p];
      if (c == '"') { p++; break; }
      if (c == '\\') { p++; size_t i = p; t.s.push_back(char(escape(i))); p = i; }
      else { t.s.push_back(c); p++; }
    }
    if (idStart(at(p))) { size_t i = p; t.udl = suffix(i); p = i; if (t.udl[0] != '_') fail("E_LEX", "gecersiz dize soneki: " + t.udl, line); }
    pushStr(t);
  }
  void rawString() {   // R"delim( ... )delim"  (02.027) kacis islenmez, satir sonlari korunur
    Tok t; t.k = TK_STR; t.line = line; p += 2; string delim;
    while (at(p) != '(') { if (p >= src.size() || delim.size() > 16 || at(p) == ' ' || at(p) == ')' || at(p) == '\\' || at(p) == '"') fail("E_LEX", "gecersiz ham dize ayiricisi", line); delim += src[p++]; }
    p++; string close = ")" + delim + "\"";
    size_t e = src.find(close, p); if (e == string::npos) fail("E_LEX", "kapanmamis ham dize", line);
    t.s = src.substr(p, e - p); for (char c : t.s) if (c == '\n') line++;
    p = e + close.size();
    if (idStart(at(p))) { size_t i = p; t.udl = suffix(i); p = i; if (t.udl[0] != '_') fail("E_LEX", "gecersiz dize soneki: " + t.udl, line); }
    pushStr(t);
  }
  void pushStr(Tok t) {
    if (!out.empty() && out.back().k == TK_STR) {   // bitisik birlestirme; UDL: ikisi de ayni ya da yalniz biri
      Tok& b = out.back();
      if (!t.udl.empty() && !b.udl.empty() && t.udl != b.udl) fail("E_LEX", "bitisik dizelerde farkli UDL sonekleri", t.line);
      b.s += t.s; if (b.udl.empty()) b.udl = t.udl; return;
    }
    out.push_back(t);
  }
  void chr() {
    Tok t; t.k = TK_CHR; t.line = line; t.tcode = Y_CHAR; p++;
    if (at(p) == '\'' ) fail("E_LEX", "bos karakter sabiti", line);
    int64_t v;
    if (at(p) == '\\') { p++; size_t i = p; v = escape(i); p = i; } else { v = (signed char)src[p]; p++; }
    if (at(p) != '\'') fail("E_LEX", "karakter sabiti tek karakter olmali (cok karakterli sabit desteklenmiyor)", line);
    p++; t.u = uint64_t(v); t.d = double(v);
    if (idStart(at(p))) { size_t i = p; t.udl = suffix(i); p = i; if (t.udl[0] != '_') fail("E_LEX", "gecersiz karakter soneki", line); }
    out.push_back(t);
  }
};

// ============================== AST + PMX1 ==============================
enum NK {
  // ifadeler
  N_INT, N_FLT, N_CHR, N_STR, N_BOOL, N_NULLPTR, N_THIS, N_ID, N_UNARY, N_BINARY, N_ASSIGN, N_COND, N_COMMA, N_CALL, N_INDEX, N_MEMBER, N_ARROW,
  N_PREINC, N_POSTINC, N_SIZEOF_E, N_SIZEOF_T, N_ALIGNOF, N_NOEXCEPT, N_TYPEID_E, N_TYPEID_T, N_CAST, N_FCAST, N_UDL, N_INITLIST,
  // tur dugumleri
  N_TY, N_TPTR, N_TREF, N_TRREF, N_TARR,
  // deyimler / bildirimler
  N_BLOCK, N_VAR, N_DECLS, N_EXPRSTMT, N_IF, N_WHILE, N_DO, N_FOR, N_RETURN, N_BREAK, N_CONTINUE, N_EMPTY, N_FUNC, N_PARAMS, N_CLASS, N_PROG, N_USING, N_THROW, N_TRY, N_CATCH,
  // Bolum 03: denetim akisi
  N_SWITCH, N_CASE, N_DEFAULT, N_GOTO, N_LABEL, N_RANGEFOR, N_MATCH, N_ARM, N_PWILD, N_PCONST, N_PRANGE, N_PBIND, N_PALT, N_PSTRUCT, N_COYIELD, N_CORETURN, N_COAWAIT, N_NONE
};
struct Node {
  int k = N_NONE; int64_t i = 0, j = 0; double d = 0; string s, s2; vector<int> kid; int line = 0;
};
struct Ast {
  std::deque<Node> n;
  int mk(int k, int line = 0) { Node x; x.k = k; x.line = line; n.push_back(x); return int(n.size()) - 1; }
  Node& operator[](int i) { return n[size_t(i)]; }
  const Node& operator[](int i) const { return n[size_t(i)]; }
};

// PMX1 kanonik sayi bicimi:  0x504D5831, surum, dizge sayisi, {uzunluk, bayt...}*, dugum sayisi, kok,
//   her dugum: kind, i, j, d(ham 64 bit), dizge1(-1/ indeks), dizge2, satir, cocuk sayisi, cocuklar...
const int64_t PMX_MAGIC = 0x504D5831;
vector<int64_t> pmx_encode(const Ast& a, int root) {
  vector<string> strs; std::map<string, int> sid;
  auto S = [&](const string& s) -> int { if (s.empty()) return -1; auto it = sid.find(s); if (it != sid.end()) return it->second; sid[s] = int(strs.size()); strs.push_back(s); return int(strs.size()) - 1; };
  vector<int> s1(a.n.size()), s2(a.n.size());
  for (size_t i = 0; i < a.n.size(); i++) { s1[i] = S(a.n[i].s); s2[i] = S(a.n[i].s2); }
  vector<int64_t> o = {PMX_MAGIC, 1, int64_t(strs.size())};
  for (auto& s : strs) { o.push_back(int64_t(s.size())); for (unsigned char c : s) o.push_back(c); }
  o.push_back(int64_t(a.n.size())); o.push_back(root);
  for (size_t i = 0; i < a.n.size(); i++) {
    const Node& x = a.n[i]; int64_t db; memcpy(&db, &x.d, 8);
    o.insert(o.end(), {x.k, x.i, x.j, db, s1[i], s2[i], x.line, int64_t(x.kid.size())});
    for (int k : x.kid) o.push_back(k);
  }
  return o;
}
// PMX1 yapisal dogrulama: dugum turu araligi, tur basina cocuk sayisi, cocuk turu kisitlari, dongusuzluk ve derinlik siniri
struct Arity { int k, lo, hi; };
void pmx_validate(const Ast& a, int root) {
  static const Arity ar[] = {{N_INT,0,0},{N_FLT,0,0},{N_CHR,0,0},{N_STR,0,0},{N_BOOL,0,0},{N_NULLPTR,0,0},{N_THIS,0,0},{N_ID,0,0},{N_UNARY,1,1},{N_BINARY,2,2},{N_ASSIGN,2,2},{N_COND,3,3},{N_COMMA,2,2},
    {N_CALL,1,-1},{N_INDEX,2,2},{N_MEMBER,1,1},{N_ARROW,1,1},{N_PREINC,1,1},{N_POSTINC,1,1},{N_SIZEOF_E,1,1},{N_SIZEOF_T,1,1},{N_ALIGNOF,1,1},{N_NOEXCEPT,1,1},{N_TYPEID_E,1,1},{N_TYPEID_T,1,1},
    {N_CAST,2,2},{N_FCAST,1,2},{N_UDL,0,-1},{N_INITLIST,0,-1},{N_TY,0,0},{N_TPTR,1,1},{N_TREF,1,1},{N_TRREF,1,1},{N_TARR,1,2},{N_BLOCK,0,-1},{N_VAR,1,2},{N_DECLS,0,-1},{N_EXPRSTMT,1,1},{N_IF,2,3},
    {N_WHILE,2,2},{N_DO,2,2},{N_FOR,4,4},{N_RETURN,0,1},{N_BREAK,0,0},{N_CONTINUE,0,0},{N_EMPTY,0,0},{N_FUNC,2,3},{N_PARAMS,0,-1},{N_CLASS,0,-1},{N_PROG,0,-1},{N_USING,0,-1},{N_THROW,0,1},{N_TRY,2,-1},{N_CATCH,2,2},{N_SWITCH,2,2},{N_CASE,1,2},{N_DEFAULT,0,0},{N_GOTO,0,0},{N_LABEL,0,0},{N_RANGEFOR,3,3},{N_MATCH,2,-1},{N_ARM,3,3},{N_PWILD,0,0},{N_PCONST,1,1},{N_PRANGE,2,2},{N_PBIND,1,1},{N_PALT,2,-1},{N_PSTRUCT,1,-1},{N_COYIELD,1,1},{N_CORETURN,0,1},{N_COAWAIT,1,1},{N_NONE,0,0}};
  for (size_t i = 0; i < a.n.size(); i++) {
    const Node& x = a.n[i];
    if (x.k < 0 || x.k > N_NONE) fail("E_PMX", "PMX1 dugum turu gecersiz");
    const Arity* r = nullptr; for (auto& e : ar) if (e.k == x.k) r = &e;
    if (!r || int(x.kid.size()) < r->lo || (r->hi >= 0 && int(x.kid.size()) > r->hi)) fail("E_PMX", "PMX1 dugumun cocuk sayisi bu tur icin gecersiz");
    if (x.k == N_TY && (x.i < 0 || x.i > Y_FLOAT128)) fail("E_PMX", "PMX1 tur kodu gecersiz");
    auto need = [&](int c, std::initializer_list<int> kinds) { for (int k : kinds) if (a.n[size_t(c)].k == k) return; fail("E_PMX", "PMX1 cocuk dugum turu bu konumda gecersiz"); };
    if (x.k == N_PROG) for (int c : x.kid) need(c, {N_CLASS, N_FUNC, N_VAR});
    if (x.k == N_CLASS) for (int c : x.kid) need(c, {N_VAR, N_FUNC});
    if (x.k == N_PARAMS) for (int c : x.kid) need(c, {N_VAR});
    if (x.k == N_DECLS) for (int c : x.kid) need(c, {N_VAR});
    if (x.k == N_FUNC) { need(x.kid[1], {N_PARAMS}); if (x.kid.size() > 2) need(x.kid[2], {N_BLOCK}); }
    if (x.k == N_FOR) { need(x.kid[0], {N_NONE, N_DECLS, N_EXPRSTMT}); }
    if (x.k == N_TRY) { need(x.kid[0], {N_BLOCK}); for (size_t q = 1; q < x.kid.size(); q++) need(x.kid[q], {N_CATCH}); }
    if (x.k == N_SWITCH) need(x.kid[1], {N_BLOCK});
    if (x.k == N_RANGEFOR) { need(x.kid[0], {N_DECLS}); }
    if (x.k == N_ARM) need(x.kid[0], {N_PWILD, N_PCONST, N_PRANGE, N_PBIND, N_PALT, N_PSTRUCT});
    if (x.k == N_PALT || x.k == N_PSTRUCT) for (int c : x.kid) need(c, {N_PWILD, N_PCONST, N_PRANGE, N_PBIND, N_PALT, N_PSTRUCT});
    if (x.k == N_CATCH) { need(x.kid[0], {N_NONE, N_TY, N_TPTR, N_TREF, N_TRREF, N_TARR}); need(x.kid[1], {N_BLOCK}); }
  }
  // dongusuzluk + derinlik (yinelemesiz DFS)
  vector<int> state(a.n.size(), 0), stk; vector<size_t> idx; stk.push_back(root); idx.push_back(0); state[size_t(root)] = 1;
  while (!stk.empty()) {
    int u = stk.back(); size_t& i = idx.back();
    const Node& x = a.n[size_t(u)];
    if (i < x.kid.size()) { int c = x.kid[i]; i++; if (state[size_t(c)] == 1) fail("E_PMX", "PMX1 dugum grafigi dongulu"); if (state[size_t(c)] == 0) { if (stk.size() > 3000) fail("E_PMX", "PMX1 derinlik siniri asildi"); state[size_t(c)] = 1; stk.push_back(c); idx.push_back(0); } }
    else { state[size_t(u)] = 2; stk.pop_back(); idx.pop_back(); }
  }
}
Ast pmx_decode(const vector<int64_t>& v, int& root) {
  size_t p = 0; auto next = [&]() -> int64_t { if (p >= v.size()) fail("E_PMX", "PMX1 sayi listesi erken bitti"); return v[p++]; };
  if (next() != PMX_MAGIC) fail("E_PMX", "PMX1 sihirli sayisi yanlis"); if (next() != 1) fail("E_PMX", "PMX1 surumu desteklenmiyor");
  int64_t ns = next(); if (ns < 0 || ns > (1 << 24)) fail("E_PMX", "PMX1 dizge sayisi gecersiz");
  vector<string> strs; strs.resize(size_t(ns));
  for (auto& s : strs) { int64_t l = next(); if (l < 0 || l > (1 << 24)) fail("E_PMX", "PMX1 dizge uzunlugu gecersiz"); for (int64_t i = 0; i < l; i++) s.push_back(char(next())); }
  int64_t nn = next(); root = int(next()); if (nn < 0 || nn > (1 << 24) || root < 0 || root >= nn) fail("E_PMX", "PMX1 dugum sayisi/kok gecersiz");
  Ast a; a.n.resize(size_t(nn));
  for (auto& x : a.n) {
    x.k = int(next()); x.i = next(); x.j = next(); int64_t db = next(); memcpy(&x.d, &db, 8);
    int64_t i1 = next(), i2 = next(); x.line = int(next());
    if (i1 >= ns || i2 >= ns) fail("E_PMX", "PMX1 dizge indeksi gecersiz"); if (i1 >= 0) x.s = strs[size_t(i1)]; if (i2 >= 0) x.s2 = strs[size_t(i2)];
    int64_t nk = next(); if (nk < 0 || nk > nn) fail("E_PMX", "PMX1 cocuk sayisi gecersiz");
    for (int64_t k = 0; k < nk; k++) { int64_t c = next(); if (c < 0 || c >= nn) fail("E_PMX", "PMX1 cocuk indeksi gecersiz"); x.kid.push_back(int(c)); }
  }
  if (p != v.size()) fail("E_PMX", "PMX1 sonunda artik sayi var");
  pmx_validate(a, root);
  return a;
}

// ============================== OPERATOR TABLOSU + AYRISTIRICI (02.004-011) ==============================
// Tek kaynak: ikili operator oncelik tablosu (buyuk sayi = siki baglanir). Ayristirici bu tabloyu okur; anlam asamasi AST'deki operator dizgesini kullanir.
#include "ifade_ops_gen.h"
struct OpRow { int cls, prec, assoc; const char* tok; };
const OpRow kOps[] = {
#define X(c, p, a, t) {c, p, a, t},
  PM_IFADE_OPS(X)
#undef X
};
const OpRow* opRow(int cls, const string& tok) { for (auto& r : kOps) if (r.cls == cls && tok == r.tok) return &r; return nullptr; }

struct Parser {
  Ast& a; vector<Tok>& t; size_t p = 0; int depth = 0;
  struct Dp { Parser& q; Dp(Parser& pq) : q(pq) { if (++q.depth > 300) fail("E_PARSE", "ifade/deyim ic ice gecme siniri asildi (300)", q.cur().line); } ~Dp() { q.depth--; } };
  std::set<string> classes; std::map<string, int> alias;   // ad -> N_TY/N_T* dugumu
  Parser(Ast& ast, vector<Tok>& toks) : a(ast), t(toks) {
    const char* sc[] = {"std::strong_ordering", "std::weak_ordering", "std::partial_ordering", "std::type_info"};
    for (auto s : sc) classes.insert(s);
    struct { const char* n; int ty; } al[] = {{"size_t", Y_ULONG}, {"ptrdiff_t", Y_LONG}, {"intptr_t", Y_LONG}, {"uintptr_t", Y_ULONG}, {"int8_t", Y_SCHAR}, {"uint8_t", Y_UCHAR}, {"int16_t", Y_SHORT},
      {"uint16_t", Y_USHORT}, {"int32_t", Y_INT}, {"uint32_t", Y_UINT}, {"int64_t", Y_LONG}, {"uint64_t", Y_ULONG}, {"nullptr_t", Y_NULLPTR}};
    for (auto& x : al) { int n = a.mk(N_TY); a[n].i = x.ty; alias[x.n] = n; alias[string("std::") + x.n] = n; }
  }
  const Tok& cur() const { return t[p]; }
  const Tok& peek(size_t k = 1) const { return t[std::min(p + k, t.size() - 1)]; }
  bool isP(const char* s) const { return cur().k == TK_P && cur().s == s; }
  bool isPk(size_t k, const char* s) const { return peek(k).k == TK_P && peek(k).s == s; }
  bool isId(const char* s) const { return cur().k == TK_ID && cur().s == s; }
  bool accept(const char* s) { if (isP(s)) { p++; return true; } return false; }
  bool acceptId(const char* s) { if (isId(s)) { p++; return true; } return false; }
  void expect(const char* s) { if (!accept(s)) fail("E_PARSE", string("'") + s + "' bekleniyordu, bulunan '" + (cur().k == TK_EOF ? "<son>" : cur().s) + "'", cur().line); }
  string ident() { if (cur().k != TK_ID) fail("E_PARSE", "kimlik bekleniyordu", cur().line); return t[p++].s; }
  int mk(int k) { return a.mk(k, cur().line); }

  static bool isTypeKw(const string& s) {
    static const char* kw[] = {"void", "bool", "char", "short", "int", "long", "signed", "unsigned", "float", "double", "const", "volatile", "auto", "struct", "class", "__float128"};
    for (auto k : kw) if (s == k) return true; return false;
  }
  // niteleyici ad: A::B::C
  string qname(size_t& q) const {
    string s = t[q].s; q++;
    while (t[q].k == TK_P && t[q].s == "::" && t[q + 1].k == TK_ID) { s += "::" + t[q + 1].s; q += 2; }
    return s;
  }
  bool typeStartAt(size_t q) const {
    if (t[q].k != TK_ID) return false;
    if (isTypeKw(t[q].s)) return true;
    size_t z = q; string n = qname(z);
    return classes.count(n) || alias.count(n) || n == "std::generator";
  }
  bool typeStart() const { return typeStartAt(p); }

  int cloneTy(int n) { Node x = a[n]; a.n.push_back(x); return int(a.n.size()) - 1; }
  // tur belirtimi: cv + temel tur / sinif adi
  int typeSpec() {
    int cv = 0; int sg = 0, sh = 0, lg = 0; int base = -1; string cls; int aliasNode = -1; int line = cur().line;
    while (cur().k == TK_ID) {
      const string& s = cur().s;
      if (s == "const") { cv |= 1; p++; }
      else if (s == "volatile") { cv |= 2; p++; }
      else if (s == "unsigned") { sg = 2; p++; } else if (s == "signed") { sg = 1; p++; }
      else if (s == "short") { sh = 1; p++; } else if (s == "long") { lg++; p++; }
      else if (s == "int") { if (base != -1 && base != Y_INT) fail("E_PARSE", "cok fazla tur belirtimi", line); base = Y_INT; p++; }
      else if (s == "char" || s == "bool" || s == "float" || s == "double" || s == "void" || s == "auto") {
        if (base != -1 || !cls.empty() || aliasNode >= 0) fail("E_PARSE", "cok fazla tur belirtimi", line);
        base = s == "char" ? Y_CHAR : s == "bool" ? Y_BOOL : s == "float" ? Y_FLOAT : s == "double" ? Y_DOUBLE : s == "void" ? Y_VOID : Y_AUTO; p++;
      }
      else if (s == "__float128") { if (base != -1 || !cls.empty() || aliasNode >= 0 || sg || sh || lg) fail("E_PARSE", "cok fazla tur belirtimi", line); base = Y_FLOAT128; p++; }
      else if (s == "struct" || s == "class") { p++; }
      else if (base == -1 && sg == 0 && sh == 0 && lg == 0 && cls.empty() && aliasNode < 0 && (classes.count(s) || alias.count(s) || (s == "std" && isPk(1, "::")))) {
        size_t z = p; string n = qname(z); p = z;
        if (n == "std::generator" && isP("<")) {   // Bolum 03 korutin ureteci: std::generator<T> (T = yerlesik skaler tur); sinif adi tur koduyla: std::generator<kod>
          p++; int at = typeSpec(); if (a[at].k != N_TY || a[at].i == Y_CLASS || a[at].i == Y_VOID) fail("E_PARSE", "std::generator<T> icin T yerlesik skaler tur olmali", line);
          if (!isP(">")) fail("E_PARSE", "'>' bekleniyordu", line); p++; n = "std::generator<" + std::to_string(a[at].i) + ">"; cls = n;
        }
        else if (alias.count(n)) aliasNode = alias[n]; else if (classes.count(n)) cls = n; else fail("E_PARSE", "bilinmeyen tur adi: " + n, line);
      } else break;
    }
    int n;
    if (aliasNode >= 0) { n = cloneTy(aliasNode); }
    else if (!cls.empty()) { n = a.mk(N_TY, line); a[n].i = Y_CLASS; a[n].s = cls; }
    else {
      int ty;
      if (base == Y_CHAR) ty = sg == 2 ? Y_UCHAR : sg == 1 ? Y_SCHAR : Y_CHAR;
      else if (base == Y_DOUBLE) ty = lg ? Y_LDOUBLE : Y_DOUBLE;
      else if (base == Y_BOOL || base == Y_FLOAT || base == Y_VOID || base == Y_AUTO) { if (sg || sh || lg) fail("E_PARSE", "gecersiz tur belirtimi birlesimi", line); ty = base; }
      else {   // int ailesi
        if (base == -1 && sg == 0 && sh == 0 && lg == 0) fail("E_PARSE", "tur bekleniyordu", cur().line);
        if (sh && lg) fail("E_PARSE", "short ve long birlikte", line);
        if (lg > 2) fail("E_PARSE", "long long long gecersiz", line);
        bool u = sg == 2;
        ty = sh ? (u ? Y_USHORT : Y_SHORT) : lg == 1 ? (u ? Y_ULONG : Y_LONG) : lg == 2 ? (u ? Y_ULLONG : Y_LLONG) : (u ? Y_UINT : Y_INT);
      }
      if (base == Y_FLOAT128) ty = Y_FLOAT128;
      n = a.mk(N_TY, line); a[n].i = ty;
    }
    a[n].j |= cv;
    // cv sonradan da gelebilir:  int const
    while (cur().k == TK_ID && (cur().s == "const" || cur().s == "volatile")) { a[n].j |= (cur().s == "const" ? 1 : 2); p++; }
    return n;
  }
  // soyut/adli bildirici onekleri: * & &&
  int ptrOps(int base) {
    while (true) {
      if (isP("*")) { p++; int n = a.mk(N_TPTR, cur().line); a[n].kid = {base}; while (cur().k == TK_ID && (cur().s == "const" || cur().s == "volatile")) { a[n].j |= (cur().s == "const" ? 1 : 2); p++; } base = n; }
      else if (isP("&")) { p++; int n = a.mk(N_TREF, cur().line); a[n].kid = {base}; base = n; }
      else if (isP("&&")) { p++; int n = a.mk(N_TRREF, cur().line); a[n].kid = {base}; base = n; }
      else break;
    }
    return base;
  }
  int arraySuffix(int base) {
    vector<int> dims; vector<int> line;
    while (isP("[")) { p++; int e = -1; if (!isP("]")) e = expr(); expect("]"); dims.push_back(e); }
    for (size_t i = dims.size(); i-- > 0;) { int n = a.mk(N_TARR, cur().line); a[n].kid = {base}; if (dims[i] >= 0) a[n].kid.push_back(dims[i]); base = n; }
    return base;
  }
  int fullType() { int b = typeSpec(); b = ptrOps(b); if (isP("[")) b = arraySuffix(b); return b; }

  // ---------------- ifadeler ----------------
  int expr() {   // virgul (en dusuk)
    int l = assign();
    while (isP(",")) { int line = cur().line; p++; int r = assign(); int n = a.mk(N_COMMA, line); a[n].kid = {l, r}; l = n; }
    return l;
  }
  int assign() {
    Dp dp(*this); int l = cond();
    if (cur().k == TK_P && opRow(2, cur().s)) {
      string op = cur().s; int line = cur().line; p++; int r = isP("{") ? initList() : assign();   // sag birlesimli
      int n = a.mk(N_ASSIGN, line); a[n].s = op; a[n].kid = {l, r}; return n;
    }
    return l;
  }
  int cond() {
    int c = binary(3);
    if (isP("?")) {
      int line = cur().line; p++; int x = expr(); expect(":"); int y = assign();
      int n = a.mk(N_COND, line); a[n].kid = {c, x, y}; return n;
    }
    return c;
  }
  int binPrec(const Tok& k) const { if (k.k != TK_P) return -1; const OpRow* r = opRow(1, k.s); return r ? r->prec : -1; }
  int binary(int minPrec) {
    int l = unary();
    while (true) {
      int pr = binPrec(cur()); if (pr < minPrec) break;
      string op = cur().s; int line = cur().line; p++;
      int r = binary(pr + 1);   // sol birlesimli
      int n = a.mk(N_BINARY, line); a[n].s = op; a[n].kid = {l, r}; l = n;
    }
    return l;
  }
  // Bolum 03 desen eslestirme (__match): desen = secenek ('|' secenek)* ; secenek = '_' | sabit ['...' sabit] | tur ad (baglama) | '{' desen, ... '}' (sinif alanlari sirayla)
  int patAtom() {
    Dp dp(*this); int line = cur().line;
    if (cur().k == TK_ID && cur().s == "_") { p++; return a.mk(N_PWILD, line); }
    if (isP("{")) {
      p++; int n = a.mk(N_PSTRUCT, line);
      while (true) { a[n].kid.push_back(pattern()); if (!accept(",")) break; }
      expect("}"); return n;
    }
    if (declStart()) { int b = typeSpec(); int tt = ptrOps(b); int n = a.mk(N_PBIND, line); a[n].s = ident(); a[n].kid = {tt}; return n; }
    int lo = unary();
    if (isP("...")) { p++; int hi = unary(); int n = a.mk(N_PRANGE, line); a[n].kid = {lo, hi}; return n; }
    int n = a.mk(N_PCONST, line); a[n].kid = {lo}; return n;
  }
  int pattern() {
    int line = cur().line; int l = patAtom(); if (!isP("|")) return l;
    int n = a.mk(N_PALT, line); a[n].kid = {l}; while (accept("|")) a[n].kid.push_back(patAtom()); return n;
  }
  int initList() {
    Dp dp(*this); int n = mk(N_INITLIST); expect("{");
    while (!isP("}")) { a[n].kid.push_back(isP("{") ? initList() : assign()); if (!accept(",")) break; }
    expect("}"); return n;
  }
  int unary() {
    Dp dp(*this); int line = cur().line;
    if (cur().k == TK_P) {
      const string& s = cur().s;
      if (opRow(3, s) && s != "++" && s != "--") { string op = s; p++; int e = unary(); int n = a.mk(N_UNARY, line); a[n].s = op; a[n].kid = {e}; return n; }
      if (opRow(3, s) && (s == "++" || s == "--")) { string op = s; p++; int e = unary(); int n = a.mk(N_PREINC, line); a[n].s = op; a[n].kid = {e}; return n; }
      if (s == "(" && typeStartAt(p + 1)) {   // C tipi donusum
        size_t save = p; p++; int ty = fullType(); if (accept(")")) {
          if (!(isP("{"))) { int e = unary(); int n = a.mk(N_CAST, line); a[n].s = "c"; a[n].kid = {ty, e}; return n; }
        }
        p = save;
      }
    }
    if (cur().k == TK_ID) {
      const string& s = cur().s;
      if (s == "sizeof") {
        p++;
        if (isP("(") && typeStartAt(p + 1)) { p++; int ty = fullType(); expect(")"); int n = a.mk(N_SIZEOF_T, line); a[n].kid = {ty}; return n; }
        int e = unary(); int n = a.mk(N_SIZEOF_E, line); a[n].kid = {e}; return n;
      }
      if (s == "alignof" || s == "_Alignof") { p++; expect("("); int ty = fullType(); expect(")"); int n = a.mk(N_ALIGNOF, line); a[n].kid = {ty}; return n; }
      if (s == "noexcept") { p++; expect("("); int e = expr(); expect(")"); int n = a.mk(N_NOEXCEPT, line); a[n].kid = {e}; return n; }
      if (s == "typeid") {
        p++; expect("(");
        if (typeStart()) { int ty = fullType(); expect(")"); int n = a.mk(N_TYPEID_T, line); a[n].kid = {ty}; return postfixRest(n); }
        int e = expr(); expect(")"); int n = a.mk(N_TYPEID_E, line); a[n].kid = {e}; return postfixRest(n);
      }
    }
    return postfix();
  }
  int castExpr(const string& kind, int line) {   // static_cast<T>(e) ...
    expect("<"); int ty = fullType(); expect(">"); expect("("); int e = expr(); expect(")");
    int n = a.mk(N_CAST, line); a[n].s = kind; a[n].kid = {ty, e}; return n;
  }
  int primary() {
    const Tok& k = cur(); int line = k.line;
    switch (k.k) {
      case TK_INT: { int n = a.mk(N_INT, line); a[n].i = int64_t(k.u); a[n].j = k.tcode; a[n].s2 = k.udl; p++; return n; }
      case TK_FLT: {
        int n = a.mk(N_FLT, line); a[n].d = k.d; a[n].j = k.tcode; a[n].s2 = k.udl;
        if (k.tcode == Y_LDOUBLE || !k.udl.empty()) {   // tam kesinlikli deger: i = ham mantis, d'nin ham bitleri = (isaret<<15)|us
          unsigned char raw[16] = {0}; long double v = k.ld; memcpy(raw, &v, 10); uint64_t m; memcpy(&m, raw, 8); uint64_t se = raw[8] | (uint64_t(raw[9]) << 8);
          a[n].i = int64_t(m); double dd; memcpy(&dd, &se, 8); a[n].d = dd;
        }
        if (k.tcode == Y_FLOAT128) {   // tam kesinlikli binary128: i = dusuk 64 bit, d'nin ham bitleri = yuksek 64 bit (isaret|us15|mantis ust 48)
          unsigned char raw[16]; __float128 v = k.q128; memcpy(raw, &v, 16); uint64_t lo, hi; memcpy(&lo, raw, 8); memcpy(&hi, raw + 8, 8);
          a[n].i = int64_t(lo); double dd; memcpy(&dd, &hi, 8); a[n].d = dd;
        }
        p++; return n; }
      case TK_CHR: { int n = a.mk(N_CHR, line); a[n].i = int64_t(k.u); a[n].j = k.tcode; a[n].s2 = k.udl; p++; return n; }
      case TK_STR: { int n = a.mk(N_STR, line); a[n].s = k.s; a[n].s2 = k.udl; p++; return n; }
      case TK_P:
        if (k.s == "(") { p++; int e = expr(); expect(")"); return e; }
        fail("E_PARSE", "ifade bekleniyordu, bulunan '" + k.s + "'", line);
      case TK_ID: {
        const string& s = k.s;
        if (s == "true" || s == "false") { int n = a.mk(N_BOOL, line); a[n].i = s == "true"; p++; return n; }
        if (s == "nullptr") { p++; return a.mk(N_NULLPTR, line); }
        if (s == "this") { p++; return a.mk(N_THIS, line); }
        if (s == "static_cast" || s == "dynamic_cast" || s == "const_cast" || s == "reinterpret_cast") { string kd = s; p++; return castExpr(kd, line); }
        if (typeStart()) {   // islevsel donusum  T(e) / T{...}
          {
            int ty = typeSpec();
            if (isP("(")) { p++; int e = isP(")") ? -1 : expr(); expect(")"); int n = a.mk(N_FCAST, line); a[n].kid = {ty}; if (e >= 0) a[n].kid.push_back(e); return n; }
            if (isP("{")) { int il = initList(); int n = a.mk(N_FCAST, line); a[n].kid = {ty, il}; a[n].i = 1; return n; }
            // niteleyici ad: sinif::uye (tur adi degil, statik uye vb.) -> desteklenmez
            fail("E_PARSE", "tur adindan sonra '(' ya da '{' bekleniyordu", line);
          }
        }
        size_t z = p; string qn = qname(z); p = z;
        int n = a.mk(N_ID, line); a[n].s = qn; return n;
      }
      default: fail("E_PARSE", "beklenmeyen dosya sonu", line);
    }
  }
  int postfix() { return postfixRest(primary()); }
  int postfixRest(int e) {
    while (true) {
      int line = cur().line;
      if (isP("[")) { p++; int i = expr(); expect("]"); int n = a.mk(N_INDEX, line); a[n].kid = {e, i}; e = n; }
      else if (isP("(")) { p++; int n = a.mk(N_CALL, line); a[n].kid = {e}; if (!isP(")")) { while (true) { a[n].kid.push_back(assign()); if (!accept(",")) break; } } expect(")"); e = n; }
      else if (isP(".") || isP("->")) {
        bool ar = cur().s == "->"; p++;
        if (cur().k != TK_ID) fail("E_PARSE", "uye adi bekleniyordu", line);
        size_t z = p; string qn = qname(z); p = z;
        int n = a.mk(ar ? N_ARROW : N_MEMBER, line); a[n].kid = {e};
        size_t pos = qn.rfind("::"); if (pos != string::npos) { a[n].s = qn.substr(pos + 2); a[n].s2 = qn.substr(0, pos); } else a[n].s = qn;
        e = n;
      }
      else if (cur().k == TK_P && opRow(4, cur().s) && (cur().s == "++" || cur().s == "--")) { int n = a.mk(N_POSTINC, line); a[n].s = cur().s; a[n].kid = {e}; p++; e = n; }
      else break;
    }
    return e;
  }

  // ---------------- deyimler ve bildirimler ----------------
  int block() {
    int n = mk(N_BLOCK); expect("{");
    while (!isP("}")) { if (cur().k == TK_EOF) fail("E_PARSE", "'}' eksik", cur().line); a[n].kid.push_back(stmt()); }
    expect("}"); return n;
  }
  // bir bildirimin degisken(ler)i: N_DECLS kids = N_VAR
  int varDecls(int base, bool needSemi = true) {
    int ds = mk(N_DECLS);
    while (true) {
      int tt = base; while (true) {
        if (isP("*")) { p++; int n = mk(N_TPTR); a[n].kid = {tt}; while (cur().k == TK_ID && (cur().s == "const" || cur().s == "volatile")) { a[n].j |= (cur().s == "const" ? 1 : 2); p++; } tt = n; }
        else if (isP("&")) { p++; int n = mk(N_TREF); a[n].kid = {tt}; tt = n; }
        else if (isP("&&")) { p++; int n = mk(N_TRREF); a[n].kid = {tt}; tt = n; }
        else break;
      }
      int v = mk(N_VAR); a[v].s = ident();
      if (isP("[")) tt = arraySuffix(tt);
      a[v].kid = {tt};
      if (isP("=")) { p++; a[v].i = 1; a[v].kid.push_back(isP("{") ? initList() : assign()); }
      else if (isP("(")) { p++; a[v].i = 2; int il = mk(N_INITLIST); if (!isP(")")) { while (true) { a[il].kid.push_back(assign()); if (!accept(",")) break; } } expect(")"); a[v].kid.push_back(il); }
      else if (isP("{")) { a[v].i = 3; a[v].kid.push_back(initList()); }
      a[ds].kid.push_back(v);
      if (!accept(",")) break;
    }
    if (needSemi) expect(";");
    return ds;
  }
  bool declStart() const {
    if (!typeStart()) return false;
    if (cur().k == TK_ID && isTypeKw(cur().s) && cur().s != "const" && cur().s != "struct" && cur().s != "class" && cur().s != "volatile" && cur().s != "auto" && isPk(1, "(")) return false;
    return true;
  }
  int stmt() {
    Dp dp(*this); int line = cur().line;
    if (isP("{")) return block();
    if (isP(";")) { p++; return a.mk(N_EMPTY, line); }
    if (cur().k == TK_ID) {
      const string& s = cur().s;
      if (s == "if") {
        p++; bool cxp = acceptId("constexpr"); expect("("); int pre = -1, c;
        if (declStart()) { int b = typeSpec(); int ds = varDecls(b, false); if (accept(";")) { pre = ds; c = expr(); } else { pre = ds; c = a.mk(N_ID, line); a[c].s = a[a[ds].kid.back()].s; } }
        else { int e = expr(); if (isP(";") && false) {} c = e; if (accept(";")) { pre = a.mk(N_EXPRSTMT, line); a[pre].kid = {e}; c = expr(); } }
        expect(")"); int th = stmt(); int n = a.mk(N_IF, line); a[n].i = cxp; a[n].kid = {c, th}; if (acceptId("else")) a[n].kid.push_back(stmt());
        if (pre < 0) return n;
        int bl = a.mk(N_BLOCK, line); a[bl].kid = {pre, n}; return bl;
      }
      if (s == "while") { p++; expect("("); int c = expr(); expect(")"); int b = stmt(); int n = a.mk(N_WHILE, line); a[n].kid = {c, b}; return n; }
      if (s == "do") { p++; int b = stmt(); if (!acceptId("while")) fail("E_PARSE", "do ... while bekleniyordu", line); expect("("); int c = expr(); expect(")"); expect(";"); int n = a.mk(N_DO, line); a[n].kid = {b, c}; return n; }
      if (s == "for") {
        p++; expect("("); int in = a.mk(N_NONE), c = a.mk(N_NONE), inc = a.mk(N_NONE);
        if (!isP(";")) { if (declStart()) { int b = typeSpec(); in = varDecls(b, false);
            if (isP(":")) { p++; int rng = expr(); expect(")"); int body = stmt(); int n = a.mk(N_RANGEFOR, line); a[n].kid = {in, rng, body}; return n; } } else { int e = expr(); in = a.mk(N_EXPRSTMT, line); a[in].kid = {e}; } }
        expect(";"); if (!isP(";")) c = expr(); expect(";"); if (!isP(")")) inc = expr(); expect(")");
        int body = stmt(); int n = a.mk(N_FOR, line); a[n].kid = {in, c, inc, body}; return n;
      }
      if (s == "return") { p++; int n = a.mk(N_RETURN, line); if (!isP(";")) a[n].kid.push_back(isP("{") ? initList() : expr()); expect(";"); return n; }
      if (s == "switch") {
        p++; expect("("); int pre = -1, c;
        if (declStart()) { int b = typeSpec(); int ds = varDecls(b, false); expect(";"); pre = ds; c = expr(); }
        else { int e = expr(); c = e; if (accept(";")) { pre = a.mk(N_EXPRSTMT, line); a[pre].kid = {e}; c = expr(); } }
        expect(")"); int b = stmt(); if (a[b].k != N_BLOCK) { int bl = a.mk(N_BLOCK, line); a[bl].kid = {b}; b = bl; }
        int n = a.mk(N_SWITCH, line); a[n].kid = {c, b};
        if (pre < 0) return n;
        int bl = a.mk(N_BLOCK, line); a[bl].kid = {pre, n}; return bl;
      }
      if (s == "case") { p++; int lo = cond(); int n = a.mk(N_CASE, line); a[n].kid = {lo}; if (isP("...")) { p++; a[n].kid.push_back(cond()); } expect(":"); return n; }
      if (s == "default" && isPk(1, ":")) { p += 2; return a.mk(N_DEFAULT, line); }
      if (s == "goto") { p++; int n = a.mk(N_GOTO, line); a[n].s = ident(); expect(";"); return n; }
      if (s == "co_yield") { p++; int n = a.mk(N_COYIELD, line); a[n].kid = {expr()}; expect(";"); return n; }
      if (s == "co_await") { p++; int n = a.mk(N_COAWAIT, line); a[n].kid = {expr()}; expect(";"); return n; }
      if (s == "co_return") { p++; int n = a.mk(N_CORETURN, line); if (!isP(";")) a[n].kid.push_back(expr()); expect(";"); return n; }
      if (s == "__match") {
        p++; expect("("); int subj = expr(); expect(")"); expect("{"); int n = a.mk(N_MATCH, line); a[n].kid = {subj};
        while (!isP("}")) {
          int al = cur().line; if (!acceptId("case")) fail("E_PARSE", "__match kolunda 'case' bekleniyordu", al);
          int pat = pattern(); int guard = a.mk(N_NONE, al); if (acceptId("if")) { expect("("); guard = expr(); expect(")"); }
          expect(":"); int body = a.mk(N_BLOCK, al);
          while (!isP("}") && !isId("case")) { if (cur().k == TK_EOF) fail("E_PARSE", "'}' eksik", al); a[body].kid.push_back(stmt()); }
          int arm = a.mk(N_ARM, al); a[arm].kid = {pat, guard, body}; a[n].kid.push_back(arm);
        }
        expect("}"); if (a[n].kid.size() < 2) fail("E_PARSE", "__match en az bir kol gerektirir", line); return n;
      }
      if (cur().k == TK_ID && isPk(1, ":") && !isPk(1, "::") && s != "default" && s != "case") { int n = a.mk(N_LABEL, line); a[n].s = s; p += 2; return n; }
      if (s == "throw") { p++; int n = a.mk(N_THROW, line); if (!isP(";")) a[n].kid.push_back(expr()); expect(";"); return n; }
      if (s == "try") {
        p++; int n = a.mk(N_TRY, line); a[n].kid.push_back(block());
        if (!isId("catch")) fail("E_PARSE", "try blogundan sonra catch bekleniyordu", line);
        while (isId("catch")) {
          int cl = cur().line; p++; expect("("); int c = a.mk(N_CATCH, cl);
          if (isP("...")) { p++; a[c].kid.push_back(a.mk(N_NONE, cl)); }
          else { int b = typeSpec(); int tt = ptrOps(b); if (cur().k == TK_ID) a[c].s = ident(); a[c].kid.push_back(tt); }
          expect(")"); a[c].kid.push_back(block()); a[n].kid.push_back(c);
        }
        return n;
      }
      if (s == "break") { p++; expect(";"); return a.mk(N_BREAK, line); }
      if (s == "continue") { p++; expect(";"); return a.mk(N_CONTINUE, line); }
    }
    if (declStart()) { int b = typeSpec(); return varDecls(b); }
    int e = expr(); expect(";"); int n = a.mk(N_EXPRSTMT, line); a[n].kid = {e}; return n;
  }

  // islev: tur adi() ... ; sinif uyeleri ve genel bildirimler bu yoldan
  // opName: operator belirtimi sonrasi islev adi. Donus: ad; donusum islecinde hedef tur dugumu convTy'ye yazilir
  string operatorName(int& convTy) {
    convTy = -1; int line = cur().line;
    if (cur().k == TK_STR) {   // operator"" _x  /  operator""_x
      if (!cur().s.empty()) fail("E_PARSE", "operator\"\" sonrasi dize bos olmali", line);
      string u = cur().udl; p++;
      if (u.empty()) { if (cur().k != TK_ID || cur().s[0] != '_') fail("E_PARSE", "UDL soneki '_' ile baslamali", line); u = cur().s; p++; }
      return "operator\"\"" + u;
    }
    if (typeStart()) { convTy = ptrOps(typeSpec()); return "operator conv"; }
    if (isP("(") && isPk(1, ")")) { p += 2; return "operator()"; }
    if (isP("[") && isPk(1, "]")) { p += 2; return "operator[]"; }
    if (cur().k == TK_P) { string s = cur().s; p++; return "operator" + s; }
    fail("E_PARSE", "operator belirtimi gecersiz", line);
  }
  int funcRest(int ret, const string& name, int line, int cls) {
    int f = a.mk(N_FUNC, line); a[f].s = name; int ps = mk(N_PARAMS);
    expect("(");
    if (!isP(")")) {
      if (isId("void") && isPk(1, ")")) p++;
      else while (true) {
        int b = typeSpec(); int tt = ptrOps(b); int pv = mk(N_VAR);
        if (cur().k == TK_ID) a[pv].s = ident();
        if (isP("[")) tt = arraySuffix(tt);
        a[pv].kid = {tt}; a[ps].kid.push_back(pv);
        if (!accept(",")) break;
      }
    }
    expect(")");
    int64_t fl = 0;
    while (cur().k == TK_ID) {
      if (cur().s == "const") { fl |= 1; p++; }
      else if (cur().s == "noexcept") {
        p++; fl |= 4; if (isP("(")) { p++; int e = expr(); expect(")"); if (a[e].k == N_BOOL && !a[e].i) fl &= ~int64_t(4); else if (a[e].k != N_BOOL) fail("E_PARSE", "noexcept(...) bildiriminde sabit true/false bekleniyor", line); }
      }
      else if (cur().s == "override") { fl |= 32; p++; }
      else if (cur().s == "final") { p++; }
      else break;
    }
    if (isP("=") && peek().k == TK_INT && peek().u == 0 && (isPk(2, ";"))) { fl |= 256; p += 2; expect(";"); a[f].kid = {ret, ps}; a[f].i = fl; return f; }
    if (isP("=") && peek().k == TK_ID && (peek().s == "default" || peek().s == "delete")) { fl |= peek().s == "default" ? 16 : 64; p += 2; expect(";"); a[f].kid = {ret, ps}; a[f].i = fl; return f; }
    if (isP("{")) { int b = block(); a[f].kid = {ret, ps, b}; } else { expect(";"); a[f].kid = {ret, ps}; }
    a[f].i = fl; (void)cls; return f;
  }
  // sinif govdesi
  int classDef(bool isStruct) {
    int line = cur().line; string name = ident(); while (isP("::") && peek().k == TK_ID) { p++; name += "::" + ident(); } classes.insert(name);   // niteleyicili ad yalniz yerlesik (onsoz) siniflar icin
    int c = a.mk(N_CLASS, line); a[c].s = name; a[c].i = isStruct;
    if (isP(";")) { p++; a[c].j = 1; return c; }   // ileri bildirim
    if (accept(":")) { acceptId("public"); acceptId("private"); acceptId("protected"); { size_t z = p; string bn = qname(z); if (!classes.count(bn)) fail("E_PARSE", "taban sinif bilinmiyor: " + bn, cur().line); p = z; a[c].s2 = bn; } if (isP(",")) fail("E_PARSE", "coklu kalitim desteklenmiyor", cur().line); }
    expect("{");
    while (!isP("}")) {
      if (cur().k == TK_EOF) fail("E_PARSE", "sinif '}' eksik", line);
      if ((isId("public") || isId("private") || isId("protected")) && isPk(1, ":")) { p += 2; continue; }
      int mline = cur().line; int64_t virt = 0; bool expl = false, stat = false, cexpr = false;
      while (cur().k == TK_ID && (cur().s == "virtual" || cur().s == "explicit" || cur().s == "static" || cur().s == "constexpr" || cur().s == "inline")) { if (cur().s == "virtual") virt = 2; if (cur().s == "explicit") expl = true; if (cur().s == "static") stat = true; if (cur().s == "constexpr") cexpr = true; p++; }
      (void)cexpr;
      if (stat) fail("E_PARSE", "statik uye desteklenmiyor", mline);
      if (isP("~")) {   // yok edici: yalniz bos govdeli (iz sirasi yok); sanal yok edici sinifi polimorfik yapar
        p++; string dn = "~" + ident(); expect("("); expect(")"); int64_t fl = 0; if (acceptId("noexcept")) fl |= 4;
        int f = a.mk(N_FUNC, mline); a[f].s = dn; int ps = mk(N_PARAMS); int rt = a.mk(N_TY, mline); a[rt].i = Y_VOID; int bl = mk(N_BLOCK);
        if (isP("=")) { p++; if (!acceptId("default")) fail("E_PARSE", "= default bekleniyordu", mline); expect(";"); } else { expect("{"); if (!isP("}")) fail("E_PARSE", "yok edici govdesi bos olmali (yok edici cagrilari desteklenmiyor)", mline); expect("}"); }
        a[f].kid = {rt, ps, bl}; a[f].i = fl | virt; a[c].kid.push_back(f); continue;
      }
      if (isId("operator")) {   // donusum isleci: operator T() const
        p++; int convTy; string nm = operatorName(convTy);
        int f = funcRest(convTy, nm, mline, c); a[f].i |= virt | (expl ? 128 : 0); a[c].kid.push_back(f); continue;
      }
      int b = typeSpec(); size_t savePos = p; int tt = ptrOps(b);
      if (isId("operator")) { p++; int convTy; string nm = operatorName(convTy); int f = funcRest(tt, nm, mline, c); a[f].i |= virt; a[c].kid.push_back(f); continue; }
      if (cur().k == TK_ID && isPk(1, "(")) { string nm = ident(); int f = funcRest(tt, nm, mline, c); a[f].i |= virt; a[c].kid.push_back(f); continue; }
      // alan(lar)
      p = savePos; int ds = varDeclsFrom(b);
      for (int v : a[ds].kid) a[c].kid.push_back(v);
    }
    expect("}"); expect(";"); return c;
  }
  int varDeclsFrom(int base) { return varDecls(base, true); }
  int topLevel() {
    int prog = mk(N_PROG);
    while (cur().k != TK_EOF) {
      int line = cur().line;
      if (isId("struct") || isId("class")) {
        // "struct X {" -> sinif tanimi ; "struct X x;" -> degisken (desteklenmez: elaborated tur kullanimi yalniz tanim)
        bool st = cur().s == "struct";
        if (peek().k == TK_ID) { size_t z = p + 1; qname(z); if (t[z].k == TK_P && (t[z].s == "{" || t[z].s == ":" || t[z].s == ";")) { p++; a[prog].kid.push_back(classDef(st)); continue; } }
      }
      if (isId("using")) {
        p++; string nm = ident(); expect("="); int ty = fullType(); expect(";");
        alias[nm] = ty; continue;
      }
      if (isId("typedef")) { p++; int ty = fullType(); string nm = ident(); expect(";"); alias[nm] = ty; continue; }
      int64_t virt = 0; bool ext = false;
      while (cur().k == TK_ID && (cur().s == "inline" || cur().s == "static" || cur().s == "constexpr" || cur().s == "extern")) { if (cur().s == "extern") ext = true; p++; }
      (void)ext;
      int b = typeSpec(); size_t savePos = p; int tt = ptrOps(b);
      if (isId("operator")) { p++; int convTy; string nm = operatorName(convTy); int f = funcRest(tt, nm, line, -1); a[f].i |= virt; a[prog].kid.push_back(f); continue; }
      if (cur().k == TK_ID && isPk(1, "(")) { string nm = ident(); int f = funcRest(tt, nm, line, -1); a[prog].kid.push_back(f); continue; }
      p = savePos; int ds = varDecls(b, true);
      for (int v : a[ds].kid) a[prog].kid.push_back(v);
    }
    return prog;
  }
};

// ============================== TURLER / SINIFLAR (anlam) ==============================
struct Ty { int k = Y_VOID; int e = -1; int64_t n = 0; int cls = -1; bool c = false, v = false;
  bool operator<(const Ty& o) const { return std::tie(k, e, n, cls, c, v) < std::tie(o.k, o.e, o.n, o.cls, o.c, o.v); } };
struct Fld { string name; int ty; int woff; int64_t boff; };
struct Cls {
  string name; int base = -1; bool poly = false, defined = false; int id = 0; vector<Fld> flds; vector<int> meths;
  int words = 0; int64_t size = 1, align = 1, dsize = 0; bool podLayout = true; int builtin = 0;   // 1 strong 2 weak 3 partial 4 type_info
  bool hasData() const { return !flds.empty() || poly; }
};
struct Fn {
  string name, pma; int ret = -1; vector<int> params; int cls = -1; bool isConst = false, isVirt = false, noex = false, defaulted = false, deleted = false, pure = false, explicitConv = false, hasOverride = false;
  int body = -1; int node = -1; int line = 0; int vroot = -1; bool generated = false, generating = false, autoRet = false, used = false, hasBody = false; vector<string> lines; int frame = 0; string builtin;
};
struct Var { string name; int ty; int off = 0; bool isRef = false, global = false; string sym; bool ci = false; int64_t cval = 0; int uid = 0; };
struct Eff { string key; int rw; };
struct V {
  int ty = -1; int cat = 0; string reg; string key; bool ci = false; int64_t cval = 0; bool viaPtr = false; vector<Eff> eff; bool isVoid = false; int nullLit = 0;
};
enum Cat { C_PR = 0, C_LV = 1, C_XV = 2 };

struct Sema {
  Ast& a; vector<Ty> T; std::map<Ty, int> tmap; vector<Cls> C; std::map<string, int> cmap; vector<Fn> F; std::map<string, vector<int>> fmap;
  vector<Var> globals; std::map<string, int> gmap; vector<string> dataLines; vector<string> gInit; int uidn = 0; int strn = 0;
  int tVoid, tBool, tChar, tInt, tUInt, tLong, tULong, tLLong, tULLong, tFloat, tDouble, tLDouble, tNullptr, tSChar, tUChar, tShort, tUShort;
  explicit Sema(Ast& ast) : a(ast) {
    auto mk = [&](int k) { Ty t; t.k = k; return intern(t); };
    tVoid = mk(Y_VOID); tBool = mk(Y_BOOL); tChar = mk(Y_CHAR); tSChar = mk(Y_SCHAR); tUChar = mk(Y_UCHAR); tShort = mk(Y_SHORT); tUShort = mk(Y_USHORT); tInt = mk(Y_INT); tUInt = mk(Y_UINT);
    tLong = mk(Y_LONG); tULong = mk(Y_ULONG); tLLong = mk(Y_LLONG); tULLong = mk(Y_ULLONG); tFloat = mk(Y_FLOAT); tDouble = mk(Y_DOUBLE); tLDouble = mk(Y_LDOUBLE); tNullptr = mk(Y_NULLPTR);
    const char* bi[] = {"std::strong_ordering", "std::weak_ordering", "std::partial_ordering", "std::type_info"};
    for (int i = 0; i < 4; i++) { int c = newClass(bi[i]); C[size_t(c)].builtin = i + 1; C[size_t(c)].defined = true; C[size_t(c)].words = 1; C[size_t(c)].size = i == 3 ? 16 : 1; C[size_t(c)].align = i == 3 ? 8 : 1; C[size_t(c)].flds.push_back({"__v", tInt, 0, 0}); }
  }
  int intern(const Ty& t) { auto it = tmap.find(t); if (it != tmap.end()) return it->second; T.push_back(t); tmap[t] = int(T.size()) - 1; return int(T.size()) - 1; }
  const Ty& ty(int i) const { return T[size_t(i)]; }
  int newClass(const string& n) { Cls c; c.name = n; c.id = int(C.size()) + 1; C.push_back(c); cmap[n] = int(C.size()) - 1; return int(C.size()) - 1; }
  // ---- tur kurucular / sorgular ----
  int unq(int t) { Ty x = ty(t); if (!x.c && !x.v) return t; x.c = x.v = false; return intern(x); }
  int withC(int t, bool c) { Ty x = ty(t); x.c = c; return intern(x); }
  int cv(int t, bool c, bool v) { Ty x = ty(t); x.c = x.c || c; x.v = x.v || v; return intern(x); }
  int ptrTo(int t) { Ty x; x.k = Y_PTR; x.e = t; return intern(x); }
  int lref(int t) { Ty x; x.k = Y_REF; x.e = t; return intern(x); }
  int rref(int t) { Ty x; x.k = Y_RREF; x.e = t; return intern(x); }
  int arrOf(int t, int64_t n) { Ty x; x.k = Y_ARR; x.e = t; x.n = n; return intern(x); }
  int clsTy(int c) { Ty x; x.k = Y_CLASS; x.cls = c; return intern(x); }
  int K(int t) const { return ty(t).k; }
  bool isRef(int t) const { return K(t) == Y_REF || K(t) == Y_RREF; }
  int deref(int t) { return isRef(t) ? ty(t).e : t; }
  bool isInt(int t) const { int k = K(t); return k >= Y_BOOL && k <= Y_ULLONG; }
  bool isFlt(int t) const { int k = K(t); return k >= Y_FLOAT && k <= Y_LDOUBLE; }
  bool isArith(int t) const { return isInt(t) || isFlt(t); }
  bool isPtr(int t) const { return K(t) == Y_PTR; }
  bool isCls(int t) const { return K(t) == Y_CLASS; }
  bool isArr(int t) const { return K(t) == Y_ARR; }
  bool isScalar(int t) const { return isArith(t) || isPtr(t) || K(t) == Y_NULLPTR; }
  bool isSigned(int t) const { int k = K(t); return k == Y_CHAR || k == Y_SCHAR || k == Y_SHORT || k == Y_INT || k == Y_LONG || k == Y_LLONG; }
  bool isUnsigned(int t) const { return isInt(t) && !isSigned(t); }
  int bitsOf(int t) const { switch (K(t)) { case Y_BOOL: return 1; case Y_CHAR: case Y_SCHAR: case Y_UCHAR: return 8; case Y_SHORT: case Y_USHORT: return 16; case Y_INT: case Y_UINT: return 32; default: return 64; } }
  int rankOf(int t) const { switch (K(t)) { case Y_BOOL: return 1; case Y_CHAR: case Y_SCHAR: case Y_UCHAR: return 2; case Y_SHORT: case Y_USHORT: return 3; case Y_INT: case Y_UINT: return 4; case Y_LONG: case Y_ULONG: return 5; default: return 6; } }
  bool sameUnq(int x, int y) { return unq(x) == unq(y); }
  int words(int t) {
    switch (K(t)) { case Y_ARR: return int(ty(t).n) * words(ty(t).e); case Y_CLASS: return C[size_t(ty(t).cls)].words; case Y_VOID: case Y_FUNC: return 0; default: return 1; }
  }
  int64_t sizeOf(int t) {
    switch (K(t)) {
      case Y_BOOL: case Y_CHAR: case Y_SCHAR: case Y_UCHAR: return 1; case Y_SHORT: case Y_USHORT: return 2; case Y_INT: case Y_UINT: case Y_FLOAT: return 4;
      case Y_LONG: case Y_ULONG: case Y_LLONG: case Y_ULLONG: case Y_DOUBLE: case Y_PTR: case Y_NULLPTR: return 8; case Y_LDOUBLE: return 16;
      case Y_ARR: return ty(t).n * sizeOf(ty(t).e); case Y_CLASS: return C[size_t(ty(t).cls)].size; case Y_REF: case Y_RREF: return sizeOf(ty(t).e);
      default: fail("E_TYPE", "bu turun boyutu yok");
    }
  }
  int64_t alignOf(int t) {
    switch (K(t)) { case Y_ARR: return alignOf(ty(t).e); case Y_CLASS: return C[size_t(ty(t).cls)].align; case Y_REF: case Y_RREF: return alignOf(ty(t).e); case Y_LDOUBLE: return 16; default: return sizeOf(t); }
  }
  bool completeObj(int t) { int k = K(t); if (k == Y_VOID || k == Y_FUNC) return false; if (k == Y_CLASS) return C[size_t(ty(t).cls)].defined; if (k == Y_ARR) return completeObj(ty(t).e); return true; }
  string tyName(int t) {
    const Ty& x = ty(t); string q = string(x.c ? "const " : "") + (x.v ? "volatile " : "");
    static const char* nm[] = {"void", "bool", "char", "signed char", "unsigned char", "short", "unsigned short", "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long", "float", "double", "long double", "nullptr_t"};
    if (x.k <= Y_NULLPTR) return q + nm[x.k];
    switch (x.k) {
      case Y_PTR: return tyName(x.e) + "*" + (x.c ? " const" : ""); case Y_REF: return tyName(x.e) + "&"; case Y_RREF: return tyName(x.e) + "&&";
      case Y_ARR: return tyName(x.e) + "[" + std::to_string(x.n) + "]"; case Y_CLASS: return q + C[size_t(x.cls)].name; default: return "?";
    }
  }
  // sinif v ABI dizilimi (Itanium): uyeler, taban, vptr, kuyruk dolgusu
  void layoutClass(int ci) {
    Cls& c = C[size_t(ci)]; int64_t off = 0, al = 1; int w = 0;
    if (c.base >= 0) {
      Cls& b = C[size_t(c.base)]; c.poly = c.poly || b.poly;
      bool bEmpty = !b.hasData() && b.base < 0 && b.flds.empty();   // bos taban (EBO)
      if (!bEmpty) { off = b.podLayout ? b.size : b.dsize; al = b.align; w = b.words; }
    }
    if (c.poly && (c.base < 0 || !C[size_t(c.base)].poly)) {   // yeni vptr
      if (c.base >= 0 && w > 0) fail("E_CLASS", "polimorfik olmayan tabani olan sinifa sanal islev ekleme desteklenmiyor (vptr basta olmali)");
      off = 8; al = 8; w = 1;
    }
    for (auto& f : c.flds) {
      if (isRef(f.ty)) fail("E_TYPE", "referans uyeler desteklenmiyor");
      if (!completeObj(f.ty)) fail("E_TYPE", "uye turu tam degil: " + f.name);
      int64_t fa = alignOf(f.ty); off = (off + fa - 1) / fa * fa; f.boff = off; f.woff = w; off += sizeOf(f.ty); w += words(f.ty); al = std::max(al, fa);
    }
    c.dsize = off; c.align = al; c.size = std::max<int64_t>(1, (off + al - 1) / al * al);
    if (c.base >= 0 && !c.hasData() && C[size_t(c.base)].size == 1 && !C[size_t(c.base)].hasData()) c.size = 1;
    c.words = w; c.podLayout = c.base < 0 && !c.poly; c.defined = true;
  }
  bool derivedFrom(int d, int b) { for (int c = d; c >= 0; c = C[size_t(c)].base) if (c == b) return true; return false; }

  // ============================== KOD URETIMI DURUMU ==============================
  struct Gen {
    int fn = -1; vector<string>* out = nullptr; int nreg = 0, nlab = 0, frameOff = 0, dry = 0; bool mayThrow = false;
    vector<std::map<string, Var>> scopes; int retTy = -1; bool sret = false; int cls = -1; vector<std::pair<string, string>> loops; string retLab;
    vector<string> pend; vector<int> retTys; bool collectRet = false;
  };
  Gen* g = nullptr;
  int curLine = 0;
  void E(const string& s) {
    if (g->dry) return;
    string line = "  ";
    for (auto& l : g->pend) line += l + ": ";
    // etiketler ayri komuta bagli: birden fazla etiket ayni satirda desteklenmez -> her etiket kendi satirinda degil, ilk komutla
    g->out->push_back(line + s); g->pend.clear();
  }
  void LAB(const string& l) {
    if (g->dry) return;
    if (!g->pend.empty()) { E("mov _lab, 0"); }   // art arda etiketler: araya zararsiz komut
    g->pend.push_back(l);
  }
  string NL() { return "L" + std::to_string(g->nlab++); }
  string R() { return "t" + std::to_string(g->nreg++); }
  int slot(int w) { int o = g->frameOff; g->frameOff += std::max(w, 1); return o; }
  string slotAddr(int off) { string r = R(); E("add " + r + ", fp, " + std::to_string(off)); return r; }
  string num(int64_t v) { return std::to_string(v); }
  string fnum(double d) {
    if (std::isnan(d) || std::isinf(d)) fail("E_CONST", "ondalik sabit sonsuz/NaN", curLine);
    char b[64]; snprintf(b, sizeof b, "%.17g", d); string s = b; if (s.find_first_of(".eE") == string::npos) s += ".0"; else if (s.find('.') == string::npos) { size_t e = s.find_first_of("eE"); s = s.substr(0, e) + ".0" + s.substr(e); }
    return s;
  }
  [[noreturn]] void err(const string& code, const string& msg) { fail(code, msg, curLine); }
  void trapNow() { E("ld " + R() + ", -1, 0"); }   // adres 0 gecerli bir sozcuk (null yalniz kural); sinir disi adres -> TRAP OOB
  void nullChk(const V& pv) {   // null isaretci dolayli erisimi: TRAP OOB (calisma zamani)
    if (g->dry) return;
    if (pv.ci && pv.cval == 0) { trapNow(); return; }
    string ok = NL(); E("jnz " + immReg(pv.reg) + ", " + ok); trapNow(); LAB(ok);
  }

  // sabit katlama yardimcilari
  int64_t foldNorm(int64_t v, int t) {
    switch (K(t)) {
      case Y_BOOL: return v != 0; case Y_CHAR: case Y_SCHAR: return int8_t(v); case Y_UCHAR: return uint8_t(v); case Y_SHORT: return int16_t(v); case Y_USHORT: return uint16_t(v);
      case Y_INT: return int32_t(v); case Y_UINT: return uint32_t(v); default: return v;
    }
  }
  string normReg(const string& r, int t) {
    int k = K(t); string d;
    switch (k) {
      case Y_BOOL: d = R(); E("ne " + d + ", " + r + ", 0"); return d;
      case Y_CHAR: case Y_SCHAR: d = R(); E("sext8 " + d + ", " + r); return d; case Y_UCHAR: d = R(); E("zext8 " + d + ", " + r); return d;
      case Y_SHORT: d = R(); E("sext16 " + d + ", " + r); return d; case Y_USHORT: d = R(); E("zext16 " + d + ", " + r); return d;
      case Y_INT: d = R(); E("sext32 " + d + ", " + r); return d; case Y_UINT: d = R(); E("zext32 " + d + ", " + r); return d;
      case Y_FLOAT: d = R(); E("rnd_f32 " + d + ", " + immReg(r)); return d;
      default: return r;
    }
  }
  V pr(int t, const string& r) { V v; v.ty = t; v.cat = C_PR; v.reg = r; return v; }
  V constInt(int t, int64_t val) { V v = pr(t, num(foldNorm(val, t))); v.ci = true; v.cval = foldNorm(val, t); return v; }
  void addEff(V& v, const V& o) { v.eff.insert(v.eff.end(), o.eff.begin(), o.eff.end()); }
  // sirasiz (unsequenced) degerlendirme denetimi (02.034): iki operanddan biri ayni nesneyi degistirir, digeri okur/degistirirse tanimsiz davranis
  vector<Eff> effRead(const V& v) { vector<Eff> r = v.eff; if (v.cat == C_LV && !v.key.empty() && !isCls(v.ty) && !isArr(v.ty)) r.push_back({v.key, 1}); return r; }
  void unseq(const V& x0, const V& y0) {
    vector<Eff> xe = effRead(x0), ye = effRead(y0);
    for (auto& e : xe) for (auto& f : ye) if (!e.key.empty() && e.key == f.key && (e.rw == 2 || f.rw == 2)) err("E_UNSEQ", "ayni nesne '" + e.key.substr(e.key.find('#') + 1) + "' siralanmamis iki degerlendirmede degistiriliyor/okunuyor (tanimsiz davranis)");
  }
  // ---- sinif / uye / yontem sorgulari ----
  int findFld(int ci, const string& n, int& woff, int& fty) {
    for (int c = ci; c >= 0; c = C[size_t(c)].base) for (auto& f : C[size_t(c)].flds) if (f.name == n) { woff = f.woff; fty = f.ty; return c; }
    return -1;
  }
  // ---- yazmaca yukleme: lvalue -> prvalue, dizi -> isaretci ----
  V rval(V v) {
    if (isArr(v.ty)) { V r = pr(ptrTo(ty(v.ty).e), v.reg); r.eff = v.eff; return r; }   // dizi-isaretci bozunumu
    if (v.cat == C_PR || isCls(v.ty) || K(v.ty) == Y_VOID) { V r = v; r.ty = unq(v.ty); if (!isCls(v.ty)) { r.cat = C_PR; } return r; }
    V r = pr(unq(v.ty), ""); r.eff = v.eff;
    if (v.ci) { r.reg = num(v.cval); r.ci = true; r.cval = v.cval; if (!v.key.empty()) r.eff.push_back({v.key, 1}); return r; }
    r.reg = R(); E("ld " + r.reg + ", " + v.reg + ", 0"); if (!v.key.empty()) r.eff.push_back({v.key, 1});
    return r;
  }
  // ---- sayisal donusumler ----
  V convArith(V v, int to) {   // v prvalue arit/bool; to arit/bool
    int from = v.ty; to = unq(to);
    if ((K(from) == Y_LDOUBLE || K(to) == Y_LDOUBLE) && from != to && !g->dry) err("E_UNSUPPORTED", "long double donusumu desteklenmiyor (f64'e sessizce indirilmez)");
    if (from == to) return v;
    V r = pr(to, v.reg); r.eff = v.eff;
    if (isInt(from) && isInt(to)) {
      if (v.ci) { r.ci = true; r.cval = foldNorm(v.cval, to); r.reg = num(r.cval); return r; }
      int fb = bitsOf(from), tb = bitsOf(to); bool fs = isSigned(from), ts = isSigned(to);
      bool needs = K(to) == Y_BOOL || tb < 64;
      if (K(to) != Y_BOOL) {
        if (K(from) == Y_BOOL) needs = false;
        else if (fs == ts && tb >= fb) needs = false;
        else if (!fs && ts && tb > fb) needs = false;
        else if (tb == 64) needs = false;
      }
      if (needs) r.reg = normReg(v.reg, to);
      return r;
    }
    if (isInt(from) && isFlt(to)) {
      if (K(from) == Y_BOOL) { string t = R(); E("i2f " + t + ", " + v.reg); r.reg = t; return r; }
      string t = R(); E(string((K(from) == Y_ULONG || K(from) == Y_ULLONG) ? "u2f " : "i2f ") + t + ", " + v.reg); r.reg = K(to) == Y_FLOAT ? normReg(t, to) : t; return r;
    }
    if (isFlt(from) && isInt(to)) {
      if (K(to) == Y_BOOL) { string t = R(); E("feq " + t + ", " + immReg(v.reg) + ", 0.0"); string d = R(); E("eq " + d + ", " + t + ", 0"); r.reg = d; return r; }
      string t = R(); E(string((K(to) == Y_ULONG || K(to) == Y_ULLONG) ? "f2u " : "f2i ") + t + ", " + immReg(v.reg)); r.reg = bitsOf(to) < 64 ? normReg(t, to) : t; return r;
    }
    if (isFlt(from) && isFlt(to)) { if (K(to) == Y_FLOAT) r.reg = normReg(v.reg, to); return r; }
    err("E_TYPE", "sayisal donusum yok: " + tyName(from) + " -> " + tyName(to));
  }
  string boolReg(const V& v) {   // prvalue skaler -> 0/1 yazmaci
    if (K(v.ty) == Y_BOOL) return v.reg;
    string d = R();
    if (isFlt(v.ty)) { string t = R(); E("feq " + t + ", " + v.reg + ", 0.0"); E("eq " + d + ", " + t + ", 0"); }
    else E("ne " + d + ", " + v.reg + ", 0");
    return d;
  }
  // ---- standart donusum sirasi (asilim): -1 yok, 0 tam, 1 terfi, 2 donusum ----
  bool isNullConst(const V& v) { return v.ci && v.cval == 0 && isInt(v.ty) && v.cat != C_LV ? true : (v.ci && v.cval == 0 && isInt(v.ty) && v.nullLit); }
  int stdRank(int from, const V* v, int to) {   // from,to: referanssiz; siniflar disinda
    from = unq(from); to = unq(to);
    if (from == to) return 0;
    int fk = K(from), tk = K(to);
    if (isArith(from) && isArith(to)) {
      if (tk == Y_BOOL) return 2;
      if (tk == Y_INT && fk >= Y_BOOL && fk <= Y_USHORT) return 1;
      if (tk == Y_DOUBLE && fk == Y_FLOAT) return 1;
      return 2;
    }
    if (tk == Y_PTR) {
      if (fk == Y_NULLPTR) return 2;
      if (v && isInt(from) && v->ci && v->cval == 0 && v->nullLit) return 2;
      if (fk == Y_PTR) {
        int fe = ty(from).e, te = ty(to).e;
        if (unq(fe) == unq(te)) { const Ty &a1 = ty(fe), &b1 = ty(te); return ((!a1.c || b1.c) && (!a1.v || b1.v)) ? 0 : -1; }
        const Ty &a1 = ty(fe), &b1 = ty(te);
        if (K(te) == Y_VOID) return ((!a1.c || b1.c) && (!a1.v || b1.v)) ? 2 : -1;
        if (isCls(fe) && isCls(te) && derivedFrom(ty(fe).cls, ty(te).cls) && (!a1.c || b1.c) && (!a1.v || b1.v)) return 2;
        return -1;
      }
      return -1;
    }
    if (tk == Y_BOOL && (fk == Y_PTR || fk == Y_NULLPTR)) return 2;
    if (tk == Y_NULLPTR && v && isInt(from) && v->ci && v->cval == 0 && v->nullLit) return 2;
    return -1;
  }

  // ============================== NESNE KOPYALAMA / SIFIRLAMA ==============================
  void zeroN(const string& dst, int n) {
    if (n <= 8) { for (int i = 0; i < n; i++) E("st 0, " + dst + ", " + num(i)); return; }
    string i = R(), p = R(), c = R(), lb = NL(), en = NL();
    E("mov " + i + ", 0"); E("mov " + p + ", " + dst); LAB(lb); E("lt " + c + ", " + i + ", " + num(n)); E("jz " + c + ", " + en);
    E("st 0, " + p + ", 0"); E("add " + p + ", " + p + ", 1"); E("add " + i + ", " + i + ", 1"); E("jmp " + lb); LAB(en);
  }
  void copyN(const string& dst, const string& src, int from, int n) {   // sozcuk [from, n)
    if (n - from <= 8) { for (int i = from; i < n; i++) { string t = R(); E("ld " + t + ", " + src + ", " + num(i)); E("st " + t + ", " + dst + ", " + num(i)); } return; }
    string i = R(), c = R(), lb = NL(), en = NL(), pa = R(), pb = R(), t = R();
    E("add " + pa + ", " + dst + ", " + num(from)); E("add " + pb + ", " + src + ", " + num(from)); E("mov " + i + ", " + num(from));
    LAB(lb); E("lt " + c + ", " + i + ", " + num(n)); E("jz " + c + ", " + en); E("ld " + t + ", " + pb + ", 0"); E("st " + t + ", " + pa + ", 0");
    E("add " + pa + ", " + pa + ", 1"); E("add " + pb + ", " + pb + ", 1"); E("add " + i + ", " + i + ", 1"); E("jmp " + lb); LAB(en);
  }
  void setVptr(const string& addr, int ci) { if (C[size_t(ci)].poly) E("st " + num(C[size_t(ci)].id) + ", " + addr + ", 0"); }
  // dst sinifi dc, src sinifi sc (sc, dc'den turemis ya da ayni): dc'nin sozcuklerini kopyalar; yeni nesne icin vptr'yi dc'ye ayarlar
  void copyObj(const string& dst, const string& src, int dc, bool construct) {
    const Cls& c = C[size_t(dc)];
    copyN(dst, src, c.poly ? 1 : 0, c.words);
    if (construct) setVptr(dst, dc);
  }
  V tempObj(int clsT) {
    int ci = ty(clsT).cls; int off = slot(C[size_t(ci)].words); string a = slotAddr(off); setVptr(a, ci);
    V v = pr(unq(clsT), a); return v;
  }
  V tempScalar(int t, const string& valReg) {   // skaler degeri gecici bellege yaz, lvalue adresi dondur
    int off = slot(1); string a = slotAddr(off); E("st " + valReg + ", " + a + ", 0"); V v; v.ty = t; v.cat = C_LV; v.reg = a; return v;
  }

  // ============================== YONTEM / ISLEV ARAMA ==============================
  vector<int> methodsNamed(int ci, const string& n) {
    for (int c = ci; c >= 0; c = C[size_t(c)].base) { vector<int> r; for (int m : C[size_t(c)].meths) if (F[size_t(m)].name == n) r.push_back(m); if (!r.empty()) return r; }
    return {};
  }
  vector<int> convFuncsOf(int ci) {
    vector<int> r; for (int c = ci; c >= 0; c = C[size_t(c)].base) for (int m : C[size_t(c)].meths) if (F[size_t(m)].name == "operator conv") {
      bool hidden = false; for (int q : r) if (unq(F[size_t(q)].ret) == unq(F[size_t(m)].ret)) hidden = true;
      if (!hidden) r.push_back(m);
    }
    return r;
  }
  struct Rk { int r = 99, t = 0; bool ok = false; };
  // kullanici tanimli donusum: sinif nesnesinden 'to' turune. dondurur: islev ya da -1; belirsizse hata
  int pickUserConv(int clsT, const V* av, int to, bool allowExplicit, int& subRank) {
    to = unq(deref(to)); int best = -1; int bestR = 99; bool amb = false; int ci = ty(clsT).cls;
    for (int m : convFuncsOf(ci)) {
      Fn& f = F[size_t(m)]; if (f.explicitConv && !allowExplicit) continue;
      int rt = unq(deref(f.ret)); int r;
      if (isCls(rt) || isCls(to)) r = (rt == to) ? 0 : (isCls(rt) && isCls(to) && derivedFrom(ty(rt).cls, ty(to).cls)) ? 2 : -1;
      else r = stdRank(rt, nullptr, to);
      if (r < 0) continue;
      if (r < bestR) { best = m; bestR = r; amb = false; } else if (r == bestR) amb = true;
    }
    if (amb) err("E_AMBIG", "belirsiz kullanici donusumu: " + tyName(clsT) + " -> " + tyName(to));
    subRank = bestR; return best;
  }
  // arguman -> parametre sirasi
  Rk argRank(const V& a, int P) {
    Rk rk; int aTy = a.ty; bool lv = a.cat == C_LV;
    if (K(P) == Y_REF || K(P) == Y_RREF) {
      bool rr = K(P) == Y_RREF; int tgt = ty(P).e; int tu = unq(tgt); bool tc = ty(tgt).c; int au = unq(aTy); bool ac = ty(aTy).c;
      bool exactish = (au == tu) || (isCls(au) && isCls(tu) && derivedFrom(ty(au).cls, ty(tu).cls));
      if (isArr(au) && isArr(tu) && ty(au).n == ty(tu).n && unq(ty(au).e) == unq(ty(tu).e)) exactish = true;
      if (exactish) {
        if (ac && !tc) return rk;   // const nesne -> const olmayan referans yok
        if (!rr) {
          if (!tc && !lv) return rk;   // T& rvalue'ya baglanmaz
          rk.ok = true; rk.r = (au == tu) ? 0 : 2; rk.t = (tc ? 1 : 0) + (lv ? 0 : 2); return rk;
        }
        if (lv) return rk;   // T&& lvalue'ya baglanmaz
        rk.ok = true; rk.r = (au == tu) ? 0 : 2; rk.t = tc ? 1 : 0; return rk;
      }
      // donusum ile gecici olusturup baglama: yalniz const T& ve T&&
      if (!tc && !rr) return rk;
      V av = a; int r; V tmp = a;
      if (isCls(au)) {
        if (isCls(tu)) { int sr; int m = pickUserConv(au, &a, tu, false, sr); if (m < 0) return rk; r = 4; (void)m; }
        else { int sr; int m = pickUserConv(au, &a, tu, false, sr); if (m < 0) return rk; r = 4; }
      } else if (isCls(tu)) return rk;
      else { int sr = stdRank(isArr(au) ? ptrTo(ty(au).e) : au, &a, tu); if (sr < 0) return rk; r = sr; }
      rk.ok = true; rk.r = std::max(r, 1); rk.t = rr ? (tc ? 1 : 0) : 3; return rk;
    }
    // deger parametresi
    int pu = unq(P), au = unq(isArr(aTy) ? ptrTo(ty(aTy).e) : aTy);
    if (isCls(pu)) {
      if (au == pu) { rk.ok = true; rk.r = 0; return rk; }
      if (isCls(au) && derivedFrom(ty(au).cls, ty(pu).cls)) { rk.ok = true; rk.r = 2; return rk; }
      if (isCls(au)) { int sr; int m = pickUserConv(au, &a, pu, false, sr); if (m >= 0) { rk.ok = true; rk.r = 4; return rk; } }
      return rk;
    }
    if (isCls(au)) { int sr; int m = pickUserConv(au, &a, pu, false, sr); if (m >= 0) { rk.ok = true; rk.r = 4 + sr; return rk; } return rk; }
    int r = stdRank(au, &a, pu); if (r < 0) return rk; rk.ok = true; rk.r = r; return rk;
  }
  // aday secimi. self: ortuk nesne (yontem adaylari icin). -1: uygun yok, -2: belirsiz
  int resolve(const vector<int>& cands, const V* self, const vector<V>& args, vector<Rk>* chosen = nullptr) {
    struct CR { int fn; vector<Rk> rk; };
    vector<CR> ok;
    for (int ci : cands) {
      Fn& f = F[size_t(ci)];
      if (f.deleted && false) continue;
      if (f.params.size() != args.size()) continue;
      CR cr; cr.fn = ci; bool good = true;
      if (self && f.cls >= 0) {   // ortuk nesne parametresi
        bool objC = ty(self->ty).c; if (objC && !f.isConst) continue;
        Rk r; r.ok = true; r.r = derivedFrom(ty(unq(self->ty)).cls, f.cls) ? (unq(self->ty) == clsTy(f.cls) ? 0 : 2) : 99; r.t = (objC == f.isConst) ? 0 : 1;
        if (r.r == 99) continue; cr.rk.push_back(r);
      } else if (!self && f.cls >= 0) continue;
      for (size_t i = 0; i < args.size() && good; i++) { Rk r = argRank(args[i], f.params[i]); if (!r.ok) good = false; cr.rk.push_back(r); }
      if (good) ok.push_back(cr);
    }
    if (ok.empty()) return -1;
    int best = -1;
    for (size_t i = 0; i < ok.size(); i++) {
      bool isBest = true;
      for (size_t j = 0; j < ok.size() && isBest; j++) {
        if (i == j) continue;
        bool allLe = true, anyLt = false;
        for (size_t k = 0; k < ok[i].rk.size(); k++) {
          auto x = std::make_pair(ok[i].rk[k].r, ok[i].rk[k].t), y = std::make_pair(ok[j].rk[k].r, ok[j].rk[k].t);
          if (x > y) allLe = false; if (x < y) anyLt = true;
        }
        if (!(allLe && anyLt)) isBest = false;
      }
      if (isBest) { best = int(i); break; }
    }
    if (best < 0) {
      // tamamen esit adaylar: ayni imzalar -> belirsiz
      return -2;
    }
    if (chosen) *chosen = ok[size_t(best)].rk;
    return ok[size_t(best)].fn;
  }

  // ============================== ORTUK / KULLANICI DONUSUMLERI ==============================
  V userConvCall(V obj, int m) {   // obj sinif nesnesi; m donusum islevi
    vector<V> none; V r = callFn(m, &obj, none, false);
    return r;
  }
  // skaler hedefe (aritmetik/isaretci/bool) ortuk donusum. explicitCtx: acik (explicit) donusum islevlerine izin (baglamsal bool, static_cast)
  V toScalar(V v, int to, bool allowExplicit = false) {
    to = unq(to);
    if (isCls(unq(v.ty))) {
      int sr; int m = pickUserConv(unq(v.ty), &v, to, allowExplicit, sr);
      if (m < 0) err("E_CONV", "donusum yok: " + tyName(v.ty) + " -> " + tyName(to));
      V r = userConvCall(v, m); v = r;
    }
    v = rval(v);
    if (v.ty == to) return v;
    if (K(v.ty) == Y_VOID) err("E_CONV", "void deger kullanilamaz");
    int r = stdRank(v.ty, &v, to);
    if (r < 0) err("E_CONV", "ortuk donusum yok: " + tyName(v.ty) + " -> " + tyName(to));
    if (isArith(v.ty) && isArith(to)) return convArith(v, to);
    V o = pr(to, v.reg); o.eff = v.eff;
    if (K(to) == Y_BOOL) { o.reg = boolReg(v.ty == tNullptr ? pr(tLong, "0") : v); if (v.ci) { o.ci = true; o.cval = v.cval != 0; o.reg = num(o.cval); } return o; }
    if (K(v.ty) == Y_NULLPTR || (isInt(v.ty) && v.ci)) { o.reg = "0"; return o; }
    return o;   // isaretci donusumleri sozcuk adresini degistirmez (tek kalitim: taban alt nesnesi 0. sozcukte)
  }
  bool contextualBoolOk(int t) { t = unq(t); if (isScalar(t)) return true; if (isCls(t)) { int sr; try { return pickUserConv(t, nullptr, tBool, true, sr) >= 0; } catch (IErr&) { return false; } } return false; }
  V condBool(V v) {   // baglamsal bool donusumu
    if (isCls(unq(v.ty))) { return toScalar(v, tBool, true); }
    v = rval(v);
    if (!isScalar(v.ty)) err("E_TYPE", "kosul ifadesi skaler tur olmali: " + tyName(v.ty));
    if (K(v.ty) == Y_NULLPTR) { V o = pr(tBool, "0"); o.ci = true; o.cval = 0; return o; }
    return toScalar(v, tBool, true);
  }
  // sinif degeri (yeni nesne olarak): v ifadesinden 'to' sinifinin gecici nesnesi
  V toClassValue(V v, int to) {
    to = unq(to); int tc = ty(to).cls;
    if (isCls(unq(v.ty))) {
      int vc = ty(unq(v.ty)).cls;
      if (vc != tc && isOrd(unq(v.ty)) && isOrd(to) && ordConvertible(unq(v.ty), to)) { V t = tempObj(to); string w = R(); E("ld " + w + ", " + v.reg + ", 0"); E("st " + w + ", " + t.reg + ", 0"); t.eff = v.eff; return t; }
      if (vc == tc) {
        if (v.cat == C_PR) return v;   // kopya eksiltme: ayni gecici nesne
        V t = tempObj(to); copyObj(t.reg, v.reg, tc, true); t.eff = v.eff; return t;
      }
      if (derivedFrom(vc, tc)) { V t = tempObj(to); copyObj(t.reg, v.reg, tc, true); t.eff = v.eff; return t; }
      int sr; int m = pickUserConv(unq(v.ty), &v, to, false, sr);
      if (m >= 0) return toClassValue(userConvCall(v, m), to);
    }
    err("E_CONV", "ortuk donusum yok: " + tyName(v.ty) + " -> " + tyName(to));
  }
  // parametreye baglama: gecirilecek yazmaci dondurur
  string bindArg(V a, int P, vector<V>* effs = nullptr) {
    if (effs) effs->push_back(a);
    if (K(P) == Y_REF || K(P) == Y_RREF) {
      bool rr = K(P) == Y_RREF; int tgt = ty(P).e; int tu = unq(tgt); int au = unq(a.ty); bool tc = ty(tgt).c;
      bool direct = (au == tu) || (isCls(au) && isCls(tu) && derivedFrom(ty(au).cls, ty(tu).cls)) || (isArr(au) && isArr(tu) && ty(au).n == ty(tu).n && unq(ty(au).e) == unq(ty(tu).e));
      if (direct && !(ty(a.ty).c && !tc)) {
        if (a.cat == C_LV && rr) err("E_BIND", "rvalue referansi lvalue'ya baglanamaz");
        if (a.cat != C_LV && !tc && !rr) err("E_BIND", "const olmayan lvalue referansi rvalue'ya baglanamaz");
        if (a.cat == C_PR && !isCls(au)) { V t = tempScalar(au, a.reg); return t.reg; }
        return a.reg;   // sinif: nesne adresi; skaler lvalue/xvalue: adres
      }
      if (!tc && !rr) err("E_BIND", "const olmayan lvalue referansi icin " + tyName(a.ty) + " -> " + tyName(P) + " baglanamaz");
      // donusturup gecici olustur
      if (isCls(tu)) { V c = toClassValue(a, tu); return c.reg; }
      V s = toScalar(a, tu); V t = tempScalar(tu, s.reg); return t.reg;
    }
    int pu = unq(P);
    if (isCls(pu)) { V c = toClassValue(a, pu); return c.reg; }
    V s = toScalar(a, pu); return s.reg;
  }
  string retNameOf(int fi) { return F[size_t(fi)].pma; }
  V callFn(int fi, const V* self, vector<V>& args, bool allowVirt) {
    Fn& f = F[size_t(fi)];
    if (f.deleted) err("E_DELETED", "silinmis islev cagrildi: " + f.name);
    if (f.pure && !allowVirt) err("E_PURE", "saf sanal islev dogrudan cagrilamaz: " + f.name);
    ensureRet(fi); f.used = true;
    string selfReg; vector<string> regs; V res; V out;
    int retTy = f.ret; bool sret = isCls(unq(deref(retTy))) && !isRef(retTy);
    if (sret) { V t = tempObj(unq(retTy)); regs.push_back(t.reg); out = t; }
    if (f.cls >= 0) { if (!self) err("E_THIS", "uye islev nesnesiz cagrildi: " + f.name); regs.push_back(self->reg); }
    for (size_t i = 0; i < args.size(); i++) regs.push_back(bindArg(args[i], f.params[i]));
    if (!f.noex) g->mayThrow = true;
    string d = R(); bool isVoid = K(retTy) == Y_VOID;
    string target = f.pma; bool virt = f.isVirt && allowVirt && self && f.cls >= 0;
    if (virt) target = "vd" + std::to_string(f.vroot);
    if (!f.builtin.empty()) {
      if (f.builtin == "getc") { E("host " + d + ", getc"); return pr(tInt, d); }
      E("host " + f.builtin + ", " + regs[0]);
      V v; v.ty = tVoid; v.isVoid = true; return v;
    }
    string line = "call " + (isVoid || sret ? string() : d + ", ") + target; for (auto& r : regs) line += ", " + r;
    if (isVoid || sret) { E(line); }
    else E(line);
    for (auto& a : args) addEff(res, a); if (self) addEff(res, *self);
    if (isVoid) { res.ty = tVoid; res.isVoid = true; return res; }
    if (sret) { out.eff = res.eff; return out; }
    if (isRef(retTy)) { res.ty = ty(retTy).e; res.cat = K(retTy) == Y_REF ? C_LV : C_XV; res.reg = d; res.viaPtr = true; return res; }
    res.ty = unq(retTy); res.cat = C_PR; res.reg = d;
    if (!isScalar(res.ty) && !isCls(res.ty)) err("E_TYPE", "islev bu turu donduremez: " + tyName(retTy));
    return res;
  }

  // ============================== TURLER (AST -> tur) ==============================
  int typeOf(int n) {
    const Node& x = a[n];
    switch (x.k) {
      case N_TY: {
        if (x.i == Y_FLOAT128) fail("E_UNSUPPORTED", "__float128 yalniz sayisal derleyicide (ifx.pmb) tanimli; bu C++ referans yolu f128 degeri uretmez", x.line);
        Ty t; t.k = int(x.i); t.c = x.j & 1; t.v = x.j & 2;
        if (t.k == Y_CLASS) { auto it = cmap.find(x.s); if (it == cmap.end()) fail("E_TYPE", "bilinmeyen sinif: " + x.s, x.line); t.cls = it->second; }
        return intern(t);
      }
      case N_TPTR: { Ty t; t.k = Y_PTR; t.e = typeOf(a[n].kid[0]); t.c = x.j & 1; t.v = x.j & 2; return intern(t); }
      case N_TREF: return lref(typeOf(x.kid[0]));
      case N_TRREF: return rref(typeOf(x.kid[0]));
      case N_TARR: {
        int e = typeOf(x.kid[0]);
        if (x.kid.size() < 2) { Ty t; t.k = Y_ARR; t.e = e; t.n = -1; return intern(t); }
        int64_t cnt = constIntExpr(x.kid[1]);
        if (cnt <= 0) fail("E_TYPE", "dizi boyutu pozitif olmali", x.line);
        if (cnt > (int64_t(1) << 24) || cnt * std::max(1, words(e)) > (int64_t(1) << 24)) fail("E_TYPE", "dizi cok buyuk (en fazla 16M sozcuk)", x.line);
        if (!completeObj(e)) fail("E_TYPE", "dizi eleman turu tam degil", x.line);
        if (isRef(e)) fail("E_TYPE", "referans dizisi gecersiz", x.line);
        return arrOf(e, cnt);
      }
    }
    fail("E_TYPE", "tur dugumu bekleniyordu", x.line);
  }
  Var* findVar(const string& n) {
    for (size_t i = g->scopes.size(); i-- > 0;) { auto it = g->scopes[i].find(n); if (it != g->scopes[i].end()) return &it->second; }
    auto it = gmap.find(n); if (it != gmap.end()) return &globals[size_t(it->second)];
    return nullptr;
  }
  V varRef(Var& v) {
    V r; r.ty = v.ty; r.cat = C_LV; r.key = "v" + std::to_string(v.uid) + "#" + v.name;
    if (v.global) { r.reg = R(); E("mov " + r.reg + ", " + v.sym); }
    else if (v.isRef) { r.reg = R(); E("ld " + r.reg + ", fp, " + num(v.off)); r.viaPtr = true; r.key.clear(); }
    else r.reg = slotAddr(v.off);
    if (v.ci) { r.ci = true; r.cval = v.cval; }
    return r;
  }
  string strData(const string& s) {
    string nm = "s" + std::to_string(strn++); string line = "data " + nm + " = ";
    for (size_t i = 0; i < s.size(); i++) line += num((signed char)s[i]) + ", "; line += "0";
    dataLines.push_back(line); return nm;
  }
  // ============================== YARDIMCI ISLEMLER ==============================
  int promote(int t) { t = unq(t); int k = K(t); if (k >= Y_BOOL && k <= Y_USHORT) return tInt; return t; }
  int uac(int x, int y) {   // olagan aritmetik donusumler
    x = unq(x); y = unq(y);
    if (K(x) == Y_LDOUBLE || K(y) == Y_LDOUBLE) { if (!g->dry) err("E_UNSUPPORTED", "long double aritmetigi desteklenmiyor (f80 komutlariyla gercek 80 bit islem bu on yuzde yok; f64'e sessizce indirilmez)"); return tLDouble; } if (K(x) == Y_DOUBLE || K(y) == Y_DOUBLE) return tDouble; if (K(x) == Y_FLOAT || K(y) == Y_FLOAT) return tFloat;
    x = promote(x); y = promote(y);
    if (x == y) return x;
    bool sx = isSigned(x), sy = isSigned(y); int rx = rankOf(x), ry = rankOf(y);
    if (sx == sy) return rx >= ry ? x : y;
    int u = sx ? y : x, s = sx ? x : y; int ru = rankOf(u), rs = rankOf(s);
    if (ru >= rs) return u;
    if (bitsOf(s) > bitsOf(u)) return s;
    // isaretli tur isaretsizin tum degerlerini tutamaz -> isaretlinin isaretsiz karsiligi
    switch (K(s)) { case Y_LONG: return tULong; case Y_LLONG: return tULLong; default: return tUInt; }
  }
  bool foldBin(const string& op, int t, int64_t x, int64_t y, int64_t& out) {
    bool uns = isUnsigned(t) && bitsOf(t) == 64; uint64_t ux = uint64_t(x), uy = uint64_t(y);
    if (op == "+") out = int64_t(ux + uy); else if (op == "-") out = int64_t(ux - uy); else if (op == "*") out = int64_t(ux * uy);
    else if (op == "/") { if (y == 0) return false; if (uns) out = int64_t(ux / uy); else { if (x == INT64_MIN && y == -1) return false; out = x / y; } }
    else if (op == "%") { if (y == 0) return false; if (uns) out = int64_t(ux % uy); else { if (x == INT64_MIN && y == -1) return false; out = x % y; } }
    else if (op == "&") out = x & y; else if (op == "|") out = x | y; else if (op == "^") out = x ^ y;
    else if (op == "<<") out = int64_t(ux << (uy & 63)); else if (op == ">>") out = (isUnsigned(t)) ? int64_t(ux >> (uy & 63)) : (x >> (uy & 63));
    else return false;
    out = foldNorm(out, t); return true;
  }
  string immReg(const string& r) { if (r.empty() || isdigit((unsigned char)r[0]) || r[0] == '-') { string t = R(); E("mov " + t + ", " + r); return t; } return r; }
  // yaygin ikili aritmetik (iki rvalue, ayni tur t): isleci uret
  V arithOp(const string& op, V l, V r, int t) {
    V res = pr(t, ""); res.eff = l.eff; addEff(res, r);
    if (isInt(t)) {
      int64_t fv;
      if (l.ci && r.ci && foldBin(op, t, l.cval, r.cval, fv)) { res.ci = true; res.cval = fv; res.reg = num(fv); return res; }
      if ((op == "/" || op == "%") && r.ci && r.cval == 0 && l.ci) { /* sabit sifira bolme: calisma zamani tuzagi */ }
      string d = R(); bool u = isUnsigned(t) && bitsOf(t) == 64;
      if (op == "+") E("add " + d + ", " + l.reg + ", " + r.reg); else if (op == "-") E("sub " + d + ", " + l.reg + ", " + r.reg); else if (op == "*") E("mul " + d + ", " + l.reg + ", " + r.reg);
      else if (op == "/") E(string(u ? "divu " : "div ") + d + ", " + immReg(l.reg) + ", " + immReg(r.reg)); else if (op == "%") E(string(u ? "modu " : "mod ") + d + ", " + immReg(l.reg) + ", " + immReg(r.reg));
      else if (op == "&") E("and " + d + ", " + l.reg + ", " + r.reg); else if (op == "|") E("or " + d + ", " + l.reg + ", " + r.reg); else if (op == "^") E("xor " + d + ", " + l.reg + ", " + r.reg);
      else if (op == "<<") E("shl " + d + ", " + l.reg + ", " + r.reg); else if (op == ">>") E(string(isSigned(t) ? "sar " : "shr ") + d + ", " + l.reg + ", " + r.reg);
      else err("E_OP", "bilinmeyen ikili islec " + op);
      res.reg = (op == "&" || op == "|" || op == "^" || op == ">>") ? d : normReg(d, t);
      return res;
    }
    string d = R();
    if (op == "+") E("fadd " + d + ", " + l.reg + ", " + r.reg); else if (op == "-") E("fsub " + d + ", " + l.reg + ", " + r.reg);
    else if (op == "*") E("fmul " + d + ", " + l.reg + ", " + r.reg); else if (op == "/") E("fdiv " + d + ", " + l.reg + ", " + r.reg);
    else err("E_TYPE", "'" + op + "' kayan nokta turlerinde tanimli degil");
    res.reg = normReg(d, t); return res;
  }
  V cmpOp(const string& op, V l, V r, int t) {   // iki rvalue, ayni tur t -> bool
    V res = pr(tBool, ""); res.eff = l.eff; addEff(res, r);
    if (isInt(t) || isPtr(t)) {
      if (l.ci && r.ci) { int64_t x = l.cval, y = r.cval; bool u = isUnsigned(t) && bitsOf(t) == 64; bool b;
        if (op == "==") b = x == y; else if (op == "!=") b = x != y; else if (op == "<") b = u ? uint64_t(x) < uint64_t(y) : x < y; else if (op == "<=") b = u ? uint64_t(x) <= uint64_t(y) : x <= y;
        else if (op == ">") b = u ? uint64_t(x) > uint64_t(y) : x > y; else b = u ? uint64_t(x) >= uint64_t(y) : x >= y;
        res.ci = true; res.cval = b; res.reg = num(b); return res; }
      bool u = (isUnsigned(t) && bitsOf(t) == 64) || isPtr(t); string d = R();
      string o = op == "==" ? "eq" : op == "!=" ? "ne" : op == "<" ? (u ? "ltu" : "lt") : op == "<=" ? (u ? "leu" : "le") : op == ">" ? (u ? "gtu" : "gt") : (u ? "geu" : "ge");
      if (o == "gtu") E("ltu " + d + ", " + immReg(r.reg) + ", " + l.reg); else if (o == "geu") E("leu " + d + ", " + immReg(r.reg) + ", " + l.reg);
      else E(o + " " + d + ", " + l.reg + ", " + r.reg);
      res.reg = d; return res;
    }
    string d = R();
    if (op == "==") E("feq " + d + ", " + l.reg + ", " + r.reg);
    else if (op == "!=") { string t2 = R(); E("feq " + t2 + ", " + l.reg + ", " + r.reg); E("eq " + d + ", " + t2 + ", 0"); }
    else E(string(op == "<" ? "flt " : op == "<=" ? "fle " : op == ">" ? "fgt " : "fge ") + d + ", " + l.reg + ", " + r.reg);
    res.reg = d; return res;
  }

  // ============================== SIRALAMA SINIFLARI (<=>) ==============================
  int ordCls(int kind) { return cmap[kind == 1 ? "std::strong_ordering" : kind == 2 ? "std::weak_ordering" : "std::partial_ordering"]; }
  bool isOrd(int t) { t = unq(t); return isCls(t) && C[size_t(ty(t).cls)].builtin >= 1 && C[size_t(ty(t).cls)].builtin <= 3; }
  V ordObj(int kind, const string& valReg) {
    V t = tempObj(clsTy(ordCls(kind))); E("st " + valReg + ", " + t.reg + ", 0"); return t;
  }
  // siralama nesnesi <op> 0
  V ordCmpZero(const string& op, V ord) {
    string v = R(); E("ld " + v + ", " + ord.reg + ", 0"); string d = R();
    if (op == "<") E("eq " + d + ", " + v + ", -1"); else if (op == "<=") E("le " + d + ", " + v + ", 0"); else if (op == ">") E("eq " + d + ", " + v + ", 1");
    else if (op == ">=") E("ltu " + d + ", " + v + ", 2"); else if (op == "==") E("eq " + d + ", " + v + ", 0"); else if (op == "!=") E("ne " + d + ", " + v + ", 0");
    else err("E_OP", "siralama nesnesi icin gecersiz islec " + op);
    V r = pr(tBool, d); r.eff = ord.eff; return r;
  }
  V builtinSpaceship(V l, V r) {   // iki rvalue skaler
    int t;
    if (isArith(l.ty) && isArith(r.ty)) t = uac(l.ty, r.ty);
    else if (isPtr(l.ty) || isPtr(r.ty) || K(l.ty) == Y_NULLPTR || K(r.ty) == Y_NULLPTR) {
      if (isPtr(l.ty) && isPtr(r.ty) && unq(l.ty) != unq(r.ty)) { if (stdRank(r.ty, &r, l.ty) >= 0) t = unq(l.ty); else if (stdRank(l.ty, &l, r.ty) >= 0) t = unq(r.ty); else err("E_TYPE", "<=> farkli isaretci turleri"); }
      else t = isPtr(l.ty) ? unq(l.ty) : unq(r.ty);
    } else err("E_TYPE", "<=> bu turler icin tanimli degil");
    l = toScalar(l, t); r = toScalar(r, t);
    bool flt = isFlt(t); string res;
    if (!flt) {
      bool u = (isUnsigned(t) && bitsOf(t) == 64) || isPtr(t); string g1 = R(), l1 = R(); res = R();
      if (u) E("ltu " + g1 + ", " + immReg(r.reg) + ", " + l.reg); else E("gt " + g1 + ", " + immReg(l.reg) + ", " + r.reg); E(string(u ? "ltu " : "lt ") + l1 + ", " + l.reg + ", " + r.reg); E("sub " + res + ", " + g1 + ", " + l1);
    } else {
      string g1 = R(), l1 = R(), r0 = R(), e = R(), z = R(), nz = R(), un = R(), u2 = R(); res = R();
      E("fgt " + g1 + ", " + l.reg + ", " + r.reg); E("flt " + l1 + ", " + l.reg + ", " + r.reg); E("sub " + r0 + ", " + g1 + ", " + l1);
      E("feq " + e + ", " + l.reg + ", " + r.reg); E("eq " + z + ", " + r0 + ", 0"); E("eq " + nz + ", " + e + ", 0"); E("and " + un + ", " + z + ", " + nz); E("mul " + u2 + ", " + un + ", 2"); E("add " + res + ", " + r0 + ", " + u2);
    }
    V o = ordObj(flt ? 3 : 1, res); o.eff = l.eff; addEff(o, r); return o;
  }
  // iki siralama sinifi arasi (strong -> weak -> partial)
  bool ordConvertible(int from, int to) { int a1 = C[size_t(ty(unq(from)).cls)].builtin, b1 = C[size_t(ty(unq(to)).cls)].builtin; return a1 >= 1 && a1 <= 3 && b1 >= 1 && b1 <= 3 && a1 <= b1; }

  // ============================== KULLANICI OPERATORLERI ==============================
  V noV() { V v; v.ty = -1; return v; }
  V userOp(const string& name, vector<V>& ops, bool allowReversed) {
    string fnName = "operator" + name;
    auto attempt = [&](vector<V>& o) -> V {
      vector<int> mc, fc;
      if (isCls(unq(o[0].ty))) mc = methodsNamed(ty(unq(o[0].ty)).cls, fnName);
      auto it = fmap.find(fnName); if (it != fmap.end()) fc = it->second;
      int mr = -1, fr = -1; vector<V> rest(o.begin() + 1, o.end());
      if (!mc.empty()) mr = resolve(mc, &o[0], rest);
      if (!fc.empty()) fr = resolve(fc, nullptr, o);
      if (mr == -2 || fr == -2) err("E_AMBIG", "belirsiz operator cagrisi: operator" + name);
      if (mr >= 0 && fr >= 0) err("E_AMBIG", "uye ve serbest operator" + name + " ayni derecede uygun");
      if (mr >= 0) return callFn(mr, &o[0], rest, true);
      if (fr >= 0) { vector<V> a2 = o; return callFn(fr, nullptr, a2, false); }
      return noV();
    };
    V r = attempt(ops);
    if (r.ty >= 0 || r.isVoid) return r;
    if (allowReversed && ops.size() == 2) { vector<V> rv = {ops[1], ops[0]}; return attempt(rv); }
    return noV();
  }

  // ============================== IFADE URETIMI ==============================
  V unwrapConv(V v) {   // sinif -> skaler (yerlesik operatorun ihtiyaci): tek acik olmayan donusum islevi
    vector<int> cands; for (int m : convFuncsOf(ty(unq(v.ty)).cls)) { Fn& f = F[size_t(m)]; if (!f.explicitConv && isScalar(unq(deref(f.ret)))) cands.push_back(m); }
    if (cands.empty()) err("E_NOVIABLE", "operator icin uygun operand yok: " + tyName(v.ty) + " (kullanici donusumu yok)");
    if (cands.size() > 1) err("E_AMBIG", "belirsiz kullanici donusumu: " + tyName(v.ty) + " birden fazla skalere donusebilir");
    return rval(userConvCall(v, cands[0]));
  }
  V genLogical(int n, bool isAnd) {
    const Node& x = a[n];
    V l = condBool(gen(x.kid[0])); string res = R();
    if (l.ci) {
      int cur = isAnd ? l.cval != 0 : l.cval == 0;   // sag taraf degerlendirilecek mi
      if (!cur) { g->dry++; V r = condBool(gen(x.kid[1])); g->dry--; (void)r; V o = pr(tBool, num(isAnd ? 0 : 1)); o.ci = true; o.cval = isAnd ? 0 : 1; o.eff = l.eff; return o; }
      V r = condBool(gen(x.kid[1])); V o = r; o.cat = C_PR; addEff(o, l); return o;
    }
    string end = NL(); E("mov " + res + ", " + l.reg); E((isAnd ? "jz " : "jnz ") + res + ", " + end);
    V r = condBool(gen(x.kid[1])); E("mov " + res + ", " + r.reg); LAB(end);
    V o = pr(tBool, res); o.eff = l.eff; addEff(o, r); return o;
  }
  V builtinBin(const string& op, V l, V r) {   // iki rvalue skaler
    bool rel = op == "<" || op == ">" || op == "<=" || op == ">=", eq = op == "==" || op == "!=";
    if (K(l.ty) == Y_VOID || K(r.ty) == Y_VOID) err("E_TYPE", "void deger islecte kullanilamaz");
    if (op == "<=>") return builtinSpaceship(l, r);
    bool lp = isPtr(l.ty) || K(l.ty) == Y_NULLPTR, rp = isPtr(r.ty) || K(r.ty) == Y_NULLPTR;
    if (lp || rp) {
      if (rel || eq) {
        int t;
        if (isPtr(l.ty) && isPtr(r.ty)) { if (unq(l.ty) == unq(r.ty)) t = unq(l.ty); else if (stdRank(r.ty, &r, l.ty) >= 0) t = unq(l.ty); else if (stdRank(l.ty, &l, r.ty) >= 0) t = unq(r.ty); else err("E_TYPE", "isaretci karsilastirmasi: uyumsuz turler " + tyName(l.ty) + " ve " + tyName(r.ty)); }
        else if (isPtr(l.ty)) { if (!eq && K(r.ty) != Y_NULLPTR) err("E_TYPE", "isaretci ile tamsayi siralanamaz"); if (stdRank(r.ty, &r, l.ty) < 0) err("E_TYPE", "isaretci ile " + tyName(r.ty) + " karsilastirilamaz"); t = unq(l.ty); }
        else if (isPtr(r.ty)) { if (stdRank(l.ty, &l, r.ty) < 0) err("E_TYPE", "isaretci ile " + tyName(l.ty) + " karsilastirilamaz"); t = unq(r.ty); }
        else t = tLong;   // nullptr == nullptr
        if (K(l.ty) == Y_NULLPTR) l = pr(t, "0"); if (K(r.ty) == Y_NULLPTR) r = pr(t, "0");
        l = toScalar(l, t); r = toScalar(r, t); if (K(l.ty) == Y_LONG && l.ci) {}
        return cmpOp(op, l, r, t);
      }
      if (op == "+" || op == "-") {
        if (op == "-" && isPtr(l.ty) && isPtr(r.ty)) {
          if (unq(ty(l.ty).e) != unq(ty(r.ty).e)) err("E_TYPE", "isaretci farki: farkli turler");
          int et = ty(l.ty).e; if (!completeObj(et)) err("E_PTRARITH", "tamamlanmamis/void turde isaretci aritmetigi");
          string d = R(); E("sub " + d + ", " + l.reg + ", " + r.reg); int w = words(et);
          if (w != 1) { string q = R(); E("div " + q + ", " + d + ", " + num(w)); d = q; }
          V o = pr(tLong, d); o.eff = l.eff; addEff(o, r); return o;
        }
        V p = isPtr(l.ty) ? l : r, i = isPtr(l.ty) ? r : l;
        if (!isPtr(p.ty) || !isInt(i.ty)) err("E_TYPE", "isaretci aritmetigi: " + tyName(l.ty) + " " + op + " " + tyName(r.ty));
        if (op == "-" && !isPtr(l.ty)) err("E_TYPE", "tamsayi - isaretci gecersiz");
        int et = ty(p.ty).e; if (!completeObj(et)) err("E_PTRARITH", "tamamlanmamis/void turde isaretci aritmetigi (" + tyName(p.ty) + ")");
        i = convArith(i, tLong); string off = i.reg; int w = words(et);
        if (w != 1) { off = R(); E("mul " + off + ", " + i.reg + ", " + num(w)); }
        string d = R(); E(string(op == "+" ? "add " : "sub ") + d + ", " + p.reg + ", " + off);
        V o = pr(unq(p.ty), d); o.eff = l.eff; addEff(o, r); return o;
      }
      err("E_TYPE", "'" + op + "' isaretcilerde tanimli degil");
    }
    if (!isArith(l.ty) || !isArith(r.ty)) err("E_TYPE", "'" + op + "' icin gecersiz operand turleri: " + tyName(l.ty) + ", " + tyName(r.ty));
    bool intOnly = op == "%" || op == "&" || op == "|" || op == "^" || op == "<<" || op == ">>";
    if (intOnly && (!isInt(l.ty) || !isInt(r.ty))) err("E_TYPE", "'" + op + "' yalniz tamsayi operandlarla kullanilir (" + tyName(l.ty) + ", " + tyName(r.ty) + ")");
    if (op == "<<" || op == ">>") {
      int t = promote(l.ty), rt = promote(r.ty); l = convArith(l, t); r = convArith(r, rt);
      if (r.ci) { if (r.cval < 0) err("E_SHIFT", "negatif kaydirma miktari"); if (r.cval >= bitsOf(t)) err("E_SHIFT", "kaydirma miktari tur genisligini asiyor (" + num(r.cval) + " >= " + num(bitsOf(t)) + ")"); }
      return arithOp(op, l, r, t);
    }
    int t = uac(l.ty, r.ty); l = convArith(l, t); r = convArith(r, t);
    if (rel || eq) return cmpOp(op, l, r, t);
    return arithOp(op, l, r, t);
  }
  V rvalS(V v) { return rval(v); }
  V genBinary(int n) {
    const Node& x = a[n]; const string& op = x.s; curLine = x.line;
    if (op == "&&") return genLogical(n, true);
    if (op == "||") return genLogical(n, false);
    V L = gen(x.kid[0]); V Rv = gen(x.kid[1]);
    if (op != "<<" && op != ">>") unseq(L, Rv);
    bool lc = isCls(unq(L.ty)), rc = isCls(unq(Rv.ty));
    if (lc || rc) {
      bool zeroL = Rv.nullLit && Rv.ci, zeroR = L.nullLit && L.ci;
      if (lc && isOrd(L.ty) && zeroL) return ordCmpZero(op, L);
      if (rc && isOrd(Rv.ty) && zeroR) { string sw = op == "<" ? ">" : op == ">" ? "<" : op == "<=" ? ">=" : op == ">=" ? "<=" : op; return ordCmpZero(sw, Rv); }
      if (lc && rc && unq(L.ty) == unq(Rv.ty) && (isOrd(L.ty) || C[size_t(ty(unq(L.ty)).cls)].builtin == 4) && (op == "==" || op == "!=")) {
        string p1 = R(), p2 = R(), d = R(); E("ld " + p1 + ", " + L.reg + ", 0"); E("ld " + p2 + ", " + Rv.reg + ", 0"); E(string(op == "==" ? "eq " : "ne ") + d + ", " + p1 + ", " + p2);
        V o = pr(tBool, d); o.eff = L.eff; addEff(o, Rv); return o;
      }
      vector<V> ops = {L, Rv};
      V r = userOp(op, ops, op == "==" || op == "!=");
      if (r.ty >= 0) return r;
      if (op == "!=") { V e = userOp("==", ops, true); if (e.ty >= 0) { V b = condBool(e); string d = R(); E("eq " + d + ", " + b.reg + ", 0"); V o = pr(tBool, d); o.eff = b.eff; return o; } }
      if (op == "<" || op == ">" || op == "<=" || op == ">=") {
        V s = userOp("<=>", ops, false);
        if (s.ty >= 0) { if (!isOrd(s.ty)) err("E_TYPE", "<=> sonucu siralama turu degil"); return ordCmpZero(op, s); }
        vector<V> rv = {Rv, L}; V s2 = userOp("<=>", rv, false);
        if (s2.ty >= 0) { string sw = op == "<" ? ">" : op == ">" ? "<" : op == "<=" ? ">=" : "<="; return ordCmpZero(sw, s2); }
      }
      // yerlesik operator: sinif operandlar kullanici donusumuyle skalere
      if (lc) L = unwrapConv(L); if (rc) Rv = unwrapConv(Rv);
      return builtinBin(op, rval(L), rval(Rv));
    }
    L = rval(L); Rv = rval(Rv);
    return builtinBin(op, L, Rv);
  }
  // atama hedefi degistirilebilir lvalue olmali
  void needModifiable(const V& v, const string& what) {
    if (v.cat != C_LV) err("E_LVALUE", what + ": sol operand degistirilebilir lvalue olmali (deger kategorisi: " + string(v.cat == C_XV ? "xvalue" : "prvalue") + ")");
    if (ty(v.ty).c) err("E_CONST", what + ": const nesneye atama");
    if (isArr(v.ty)) err("E_LVALUE", what + ": diziye atanamaz");
  }
  V storeScalar(V lv, V val) {   // lv lvalue, val prvalue (hedef turune donusturulmus)
    E("st " + val.reg + ", " + lv.reg + ", 0");
    V r = lv; r.eff = lv.eff; addEff(r, val); if (!lv.key.empty()) r.eff.push_back({lv.key, 2}); r.ci = false; return r;
  }
  V genAssign(int n) {
    const Node& x = a[n]; const string& op = x.s; curLine = x.line;
    // C++17: sag operand sol operanddan ONCE degerlendirilir
    if (a[x.kid[1]].k == N_INITLIST) {   // a = {..}
      V Lh = gen(x.kid[0]); needModifiable(Lh, "atama");
      if (isCls(unq(Lh.ty))) { V t = tempObj(unq(Lh.ty)); initObj(t.reg, unq(Lh.ty), x.kid[1], 3); copyN(Lh.reg, t.reg, C[size_t(ty(unq(Lh.ty)).cls)].poly ? 1 : 0, C[size_t(ty(unq(Lh.ty)).cls)].words); return Lh; }
      if (isScalar(Lh.ty)) { string tmp = R(); (void)tmp; initObj(Lh.reg, unq(Lh.ty), x.kid[1], 3); return Lh; }
      err("E_TYPE", "bu hedefe {..} atanamaz");
    }
    V Rv = gen(x.kid[1]); V Lv = gen(x.kid[0]);
    if (op == "=") {
      if (isCls(unq(Lv.ty))) {
        if (Lv.cat != C_LV) err("E_LVALUE", "atama: sol operand lvalue olmali");
        vector<V> ops = {Lv, Rv}; V u = userOp("=", ops, false); if (u.ty >= 0) return Lv;
        if (ty(Lv.ty).c) err("E_CONST", "atama: const nesneye atama");
        int dc = ty(unq(Lv.ty)).cls;
        V rc = isCls(unq(Rv.ty)) ? Rv : toClassValue(Rv, unq(Lv.ty));
        if (!isCls(unq(Rv.ty)) || !(ty(unq(Rv.ty)).cls == dc || derivedFrom(ty(unq(Rv.ty)).cls, dc))) { rc = toClassValue(Rv, unq(Lv.ty)); }
        if (C[size_t(dc)].builtin) { copyN(Lv.reg, rc.reg, 0, 1); } else copyN(Lv.reg, rc.reg, C[size_t(dc)].poly ? 1 : 0, C[size_t(dc)].words);
        V r = Lv; addEff(r, Rv); return r;
      }
      needModifiable(Lv, "atama");
      V val = toScalar(Rv, unq(Lv.ty)); return storeScalar(Lv, val);
    }
    string bop = op.substr(0, op.size() - 1);
    if (isCls(unq(Lv.ty))) {
      vector<V> ops = {Lv, Rv}; V u = userOp(op, ops, false); if (u.ty >= 0) return u;
      err("E_NOVIABLE", "operator" + op + " " + tyName(Lv.ty) + " icin tanimli degil");
    }
    needModifiable(Lv, "bilesik atama");
    if (isCls(unq(Rv.ty))) Rv = unwrapConv(Rv); else Rv = rval(Rv);
    V cur = pr(unq(Lv.ty), ""); cur.reg = R(); E("ld " + cur.reg + ", " + Lv.reg + ", 0"); cur.eff = Lv.eff; if (!Lv.key.empty()) cur.eff.push_back({Lv.key, 1});
    if (isPtr(Lv.ty) && !(bop == "+" || bop == "-")) err("E_TYPE", "isaretciye '" + op + "' uygulanamaz");
    V res = builtinBin(bop, cur, Rv);
    if (K(Lv.ty) == Y_BOOL || isPtr(Lv.ty)) res = toScalar(res, unq(Lv.ty)); else res = convArith(res, unq(Lv.ty));
    return storeScalar(Lv, res);
  }
  V genIncDec(int n) {
    const Node& x = a[n]; curLine = x.line; bool post = x.k == N_POSTINC; bool inc = x.s == "++";
    V Lv = gen(x.kid[0]);
    if (isCls(unq(Lv.ty))) {
      vector<V> ops = {Lv}; if (post) { ops.push_back(constInt(tInt, 0)); }
      V u = userOp(x.s, ops, false); if (u.ty >= 0) return u; err("E_NOVIABLE", "operator" + x.s + " " + tyName(Lv.ty) + " icin tanimli degil");
    }
    needModifiable(Lv, string("'") + x.s + "'");
    int t = unq(Lv.ty);
    if (K(t) == Y_BOOL) err("E_TYPE", "bool uzerinde " + x.s + " gecersiz (C++17)");
    if (!isArith(t) && !isPtr(t)) err("E_TYPE", x.s + " yalniz aritmetik/isaretci turlerinde: " + tyName(t));
    V old = pr(t, R()); E("ld " + old.reg + ", " + Lv.reg + ", 0");
    V nv;
    if (isPtr(t)) nv = builtinBin(inc ? "+" : "-", old, constInt(tInt, 1));
    else if (isFlt(t)) { V one = pr(t, "1.0"); nv = arithOp(inc ? "+" : "-", old, one, t); }
    else { V one = constInt(promote(t), 1); V o2 = convArith(old, promote(t)); V r2 = arithOp(inc ? "+" : "-", o2, one, promote(t)); nv = convArith(r2, t); }
    E("st " + nv.reg + ", " + Lv.reg + ", 0");
    V res;
    if (post) { res = old; res.cat = C_PR; res.eff = Lv.eff; res.eff.push_back({Lv.key, 2}); }
    else { res = Lv; res.eff.push_back({Lv.key, 2}); }
    if (Lv.key.empty()) { res.eff.erase(std::remove_if(res.eff.begin(), res.eff.end(), [](const Eff& e) { return e.key.empty(); }), res.eff.end()); }
    return res;
  }

  V genUnary(int n) {
    const Node& x = a[n]; const string& op = x.s; curLine = x.line;
    V v = gen(x.kid[0]);
    if (op == "&") {
      if (isCls(unq(v.ty))) { vector<V> ops = {v}; V u = userOp("&", ops, false); if (u.ty >= 0) return u; }
      if (v.cat != C_LV) err("E_ADDR", string("adresi alinamaz: ifade ") + (v.cat == C_XV ? "xvalue" : "prvalue") + " (lvalue gerekli)");
      V r = pr(ptrTo(v.ty), v.reg); r.eff = v.eff; return r;
    }
    if (op == "*") {
      if (isCls(unq(v.ty))) { vector<V> ops = {v}; V u = userOp("*", ops, false); if (u.ty >= 0) return u; v = unwrapConv(v); }
      v = rval(v);
      if (!isPtr(v.ty)) err("E_TYPE", "'*' isaretci operand gerektirir, bulunan " + tyName(v.ty));
      int pt = ty(v.ty).e; if (K(pt) == Y_VOID) err("E_TYPE", "void isaretcisi dolayli erisilemez");
      nullChk(v);
      V r; r.ty = pt; r.cat = C_LV; r.reg = v.reg; r.viaPtr = true; r.eff = v.eff; return r;
    }
    if (isCls(unq(v.ty))) {
      vector<V> ops = {v}; V u = userOp(op, ops, false); if (u.ty >= 0) return u;
      if (op == "!") { V b = condBool(v); string d = R(); E("eq " + d + ", " + b.reg + ", 0"); V o = pr(tBool, d); o.eff = b.eff; return o; }
      v = unwrapConv(v);
    }
    v = rval(v);
    if (op == "!") { V b = condBool(v); if (b.ci) { V o = constInt(tBool, !b.cval); o.eff = v.eff; return o; } string d = R(); E("eq " + d + ", " + b.reg + ", 0"); V o = pr(tBool, d); o.eff = v.eff; return o; }
    if (op == "-" || op == "+") {
      if (!isArith(v.ty)) err("E_TYPE", "'" + op + "' aritmetik operand gerektirir: " + tyName(v.ty));
      if (op == "+") { int t = isInt(v.ty) ? promote(v.ty) : unq(v.ty); return convArith(v, t); }
      if (isFlt(v.ty)) { string d = R(); E("fneg " + d + ", " + v.reg); V o = pr(unq(v.ty), d); o.eff = v.eff; return o; }
      int t = promote(v.ty); v = convArith(v, t);
      if (v.ci) { V o = constInt(t, int64_t(0 - uint64_t(v.cval))); o.eff = v.eff; return o; }
      string d = R(); E("neg " + d + ", " + v.reg); V o = pr(t, normReg(d, t)); o.eff = v.eff; return o;
    }
    if (op == "~") {
      if (!isInt(v.ty)) err("E_TYPE", "'~' tamsayi operand gerektirir: " + tyName(v.ty));
      int t = promote(v.ty); v = convArith(v, t);
      if (v.ci) { V o = constInt(t, ~v.cval); o.eff = v.eff; return o; }
      string d = R(); E("not " + d + ", " + v.reg); V o = pr(t, normReg(d, t)); o.eff = v.eff; return o;
    }
    err("E_OP", "bilinmeyen tekli islec " + op);
  }
  V genId(int n) {
    const Node& x = a[n]; curLine = x.line; const string& nm = x.s;
    if (nm.find("::") != string::npos) {   // std::strong_ordering::less gibi siralama sabitleri
      size_t p = nm.rfind("::"); string cn = nm.substr(0, p), mem = nm.substr(p + 2);
      auto it = cmap.find(cn);
      if (it != cmap.end() && C[size_t(it->second)].builtin >= 1 && C[size_t(it->second)].builtin <= 3) {
        int kind = C[size_t(it->second)].builtin; int64_t val;
        if (mem == "less") val = -1; else if (mem == "greater") val = 1; else if (mem == "equal" && kind == 1) val = 0; else if (mem == "equivalent" && kind != 1) val = 0; else if (mem == "unordered" && kind == 3) val = 2;
        else err("E_UNDECL", "bilinmeyen siralama sabiti: " + nm);
        return ordObj(kind, num(val));
      }
      err("E_UNDECL", "bilinmeyen niteleyici ad: " + nm);
    }
    Var* v = findVar(nm);
    if (v) return varRef(*v);
    if (g->cls >= 0) {   // ortuk this->alan
      int wo, ft; int oc = findFld(g->cls, nm, wo, ft);
      if (oc >= 0) {
        Var* th = findVar("this"); V t = rval(varRef(*th));
        V r; r.ty = ft; r.cat = C_LV; r.viaPtr = true; r.reg = wo ? R() : t.reg; if (wo) E("add " + r.reg + ", " + t.reg + ", " + num(wo));
        if (ty(th->ty).e >= 0 && ty(ty(th->ty).e).c) r.ty = cv(ft, true, false);
        return r;
      }
    }
    err("E_UNDECL", "tanimsiz ad: " + nm);
  }
  V genMember(int n) {
    const Node& x = a[n]; curLine = x.line; bool arrow = x.k == N_ARROW;
    V o = gen(x.kid[0]); int clsT;
    if (arrow) {
      if (isCls(unq(o.ty))) { vector<V> ops = {o}; V u = userOp("->", ops, false); if (u.ty >= 0) o = u; }
      o = rval(o); if (!isPtr(o.ty) || !isCls(unq(ty(o.ty).e))) err("E_TYPE", "'->' sinif isaretcisi gerektirir, bulunan " + tyName(o.ty));
      nullChk(o);
      V l; l.ty = ty(o.ty).e; l.cat = C_LV; l.reg = o.reg; l.viaPtr = true; l.eff = o.eff; o = l;
    }
    clsT = unq(o.ty); if (!isCls(clsT)) err("E_TYPE", "'.' sinif nesnesi gerektirir, bulunan " + tyName(o.ty));
    int wo, ft; int oc = findFld(ty(clsT).cls, x.s, wo, ft);
    if (oc < 0) err("E_UNDECL", "'" + C[size_t(ty(clsT).cls)].name + "' sinifinda '" + x.s + "' uyesi yok");
    V r; r.ty = ty(o.ty).c ? cv(ft, true, false) : ft; r.cat = o.cat == C_LV ? C_LV : C_XV; r.viaPtr = o.viaPtr; r.eff = o.eff;
    r.reg = wo ? R() : o.reg; if (wo) E("add " + r.reg + ", " + o.reg + ", " + num(wo));
    if (!o.key.empty()) r.key = o.key + "." + x.s;
    return r;
  }
  V genIndexV(int n) {
    const Node& x = a[n]; curLine = x.line;
    V b = gen(x.kid[0]); V i = gen(x.kid[1]);   // C++17: E1 once
    if (isCls(unq(b.ty))) { vector<V> ops = {b, i}; V u = userOp("[]", ops, false); if (u.ty >= 0) return u; b = unwrapConv(b); }
    if (isCls(unq(i.ty))) i = unwrapConv(i);
    bool bArr = isArr(b.ty), iArr = isArr(i.ty);
    if (!(bArr || isPtr(rval(b).ty) ) && (iArr || isPtr(i.ty))) std::swap(b, i), std::swap(bArr, iArr);   // i[a]
    V bp = rval(b); i = rval(i);
    if (!isPtr(bp.ty)) err("E_TYPE", "dizi/isaretci olmayan ifade indekslenemez: " + tyName(b.ty));
    if (!isInt(i.ty)) err("E_TYPE", "dizi indeksi tamsayi olmali, bulunan " + tyName(i.ty));
    int et = ty(bp.ty).e; if (!completeObj(et)) err("E_PTRARITH", "tamamlanmamis turde eleman indekslenemez");
    if (!bArr) nullChk(bp);
    int w = words(et); i = convArith(i, tLong);
    if (bArr) {   // bilinen sinirli dizi: calisma zamani sinir denetimi (bellek guvenligi)
      int64_t cnt = ty(b.ty).n;
      if (i.ci && (i.cval < 0 || i.cval >= cnt)) err("E_BOUNDS", "sabit dizi indeksi sinir disi: " + num(i.cval) + " (boyut " + num(cnt) + ")");
      if (!i.ci) { string c = R(), ok = NL(); E("ltu " + c + ", " + i.reg + ", " + num(cnt)); E("jnz " + c + ", " + ok); trapNow(); LAB(ok); }
    }
    string off = i.reg; if (w != 1) { off = R(); E("mul " + off + ", " + i.reg + ", " + num(w)); }
    string d = R(); E("add " + d + ", " + bp.reg + ", " + off);
    V r; r.ty = et; r.cat = C_LV; r.reg = d; r.viaPtr = true; r.eff = b.eff; addEff(r, i);
    if (bArr && ty(b.ty).c) r.ty = cv(et, true, false);
    return r;
  }
  V genCond(int n) {
    const Node& x = a[n]; curLine = x.line;
    V c = condBool(gen(x.kid[0]));
    // tur belirleme: iki kolu kuru calistir
    g->dry++; V A0 = gen(x.kid[1]); V B0 = gen(x.kid[2]); g->dry--;
    int at = unq(A0.ty), bt = unq(B0.ty); string res = R(); string els = NL(), end = NL();
    bool voidA = K(at) == Y_VOID, voidB = K(bt) == Y_VOID;
    if (voidA || voidB) { if (!(voidA && voidB)) err("E_TYPE", "?: kollarindan biri void, digeri degil"); E("jz " + c.reg + ", " + els); gen(x.kid[1]); E("jmp " + end); LAB(els); gen(x.kid[2]); LAB(end); V v; v.ty = tVoid; v.isVoid = true; return v; }
    // ayni sinif ve ayni kategori lvalue -> lvalue
    if (isCls(at) && at == bt) {
      bool lv = A0.cat == C_LV && B0.cat == C_LV && ty(A0.ty).c == ty(B0.ty).c;
      E("jz " + c.reg + ", " + els); V a1 = gen(x.kid[1]);
      if (lv) { E("mov " + res + ", " + a1.reg); } else { V t = toClassValue(a1, at); E("mov " + res + ", " + t.reg); }
      E("jmp " + end); LAB(els); V b1 = gen(x.kid[2]);
      if (lv) E("mov " + res + ", " + b1.reg); else { V t = toClassValue(b1, at); E("mov " + res + ", " + t.reg); }
      LAB(end);
      V r; r.ty = A0.ty; r.cat = lv ? C_LV : (A0.cat == C_XV && B0.cat == C_XV ? C_XV : C_PR); r.reg = res; r.viaPtr = true; addEff(r, c); return r;
    }
    if (isCls(at) || isCls(bt)) err("E_TYPE", "?: kollari uyumsuz sinif turleri: " + tyName(at) + ", " + tyName(bt));
    if (A0.cat == C_LV && B0.cat == C_LV && at == bt && !isArr(at)) {   // ayni tur lvalue'lar
      E("jz " + c.reg + ", " + els); V a1 = gen(x.kid[1]); E("mov " + res + ", " + a1.reg); E("jmp " + end); LAB(els); V b1 = gen(x.kid[2]); E("mov " + res + ", " + b1.reg); LAB(end);
      V r; r.ty = A0.ty; r.cat = C_LV; r.reg = res; r.viaPtr = true; addEff(r, c); return r;
    }
    g->dry++; V Ar = rval(A0), Br = rval(B0); g->dry--; int rt;
    if (isArith(Ar.ty) && isArith(Br.ty)) rt = Ar.ty == Br.ty ? Ar.ty : uac(Ar.ty, Br.ty);
    else if (isPtr(Ar.ty) && isPtr(Br.ty)) { if (Ar.ty == Br.ty) rt = Ar.ty; else if (stdRank(Br.ty, &Br, Ar.ty) >= 0) rt = Ar.ty; else if (stdRank(Ar.ty, &Ar, Br.ty) >= 0) rt = Br.ty; else err("E_TYPE", "?: isaretci turleri uyumsuz"); }
    else if (isPtr(Ar.ty) && (K(Br.ty) == Y_NULLPTR || (B0.nullLit && B0.ci))) rt = Ar.ty;
    else if (isPtr(Br.ty) && (K(Ar.ty) == Y_NULLPTR || (A0.nullLit && A0.ci))) rt = Br.ty;
    else if (K(Ar.ty) == Y_NULLPTR && K(Br.ty) == Y_NULLPTR) rt = tNullptr;
    else err("E_TYPE", "?: kollari uyumsuz: " + tyName(Ar.ty) + ", " + tyName(Br.ty));
    E("jz " + c.reg + ", " + els); V a1 = toScalar(gen(x.kid[1]), rt); E("mov " + res + ", " + a1.reg); E("jmp " + end); LAB(els);
    V b1 = toScalar(gen(x.kid[2]), rt); E("mov " + res + ", " + b1.reg); LAB(end);
    V r = pr(rt, res); addEff(r, c); addEff(r, a1); addEff(r, b1);
    if (c.ci && a1.ci && b1.ci) { r.ci = true; r.cval = c.cval ? a1.cval : b1.cval; }
    return r;
  }

  // ---------------- cagri ----------------
  V genCall(int n) {
    const Node& x = a[n]; curLine = x.line; int cn = x.kid[0]; const Node& c = a[cn];
    vector<int> cands; V self; bool haveSelf = false; bool allowVirt = true; V calleeObj; bool opCall = false;
    // 1) cagrilan ifade: islev adi / uye / nesne
    if (c.k == N_ID && c.s.find("::") == string::npos) {
      Var* var = findVar(c.s);
      if (var && isCls(unq(var->ty))) { calleeObj = varRef(*var); opCall = true; }
      else if (var) err("E_TYPE", "'" + c.s + "' cagrilabilir degil");
      else {
        if (g->cls >= 0) { vector<int> mc = methodsNamed(g->cls, c.s); if (!mc.empty()) { cands = mc; Var* th = findVar("this"); V t = rval(varRef(*th)); self.ty = ty(t.ty).e; self.cat = C_LV; self.reg = t.reg; self.viaPtr = true; haveSelf = true; } }
        if (cands.empty()) { auto it = fmap.find(c.s); if (it != fmap.end()) cands = it->second; }
        if (cands.empty()) err("E_UNDECL", "tanimsiz islev: " + c.s);
      }
    } else if (c.k == N_MEMBER || c.k == N_ARROW) {
      V o = gen(c.kid[0]);
      if (c.k == N_ARROW) {
        if (isCls(unq(o.ty))) { vector<V> ops = {o}; V u = userOp("->", ops, false); if (u.ty >= 0) o = u; }
        o = rval(o); if (!isPtr(o.ty) || !isCls(unq(ty(o.ty).e))) err("E_TYPE", "'->' sinif isaretcisi gerektirir: " + tyName(o.ty));
        nullChk(o);
        V l; l.ty = ty(o.ty).e; l.cat = C_LV; l.reg = o.reg; l.viaPtr = true; l.eff = o.eff; o = l;
      }
      if (!isCls(unq(o.ty))) err("E_TYPE", "uye islev cagrisi icin sinif nesnesi gerekli: " + tyName(o.ty));
      int ocls = ty(unq(o.ty)).cls; int searchCls = ocls;
      if (C[size_t(ocls)].builtin == 4) {   // std::type_info::name()
        if (c.s != "name" || x.kid.size() != 1) err("E_UNDECL", "std::type_info yalniz name() uye islevini destekler");
        tiNameUsed = true; string id = R(), r = R(); E("ld " + id + ", " + o.reg + ", 0"); E("call " + r + ", tiname, " + id);
        V v = pr(ptrTo(cv(tChar, true, false)), r); v.eff = o.eff; return v;
      }
      if (!c.s2.empty()) { auto it = cmap.find(c.s2); if (it == cmap.end() || !derivedFrom(ocls, it->second)) err("E_UNDECL", "niteleyici sinif gecersiz: " + c.s2); searchCls = it->second; allowVirt = false; }
      cands = methodsNamed(searchCls, c.s);
      if (cands.empty()) {
        // alan nesnesi operator() ?
        int wo, ft; int oc = findFld(ocls, c.s, wo, ft);
        if (oc >= 0 && isCls(unq(ft))) { V f; f.ty = ft; f.cat = C_LV; f.reg = wo ? R() : o.reg; if (wo) E("add " + f.reg + ", " + o.reg + ", " + num(wo)); f.viaPtr = o.viaPtr; calleeObj = f; opCall = true; }
        else err("E_UNDECL", "'" + C[size_t(ocls)].name + "' sinifinda '" + c.s + "' islevi yok");
      } else { self = o; haveSelf = true; if (!o.viaPtr && o.cat != C_LV) {} if (!o.viaPtr) allowVirt = false; }
    } else {
      V o = gen(cn);
      if (isCls(unq(o.ty))) { calleeObj = o; opCall = true; } else err("E_TYPE", "cagrilabilir olmayan ifade: " + tyName(o.ty));
    }
    if (opCall) { cands = methodsNamed(ty(unq(calleeObj.ty)).cls, "operator()"); if (cands.empty()) err("E_NOVIABLE", "'" + tyName(calleeObj.ty) + "' icin operator() yok"); self = calleeObj; haveSelf = true; allowVirt = calleeObj.viaPtr; }
    // 2) argumanlar soldan saga
    vector<V> args; for (size_t i = 1; i < x.kid.size(); i++) { if (a[x.kid[i]].k == N_INITLIST) err("E_ARG", "suslu parantez listesi arguman olarak desteklenmiyor"); args.push_back(gen(x.kid[i])); }
    int fi = resolve(cands, haveSelf ? &self : nullptr, args);
    if (fi == -1) { string sig; for (auto& v : args) sig += (sig.empty() ? "" : ", ") + tyName(v.ty) + (v.cat == C_LV ? "&" : v.cat == C_XV ? "&&" : ""); err("E_NOVIABLE", "'" + F[size_t(cands[0])].name + "' icin uygun asiri yuklenmis islev yok (" + sig + ")"); }
    if (fi == -2) err("E_AMBIG", "belirsiz islev cagrisi: " + F[size_t(cands[0])].name);
    V r = callFn(fi, haveSelf ? &self : nullptr, args, allowVirt);
    if (haveSelf) { addEff(r, self); }
    return r;
  }

  // ---------------- donusumler ----------------
  bool castsAway(int from, int to) {   // from/to isaretci ya da referans hedefi zinciri: const/volatile kaybi
    while (true) {
      const Ty &f = ty(from), &t = ty(to);
      if (f.k != Y_PTR || t.k != Y_PTR) return false;
      int fe = f.e, te = t.e;
      if ((ty(fe).c && !ty(te).c) || (ty(fe).v && !ty(te).v)) return true;
      from = fe; to = te;
      if (ty(from).k != Y_PTR) return false;
    }
  }
  V staticCast(V v, int T) {
    if (K(T) == Y_VOID) { v.isVoid = false; V o; o.ty = tVoid; o.isVoid = true; o.eff = v.eff; return o; }
    if (isRef(T)) {
      bool rr = K(T) == Y_RREF; int tgt = ty(T).e; int tu = unq(tgt); int au = unq(v.ty);
      bool rel = (au == tu) || (isCls(au) && isCls(tu) && (derivedFrom(ty(au).cls, ty(tu).cls) || derivedFrom(ty(tu).cls, ty(au).cls)));
      if (rel) {
        if (ty(v.ty).c && !ty(tgt).c) err("E_CONSTCAST", "static_cast const'i kaldiramaz");
        if (!rr && v.cat != C_LV && !ty(tgt).c) err("E_BIND", "static_cast<T&>: lvalue gerekli");
        if (rr && v.cat == C_LV) { V o = v; o.cat = C_XV; o.ty = tgt; return o; }   // static_cast<T&&>(lvalue) = xvalue (std::move)
        if (v.cat == C_PR && !isCls(au)) { V t = tempScalar(au, v.reg); t.cat = rr ? C_XV : C_LV; t.ty = tgt; return t; }
        V o = v; o.ty = tgt; o.cat = rr ? C_XV : (v.cat == C_LV ? C_LV : C_LV); if (!rr && v.cat == C_PR) o.cat = C_LV; return o;
      }
      // donusturup gecici
      if (!rr && !ty(tgt).c) err("E_BIND", "static_cast<T&>: uyumsuz tur");
      V s = isCls(tu) ? toClassValue(v, tu) : toScalar(v, tu);
      if (!isCls(tu)) { V t = tempScalar(tu, s.reg); t.cat = rr ? C_XV : C_LV; t.ty = tgt; return t; }
      s.cat = rr ? C_XV : C_LV; s.ty = tgt; return s;
    }
    if (isCls(unq(T))) return toClassValue(v, T);
    if (isArith(T) || K(T) == Y_BOOL) { if (isCls(unq(v.ty))) return toScalar(v, T, true); V r = rval(v); if (!isArith(r.ty)) err("E_CAST", "static_cast: " + tyName(r.ty) + " -> " + tyName(T) + " gecersiz"); return convArith(r, T); }
    if (isPtr(T)) {
      if (isCls(unq(v.ty))) return toScalar(v, T, true);
      V r = rval(v);
      if (K(r.ty) == Y_NULLPTR || (isInt(r.ty) && r.ci && r.cval == 0 && r.nullLit)) { V o = pr(unq(T), "0"); o.eff = r.eff; return o; }
      if (!isPtr(r.ty)) err("E_CAST", "static_cast: " + tyName(r.ty) + " -> " + tyName(T) + " gecersiz (reinterpret_cast gerekir)");
      int fe = ty(r.ty).e, te = ty(T).e;
      if (castsAway(r.ty, T)) err("E_CONSTCAST", "static_cast const'i kaldiramaz (const_cast gerekir)");
      bool ok = unq(fe) == unq(te) || K(te) == Y_VOID || (K(fe) == Y_VOID && true) || (isCls(fe) && isCls(te) && (derivedFrom(ty(fe).cls, ty(te).cls) || derivedFrom(ty(te).cls, ty(fe).cls)));
      if (!ok) err("E_CAST", "static_cast: " + tyName(r.ty) + " -> " + tyName(T) + " iliskisiz isaretci turleri");
      V o = pr(unq(T), r.reg); o.eff = r.eff; return o;
    }
    if (K(T) == Y_NULLPTR) { V r = rval(v); if (K(r.ty) != Y_NULLPTR) err("E_CAST", "nullptr_t'ye donusum yok"); return r; }
    err("E_CAST", "static_cast hedef turu desteklenmiyor: " + tyName(T));
  }
  V reinterpretCast(V v, int T) {
    if (isRef(T)) {
      if (v.cat != C_LV && !(K(T) == Y_RREF)) err("E_LVALUE", "reinterpret_cast<T&>: lvalue gerekli");
      int tgt = ty(T).e, vt = v.ty;
      if (ty(vt).c && !ty(tgt).c) err("E_CONSTCAST", "reinterpret_cast const'i kaldiramaz");
      if (isScalar(unq(vt)) && isScalar(unq(tgt)) && sizeOf(vt) != sizeOf(tgt)) err("E_REINTERP", "reinterpret_cast: boyutlar farkli (" + num(sizeOf(vt)) + " ve " + num(sizeOf(tgt)) + ") -- motor sozcuk modelinde bayt gorunumu yok");
      V o = v; o.ty = tgt; o.cat = K(T) == Y_RREF ? C_XV : C_LV; return o;
    }
    V r = rval(v); T = unq(T);
    if (r.ty == T && (isInt(T) || isPtr(T) || isFlt(T) || K(T) == Y_BOOL)) { V o = r; return o; }   // kendi turune donusum (standart izin verir)
    if (isPtr(T)) {
      if (isPtr(r.ty)) { if (castsAway(r.ty, T)) err("E_CONSTCAST", "reinterpret_cast const'i kaldiramaz"); V o = pr(T, r.reg); o.eff = r.eff; return o; }
      if (isInt(r.ty)) {
        if (sizeOf(r.ty) != 8) err("E_REINTERP", "tamsayidan isaretciye yalniz 64 bit tamsayi");
        V o = pr(T, ""); o.eff = r.eff; if (r.ci && r.cval == 0) { o.reg = "0"; return o; }
        o.reg = R(); E("sar " + o.reg + ", " + r.reg + ", 3"); return o;
      }
      err("E_REINTERP", "reinterpret_cast: " + tyName(r.ty) + " -> " + tyName(T) + " gecersiz");
    }
    if (isInt(T) && isPtr(r.ty)) {
      if (K(T) == Y_BOOL || sizeOf(T) != 8) err("E_REINTERP", "isaretciden tamsayiya yalniz 64 bit tamsayi (" + tyName(T) + " yetersiz)");
      V o = pr(T, R()); E("shl " + o.reg + ", " + r.reg + ", 3"); o.eff = r.eff; return o;
    }
    err("E_REINTERP", "reinterpret_cast: " + tyName(r.ty) + " -> " + tyName(T) + " gecersiz");
  }
  V constCast(V v, int T) {
    if (isRef(T)) {
      if (v.cat != C_LV && K(T) != Y_RREF) err("E_LVALUE", "const_cast<T&>: lvalue gerekli");
      if (unq(v.ty) != unq(ty(T).e)) err("E_CONSTCAST", "const_cast yalniz cv niteleyicilerini degistirir: " + tyName(v.ty) + " -> " + tyName(ty(T).e));
      V o = v; o.ty = ty(T).e; if (K(T) == Y_RREF) o.cat = C_XV; return o;
    }
    V r = rval(v); T = unq(T);
    if (!isPtr(T) || !isPtr(r.ty)) err("E_CONSTCAST", "const_cast yalniz isaretci/referans turlerinde");
    // cv'ler disinda ayni zincir
    int f = r.ty, t = T;
    while (isPtr(f) && isPtr(t)) { f = ty(f).e; t = ty(t).e; if (unq(f) == unq(t)) break; }
    if (unq(f) != unq(t)) err("E_CONSTCAST", "const_cast yalniz cv niteleyicilerini degistirir: " + tyName(r.ty) + " -> " + tyName(T));
    V o = pr(T, r.reg); o.eff = r.eff; return o;
  }
  bool tiNameUsed = false;
  std::map<int, int> tiTypes;   // tur kimligi -> tur (typeid(...).name() icin)
  int typeIdOf(int t) { t = unq(deref(t)); int id = isCls(t) ? C[size_t(ty(t).cls)].id : 1000 + t; tiTypes[id] = t; return id; }
  string mangle(int t) {   // Itanium ABI tur adi (std::type_info::name() ile ayni)
    const Ty& x = ty(t); string q = string(x.v ? "V" : "") + (x.c ? "K" : "");
    static const char* nm[] = {"v", "b", "c", "a", "h", "s", "t", "i", "j", "l", "m", "x", "y", "f", "d", "e", "Dn"};
    if (x.k <= Y_NULLPTR) return q + nm[x.k];
    switch (x.k) { case Y_PTR: return q + "P" + mangle(x.e); case Y_ARR: return q + "A" + std::to_string(x.n) + "_" + mangle(x.e); case Y_CLASS: { const string& n = C[size_t(x.cls)].name; return q + std::to_string(n.size()) + n; } default: return "?"; }
  }
  std::set<int> isaNeeded;
  V dynamicCast(V v, int T) {
    bool ref = isRef(T); int tgtT = ref ? ty(T).e : (isPtr(T) ? ty(T).e : -1);
    if (tgtT < 0) err("E_DYNCAST", "dynamic_cast hedefi sinif isaretcisi/referansi olmali");
    bool toVoid = !ref && K(tgtT) == Y_VOID;
    if (!toVoid && !isCls(unq(tgtT))) err("E_DYNCAST", "dynamic_cast hedef sinif turu degil: " + tyName(T));
    string objAddr; int srcCls; V base;
    if (ref) {
      if (!isCls(unq(v.ty))) err("E_DYNCAST", "dynamic_cast<T&> sinif nesnesi gerektirir");
      if (K(T) == Y_REF && v.cat != C_LV && !ty(tgtT).c) err("E_LVALUE", "dynamic_cast<T&>: lvalue gerekli");
      srcCls = ty(unq(v.ty)).cls; objAddr = v.reg; if (ty(v.ty).c && !ty(tgtT).c) err("E_CONSTCAST", "dynamic_cast const'i kaldiramaz");
    } else {
      base = rval(v); if (!isPtr(base.ty) || !isCls(unq(ty(base.ty).e))) err("E_DYNCAST", "dynamic_cast kaynak sinif isaretcisi olmali: " + tyName(base.ty));
      srcCls = ty(unq(ty(base.ty).e)).cls; objAddr = base.reg; if (ty(ty(base.ty).e).c && !ty(tgtT).c) err("E_CONSTCAST", "dynamic_cast const'i kaldiramaz");
    }
    if (!C[size_t(srcCls)].poly && !(!toVoid && derivedFrom(srcCls, ty(unq(tgtT)).cls))) err("E_DYNCAST", "dynamic_cast kaynak sinifi polimorfik degil: " + C[size_t(srcCls)].name);
    if (toVoid) { if (!C[size_t(srcCls)].poly) err("E_DYNCAST", "dynamic_cast<void*> polimorfik sinif gerektirir"); V o = pr(unq(T), objAddr); o.eff = base.eff; return o; }
    int dc = ty(unq(tgtT)).cls;
    if (derivedFrom(srcCls, dc)) {   // yukari donusum: statik
      if (ref) { V o = v; o.ty = tgtT; if (K(T) == Y_RREF) o.cat = C_XV; return o; }
      V o = pr(unq(T), objAddr); o.eff = base.eff; return o;
    }
    isaNeeded.insert(dc);
    string id = R(), c = R(), ok = NL();
    if (ref) {
      g->mayThrow = true;
      E("ld " + id + ", " + objAddr + ", 0"); E("call " + c + ", isa" + num(dc) + ", " + id); E("jnz " + c + ", " + ok); trapNow(); LAB(ok);
      V o = v; o.ty = tgtT; o.cat = K(T) == Y_RREF ? C_XV : C_LV; o.viaPtr = true; return o;
    }
    string res = R(), end = NL(); E("mov " + res + ", 0"); E("jz " + objAddr + ", " + end);
    E("ld " + id + ", " + objAddr + ", 0"); E("call " + c + ", isa" + num(dc) + ", " + id); E("jz " + c + ", " + end); E("mov " + res + ", " + objAddr); LAB(end);
    V o = pr(unq(T), res); o.eff = base.eff; return o;
  }
  V genCast(int n) {
    const Node& x = a[n]; curLine = x.line; int T = typeOf(x.kid[0]);
    if (K(T) == Y_AUTO) err("E_TYPE", "auto donusum hedefi olamaz");
    V v = gen(x.kid[1]); const string& kd = x.s;
    if (kd == "static_cast") return staticCast(v, T);
    if (kd == "dynamic_cast") return dynamicCast(v, T);
    if (kd == "const_cast") return constCast(v, T);
    if (kd == "reinterpret_cast") return reinterpretCast(v, T);
    // C tipi: const_cast, static_cast, reinterpret_cast sirasi
    try { return staticCast(v, T); } catch (IErr& e1) {
      if (e1.code == "E_CONSTCAST") {
        try { return constCast(v, T); } catch (IErr&) {}
      }
      try { return reinterpretCast(v, T); } catch (IErr&) { throw e1; }
    }
  }

  // ---------------- tur sabitleri / typeid / UDL ----------------
  int litTy(int code) { switch (code) { case Y_INT: return tInt; case Y_UINT: return tUInt; case Y_LONG: return tLong; case Y_ULONG: return tULong; case Y_LLONG: return tLLong; case Y_ULLONG: return tULLong; case Y_FLOAT: return tFloat; case Y_DOUBLE: return tDouble; case Y_LDOUBLE: return tLDouble; case Y_CHAR: return tChar; default: return tInt; } }
  V genUDL(int n) {
    const Node& x = a[n]; curLine = x.line; string nm = "operator\"\"" + x.s2;
    auto it = fmap.find(nm); if (it == fmap.end()) err("E_UDL", "kullanici tanimli sabit isleci yok: " + nm);
    vector<V> args; int wantTy;
    if (x.k == N_INT) { wantTy = tULLong; args.push_back(constInt(tULLong, x.i)); }
    else if (x.k == N_FLT) { err("E_UNSUPPORTED", "ondalik UDL sabiti long double parametresi gerektirir; long double desteklenmiyor (f64'e sessizce indirilmez)"); wantTy = tLDouble; }
    else if (x.k == N_CHR) { wantTy = tChar; args.push_back(constInt(tChar, x.i)); }
    else {   // dize: (const char*, size_t)
      string sym = strData(x.s); V p = pr(ptrTo(cv(tChar, true, false)), R()); E("mov " + p.reg + ", " + sym); args.push_back(p); args.push_back(constInt(tULong, int64_t(x.s.size()))); wantTy = -1;
    }
    vector<int> ok;
    for (int f : it->second) {
      Fn& fn = F[size_t(f)];
      if (x.k == N_STR) { if (fn.params.size() == 2 && isPtr(fn.params[0]) && unq(fn.params[1]) == tULong) ok.push_back(f); }
      else if (fn.params.size() == 1 && unq(fn.params[0]) == wantTy) ok.push_back(f);
    }
    if (ok.empty()) {
      if (x.k == N_INT || x.k == N_FLT) {   // ham sabit isleci: const char* (ondalik/tamsayi metni yoktur) -> desteklenmiyor
        err("E_UDL", nm + ": uygun parametre listesi yok (" + (x.k == N_INT ? "unsigned long long" : "long double") + " bekleniyor)");
      }
      err("E_UDL", nm + ": uygun parametre listesi yok");
    }
    if (ok.size() > 1) err("E_AMBIG", nm + ": birden fazla uygun sabit isleci");
    return callFn(ok[0], nullptr, args, false);
  }
  V typeidV(int id) { int ti = cmap["std::type_info"]; V t = tempObj(cv(clsTy(ti), true, false)); E("st " + num(id) + ", " + t.reg + ", 0"); t.cat = C_LV; return t; }
  V genTypeid(int n) {
    const Node& x = a[n]; curLine = x.line;
    if (x.k == N_TYPEID_T) { int t = typeOf(x.kid[0]); return typeidV(typeIdOf(t)); }
    bool svt = g->mayThrow; g->dry++; V probe = gen(x.kid[0]); g->dry--; g->mayThrow = svt;
    int pt = unq(probe.ty);
    if (isCls(pt) && C[size_t(ty(pt).cls)].poly && probe.cat != C_PR) {   // polimorfik glvalue: dinamik tur
      V o = gen(x.kid[0]); g->mayThrow = true;
      string id = R(); E("ld " + id + ", " + o.reg + ", 0");
      int ti = cmap["std::type_info"]; V t = tempObj(cv(clsTy(ti), true, false)); E("st " + id + ", " + t.reg + ", 0"); t.cat = C_LV; return t;
    }
    return typeidV(typeIdOf(probe.ty));
  }
  V genSizeofAlign(int n) {
    const Node& x = a[n]; curLine = x.line; int t;
    if (x.k == N_SIZEOF_E) { bool svt = g->mayThrow; g->dry++; V v = gen(x.kid[0]); g->dry--; g->mayThrow = svt; t = v.ty; if (v.isVoid) err("E_SIZEOF", "void ifadenin boyutu alinamaz"); }
    else t = typeOf(x.kid[0]);
    t = deref(t);
    if (K(t) == Y_VOID || K(t) == Y_FUNC || !completeObj(t)) err("E_SIZEOF", "tamamlanmamis/void turun boyutu alinamaz: " + tyName(t));
    if (K(t) == Y_ARR && ty(t).n < 0) err("E_SIZEOF", "boyutsuz dizi");
    return constInt(tULong, x.k == N_ALIGNOF ? alignOf(t) : sizeOf(t));
  }
  V gen(int n) {
    const Node& x = a[n]; curLine = x.line;
    switch (x.k) {
      case N_INT: { if (!x.s2.empty()) return genUDL(n); V v = constInt(litTy(int(x.j)), x.i); v.nullLit = x.i == 0; return v; }
      case N_FLT: {
        if (x.j == Y_FLOAT128) fail("E_UNSUPPORTED", "__float128 sabiti yalniz sayisal derleyicide tanimli", x.line);
        if (!x.s2.empty()) return genUDL(n);
        int t = litTy(int(x.j)); double d = x.d; if (K(t) == Y_FLOAT) d = double(float(d));
        if (K(t) == Y_LDOUBLE && !g->dry) err("E_UNSUPPORTED", "long double sabiti desteklenmiyor (f64'e sessizce indirilmez)");
        if (std::isinf(d)) err("E_CONST", "ondalik sabit tur araligini asiyor"); return pr(t, fnum(d)); }
      case N_CHR: { if (!x.s2.empty()) return genUDL(n); return constInt(tChar, x.i); }
      case N_BOOL: return constInt(tBool, x.i);
      case N_NULLPTR: return pr(tNullptr, "0");
      case N_STR: {
        if (!x.s2.empty()) return genUDL(n);
        string sym = strData(x.s); V v; v.ty = arrOf(cv(tChar, true, false), int64_t(x.s.size()) + 1); v.cat = C_LV; v.reg = R(); E("mov " + v.reg + ", " + sym); return v; }
      case N_THIS: { if (g->cls < 0) err("E_THIS", "'this' uye islev disinda kullanilamaz"); Var* th = findVar("this"); return rval(varRef(*th)); }
      case N_ID: return genId(n);
      case N_UNARY: return genUnary(n);
      case N_BINARY: return genBinary(n);
      case N_ASSIGN: return genAssign(n);
      case N_PREINC: case N_POSTINC: return genIncDec(n);
      case N_COND: return genCond(n);
      case N_COMMA: { V l = gen(x.kid[0]); V r = gen(x.kid[1]); r.eff.insert(r.eff.begin(), l.eff.begin(), l.eff.end()); if (r.cat == C_PR && !isCls(r.ty)) { /* prvalue */ } return r; }
      case N_CALL: return genCall(n);
      case N_INDEX: return genIndexV(n);
      case N_MEMBER: case N_ARROW: return genMember(n);
      case N_SIZEOF_E: case N_SIZEOF_T: case N_ALIGNOF: return genSizeofAlign(n);
      case N_NOEXCEPT: { bool sv = g->mayThrow; g->mayThrow = false; g->dry++; gen(x.kid[0]); g->dry--; bool thr = g->mayThrow; g->mayThrow = sv; return constInt(tBool, !thr); }
      case N_TYPEID_E: case N_TYPEID_T: return genTypeid(n);
      case N_CAST: return genCast(n);
      case N_FCAST: {
        int T = typeOf(x.kid[0]); if (K(T) == Y_AUTO) err("E_TYPE", "auto islevsel donusum hedefi olamaz");
        if (x.i == 1) {   // T{...}
          if (isCls(T)) { V t = tempObj(unq(T)); initObj(t.reg, unq(T), x.kid[1], 3); return t; }
          if (!isScalar(T) ) err("E_TYPE", "bu tur {..} ile olusturulamaz");
          int tmp = slot(1); string ad = slotAddr(tmp); initObj(ad, unq(T), x.kid[1], 3); V v = pr(unq(T), R()); E("ld " + v.reg + ", " + ad + ", 0"); return v;
        }
        if (x.kid.size() < 2) {   // T() deger ilklendirme
          if (isCls(T)) { V t = tempObj(unq(T)); zeroN(t.reg, C[size_t(ty(T).cls)].words); setVptr(t.reg, ty(T).cls); return t; }
          if (K(T) == Y_VOID) { V v; v.ty = tVoid; v.isVoid = true; return v; }
          if (isFlt(T)) return pr(unq(T), "0.0"); V v = constInt(unq(T), 0); if (isPtr(T)) v.ci = false; return v;
        }
        V v = gen(x.kid[1]); try { return staticCast(v, T); } catch (IErr& e1) { try { return reinterpretCast(v, T); } catch (IErr&) { throw e1; } }
      }
      case N_INITLIST: err("E_INIT", "suslu parantez listesi burada kullanilamaz");
      default: err("E_INTERNAL", "ifade dugumu bekleniyordu: " + std::to_string(x.k));
    }
  }
  // ============================== BASLATMA ==============================
  void checkNarrow(const V& v, int to, bool brace) {
    if (!brace) return;
    int f = unq(v.ty); to = unq(to);
    if (!isArith(f) || !isArith(to)) return;
    bool narrow = false;
    if (isFlt(f) && isInt(to)) narrow = true;
    else if (isInt(f) && isFlt(to)) narrow = !(v.ci);
    else if (isFlt(f) && isFlt(to)) narrow = bitsOf(to) < bitsOf(f) && K(to) == Y_FLOAT && K(f) != Y_FLOAT;
    else if (isInt(f) && isInt(to) && K(to) != Y_BOOL) {
      bool fs = isSigned(f), ts = isSigned(to); int fb = bitsOf(f), tb = bitsOf(to);
      bool fits = (fs == ts && tb >= fb) || (!fs && ts && tb > fb);
      if (!fits) { if (v.ci) { int64_t cv_ = v.cval; bool inr = ts ? (cv_ >= -(int64_t(1) << (tb - 1)) && (tb == 64 || cv_ < (int64_t(1) << (tb - 1))) ) : (cv_ >= 0 && (tb == 64 || cv_ < (int64_t(1) << tb))); if (!fs && fb == 64 && cv_ < 0) inr = (tb == 64 && !ts); narrow = !inr; } else narrow = true; }
    }
    else if (isInt(f) && K(to) == Y_BOOL) narrow = !(v.ci && (v.cval == 0 || v.cval == 1));
    if (narrow) err("E_NARROW", "daraltici donusum {..} icinde yasak: " + tyName(f) + " -> " + tyName(to));
  }
  void fillDefault(const string& addr, int t) {   // vptr'leri kur (nesne zaten sifirlandi/ilklendirildi)
    if (isCls(t)) { int ci = ty(t).cls; setVptr(addr, ci);
      for (auto& f : C[size_t(ci)].flds) if (isCls(unq(f.ty)) || isArr(f.ty)) { string fa = R(); E("add " + fa + ", " + addr + ", " + num(f.woff)); fillDefault(fa, unq(f.ty)); }
      if (C[size_t(ci)].base >= 0) {} }
    else if (isArr(t) && isCls(unq(ty(t).e)) && C[size_t(ty(unq(ty(t).e)).cls)].poly) { int w = words(ty(t).e); for (int64_t i = 0; i < ty(t).n; i++) { string fa = R(); E("add " + fa + ", " + addr + ", " + num(i * w)); fillDefault(fa, unq(ty(t).e)); } }
  }
  // addr adresindeki 't' turundeki nesneyi n ilklendiricisiyle baslat. style: 0 yok, 1 =, 2 (), 3 {}
  void initObj(const string& addr, int t, int n, int style) {
    t = unq(t); bool brace = style == 3;
    if (isCls(t)) {
      int ci = ty(t).cls; Cls& c = C[size_t(ci)];
      if (isAbstract(ci)) err("E_ABSTRACT", "soyut sinif '" + c.name + "' ornegi olusturulamaz");
      if (n < 0 || (a[n].k == N_INITLIST && a[n].kid.empty() && style != 2)) { zeroN(addr, c.words); fillDefault(addr, t); return; }
      if (a[n].k == N_INITLIST) {
        if (c.builtin) err("E_INIT", "bu sinif {..} ile baslatilamaz");
        if (style == 2 && a[n].kid.size() == 1) { initObj(addr, t, a[n].kid[0], 1); return; }   // T x(e)
        if (c.poly) err("E_INIT", "'" + c.name + "' kume (aggregate) degil: sanal islevi var");
        zeroN(addr, c.words);
        size_t idx = 0; auto& kids = a[n].kid;
        if (c.base >= 0) {
          if (idx < kids.size()) {
            int bt = clsTy(c.base);
            if (a[kids[idx]].k == N_INITLIST) { initObj(addr, bt, kids[idx], 3); idx++; }
            else { g->dry++; V probe = gen(kids[idx]); g->dry--; if (isCls(unq(probe.ty)) && derivedFrom(ty(unq(probe.ty)).cls, c.base)) { V v = gen(kids[idx]); copyN(addr, v.reg, 0, C[size_t(c.base)].words); idx++; } }
          }
        }
        for (auto& f : c.flds) {
          if (idx >= kids.size()) break;
          string fa = f.woff ? R() : addr; if (f.woff) E("add " + fa + ", " + addr + ", " + num(f.woff));
          initObj(fa, unq(f.ty), kids[idx], 3); idx++;
        }
        if (idx < kids.size()) err("E_INIT", "ilklendirici listesinde fazla eleman ('" + c.name + "')");
        fillDefault(addr, t); return;
      }
      V v = gen(n);
      if (style == 3 && !isCls(unq(v.ty))) err("E_INIT", "sinif nesnesi bu degerle {..} ile baslatilamaz");
      V s = toClassValue(v, t);
      if (c.builtin) copyN(addr, s.reg, 0, 1); else copyObj(addr, s.reg, ci, true);
      return;
    }
    if (isArr(t)) {
      int et = unq(ty(t).e); int64_t cnt = ty(t).n; int w = words(et);
      if (n >= 0 && a[n].k == N_STR && K(et) == Y_CHAR) {   // char s[N] = "..."
        const string& s = a[n].s; if (int64_t(s.size()) + 1 > cnt) err("E_INIT", "dize sabiti diziye sigmiyor"); zeroN(addr, int(cnt));
        for (size_t i = 0; i < s.size(); i++) E("st " + num((signed char)s[i]) + ", " + addr + ", " + num(i)); return;
      }
      if (n < 0 || a[n].k != N_INITLIST) { if (n < 0) { fillDefault(addr, t); return; } err("E_INIT", "dizi {..} listesiyle baslatilmali"); }
      auto& kids = a[n].kid; if (int64_t(kids.size()) > cnt) err("E_INIT", "ilklendirici listesinde fazla eleman (dizi boyutu " + num(cnt) + ")");
      zeroN(addr, int(cnt) * w);
      for (size_t i = 0; i < kids.size(); i++) { string ea = i ? R() : addr; if (i) E("add " + ea + ", " + addr + ", " + num(int64_t(i) * w)); initObj(ea, et, kids[i], 3); }
      fillDefault(addr, t); return;
    }
    // skaler
    if (n < 0) return;
    int e = n;
    if (a[n].k == N_INITLIST) {
      auto& kids = a[n].kid; if (kids.size() > 1) err("E_INIT", "skaler tek degerle ilklendirilir");
      if (kids.empty()) { E("st " + string(isFlt(t) ? "0.0" : "0") + ", " + addr + ", 0"); return; }
      e = kids[0];
    }
    V v = gen(e);
    if (brace) { V r = isCls(unq(v.ty)) ? v : rval(v); checkNarrow(r, t, true); }
    V s = toScalar(v, t, style == 2 || style == 3 ? false : false);
    E("st " + s.reg + ", " + addr + ", 0");
  }
  Var& declLocal(const string& name, int t) {
    auto& sc = g->scopes.back(); if (sc.count(name)) err("E_REDECL", "'" + name + "' ayni kapsamda yeniden tanimlandi");
    Var v; v.name = name; v.ty = t; v.uid = ++uidn; sc[name] = v; return sc[name];
  }

  // ============================== BILDIRIMLER / DEYIMLER ==============================
  int deduceAuto(int T, const V& probe, bool isLvalueInit) {
    auto inner = [&](int t) { return t; };
    if (K(T) == Y_AUTO) {
      int base = isArr(probe.ty) ? ptrTo(ty(probe.ty).e) : unq(probe.ty);
      if (K(probe.ty) == Y_FUNC) err("E_TYPE", "auto islev");
      if (K(base) == Y_VOID) err("E_TYPE", "auto void'den cikarilamaz");
      return ty(T).c ? withC(base, true) : base;
    }
    if (isRef(T) && K(ty(T).e) == Y_AUTO) {
      bool c = ty(ty(T).e).c; int bt = probe.ty;
      if (K(T) == Y_REF) { if (!isLvalueInit && !(c || ty(bt).c)) err("E_BIND", "auto& rvalue'ya baglanamaz"); int r = c ? withC(bt, true) : bt; return lref(r); }
      if (isLvalueInit) return lref(c ? withC(probe.ty, true) : probe.ty);
      return rref(c ? withC(unq(probe.ty), true) : unq(probe.ty));
    }
    (void)inner; return T;
  }
  void declVar(int vn) {
    const Node& v = a[vn]; curLine = v.line; int T = typeOf(v.kid[0]); int init = v.kid.size() > 1 ? v.kid[1] : -1; int style = int(v.i);
    int initExpr = init; if (init >= 0 && a[init].k == N_INITLIST && a[init].kid.size() == 1 && (K(T) == Y_AUTO || (isRef(T) && K(ty(T).e) == Y_AUTO) || isRef(T))) initExpr = a[init].kid[0];
    bool hasAuto = K(T) == Y_AUTO || (isRef(T) && K(ty(T).e) == Y_AUTO);
    if (hasAuto) {
      if (init < 0) err("E_TYPE", "auto bildirimi ilklendirici gerektirir");
      g->dry++; V pb = gen(initExpr); g->dry--; T = deduceAuto(T, pb, pb.cat == C_LV);
    }
    if (isArr(T) && ty(T).n < 0) {   // boyutsuz dizi: ilklendiriciden cikar
      if (init < 0) err("E_TYPE", "boyutsuz dizi ilklendirici gerektirir");
      int64_t cnt;
      if (a[init].k == N_STR) cnt = int64_t(a[init].s.size()) + 1; else if (a[init].k == N_INITLIST) cnt = int64_t(a[init].kid.size()); else err("E_TYPE", "boyutsuz dizi {..} ya da dize gerektirir");
      if (cnt == 0) err("E_TYPE", "bos dizi"); T = arrOf(ty(T).e, cnt);
    }
    if (K(T) == Y_VOID) err("E_TYPE", "void turunde degisken");
    if (K(T) == Y_AUTO) err("E_TYPE", "auto cikarilamadi");
    if (isRef(T)) {
      if (init < 0) err("E_INIT", "referans ilklendirici gerektirir");
      V iv = gen(initExpr); string ad = bindArg(iv, T);
      Var& var = declLocal(v.s, ty(T).e); int off = slot(1); var.off = off; var.isRef = true; E("st " + ad + ", fp, " + num(off)); return;
    }
    if (!completeObj(T)) err("E_TYPE", "tamamlanmamis tur: " + tyName(T));
    if (isCls(unq(T)) && isAbstract(ty(unq(T)).cls)) err("E_ABSTRACT", "soyut sinif '" + C[size_t(ty(unq(T)).cls)].name + "' ornegi olusturulamaz");
    int off = slot(words(T)); string ad = slotAddr(off);
    // degisken kapsamda ilklendirici ifadesinden SONRA gorunur (int x = x; gecersiz kapsam kurallari sade tutuldu)
    bool ciCand = (style == 1 || style == 3 || style == 2) && ty(T).c && isInt(T) && initExpr >= 0 && a[initExpr].k != N_INITLIST;
    V constProbe; bool isCi = false;
    if (ciCand) { g->dry++; try { constProbe = gen(initExpr); } catch (IErr&) { g->dry--; throw; } g->dry--; isCi = constProbe.ci; }
    if (init >= 0 && a[init].k == N_INITLIST && style == 2 && isScalar(T) && a[init].kid.size() == 1) { init = a[init].kid[0]; style = 1; }
    initObj(ad, T, init, style == 0 ? 0 : style);
    if (init < 0 && isCls(unq(T))) {}
    Var& var = declLocal(v.s, T); var.off = off;
    if (isCi) { var.ci = true; var.cval = foldNorm(constProbe.cval, unq(T)); }
  }
  void declGlobalVar(int vn) {
    const Node& v = a[vn]; curLine = v.line; int T = typeOf(v.kid[0]); int init = v.kid.size() > 1 ? v.kid[1] : -1;
    if (K(T) == Y_AUTO) err("E_TYPE", "global auto desteklenmiyor");
    if (isRef(T)) err("E_TYPE", "global referans desteklenmiyor");
    if (isArr(T) && ty(T).n < 0) { if (init >= 0 && a[init].k == N_INITLIST) T = arrOf(ty(T).e, int64_t(a[init].kid.size())); else if (init >= 0 && a[init].k == N_STR) T = arrOf(ty(T).e, int64_t(a[init].s.size()) + 1); else err("E_TYPE", "boyutsuz global dizi"); }
    if (!completeObj(T)) err("E_TYPE", "tamamlanmamis tur");
    if (gmap.count(v.s)) err("E_REDECL", "global '" + v.s + "' yeniden tanimlandi");
    Var var; var.name = v.s; var.ty = T; var.global = true; var.uid = ++uidn; var.sym = "g" + std::to_string(var.uid) + "_" + v.s;
    dataLines.push_back("space " + var.sym + " " + num(std::max(words(T), 1)));
    if (init >= 0 && ty(T).c && isInt(T) && a[init].k != N_INITLIST) { g->dry++; V pr_ = gen(init); g->dry--; if (pr_.ci) { var.ci = true; var.cval = foldNorm(pr_.cval, unq(T)); } }
    globals.push_back(var); gmap[v.s] = int(globals.size()) - 1; gInitList.push_back(vn);
  }
  vector<int> gInitList;

  void emitRet(const string& val) {
    string t = R(); E("mov " + t + ", __sp"); E("st fp, " + t + ", 0"); E("ret " + val);
  }
  void stmtReturn(int n) {
    const Node& x = a[n]; curLine = x.line; Fn& f = F[size_t(g->fn)]; int rt = f.ret;
    if (x.kid.empty()) {
      if (f.autoRet && rt < 0) { f.ret = tVoid; rt = tVoid; }
      if (K(rt) != Y_VOID) err("E_RETURN", "deger donduren islevde 'return' degersiz");
      emitRet("0"); return;
    }
    if (f.autoRet && rt < 0) {   // auto: ilk return'den cikar
      g->dry++; V pb = gen(x.kid[0]); g->dry--; if (pb.isVoid) { f.ret = tVoid; } else f.ret = isArr(pb.ty) ? ptrTo(ty(pb.ty).e) : unq(pb.ty);
      rt = f.ret;
    }
    if (K(rt) == Y_VOID) {
      if (a[x.kid[0]].k == N_INITLIST) err("E_RETURN", "void islevden deger donduruluyor");
      V v = gen(x.kid[0]); if (!v.isVoid) err("E_RETURN", "void islevden deger donduruluyor"); emitRet("0"); return;
    }
    if (isRef(rt)) {
      V v = gen(x.kid[0]); bool rr = K(rt) == Y_RREF; int tu = unq(ty(rt).e);
      if (unq(v.ty) != tu && !(isCls(unq(v.ty)) && isCls(tu) && derivedFrom(ty(unq(v.ty)).cls, ty(tu).cls))) err("E_RETURN", "referans donusu: tur uyumsuz (" + tyName(v.ty) + " -> " + tyName(rt) + ")");
      if (!rr && v.cat != C_LV && !ty(ty(rt).e).c) err("E_BIND", "lvalue referans rvalue'ya baglanamaz");
      if (rr && v.cat == C_LV) err("E_BIND", "rvalue referans lvalue'ya baglanamaz");
      if (ty(v.ty).c && !ty(ty(rt).e).c) err("E_CONST", "const nesne const olmayan referansla donduruluyor");
      string ad = v.reg; if (v.cat == C_PR && !isCls(tu)) { V t = tempScalar(tu, v.reg); ad = t.reg; }
      emitRet(ad); return;
    }
    if (isCls(unq(rt))) {
      V v = a[x.kid[0]].k == N_INITLIST ? tempObj(unq(rt)) : gen(x.kid[0]);
      if (a[x.kid[0]].k == N_INITLIST) { initObj(v.reg, unq(rt), x.kid[0], 3); }
      V c = toClassValue(v, unq(rt)); Var* sr = findVar("__sret"); if (!sr) { emitRet("0"); return; } V sp = rval(varRef(*sr));
      if (C[size_t(ty(unq(rt)).cls)].builtin) copyN(sp.reg, c.reg, 0, 1); else copyObj(sp.reg, c.reg, ty(unq(rt)).cls, true);
      emitRet("0"); return;
    }
    V v = a[x.kid[0]].k == N_INITLIST ? V() : gen(x.kid[0]);
    if (a[x.kid[0]].k == N_INITLIST) { int off = slot(1); string ad = slotAddr(off); initObj(ad, unq(rt), x.kid[0], 3); v = pr(unq(rt), R()); E("ld " + v.reg + ", " + ad + ", 0"); }
    V s = toScalar(v, unq(rt)); emitRet(s.reg);
  }
  void stmt(int n) {
    const Node& x = a[n]; curLine = x.line; int base = g->nreg;
    switch (x.k) {
      case N_EMPTY: break;
      case N_BLOCK: g->scopes.emplace_back(); for (int k : x.kid) stmt(k); g->scopes.pop_back(); break;
      case N_DECLS: for (int k : x.kid) { declVar(k); g->nreg = base; } break;
      case N_EXPRSTMT: gen(x.kid[0]); g->nreg = base; break;
      case N_IF: {
        V c = condBool(gen(x.kid[0])); string els = NL(), end = NL(); E("jz " + c.reg + ", " + els); g->nreg = base;
        g->scopes.emplace_back(); stmt(x.kid[1]); g->scopes.pop_back();
        if (x.kid.size() > 2) { E("jmp " + end); LAB(els); g->scopes.emplace_back(); stmt(x.kid[2]); g->scopes.pop_back(); LAB(end); } else LAB(els);
        break; }
      case N_WHILE: {
        string top = NL(), end = NL(); LAB(top); V c = condBool(gen(x.kid[0])); E("jz " + c.reg + ", " + end); g->nreg = base;
        g->loops.push_back({end, top}); g->scopes.emplace_back(); stmt(x.kid[1]); g->scopes.pop_back(); g->loops.pop_back(); E("jmp " + top); LAB(end); break; }
      case N_DO: {
        string top = NL(), cont = NL(), end = NL(); LAB(top); g->loops.push_back({end, cont}); g->scopes.emplace_back(); stmt(x.kid[0]); g->scopes.pop_back(); g->loops.pop_back();
        LAB(cont); V c = condBool(gen(x.kid[1])); E("jnz " + c.reg + ", " + top); g->nreg = base; LAB(end); break; }
      case N_FOR: {
        g->scopes.emplace_back(); if (a[x.kid[0]].k != N_NONE) stmt(x.kid[0]);
        string top = NL(), cont = NL(), end = NL(); LAB(top);
        if (a[x.kid[1]].k != N_NONE) { V c = condBool(gen(x.kid[1])); E("jz " + c.reg + ", " + end); g->nreg = base; }
        g->loops.push_back({end, cont}); g->scopes.emplace_back(); stmt(x.kid[3]); g->scopes.pop_back(); g->loops.pop_back();
        LAB(cont); if (a[x.kid[2]].k != N_NONE) { gen(x.kid[2]); g->nreg = base; } E("jmp " + top); LAB(end); g->scopes.pop_back(); break; }
      case N_RETURN: stmtReturn(n); g->nreg = base; break;
      case N_BREAK: if (g->loops.empty()) err("E_STMT", "'break' dongu disinda"); E("jmp " + g->loops.back().first); break;
      case N_CONTINUE: if (g->loops.empty()) err("E_STMT", "'continue' dongu disinda"); E("jmp " + g->loops.back().second); break;
      case N_SWITCH: case N_CASE: case N_DEFAULT: case N_GOTO: case N_LABEL: case N_RANGEFOR: case N_MATCH: case N_COYIELD: case N_CORETURN: case N_COAWAIT: err("E_UNSUPPORTED", "Bolum 03 denetim akisi yapisi (switch/goto/range-for/match/korutin) yalniz sayisal derleyicide (ifx.pmb) tanimli");
      case N_THROW: case N_TRY: err("E_UNSUPPORTED", "istisna (try/catch/throw) yalniz sayisal derleyicide (ifx.pmb) tanimli; bu C++ referans yolu istisna uretmez");
      default: err("E_INTERNAL", "deyim bekleniyordu");
    }
  }

  // ============================== SINIF / ISLEV BILDIRIMI ==============================
  bool sameSig(const Fn& x, const Fn& y) {
    if (x.name != y.name || x.params.size() != y.params.size() || x.isConst != y.isConst) return false;
    if (x.name == "operator conv" && x.ret != y.ret) return false;
    for (size_t i = 0; i < x.params.size(); i++) if (x.params[i] != y.params[i]) return false;
    return true;
  }
  int declareFunc(int node, int cls) {
    const Node& n = a[node]; curLine = n.line; Fn f; f.name = n.s; f.node = node; f.cls = cls; f.line = n.line;
    int64_t fl = n.i; f.isConst = fl & 1; f.isVirt = fl & 2; f.noex = fl & 4; f.defaulted = fl & 16; f.hasOverride = fl & 32; f.deleted = fl & 64; f.explicitConv = fl & 128; f.pure = fl & 256;
    f.ret = typeOf(n.kid[0]); if (K(f.ret) == Y_AUTO) { f.autoRet = true; f.ret = -1; }
    for (int pn : a[n.kid[1]].kid) {
      int pt = typeOf(a[pn].kid[0]);
      if (K(pt) == Y_ARR) pt = ptrTo(ty(pt).e);
      if (K(pt) == Y_VOID) err("E_TYPE", "void parametre");
      if (!isRef(pt) && !isPtr(pt) ) pt = unq(pt); else if (isPtr(pt)) pt = unq(pt);
      f.params.push_back(pt);
    }
    if (n.s == "operator conv") { if (!f.params.empty()) err("E_OP", "donusum islecinin parametresi olamaz"); }
    if (cls < 0 && f.isConst) err("E_OP", "uye olmayan islev const olamaz");
    if (n.s.rfind("operator", 0) == 0 && n.s != "operator conv" && n.s.find("\"\"") == string::npos && cls < 0) {
      bool hasCls = false; for (int p : f.params) if (isCls(unq(deref(p)))) hasCls = true;
      if (!hasCls) err("E_OP", "operator '" + n.s + "' en az bir sinif turlu parametre gerektirir");
    }
    if (n.s.find("\"\"") != string::npos) {   // UDL parametre listesi denetimi
      bool ok = false; auto& p = f.params;
      if (p.size() == 1 && (unq(p[0]) == tULLong || unq(p[0]) == tLDouble || unq(p[0]) == tChar)) ok = true;
      if (p.size() == 2 && isPtr(p[0]) && unq(p[1]) == tULong) ok = true;
      if (!ok) err("E_UDL", "gecersiz literal operator parametre listesi: " + n.s);
    }
    f.hasBody = n.kid.size() > 2;
    // yeniden bildirim
    if (cls >= 0) {
      for (int m : C[size_t(cls)].meths) if (sameSig(F[size_t(m)], f)) err("E_REDECL", "uye islev yeniden tanimlandi: " + f.name);
    } else {
      auto it = fmap.find(f.name);
      if (it != fmap.end()) for (int m : it->second) if (sameSig(F[size_t(m)], f)) {
        Fn& o = F[size_t(m)]; if (o.hasBody && f.hasBody) err("E_REDEF", "islev iki kez tanimlandi: " + f.name);
        if (f.hasBody) { o.body = n.kid[2]; o.node = node; o.hasBody = true; }
        if (!o.autoRet && !f.autoRet && o.ret != f.ret) err("E_REDECL", "ayni parametreli islev farkli donus turuyle bildirildi: " + f.name);
        return m;
      }
    }
    if (f.hasBody) f.body = n.kid[2];
    int idx = int(F.size()); string san; for (char ch : f.name) san += isalnum((unsigned char)ch) ? ch : '_';
    f.pma = "f" + std::to_string(idx) + "_" + san;
    if (f.name == "__puti" || f.name == "__putc" || f.name == "__putf" || f.name == "__getc") f.builtin = f.name.substr(2);
    if (cls >= 0) {
      Cls& c = C[size_t(cls)]; f.vroot = -1;
      // sanal islev gecersiz kilma
      for (int b = c.base; b >= 0; b = C[size_t(b)].base) for (int m : C[size_t(b)].meths) {
        Fn& bf = F[size_t(m)];
        if (bf.isVirt && bf.name == f.name && bf.params == f.params && bf.isConst == f.isConst) {
          if (bf.ret != f.ret && !f.autoRet) err("E_OVERRIDE", "gecersiz kilan islev farkli donus turu (kovaryant donus desteklenmiyor): " + f.name);
          f.isVirt = true; f.vroot = bf.vroot; goto found;
        }
      }
      found:
      if (f.hasOverride && !(f.isVirt && f.vroot >= 0)) err("E_OVERRIDE", "'override' ama taban sinifta eslesen sanal islev yok: " + f.name);
      if (f.isVirt) { c.poly = true; if (f.vroot < 0) f.vroot = idx; }
      if (f.pure && !f.isVirt) err("E_OVERRIDE", "saf belirtici yalniz sanal islevde");
      if (f.defaulted && f.name != "operator<=>" && f.name != "operator==") err("E_DEFAULT", "yalniz karsilastirma operatorleri = default olabilir");
    } else if (f.isVirt) err("E_OVERRIDE", "'virtual' yalniz uye islevde");
    F.push_back(f);
    if (cls >= 0) C[size_t(cls)].meths.push_back(idx); else fmap[f.name].push_back(idx);
    return idx;
  }
  std::set<string>* knownClasses = nullptr;
  void synthMember(int ci, const string& text) {
    Lexer lx(text); lx.run(); Parser pp(a, lx.out);
    for (auto& kv : cmap) pp.classes.insert(kv.first);
    int b = pp.typeSpec(); int tt = pp.ptrOps(b); pp.p++;   // operator
    int convTy; string nm = pp.operatorName(convTy);
    int f = pp.funcRest(tt, nm, curLine, ci);
    declareFunc(f, ci);
  }
  void declareClass(int node) {
    const Node& n = a[node]; curLine = n.line; int ci;
    auto it = cmap.find(n.s);
    if (n.j == 1) { if (it == cmap.end()) newClass(n.s); return; }
    if (it != cmap.end()) { ci = it->second; if (C[size_t(ci)].defined) err("E_REDEF", "sinif iki kez tanimlandi: " + n.s); } else ci = newClass(n.s);
    if (!n.s2.empty()) { auto bt = cmap.find(n.s2); if (bt == cmap.end() || !C[size_t(bt->second)].defined) err("E_CLASS", "taban sinif tanimli degil: " + n.s2); C[size_t(ci)].base = bt->second; }
    vector<int> methods;
    for (int k : n.kid) {
      const Node& m = a[k];
      if (m.k == N_VAR) {
        int ft = typeOf(m.kid[0]); if (m.kid.size() > 1) err("E_CLASS", "sinif ici uye ilklendiricisi desteklenmiyor: " + m.s);
        if (K(ft) == Y_AUTO) err("E_TYPE", "uye auto olamaz");
        if (K(ft) == Y_ARR && ty(ft).n < 0) err("E_TYPE", "boyutsuz dizi uye");
        for (auto& f : C[size_t(ci)].flds) if (f.name == m.s) err("E_REDECL", "uye yeniden tanimlandi: " + m.s);
        C[size_t(ci)].flds.push_back({m.s, ft, 0, 0});
      } else methods.push_back(k);
    }
    // sinifin kendisi eksik turdur, ama yontemlerin imzalarinda referans/isaretci olarak kullanilabilir
    vector<int> fi; for (int k : methods) fi.push_back(declareFunc(k, ci));
    layoutClass(ci);
    // varsayilan karsilastirmalar
    bool hasEq = false, defCmp = false; for (int m : C[size_t(ci)].meths) { if (F[size_t(m)].name == "operator==") hasEq = true; if (F[size_t(m)].name == "operator<=>" && F[size_t(m)].defaulted) defCmp = true; }
    for (int m : vector<int>(C[size_t(ci)].meths)) if (F[size_t(m)].defaulted) synthDefaulted(ci, m);
    if (defCmp && !hasEq) synthDefaulted(ci, -1);
  }
  int cmpKindOfType(int t) {   // 1 strong, 3 partial
    t = unq(t);
    if (isFlt(t)) return 3; if (isInt(t) || isPtr(t)) return 1;
    if (isArr(t)) return cmpKindOfType(ty(t).e);
    if (isCls(t)) {
      for (int m : methodsNamed(ty(t).cls, "operator<=>")) { Fn& f = F[size_t(m)]; if (f.ret >= 0 && isOrd(f.ret)) return C[size_t(ty(unq(f.ret)).cls)].builtin; ensureRet(m); if (f.ret >= 0 && isOrd(f.ret)) return C[size_t(ty(unq(f.ret)).cls)].builtin; }
      err("E_DEFAULT", "uye sinifi '" + C[size_t(ty(t).cls)].name + "' icin <=> tanimli degil: varsayilan karsilastirma silinmis");
    }
    err("E_DEFAULT", "bu uye turu karsilastirilamaz: " + tyName(t));
  }
  void synthDefaulted(int ci, int m) {
    Cls& c = C[size_t(ci)]; string S = c.name; bool eqOnly = (m >= 0 && F[size_t(m)].name == "operator==");
    if (m >= 0 && F[size_t(m)].params.size() != 1) err("E_DEFAULT", "varsayilan karsilastirma tek parametre alir");
    if (m >= 0) { Fn& f = F[size_t(m)]; int p = f.params[0]; if (!(K(p) == Y_REF && ty(ty(p).e).c && unq(ty(p).e) == clsTy(ci)) || !f.isConst) err("E_DEFAULT", "varsayilan karsilastirma 'const " + S + "&' parametreli const uye islev olmali"); }
    string body, ret;
    if (eqOnly || m < 0) {
      if (m >= 0) { F[size_t(m)].deleted = false; }
      string cond;
      if (c.base >= 0) cond += "static_cast<const " + C[size_t(c.base)].name + "&>(*this) == static_cast<const " + C[size_t(c.base)].name + "&>(o)";
      for (auto& f : c.flds) { if (isArr(f.ty)) err("E_DEFAULT", "dizi uyeli varsayilan karsilastirma desteklenmiyor"); cond += string(cond.empty() ? "" : " && ") + "this->" + f.name + " == o." + f.name; }
      if (cond.empty()) cond = "true";
      string text = "bool operator==(const " + S + "& o) const { return " + cond + "; }";
      if (m >= 0) { // yer tutucu islevi sentezlenen govdeyle degistir
        F[size_t(m)].defaulted = false; synthReplace(ci, m, text);
      } else synthMember(ci, text);
      return;
    }
    int kind = 1;
    if (c.base >= 0) kind = std::max(kind, cmpKindOfType(clsTy(c.base)));
    for (auto& f : c.flds) { if (isArr(f.ty)) err("E_DEFAULT", "dizi uyeli varsayilan karsilastirma desteklenmiyor"); kind = std::max(kind, cmpKindOfType(f.ty)); }
    string rt = kind == 1 ? "std::strong_ordering" : kind == 2 ? "std::weak_ordering" : "std::partial_ordering";
    Fn& f = F[size_t(m)];
    if (f.autoRet || f.ret < 0) { /* auto: cikarilan */ }
    else { int declared = C[size_t(ty(unq(f.ret)).cls)].builtin; if (!isOrd(f.ret) || declared < kind) err("E_DEFAULT", "bildirilen donus turu uyelerin ortak karsilastirma kategorisini karsilamiyor"); rt = C[size_t(ty(unq(f.ret)).cls)].name; }
    string text = rt + " operator<=>(const " + S + "& o) const { ";
    int i = 0;
    if (c.base >= 0) { string bn = C[size_t(c.base)].name; text += "auto c" + std::to_string(i) + " = static_cast<const " + bn + "&>(*this) <=> static_cast<const " + bn + "&>(o); if (c" + std::to_string(i) + " != 0) return c" + std::to_string(i) + "; "; i++; }
    for (auto& fl : c.flds) { text += "auto c" + std::to_string(i) + " = this->" + fl.name + " <=> o." + fl.name + "; if (c" + std::to_string(i) + " != 0) return c" + std::to_string(i) + "; "; i++; }
    text += "return " + string(rt) + "::" + (rt == "std::strong_ordering" ? "equal" : "equivalent") + "; }";
    f.defaulted = false; synthReplace(ci, m, text);
  }
  void synthReplace(int ci, int m, const string& text) {   // m'nin bildirimini sentezlenen tanimla degistir
    Lexer lx(text); lx.run(); Parser pp(a, lx.out); for (auto& kv : cmap) pp.classes.insert(kv.first);
    int b = pp.typeSpec(); int tt = pp.ptrOps(b); pp.p++; int convTy; string nm = pp.operatorName(convTy);
    int fnode = pp.funcRest(tt, nm, curLine, ci);
    Fn& f = F[size_t(m)]; f.node = fnode; f.body = a[fnode].kid[2]; f.hasBody = true; f.ret = typeOf(a[fnode].kid[0]); f.autoRet = false;
  }
  bool isAbstract(int ci) {
    for (int c = ci; c >= 0; c = C[size_t(c)].base) for (int m : C[size_t(c)].meths) {
      Fn& f = F[size_t(m)]; if (!f.pure) continue;
      // son gecersiz kilan?
      int fin = finalOverrider(ci, m); if (fin >= 0 && F[size_t(fin)].pure) return true;
    }
    return false;
  }
  int finalOverrider(int K_, int root) {   // K_ sinifi icin 'root' sanal islevinin son gecersiz kilicisi
    Fn& r = F[size_t(root)];
    for (int c = K_; c >= 0; c = C[size_t(c)].base) for (int m : C[size_t(c)].meths) { Fn& f = F[size_t(m)]; if (f.isVirt && f.vroot == r.vroot && f.name == r.name && f.params == r.params && f.isConst == r.isConst) return m; if (c == r.cls && m == root) return m; }
    return root;
  }

  // ============================== ISLEV GOVDESI ==============================
  vector<string> prologueLines() {
    return {"  mov _spa, __sp", "  ld fp, _spa, 0", "  add _nsp, fp, @FRAME@", "  mov _lma, __lim", "  ld _lim, _lma, 0", "  leu _okc, _nsp, _lim", "  jnz _okc, _stk_ok", "  ld _trp, -1, 0", "_stk_ok: st _nsp, _spa, 0"};
  }
  void genBody(int fi) {
    Fn* fp_ = &F[size_t(fi)]; if (fp_->generated) return;
    if (fp_->generating) err("E_AUTO", "'auto' donus turu ozyinelemeyle cikarilamaz: " + fp_->name);
    if (!fp_->hasBody) return;
    fp_->generating = true;
    Gen G; G.fn = fi; G.cls = fp_->cls; vector<string> lines; G.out = &lines; Gen* saved = g; g = &G; int savedLine = curLine;
    try {
      G.scopes.emplace_back();
      Fn& f = F[size_t(fi)]; vector<string> ps; int pi = 0;
      bool sret = f.ret >= 0 && isCls(unq(f.ret)) && !isRef(f.ret);
      // auto donus: ilk return'e kadar bilinmeyebilir; sret kararini gecikmeli ele al
      if (f.autoRet && f.ret < 0) { g->dry++; /* tur cikarimi icin kuru calistirma */ g->dry--; }
      string sretP, thisP;
      if (f.autoRet && f.ret < 0) {
        // sentinel: govdeyi bir kez kuru calistirip donus turunu bul
        Gen D; D.fn = fi; D.cls = f.cls; vector<string> dl; D.out = &dl; D.dry = 1; Gen* sv2 = g; g = &D; D.scopes.emplace_back();
        try { prepareScope(fi, true); genBodyStmts(fi); } catch (...) { g = sv2; throw; }
        g = sv2; if (F[size_t(fi)].ret < 0) F[size_t(fi)].ret = tVoid;
      }
      sret = isCls(unq(F[size_t(fi)].ret)) && !isRef(F[size_t(fi)].ret);
      if (sret) { sretP = "p" + std::to_string(pi++); }
      if (f.cls >= 0) thisP = "p" + std::to_string(pi++);
      vector<string> pn; for (size_t i = 0; i < f.params.size(); i++) pn.push_back("p" + std::to_string(pi++));
      string hdr = "func " + f.pma + "("; bool first = true;
      for (int k = 0; k < pi; k++) { hdr += (first ? "" : ", ") + string("p") + std::to_string(k); first = false; }
      hdr += ") {"; lines.push_back(hdr);
      // giris: yigin cercevesi
      for (auto& l : prologueLines()) lines.push_back(l);
      G.dry = 0; prepareScope(fi, false);
      if (sret) { Var& v = declLocal("__sret", ptrTo(unq(F[size_t(fi)].ret))); v.off = slot(1); E("st " + sretP + ", fp, " + num(v.off)); }
      if (f.cls >= 0) { Var& v = declLocal("this", ptrTo(f.isConst ? cv(clsTy(f.cls), true, false) : clsTy(f.cls))); v.off = slot(1); E("st " + thisP + ", fp, " + num(v.off)); }
      for (size_t i = 0; i < f.params.size(); i++) {
        const Node& pv = a[a[a[F[size_t(fi)].node].kid[1]].kid[i]]; int pt = F[size_t(fi)].params[i];
        string nm = pv.s.empty() ? "__arg" + std::to_string(i) : pv.s;
        if (isRef(pt)) { Var& v = declLocal(nm, ty(pt).e); v.off = slot(1); v.isRef = true; E("st " + pn[i] + ", fp, " + num(v.off)); }
        else if (isCls(unq(pt))) { Var& v = declLocal(nm, pt); v.off = slot(1); v.isRef = true; E("st " + pn[i] + ", fp, " + num(v.off)); }
        else { Var& v = declLocal(nm, pt); v.off = slot(1); E("st " + pn[i] + ", fp, " + num(v.off)); }
      }
      genBodyStmts(fi);
      // govdenin sonu: dusme
      emitRet("0");
      lines.push_back("}");
      for (auto& l : lines) { size_t p = l.find("@FRAME@"); if (p != string::npos) l.replace(p, 7, num(std::max(G.frameOff, 1))); }
    } catch (...) { g = saved; curLine = savedLine; F[size_t(fi)].generating = false; throw; }
    g = saved; curLine = savedLine; Fn& f2 = F[size_t(fi)]; f2.lines = lines; f2.generated = true; f2.generating = false;
  }
  void prepareScope(int fi, bool dry) {   // kuru calistirmada parametre/this degiskenleri (yalniz tur cikarimi icin)
    if (!dry) return;
    Fn& f = F[size_t(fi)]; Gen& G = *g;
    int unused = 0; (void)unused;
    if (f.cls >= 0) { Var& v = declLocal("this", ptrTo(f.isConst ? cv(clsTy(f.cls), true, false) : clsTy(f.cls))); v.off = slot(1); }
    for (size_t i = 0; i < f.params.size(); i++) {
      const Node& pv = a[a[a[f.node].kid[1]].kid[i]]; int pt = f.params[i]; string nm = pv.s.empty() ? "__arg" + std::to_string(i) : pv.s;
      if (isRef(pt)) { Var& v = declLocal(nm, ty(pt).e); v.off = slot(1); v.isRef = true; }
      else if (isCls(unq(pt))) { Var& v = declLocal(nm, pt); v.off = slot(1); v.isRef = true; }
      else { Var& v = declLocal(nm, pt); v.off = slot(1); }
    }
    (void)G;
  }
  void genBodyStmts(int fi) {
    Fn& f = F[size_t(fi)]; const Node& body = a[f.body];
    if (body.k != N_BLOCK) err("E_INTERNAL", "islev govdesi blok degil");
    for (int k : body.kid) stmt(k);
    if (f.ret >= 0 && K(f.ret) != Y_VOID && !g->dry && false) {}
  }

  int64_t constIntExpr(int n) {
    g->dry++; V v; try { v = gen(n); } catch (...) { g->dry--; throw; } g->dry--;
    if (!v.ci || !isInt(v.ty)) err("E_CONST", "sabit tamsayi ifade gerekli");
    return v.cval;
  }
  // ============================== PROGRAM ==============================
  void ensureRet(int fi) { Fn& f = F[size_t(fi)]; if (f.autoRet && f.ret < 0) { if (f.generating) err("E_AUTO", "'auto' donus turu ozyinelemeyle cikarilamaz: " + f.name); genBody(fi); } }
  vector<string> extraFns;   // dispatcher / isa / baslangic islevleri
  string build(int prog) {
    Gen G0; vector<string> sink; G0.out = &sink; G0.dry = 1; g = &G0; G0.scopes.emplace_back();
    for (int k : a[prog].kid) {
      const Node& n = a[k];
      if (n.k == N_CLASS) declareClass(k);
      else if (n.k == N_FUNC) declareFunc(k, -1);
      else if (n.k == N_VAR) declGlobalVar(k);
    }
    // govdeler
    for (size_t i = 0; i < F.size(); i++) if (F[i].hasBody && !F[i].deleted) genBody(int(i));
    // main
    int mainFn = -1; auto it = fmap.find("main"); if (it != fmap.end()) for (int m : it->second) if (F[size_t(m)].params.empty()) mainFn = m;
    if (mainFn < 0) fail("E_MAIN", "int main() bulunamadi");
    if (unq(F[size_t(mainFn)].ret) != tInt) fail("E_MAIN", "main() int dondurmeli");
    F[size_t(mainFn)].used = true;
    // __init: global ilklendirme
    { Gen G; G.fn = -1; vector<string> L; G.out = &L; g = &G; G.scopes.emplace_back();
      L.push_back("func __init() {");
      for (auto& l : prologueLines()) L.push_back(l);
      for (int vn : gInitList) {
        const Node& v = a[vn]; Var& var = globals[size_t(gmap[v.s])]; curLine = v.line; int init = v.kid.size() > 1 ? v.kid[1] : -1;
        if (init < 0 && !isCls(unq(var.ty)) && !isArr(var.ty)) continue;
        string ad = R(); E("mov " + ad + ", " + var.sym); initObj(ad, var.ty, init, int(v.i)); G.nreg = 0 + G.nreg;
      }
      emitRet("0"); L.push_back("}");
      string pre; for (auto& l : L) { size_t p = l.find("@FRAME@"); if (p != string::npos) l.replace(p, 7, num(std::max(G.frameOff, 1))); }
      // global ilklendirme gecici belleği icin yigin cercevesi: __start ayirir (fp = taban) -- cerceve boyutunu sakla
      initFrame = std::max(G.frameOff, 1); extraFns.insert(extraFns.end(), L.begin(), L.end()); (void)pre; }
    // sanal islev dagiticilari
    for (size_t i = 0; i < F.size(); i++) {
      Fn& f = F[i]; if (!f.isVirt || f.vroot != int(i)) continue;
      string line = "func vd" + std::to_string(i) + "("; int np = int(f.params.size()) + 1 + (isCls(unq(f.ret)) && !isRef(f.ret) ? 1 : 0); bool sret = np > int(f.params.size()) + 1;
      for (int k = 0; k < np; k++) line += (k ? ", p" : "p") + std::to_string(k); line += ") {"; extraFns.push_back(line);
      int thisIdx = sret ? 1 : 0; extraFns.push_back("  ld _id, p" + std::to_string(thisIdx) + ", 0");
      int lab = 0;
      for (size_t c = 0; c < C.size(); c++) {
        if (!C[c].defined || !derivedFrom(int(c), f.cls)) continue;
        int fin = finalOverrider(int(c), int(i)); if (F[size_t(fin)].pure || !F[size_t(fin)].hasBody) { if (F[size_t(fin)].pure) continue; }
        F[size_t(fin)].used = true;
        string nx = "_n" + std::to_string(lab++);
        extraFns.push_back("  eq _c, _id, " + num(C[c].id)); extraFns.push_back("  jz _c, " + nx);
        string call = "  call _r, " + F[size_t(fin)].pma; for (int k = 0; k < np; k++) call += ", p" + std::to_string(k); extraFns.push_back(call); extraFns.push_back("  ret _r");
        extraFns.push_back(nx + ": mov _z, 0");
      }
      extraFns.push_back("  ld _trp, -1, 0"); extraFns.push_back("  ret 0"); extraFns.push_back("}");
    }
    if (tiNameUsed) {   // typeid(...).name(): tum siniflar + typeid ile gorulen turler
      std::map<int, string> names; for (size_t c = 0; c < C.size(); c++) if (C[c].defined && !C[c].builtin) names[C[c].id] = mangle(clsTy(int(c)));
      for (auto& kv : tiTypes) names[kv.first] = mangle(kv.second);
      extraFns.push_back("func tiname(p0) {");
      for (auto& kv : names) { string sym = strData(kv.second); extraFns.push_back("  eq _c, p0, " + num(kv.first)); extraFns.push_back("  jz _c, _nx" + num(kv.first)); extraFns.push_back("  mov _r, " + sym); extraFns.push_back("  ret _r"); extraFns.push_back("_nx" + num(kv.first) + ": mov _z, 0"); }
      extraFns.push_back("  ret 0"); extraFns.push_back("}");
    }
    // isa<D>(id)
    for (int d : isaNeeded) {
      extraFns.push_back("func isa" + std::to_string(d) + "(p0) {");
      for (size_t c = 0; c < C.size(); c++) if (C[c].defined && C[c].poly && derivedFrom(int(c), d)) { extraFns.push_back("  eq _c, p0, " + num(C[c].id)); extraFns.push_back("  jnz _c, _yes"); }
      extraFns.push_back("  ret 0"); extraFns.push_back("_yes: ret 1"); extraFns.push_back("}");
    }
    // cikti
    int64_t dw = 0;   // veri sozcukleri: bellek en az 2^21; global dizi buyukse veri + yigin/yigma payi
    for (auto& d : dataLines) { if (d.rfind("space ", 0) == 0) { size_t q = d.rfind(' '); dw += atoll(d.c_str() + q + 1); } else dw += 1 + std::count(d.begin(), d.end(), ','); }
    int64_t memw = std::max<int64_t>(int64_t(1) << 21, dw + 1572864);
    string out; out += "memory " + num(memw) + "\n";
    out += "data __sp = 0\ndata __lim = 0\n";
    for (auto& d : dataLines) out += d + "\n";
    for (auto& f : F) { if (f.used && !f.hasBody && f.builtin.empty() && !f.deleted) fail("E_UNDEF", "tanimsiz islev kullanildi: " + f.name, f.line); }
    for (auto& f : F) { if (!f.generated) continue; for (auto& l : f.lines) out += l + "\n"; }
    for (auto& l : extraFns) out += l + "\n";
    out += "func __start() {\n  alloc _base, " + num(stackWords) + "\n  mov _spa, __sp\n  st _base, _spa, 0\n  add _top, _base, " + num(stackWords - 16) + "\n  mov _lma, __lim\n  st _top, _lma, 0\n  call __init\n  call _r, " + F[size_t(mainFn)].pma + "\n  ret _r\n}\nentry __start\n";
    return out;
  }
  int initFrame = 1; int64_t stackWords = 1 << 18;
};

const char* kPrelude = "int __getc(); void __puti(long long); void __putc(int); void __putf(double); void pl(long double); void pi(long long x) { __puti(x); __putc(10); } void pc(int c) { __putc(c); } void pd(double d) { __putf(d); __putc(10); } ";

// istisna onsozu: yalniz kaynakta 'try' varsa eklenir (std::exception / bad_cast / bad_typeid gercek sinif tanimlari; sayisal derleyici bunlari siradan sinif olarak derler)
const char* kPreludeExc = "class std::exception { public: virtual const char* what() const noexcept { return \"std::exception\"; } }; class std::bad_cast : std::exception { public: virtual const char* what() const noexcept { return \"std::bad_cast\"; } }; class std::bad_typeid : std::exception { public: virtual const char* what() const noexcept { return \"std::bad_typeid\"; } }; ";
bool usesF128(const string& src) { Lexer l0(src); l0.run(); for (auto& tk : l0.out) if (tk.k == TK_ID && tk.s == "__float128") return true; return false; }
bool hasTryWord(const string& src) { for (size_t i = 0; i + 3 < src.size(); i++) if (!src.compare(i, 3, "try") && (i == 0 || !(isalnum((unsigned char)src[i-1]) || src[i-1] == '_')) && !(isalnum((unsigned char)src[i+3]) || src[i+3] == '_')) return true; return false; }
bool slurp(const string& f, string& out) { FILE* fp = fopen(f.c_str(), "rb"); if (!fp) return false; char b[65536]; size_t n; out.clear(); while ((n = fread(b, 1, sizeof b, fp)) > 0) out.append(b, n); fclose(fp); return true; }
}  // namespace

// pm ifade kaynak.cx [--pmx1 sayilar.txt] [--pmx1-oku sayilar.txt] [--pma cikti.pma] [-o cikti.pmb] [--strip] [--ops]
int ifade_main(int argc, char** argv) {
  if (!strcmp(argv[2], "--ops")) { for (auto& r : kOps) printf("%d %d %d %s\n", r.cls, r.prec, r.assoc, r.tok); return 0; }   // derlenmis operator tablosu (komutlar.pmk bolum 11'den uretilen baslik)
  string file = argv[2], pmxOut, pmxIn, pmaOut, pmbOut; bool strip = false;
  for (int i = 3; i < argc; i++) {
    if (!strcmp(argv[i], "--pmx1") && i + 1 < argc) pmxOut = argv[++i]; else if (!strcmp(argv[i], "--pmx1-oku") && i + 1 < argc) pmxIn = argv[++i];
    else if (!strcmp(argv[i], "--pma") && i + 1 < argc) pmaOut = argv[++i]; else if (!strcmp(argv[i], "-o") && i + 1 < argc) pmbOut = argv[++i]; else if (!strcmp(argv[i], "--strip")) strip = true;
    else { fprintf(stderr, "kullanim: pm ifade kaynak.cx [--pmx1 sayilar.txt] [--pmx1-oku sayilar.txt] [--pma x.pma] [-o x.pmb] [--strip]\n"); return 1; }
  }
  try {
    Ast ast; int root;
    if (!pmxIn.empty()) {
      string t; if (!slurp(pmxIn, t)) fail("E_DOSYA", "PMX1 dosyasi okunamadi: " + pmxIn);
      vector<int64_t> v; std::istringstream is(t); long long x; while (is >> x) v.push_back(x);
      ast = pmx_decode(v, root);
    } else {
      string src; if (!slurp(file, src)) fail("E_DOSYA", "kaynak okunamadi: " + file);
      string full = string(kPrelude) + (usesF128(src) ? "void pq(__float128); " : "") + (hasTryWord(src) ? kPreludeExc : "") + src;
      Lexer lx(full); lx.run(); Parser ps(ast, lx.out); root = ps.topLevel();
      vector<int64_t> nums = pmx_encode(ast, root);
      if (!pmxOut.empty()) { FILE* fp = fopen(pmxOut.c_str(), "wb"); if (!fp) fail("E_DOSYA", "yazilamadi: " + pmxOut); for (size_t i = 0; i < nums.size(); i++) fprintf(fp, "%lld%c", (long long)nums[i], i + 1 == nums.size() ? '\n' : ' '); fclose(fp); }
      int r2; ast = pmx_decode(nums, r2); root = r2;   // derleme sayilardan
    }
    Sema sm(ast); string pma = sm.build(root);
    if (!pmaOut.empty()) { FILE* fp = fopen(pmaOut.c_str(), "wb"); if (!fp) fail("E_DOSYA", "yazilamadi: " + pmaOut); fwrite(pma.data(), 1, pma.size(), fp); fclose(fp); }
    if (!pmbOut.empty()) {
      Program p = assemble(pma); auto bin = encode_program(p, strip);
      FILE* fp = fopen(pmbOut.c_str(), "wb"); if (!fp) fail("E_DOSYA", "yazilamadi: " + pmbOut); fwrite(bin.data(), 1, bin.size(), fp); fclose(fp);
    }
    return 0;
  } catch (IErr& e) {
    fprintf(stderr, "HATA [%s] satir %d: %s\n", e.code.c_str(), e.line, e.msg.c_str()); return 1;
  } catch (PmError& e) {
    fprintf(stderr, "HATA [E_PMA] uretilen PMA reddedildi: %s\n", e.what()); return 1;
  }
}
