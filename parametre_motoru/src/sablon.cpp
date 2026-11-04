// Sablon / ornekleme katmani: bildirimsel uygulama tanimi (.pmt) -> yazma bicimi (.pma).
// Bu bir YORUMLAYICI degil, bir OZELLESTIRICI: tanim nesne sayisindan bagimsiz kisa kod uretir (ornek basina komut yok),
// uretilen kod mevcut ayristirici ve makine kodu ureticisinden gecer. Alan bir kuralla hic yazilmiyorsa bellekte tutulmaz,
// uretecin formulunden hesaplanir (sanal alan).
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include "program.h"

namespace {

[[noreturn]] void err(int line, const std::string& m) { throw PmError("satir " + std::to_string(line) + ": " + m); }

std::vector<std::string> split_ws(const std::string& line) {
  std::string s0 = line; size_t sc = s0.find(';'); if (sc != std::string::npos) s0.resize(sc);
  std::string s; for (char c : s0) { if (c == ',') s += " , "; else s.push_back(c); }   // virgul ayri bir simge
  std::vector<std::string> t; std::string cur;
  for (char c : s) { if (c == ' ' || c == '\t' || c == '\r') { if (!cur.empty()) { t.push_back(cur); cur.clear(); } } else cur.push_back(c); }
  if (!cur.empty()) t.push_back(cur);
  return t;
}
bool to_int(const std::string& s, long long& v) {
  if (s.empty()) return false;
  size_t k = (s[0] == '-' || s[0] == '+') ? 1 : 0;
  if (k >= s.size() || !isdigit((unsigned char)s[k])) return false;
  int base = (s.size() > k + 1 && s[k] == '0' && (s[k + 1] == 'x' || s[k + 1] == 'X')) ? 16 : 10;
  char* e; errno = 0; long long x = strtoll(s.c_str(), &e, base);
  if (*e != 0 || errno == ERANGE) return false;
  v = x; return true;
}
bool is_name(const std::string& s) {
  if (s.empty() || !(isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
  for (char c : s) if (!(isalnum((unsigned char)c) || c == '_')) return false;
  return true;
}
bool is_cmp(const std::string& s) { return s == "<" || s == ">" || s == "<=" || s == ">=" || s == "==" || s == "!="; }
bool is_binop(const std::string& s) { return s == "+" || s == "-" || s == "*" || s == "/" || s == "%" || s == "&" || s == "|" || s == "^" || s == "<<" || s == ">>"; }

typedef std::vector<std::string> Toks;
struct Line { int no; Toks t; };

struct Rule { int line; std::string target; Toks expr, cond; };
enum FKind { F_SABIT, F_DOGRUSAL, F_MOD, F_BOL, F_RASTGELE };
struct Field { std::string name; FKind kind = F_SABIT; long long p[4] = {0, 0, 0, 0}; bool written = false; };
struct Draw { int line; std::string shape; std::vector<Toks> args; };
struct Tmpl {
  std::string name; long long n = 0; int line = 0;
  std::vector<Field> fields; std::map<std::string, int> fidx;
  std::vector<Rule> rules; std::vector<Draw> draws; bool canli_written = false;
  bool stored(const Field& f) const { return f.written || f.kind == F_RASTGELE; }
};
struct Inter { int line; int a, b; Toks cond; std::vector<Rule> rules; };
struct Input { int line; long long code; int tmpl; std::vector<Rule> rules; };
struct Agg { int line; std::string g, op; int t; Toks expr, cond; };
struct Macro { std::vector<std::string> params; std::vector<Line> body; };
struct Scope { int t = -1; int a = -1, b = -1; };   // t: tek sablon (indeks yazmaci i) | a,b: etkilesim (ia, ib) | hepsi -1: genel

struct Gen {
  // ---- tanim
  int W = 0, H = 0; long long frames = 0; long long bg = 0; bool has_screen = false;
  std::vector<std::pair<std::string, long long>> globals; std::map<std::string, int> gidx;
  std::vector<Tmpl> tm; std::map<std::string, int> tidx;
  std::vector<Inter> inters; std::vector<Input> inputs; std::vector<Agg> aggs;
  Toks bitis; int bitis_line = 0; std::vector<Toks> yazdir; int yazdir_line = 0;
  std::map<std::string, Macro> macros;
  // ---- uretim
  std::vector<std::string> out; int ntmp = 0, nlab = 0;
  std::string tmp() { return "v" + std::to_string(ntmp++); }
  std::string lab() { return "L" + std::to_string(nlab++); }
  void emit(const std::string& s) { out.push_back("    " + s); }
  void label(const std::string& l) { out.push_back(l + ":"); }

  // ===================== ayrıştırma =====================
  void read_lines(const std::string& path, std::vector<Line>& lines, int depth) {
    if (depth > 8) throw PmError("ekle: cok derin ic ice dosya");
    std::ifstream f(path);
    if (!f) throw PmError("tanim dosyasi okunamadi: " + path);
    std::string raw; int no = 0;
    std::string dir = path.find('/') == std::string::npos ? "" : path.substr(0, path.rfind('/') + 1);
    while (std::getline(f, raw)) {
      no++; Toks t = split_ws(raw); if (t.empty()) continue;
      if (t[0] == "ekle") { if (t.size() != 2) err(no, "ekle DOSYA"); read_lines(dir + t[1], lines, depth + 1); continue; }
      lines.push_back({no, t});
    }
  }
  static Rule parse_rule(const Line& L, size_t from) {   // kural HEDEF = IFADE [eger KOSUL]
    const Toks& t = L.t;
    if (t.size() < from + 3 || t[from + 1] != "=") err(L.no, "kural HEDEF = IFADE [eger KOSUL] bekleniyor");
    Rule r; r.line = L.no; r.target = t[from];
    size_t i = from + 2;
    for (; i < t.size() && t[i] != "eger"; i++) r.expr.push_back(t[i]);
    if (r.expr.empty()) err(L.no, "kural: ifade bos");
    if (i < t.size()) { for (i++; i < t.size(); i++) r.cond.push_back(t[i]); if (r.cond.empty()) err(L.no, "eger: kosul bos"); }
    return r;
  }
  void add_rules(std::vector<Rule>& dst, const Line& L, std::vector<Line>& pending) {   // 'kural' ya da 'davran' satiri
    (void)pending;
    if (L.t[0] == "kural") { dst.push_back(parse_rule(L, 1)); return; }
    if (L.t[0] == "davran") {
      if (L.t.size() < 2) err(L.no, "davran AD arg...");
      auto it = macros.find(L.t[1]); if (it == macros.end()) err(L.no, "bilinmeyen davranis: " + L.t[1]);
      const Macro& m = it->second;
      if (L.t.size() - 2 != m.params.size()) err(L.no, "davranis " + L.t[1] + " " + std::to_string(m.params.size()) + " arguman ister");
      for (const Line& b : m.body) {
        Line x{L.no, {}};
        for (const std::string& tok : b.t) {
          size_t k = std::find(m.params.begin(), m.params.end(), tok) - m.params.begin();
          x.t.push_back(k < m.params.size() ? L.t[2 + k] : tok);
        }
        dst.push_back(parse_rule(x, 1));
      }
      return;
    }
    err(L.no, "beklenmeyen satir: " + L.t[0]);
  }
  int tmpl_of(const std::string& n, int line) { auto it = tidx.find(n); if (it == tidx.end()) err(line, "bilinmeyen sablon: " + n); return it->second; }

  void parse(const std::vector<Line>& L) {
    for (size_t i = 0; i < L.size(); i++) {
      const Line& l = L[i]; const Toks& t = l.t; const std::string& k = t[0];
      auto block = [&](std::vector<Line>& body) {   // 'son'a kadar
        i++; while (i < L.size() && L[i].t[0] != "son") body.push_back(L[i++]);
        if (i >= L.size()) err(l.no, "'son' eksik");
      };
      if (k == "sahne") { long long w, h; if (t.size() != 3 || !to_int(t[1], w) || !to_int(t[2], h) || w <= 0 || h <= 0 || w > 4096 || h > 4096) err(l.no, "sahne GENISLIK YUKSEKLIK"); W = int(w); H = int(h); has_screen = true; }
      else if (k == "kare") { if (t.size() != 2 || !to_int(t[1], frames) || frames < 0) err(l.no, "kare N"); }
      else if (k == "arkaplan") { if (t.size() != 2 || !to_int(t[1], bg)) err(l.no, "arkaplan RENK"); }
      else if (k == "global") {
        long long v = 0; if (t.size() < 2 || !is_name(t[1]) || t[1] == "kare") err(l.no, "global AD [= DEGER]");
        if (t.size() == 4 && t[2] == "=") { if (!to_int(t[3], v)) err(l.no, "global degeri tamsayi olmali"); } else if (t.size() != 2) err(l.no, "global AD [= DEGER]");
        if (gidx.count(t[1])) err(l.no, "global tekrarlandi: " + t[1]);
        gidx[t[1]] = int(globals.size()); globals.push_back({t[1], v});
      }
      else if (k == "davranis") {
        std::string head; for (size_t j = 1; j < t.size(); j++) head += t[j];
        size_t lp = head.find('('), rp = head.find(')');
        if (lp == std::string::npos || rp == std::string::npos || rp < lp) err(l.no, "davranis AD(p1,p2,...)");
        Macro m; std::string name = head.substr(0, lp), ps = head.substr(lp + 1, rp - lp - 1), cur;
        if (!is_name(name)) err(l.no, "gecersiz davranis adi");
        for (char c : ps + ",") { if (c == ',') { if (!cur.empty()) m.params.push_back(cur); cur.clear(); } else cur.push_back(c); }
        block(m.body); macros[name] = m;
      }
      else if (k == "sablon") {
        long long n; if (t.size() != 4 || t[2] != "adet" || !is_name(t[1]) || !to_int(t[3], n) || n <= 0 || n > 10000000) err(l.no, "sablon AD adet N");
        if (tidx.count(t[1])) err(l.no, "sablon tekrarlandi: " + t[1]);
        Tmpl T; T.name = t[1]; T.n = n; T.line = l.no;
        std::vector<Line> body; block(body);
        std::vector<Line> none;
        for (const Line& b : body) {
          const std::string& bk = b.t[0];
          if (bk == "alan") {
            if (b.t.size() < 3 || !is_name(b.t[1])) err(b.no, "alan AD baslatici...");
            if (T.fidx.count(b.t[1])) err(b.no, "alan tekrarlandi: " + b.t[1]);
            Field f; f.name = b.t[1]; const std::string& g = b.t[2]; size_t need = 0;
            if (g == "sabit") { f.kind = F_SABIT; need = 1; } else if (g == "dogrusal") { f.kind = F_DOGRUSAL; need = 2; }
            else if (g == "mod") { f.kind = F_MOD; need = 3; } else if (g == "bol") { f.kind = F_BOL; need = 3; }
            else if (g == "rastgele") { f.kind = F_RASTGELE; need = 3; } else err(b.no, "bilinmeyen baslatici: " + g);
            if (b.t.size() != 3 + need) err(b.no, g + " " + std::to_string(need) + " sayi ister");
            for (size_t j = 0; j < need; j++) if (!to_int(b.t[3 + j], f.p[j])) err(b.no, "baslatici degeri tamsayi olmali");
            if ((f.kind == F_MOD || f.kind == F_BOL) && f.p[0] <= 0) err(b.no, "mod/bol: ilk sayi pozitif olmali");
            if (f.kind == F_RASTGELE && f.p[1] < f.p[0]) err(b.no, "rastgele: lo <= hi olmali");
            T.fidx[f.name] = int(T.fields.size()); T.fields.push_back(f);
          } else if (bk == "ciz") {
            Draw d; d.line = b.no;
            if (b.t.size() < 2) err(b.no, "ciz SEKIL ...");
            d.shape = b.t[1]; size_t want = d.shape == "dikdortgen" ? 5 : d.shape == "nokta" ? 3 : 0;
            if (!want) err(b.no, "bilinmeyen sekil: " + d.shape + " (dikdortgen | nokta)");
            // argumanlar: her biri tek operand ya da "A op B" ya da "min/max A B"; ayirici ','
            Toks cur; for (size_t j = 2; j <= b.t.size(); j++) { if (j == b.t.size() || b.t[j] == ",") { if (cur.empty()) err(b.no, "ciz: bos arguman"); d.args.push_back(cur); cur.clear(); } else cur.push_back(b.t[j]); }
            if (d.args.size() != want) err(b.no, "ciz " + d.shape + " " + std::to_string(want) + " arguman ister (',' ile ayrilir)");
            T.draws.push_back(d);
          } else add_rules(T.rules, b, none);
        }
        tidx[T.name] = int(tm.size()); tm.push_back(T);
      }
      else if (k == "etkilesim") {
        if (t.size() < 3) err(l.no, "etkilesim A B [eger KOSUL]");
        Inter e; e.line = l.no; e.a = tmpl_of(t[1], l.no); e.b = tmpl_of(t[2], l.no);
        if (e.a == e.b) err(l.no, "etkilesim: iki sablon farkli olmali");
        if (t.size() > 3) { if (t[3] != "eger" || t.size() == 4) err(l.no, "etkilesim A B eger KOSUL"); e.cond.assign(t.begin() + 4, t.end()); }
        std::vector<Line> body, none; block(body);
        for (const Line& b : body) add_rules(e.rules, b, none);
        inters.push_back(e);
      }
      else if (k == "girdi") {
        long long code; if (t.size() < 2 || !to_int(t[1], code)) err(l.no, "girdi KOD [SABLON]");
        Input in; in.line = l.no; in.code = code; in.tmpl = t.size() > 2 ? tmpl_of(t[2], l.no) : -1;
        std::vector<Line> body, none; block(body);
        for (const Line& b : body) add_rules(in.rules, b, none);
        inputs.push_back(in);
      }
      else if (k == "topla") {
        if (t.size() < 5) err(l.no, "topla GLOBAL toplam|say|enbuyuk|enkucuk SABLON IFADE [eger KOSUL]");
        Agg a; a.line = l.no; a.g = t[1]; a.op = t[2];
        if (a.op != "toplam" && a.op != "say" && a.op != "enbuyuk" && a.op != "enkucuk") err(l.no, "topla islemi: toplam|say|enbuyuk|enkucuk");
        a.t = tmpl_of(t[3], l.no);
        size_t j = 4; for (; j < t.size() && t[j] != "eger"; j++) a.expr.push_back(t[j]);
        if (j < t.size()) a.cond.assign(t.begin() + j + 1, t.end());
        if (a.op == "say" && a.expr.empty()) a.expr = {"1"}; else if (a.expr.empty()) err(l.no, "topla: ifade bos");
        aggs.push_back(a);
      }
      else if (k == "bitis") { if (t.size() < 3 || t[1] != "eger") err(l.no, "bitis eger KOSUL"); bitis.assign(t.begin() + 2, t.end()); bitis_line = l.no; }
      else if (k == "yazdir") {
        if (t.size() < 2) err(l.no, "yazdir IFADE...");
        yazdir_line = l.no;
        // ifadeler: tek operand ya da "A op B"; tek satirda bosluk ayirici zor oldugu icin her operand ayri ifade (",": grup ayirici)
        Toks cur; for (size_t j = 1; j <= t.size(); j++) { if (j == t.size() || t[j] == ",") { if (!cur.empty()) yazdir.push_back(cur); cur.clear(); } else cur.push_back(t[j]); }
      }
      else err(l.no, "bilinmeyen anahtar sozcuk: " + k);
    }
    for (auto& a : aggs) if (!gidx.count(a.g)) err(a.line, "topla: tanimsiz global: " + a.g);
  }

  // ===================== analiz: hangi alanlar yazilıyor? =====================
  void mark_target(const Rule& r, const Scope& sc, int line) {
    const std::string& tg = r.target; size_t dot = tg.find('.');
    if (dot != std::string::npos) {
      std::string p = tg.substr(0, dot), f = tg.substr(dot + 1); int ti;
      if (sc.a >= 0 && p == "a") ti = sc.a; else if (sc.a >= 0 && p == "b") ti = sc.b; else err(line, "hedef alan yalniz a.alan / b.alan olabilir (etkilesimde): " + tg);
      Tmpl& T = tm[size_t(ti)];
      if (f == "canli") { T.canli_written = true; return; }
      auto it = T.fidx.find(f); if (it == T.fidx.end()) err(line, "sablon " + T.name + " alani yok: " + f);
      T.fields[size_t(it->second)].written = true; return;
    }
    if (sc.t >= 0) {
      Tmpl& T = tm[size_t(sc.t)];
      if (tg == "canli") { T.canli_written = true; return; }
      auto it = T.fidx.find(tg); if (it != T.fidx.end()) { T.fields[size_t(it->second)].written = true; return; }
    }
    if (!gidx.count(tg)) err(line, "hedef bulunamadi (alan ya da global degil): " + tg);
  }
  void analyze() {
    for (size_t i = 0; i < tm.size(); i++) { Scope s; s.t = int(i); for (auto& r : tm[i].rules) mark_target(r, s, r.line); }
    for (auto& e : inters) { Scope s; s.a = e.a; s.b = e.b; for (auto& r : e.rules) mark_target(r, s, r.line); }
    for (auto& in : inputs) { Scope s; s.t = in.tmpl; for (auto& r : in.rules) mark_target(r, s, r.line); }
  }

  // ===================== kod uretimi: ifadeler =====================
  std::string sym_field(const Tmpl& T, const Field& f) const { return "s_" + T.name + "_" + f.name; }
  // alan okuma: kayitli ise bellekten; sanal ise ureteç formulunden
  std::string read_field(const Tmpl& T, const std::string& fname, const std::string& idx, int line) {
    if (fname == "canli") {
      if (!T.canli_written) return "1";
      std::string p = tmp(), v = tmp(); emit("add " + p + ", s_" + T.name + "_canli, " + idx); emit("ld " + v + ", " + p + ", 0"); return v;
    }
    auto it = T.fidx.find(fname); if (it == T.fidx.end()) err(line, "sablon " + T.name + " alani yok: " + fname);
    const Field& f = T.fields[size_t(it->second)];
    if (T.stored(f)) { std::string p = tmp(), v = tmp(); emit("add " + p + ", " + sym_field(T, f) + ", " + idx); emit("ld " + v + ", " + p + ", 0"); return v; }
    auto S = [](long long x) { return std::to_string(x); };
    std::string v;
    switch (f.kind) {
      case F_SABIT: return S(f.p[0]);
      case F_DOGRUSAL: v = tmp(); emit("mul " + v + ", " + idx + ", " + S(f.p[1])); emit("add " + v + ", " + v + ", " + S(f.p[0])); return v;
      case F_MOD: v = tmp(); emit("mod " + v + ", " + idx + ", " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); return v;
      case F_BOL: v = tmp(); emit("div " + v + ", " + idx + ", " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); return v;
      default: err(line, "dahili hata: sanal rastgele");
    }
  }
  std::string operand(const std::string& tok, const Scope& sc, int line) {
    long long lit; if (to_int(tok, lit)) return std::to_string(lit);
    size_t dot = tok.find('.');
    if (dot != std::string::npos) {
      std::string p = tok.substr(0, dot), f = tok.substr(dot + 1);
      if (sc.a >= 0 && p == "a") return read_field(tm[size_t(sc.a)], f, "ia", line);
      if (sc.a >= 0 && p == "b") return read_field(tm[size_t(sc.b)], f, "ib", line);
      return read_field(tm[size_t(tmpl_of(p, line))], f, "0", line);   // SABLON.alan = 0. ornek
    }
    if (tok == "i") { if (sc.t < 0) err(line, "'i' yalniz tek sablonlu kuralda kullanilir"); return "i"; }
    if (sc.t >= 0) {
      const Tmpl& T = tm[size_t(sc.t)];
      if (tok == "canli" || T.fidx.count(tok)) return read_field(T, tok, "i", line);
    }
    if (tok == "kare") { std::string v = tmp(); emit("ld " + v + ", g_kare, 0"); return v; }
    if (gidx.count(tok)) { std::string v = tmp(); emit("ld " + v + ", g_" + tok + ", 0"); return v; }
    err(line, "tanimsiz ad: " + tok);
  }
  std::string expr(const Toks& e, const Scope& sc, int line) {
    if (e.size() == 1) return operand(e[0], sc, line);
    if (e.size() == 3 && (e[0] == "min" || e[0] == "max")) {
      std::string a = operand(e[1], sc, line), b = operand(e[2], sc, line), r = tmp(), c = tmp(), l = lab();
      emit("mov " + r + ", " + a); emit((e[0] == "min" ? "lt " : "gt ") + c + ", " + b + ", " + r); emit("jz " + c + ", " + l); emit("mov " + r + ", " + b); label(l); return r;
    }
    if (e.size() == 3 && is_binop(e[1])) {
      std::string a = operand(e[0], sc, line), b = operand(e[2], sc, line), r = tmp();
      static const std::map<std::string, std::string> ops = {{"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"}, {"&", "and"}, {"|", "or"}, {"^", "xor"}, {"<<", "shl"}, {">>", "sar"}};
      emit(ops.at(e[1]) + " " + r + ", " + a + ", " + b); return r;
    }
    err(line, "ifade: tek operand, 'A op B' ya da 'min|max A B' bekleniyor");
  }
  // KOSUL: E1 CMP E2 [ve E3 CMP E4 ...]; saglanmazsa failLabel'a atlar
  void cond_fail(const Toks& c, const Scope& sc, const std::string& failLabel, int line) {
    size_t i = 0;
    while (i < c.size()) {
      Toks lhs, rhs; std::string cmp;
      while (i < c.size() && !is_cmp(c[i])) lhs.push_back(c[i++]);
      if (i >= c.size()) err(line, "kosul: karsilastirma isareti yok (< > <= >= == !=)");
      cmp = c[i++];
      while (i < c.size() && c[i] != "ve") rhs.push_back(c[i++]);
      if (lhs.empty() || rhs.empty()) err(line, "kosul: ifade bos");
      if (i < c.size()) i++;   // 've'
      std::string a = expr(lhs, sc, line), b = expr(rhs, sc, line), r = tmp();
      static const std::map<std::string, std::string> ops = {{"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"}, {"==", "eq"}, {"!=", "ne"}};
      emit(ops.at(cmp) + " " + r + ", " + a + ", " + b); emit("jz " + r + ", " + failLabel);
    }
  }
  void store_target(const std::string& tg, const std::string& val, const Scope& sc, int line) {
    size_t dot = tg.find('.');
    auto st_field = [&](const Tmpl& T, const std::string& f, const std::string& idx) {
      std::string p = tmp();
      if (f == "canli") { emit("add " + p + ", s_" + T.name + "_canli, " + idx); emit("st " + val + ", " + p + ", 0"); return; }
      auto it = T.fidx.find(f); if (it == T.fidx.end()) err(line, "sablon " + T.name + " alani yok: " + f);
      emit("add " + p + ", " + sym_field(T, T.fields[size_t(it->second)]) + ", " + idx); emit("st " + val + ", " + p + ", 0");
    };
    if (dot != std::string::npos) { std::string p = tg.substr(0, dot), f = tg.substr(dot + 1); st_field(tm[size_t(p == "a" ? sc.a : sc.b)], f, p == "a" ? "ia" : "ib"); return; }
    if (sc.t >= 0) { const Tmpl& T = tm[size_t(sc.t)]; if (tg == "canli" || T.fidx.count(tg)) { st_field(T, tg, "i"); return; } }
    emit("st " + val + ", g_" + tg + ", 0");
  }
  void emit_rule(const Rule& r, const Scope& sc) {
    ntmp = 0;
    std::string skip = lab();
    if (!r.cond.empty()) cond_fail(r.cond, sc, skip, r.line);
    std::string v = expr(r.expr, sc, r.line);
    store_target(r.target, v, sc, r.line);
    label(skip);
  }

  // ===================== kod uretimi: islevler =====================
  void emit_init() {
    out.push_back("func baslat() {");
    for (auto& T : tm) {
      bool any = T.canli_written; for (auto& f : T.fields) if (T.stored(f)) any = true;
      if (!any) continue;
      std::string l = lab(), d = lab();
      emit("mov i, 0"); label(l); emit("lt c, i, " + std::to_string(T.n)); emit("jz c, " + d);
      auto S = [](long long x) { return std::to_string(x); };
      auto st = [&](const std::string& sym, const std::string& v) { emit("add p, " + sym + ", i"); emit("st " + v + ", p, 0"); };
      if (T.canli_written) st("s_" + T.name + "_canli", "1");
      for (auto& f : T.fields) {
        if (!T.stored(f)) continue;
        ntmp = 0; std::string v = tmp();
        switch (f.kind) {
          case F_SABIT: emit("mov " + v + ", " + S(f.p[0])); break;
          case F_DOGRUSAL: emit("mul " + v + ", i, " + S(f.p[1])); emit("add " + v + ", " + v + ", " + S(f.p[0])); break;
          case F_MOD: emit("mod " + v + ", i, " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); break;
          case F_BOL: emit("div " + v + ", i, " + S(f.p[0])); emit("mul " + v + ", " + v + ", " + S(f.p[2])); emit("add " + v + ", " + v + ", " + S(f.p[1])); break;
          case F_RASTGELE:   // h1=(i*1664525 + tohum*1013904223 + 12345) & 0xFFFFFF ; h2=(h1*1664525 + 1013904223) & 0xFFFFFF ; lo + (h2>>4) % (hi-lo+1)
            emit("mul " + v + ", i, 1664525"); emit("add " + v + ", " + v + ", " + S(f.p[2] * 1013904223LL + 12345)); emit("and " + v + ", " + v + ", 16777215");
            emit("mul " + v + ", " + v + ", 1664525"); emit("add " + v + ", " + v + ", 1013904223"); emit("and " + v + ", " + v + ", 16777215");
            emit("shr " + v + ", " + v + ", 4"); emit("mod " + v + ", " + v + ", " + S(f.p[1] - f.p[0] + 1)); emit("add " + v + ", " + v + ", " + S(f.p[0])); break;
        }
        st(sym_field(T, f), v);
      }
      emit("add i, i, 1"); emit("jmp " + l); label(d);
    }
    emit("ret 0"); out.push_back("}");
  }
  void alive_skip(const Tmpl& T, const std::string& idx, const std::string& skip) {
    if (!T.canli_written) return;
    emit("add p, s_" + T.name + "_canli, " + idx); emit("ld cn, p, 0"); emit("jz cn, " + skip);
  }
  void emit_template_rules(int ti) {
    const Tmpl& T = tm[size_t(ti)];
    out.push_back("func r_" + T.name + "() {");
    std::string l = lab(), nx = lab(), d = lab();
    emit("mov i, 0"); label(l); emit("lt c, i, " + std::to_string(T.n)); emit("jz c, " + d);
    alive_skip(T, "i", nx);
    Scope s; s.t = ti; for (auto& r : T.rules) emit_rule(r, s);
    label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d); emit("ret 0"); out.push_back("}");
  }
  void emit_inter(size_t k) {
    const Inter& e = inters[k]; const Tmpl& A = tm[size_t(e.a)]; const Tmpl& B = tm[size_t(e.b)];
    out.push_back("func e_" + std::to_string(k) + "() {");
    std::string la = lab(), lb = lab(), nxa = lab(), nxb = lab(), d = lab();
    emit("mov ia, 0"); label(la); emit("lt c, ia, " + std::to_string(A.n)); emit("jz c, " + d);
    alive_skip(A, "ia", nxa);
    emit("mov ib, 0"); label(lb); emit("lt c, ib, " + std::to_string(B.n)); emit("jz c, " + nxa);
    alive_skip(B, "ib", nxb);
    Scope s; s.a = e.a; s.b = e.b;
    if (!e.cond.empty()) { ntmp = 0; cond_fail(e.cond, s, nxb, e.line); }
    for (auto& r : e.rules) emit_rule(r, s);
    label(nxb); emit("add ib, ib, 1"); emit("jmp " + lb); label(nxa); emit("add ia, ia, 1"); emit("jmp " + la); label(d); emit("ret 0"); out.push_back("}");
  }
  void emit_input(size_t k) {
    const Input& in = inputs[k];
    out.push_back("func gi_" + std::to_string(k) + "() {");
    if (in.tmpl < 0) { Scope s; for (auto& r : in.rules) emit_rule(r, s); }
    else {
      const Tmpl& T = tm[size_t(in.tmpl)]; std::string l = lab(), nx = lab(), d = lab();
      emit("mov i, 0"); label(l); emit("lt c, i, " + std::to_string(T.n)); emit("jz c, " + d);
      alive_skip(T, "i", nx);
      Scope s; s.t = in.tmpl; for (auto& r : in.rules) emit_rule(r, s);
      label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d);
    }
    emit("ret 0"); out.push_back("}");
  }
  void emit_agg(size_t k) {
    const Agg& a = aggs[k]; const Tmpl& T = tm[size_t(a.t)];
    out.push_back("func a_" + std::to_string(k) + "() {");
    long long init = a.op == "enbuyuk" ? -(1LL << 62) : a.op == "enkucuk" ? (1LL << 62) : 0;
    emit("mov acc, " + std::to_string(init));
    std::string l = lab(), nx = lab(), d = lab();
    emit("mov i, 0"); label(l); emit("lt c, i, " + std::to_string(T.n)); emit("jz c, " + d);
    alive_skip(T, "i", nx);
    Scope s; s.t = a.t; ntmp = 0;
    if (!a.cond.empty()) cond_fail(a.cond, s, nx, a.line);
    std::string v = expr(a.expr, s, a.line);
    if (a.op == "toplam") emit("add acc, acc, " + v);
    else if (a.op == "say") emit("add acc, acc, 1");
    else { std::string sk = lab(), cc = tmp(); emit((a.op == "enbuyuk" ? "gt " : "lt ") + cc + ", " + v + ", acc"); emit("jz " + cc + ", " + sk); emit("mov acc, " + v); label(sk); }
    label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d);
    emit("st acc, g_" + a.g + ", 0"); emit("ret 0"); out.push_back("}");
  }
  void emit_draw(size_t ti) {
    const Tmpl& T = tm[ti];
    out.push_back("func c_" + T.name + "() {");
    std::string l = lab(), nx = lab(), d = lab();
    emit("mov i, 0"); label(l); emit("lt c, i, " + std::to_string(T.n)); emit("jz c, " + d);
    alive_skip(T, "i", nx);
    Scope s; s.t = int(ti);
    for (auto& dr : T.draws) {
      ntmp = 0; std::vector<std::string> v;
      for (auto& a : dr.args) v.push_back(expr(a, s, dr.line));
      if (dr.shape == "dikdortgen") emit("call z, rect, " + v[0] + ", " + v[1] + ", " + v[2] + ", " + v[3] + ", " + v[4]);
      else emit("call z, putpx, " + v[0] + ", " + v[1] + ", " + v[2]);
    }
    label(nx); emit("add i, i, 1"); emit("jmp " + l); label(d); emit("ret 0"); out.push_back("}");
  }
  void emit_screen_lib() {
    long long wh = (long long)W * H;
    out.push_back("func putpx(x, y, col) {");
    emit("lt t, x, 0"); emit("jnz t, out"); emit("ge t, x, " + std::to_string(W)); emit("jnz t, out");
    emit("lt t, y, 0"); emit("jnz t, out"); emit("ge t, y, " + std::to_string(H)); emit("jnz t, out");
    emit("mul k, y, " + std::to_string(W)); emit("add k, k, x"); emit("add p, fb, k"); emit("st col, p, 0"); label("out"); emit("ret 0"); out.push_back("}");
    out.push_back("func rect(x, y, w, h, col) {");
    emit("mov yy, 0"); label("ry"); emit("lt c, yy, h"); emit("jz c, done"); emit("mov xx, 0"); label("rx"); emit("lt c, xx, w"); emit("jz c, nexty");
    emit("add px, x, xx"); emit("add py, y, yy"); emit("call z, putpx, px, py, col"); emit("add xx, xx, 1"); emit("jmp rx");
    label("nexty"); emit("add yy, yy, 1"); emit("jmp ry"); label("done"); emit("ret 0"); out.push_back("}");
    out.push_back("func temizle() {");
    emit("mov i, 0"); label("l"); emit("lt c, i, " + std::to_string(wh)); emit("jz c, d"); emit("add p, fb, i"); emit("st " + std::to_string(bg) + ", p, 0"); emit("add i, i, 1"); emit("jmp l"); label("d"); emit("ret 0"); out.push_back("}");
  }

  std::string generate() {
    analyze();
    if (!has_screen && frames == 0) throw PmError("sahne yoksa 'kare N' (N > 0) gerekli: uygulama baska turlu sonlanmaz");
    for (auto& d : inputs) (void)d;
    // bellek: alanlar + cerceve + globaller
    long long words = 64 + (long long)globals.size() + 1 + (has_screen ? (long long)W * H : 0);
    for (auto& T : tm) { for (auto& f : T.fields) if (T.stored(f)) words += T.n; if (T.canli_written) words += T.n; }
    long long nvirt = 0, nstored = 0, wstored = 0;
    for (auto& T : tm) { for (auto& f : T.fields) { if (T.stored(f)) { nstored++; wstored += T.n; } else nvirt += T.n; } if (T.canli_written) { nstored++; wstored += T.n; } }
    out.push_back("; .pmt'den uretildi (sablon katmani) -- elle duzenlenmez");
    out.push_back("; ozet sanal_alan_sozcugu=" + std::to_string(nvirt) + " kayitli_alan=" + std::to_string(nstored) + " kayitli_sozcuk=" + std::to_string(wstored) + " cerceve_sozcugu=" + std::to_string(has_screen ? (long long)W * H : 0));
    out.push_back("memory " + std::to_string(std::max<long long>(4096, words + 1024)));
    for (auto& g : globals) out.push_back("data g_" + g.first + " = " + std::to_string(g.second));
    out.push_back("data g_kare = 0");
    for (auto& T : tm) {
      for (auto& f : T.fields) if (T.stored(f)) out.push_back("space " + sym_field(T, f) + " " + std::to_string(T.n));
      if (T.canli_written) out.push_back("space s_" + T.name + "_canli " + std::to_string(T.n));
    }
    if (has_screen) { out.push_back("space fb " + std::to_string((long long)W * H)); emit_screen_lib(); }
    emit_init();
    for (size_t i = 0; i < tm.size(); i++) emit_template_rules(int(i));
    for (size_t k = 0; k < inters.size(); k++) emit_inter(k);
    for (size_t k = 0; k < inputs.size(); k++) emit_input(k);
    for (size_t k = 0; k < aggs.size(); k++) emit_agg(k);
    if (has_screen) for (size_t i = 0; i < tm.size(); i++) if (!tm[i].draws.empty()) emit_draw(i);
    // ---- ana dongu
    out.push_back("func main() {");
    emit("call z, baslat"); emit("mov f, 0");
    label("fl");
    if (frames > 0) { emit("lt c, f, " + std::to_string(frames)); emit("jz c, fin"); }
    if (!inputs.empty()) {
      label("ev"); emit("host e, poll"); emit("jz e, evd"); emit("and key, e, 255");
      for (size_t k = 0; k < inputs.size(); k++) {
        std::string nk = "nk" + std::to_string(k);
        emit("eq t, key, " + std::to_string(inputs[k].code)); emit("jz t, " + nk); emit("call z, gi_" + std::to_string(k)); emit("jmp ev"); label(nk);
      }
      emit("jmp ev"); label("evd");
    }
    for (auto& T : tm) if (!T.rules.empty()) emit("call z, r_" + T.name);
    for (size_t k = 0; k < inters.size(); k++) emit("call z, e_" + std::to_string(k));
    for (size_t k = 0; k < aggs.size(); k++) emit("call z, a_" + std::to_string(k));
    if (!bitis.empty()) { ntmp = 0; Scope s; cond_fail(bitis, s, "nobit", bitis_line); emit("jmp fin"); label("nobit"); }
    if (has_screen) {
      emit("call z, temizle");
      for (size_t i = 0; i < tm.size(); i++) if (!tm[i].draws.empty()) emit("call z, c_" + tm[i].name);
      emit("host r, present, fb, " + std::to_string(W) + ", " + std::to_string(H)); emit("jnz r, fin");
    }
    emit("ld fk, g_kare, 0"); emit("add fk, fk, 1"); emit("st fk, g_kare, 0");
    emit("add f, f, 1"); emit("jmp fl");
    label("fin");
    if (!yazdir.empty()) {
      ntmp = 0; Scope s;
      for (size_t k = 0; k < yazdir.size(); k++) {
        std::string v = expr(yazdir[k], s, yazdir_line); emit("host puti, " + v); emit(std::string("host putc, ") + (k + 1 < yazdir.size() ? "32" : "10"));
      }
    }
    emit("ret 0"); out.push_back("}");
    std::string res; for (auto& l : out) res += l + "\n";
    return res;
  }
};
}  // namespace

std::string sablon_to_pma(const std::string& path) {
  Gen g; std::vector<Line> lines;
  g.read_lines(path, lines, 0);
  g.parse(lines);
  return g.generate();
}
