// Yazma bicimi (.pma) -> Program. Adli yazmaclar, etiketler, sabitler; cagri pencereleri ve sabit yazmaclari burada uretilir.
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include "program.h"

namespace {

[[noreturn]] void err(int line, const std::string& m) { throw PmError("satir " + std::to_string(line) + ": " + m); }

bool is_ident(const std::string& s) {
  if (s.empty() || !(isalpha((unsigned char)s[0]) || s[0] == '_')) return false;
  for (char c : s) if (!(isalnum((unsigned char)c) || c == '_')) return false;
  return true;
}

int esc_char(char c, bool& ok) {
  ok = true;
  switch (c) { case 'n': return '\n'; case 't': return '\t'; case 'r': return '\r'; case '0': return 0;
               case '\\': return '\\'; case '"': return '"'; case '\'': return '\''; }
  ok = false; return 0;
}

// ';' ile baslayan yorumu kes (string ve karakter sabiti icindekini atla)
std::string strip_comment(const std::string& s) {
  bool in_str = false;
  for (size_t i = 0; i < s.size(); i++) {
    char c = s[i];
    if (in_str) { if (c == '\\') i++; else if (c == '"') in_str = false; continue; }
    if (c == '"') { in_str = true; continue; }
    if (c == '\'') { i += (i + 1 < s.size() && s[i + 1] == '\\') ? 3 : 2; continue; }
    if (c == ';') return s.substr(0, i);
  }
  return s;
}

std::vector<std::string> split(const std::string& s) {
  std::vector<std::string> t; std::string cur;
  for (char c : s) {
    if (c == ' ' || c == '\t' || c == ',' || c == '\r') { if (!cur.empty()) { t.push_back(cur); cur.clear(); } }
    else cur.push_back(c);
  }
  if (!cur.empty()) t.push_back(cur);
  return t;
}

bool parse_int(const std::string& s, int64_t& v) {
  if (s.size() >= 3 && s[0] == '\'' && s.back() == '\'') {
    if (s[1] == '\\' && s.size() == 4) { bool ok; v = esc_char(s[2], ok); return ok; }
    if (s.size() == 3) { v = (unsigned char)s[1]; return true; }
    return false;
  }
  if (s.empty() || !(isdigit((unsigned char)s[0]) || s[0] == '-' || s[0] == '+')) return false;
  size_t k = (s[0] == '-' || s[0] == '+') ? 1 : 0;
  if (k >= s.size() || !isdigit((unsigned char)s[k])) return false;
  int base = (s.size() > k + 1 && s[k] == '0' && (s[k + 1] == 'x' || s[k + 1] == 'X')) ? 16 : 10;
  const char* b = s.c_str(); char* e; errno = 0;
  long long x = strtoll(b, &e, base);
  if (*e != 0 || errno == ERANGE) return false;
  v = x; return true;
}

bool parse_float(const std::string& s, double& v) {
  if (s.empty() || !(isdigit((unsigned char)s[0]) || s[0] == '-' || s[0] == '+' || s[0] == '.')) return false;
  if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos) return false;
  if (s.find('x') != std::string::npos || s.find('X') != std::string::npos) return false;
  const char* b = s.c_str(); char* e;
  double x = strtod(b, &e);
  if (*e != 0) return false;
  v = x; return true;
}

uint64_t bits_of(double d) { uint64_t x; memcpy(&x, &d, 8); return x; }

struct Opd { enum T { NONE, REG, LIT, IMM, LAB, FUN, HOST, CONST } t = NONE; int64_t v = 0; };
struct IIns {
  int op = 0, line = 0;
  Opd o[3];
  bool has_dest = false; Opd dest;
  std::vector<Opd> args;
};
struct Stmt { int line; std::vector<std::string> tok; };
struct NStmt { int line; std::string mn; std::vector<std::string> a; };
struct FuncSrc { std::string name; int line; std::vector<std::string> params; std::vector<Stmt> body; };

int host_index(const std::string& n) { for (int h = 0; h < H_COUNT; h++) if (n == kHostInfo[h].name) return h; return -1; }
int op_index(const std::string& n) { for (int i = 0; i < OP_COUNT; i++) if (n == kOpInfo[i].name) return i; return -1; }

struct Asm {
  Program prog;
  std::map<std::string, int> funcIdx;
  std::map<std::string, int64_t> dataAddr;
  std::map<uint64_t, uint32_t> constIdx;
  std::map<std::string, int64_t> spaceReq;   // space ad N (bss: sifir dolu, ikilide yalniz sayi)
  std::vector<std::string> spaceOrder;
  std::vector<FuncSrc> fsrc;
  std::string entryName = "main"; int entryLine = 0;

  uint32_t konst(uint64_t b) {
    auto it = constIdx.find(b);
    if (it != constIdx.end()) return it->second;
    uint32_t i = uint32_t(prog.consts.size()); prog.consts.push_back(b); constIdx[b] = i; return i;
  }

  void data_line(int ln, const std::string& v_all) {
    size_t eq = v_all.find('=');
    if (eq == std::string::npos) err(ln, "data ad = degerler bekleniyor");
    auto nm = split(v_all.substr(0, eq));
    if (nm.size() != 1 || !is_ident(nm[0])) err(ln, "gecersiz data adi");
    if (dataAddr.count(nm[0]) || spaceReq.count(nm[0])) err(ln, "data adi tekrarlandi: " + nm[0]);
    size_t before = prog.data.size();
    dataAddr[nm[0]] = 1 + int64_t(before);
    const std::string v = v_all.substr(eq + 1);
    for (size_t i = 0; i < v.size();) {
      char c = v[i];
      if (c == ' ' || c == '\t' || c == ',' || c == '\r') { i++; continue; }
      if (c == '"') {
        i++;
        while (i < v.size() && v[i] != '"') {
          if (v[i] == '\\') {
            if (i + 1 >= v.size()) err(ln, "bitmemis kacis dizisi");
            if (v[i + 1] == 'x') {
              if (i + 3 >= v.size()) err(ln, "\\xHH eksik");
              prog.data.push_back(strtol(v.substr(i + 2, 2).c_str(), nullptr, 16)); i += 4; continue;
            }
            bool ok; int e = esc_char(v[i + 1], ok);
            if (!ok) err(ln, "bilinmeyen kacis dizisi");
            prog.data.push_back(e); i += 2;
          } else prog.data.push_back((unsigned char)v[i++]);
        }
        if (i >= v.size()) err(ln, "kapanmamis string");
        i++;
        continue;
      }
      size_t j = i;
      if (c == '\'') j = i + ((i + 1 < v.size() && v[i + 1] == '\\') ? 4 : 3);
      else while (j < v.size() && v[j] != ' ' && v[j] != '\t' && v[j] != ',' && v[j] != '\r') j++;
      std::string tok = v.substr(i, j - i); i = j;
      int64_t iv; double fv;
      if (parse_int(tok, iv)) prog.data.push_back(iv);
      else if (parse_float(tok, fv)) prog.data.push_back(int64_t(bits_of(fv)));
      else err(ln, "gecersiz data degeri: " + tok);
    }
    if (prog.data.size() == before) err(ln, "bos data");
  }

  void parse(const std::string& text) {
    std::istringstream in(text);
    std::string raw; int ln = 0;
    bool inFunc = false;
    while (std::getline(in, raw)) {
      ln++;
      std::string s = strip_comment(raw);
      auto t = split(s);
      if (t.empty()) continue;
      if (inFunc) {
        if (t.size() == 1 && t[0] == "}") { inFunc = false; continue; }
        if (t[0].back() == ':' && t.size() > 1) {   // "etiket: komut" ayni satirda
          fsrc.back().body.push_back(Stmt{ln, {t[0]}});
          t.erase(t.begin());
        }
        fsrc.back().body.push_back(Stmt{ln, t});
        continue;
      }
      if (t[0] == "memory") {
        int64_t v; if (t.size() != 2 || !parse_int(t[1], v) || v <= 0) err(ln, "memory N (pozitif) bekleniyor");
        prog.mem_words = uint64_t(v);
      } else if (t[0] == "entry") {
        if (t.size() != 2 || !is_ident(t[1])) err(ln, "entry ad bekleniyor");
        entryName = t[1]; entryLine = ln;
      } else if (t[0] == "data") {
        data_line(ln, s.substr(s.find("data") + 4));
      } else if (t[0] == "space") {
        int64_t n; if (t.size() != 3 || !is_ident(t[1]) || !parse_int(t[2], n) || n <= 0 || n > (1 << 26)) err(ln, "space ad N bekleniyor");
        if (dataAddr.count(t[1]) || spaceReq.count(t[1])) err(ln, "data adi tekrarlandi: " + t[1]);
        spaceReq[t[1]] = n; spaceOrder.push_back(t[1]);
      } else if (t[0] == "func") {
        size_t fpos = s.find("func") + 4;
        size_t lp = s.find('('), rp = s.find(')'), br = s.find('{');
        if (lp == std::string::npos || rp == std::string::npos || br == std::string::npos || !(lp < rp && rp < br))
          err(ln, "func ad(p1, p2) { bekleniyor");
        auto nm = split(s.substr(fpos, lp - fpos));
        if (nm.size() != 1 || !is_ident(nm[0])) err(ln, "gecersiz fonksiyon adi");
        if (!split(s.substr(br + 1)).empty()) err(ln, "'{' sonrasinda ayni satirda bir sey olmamali");
        FuncSrc f; f.name = nm[0]; f.line = ln; f.params = split(s.substr(lp + 1, rp - lp - 1));
        for (auto& pn : f.params) if (!is_ident(pn)) err(ln, "gecersiz parametre adi: " + pn);
        if (funcIdx.count(f.name)) err(ln, "fonksiyon tekrarlandi: " + f.name);
        funcIdx[f.name] = int(fsrc.size());
        fsrc.push_back(std::move(f));
        inFunc = true;
      } else err(ln, "bilinmeyen ust duzey ifade: " + t[0]);
    }
    if (inFunc) err(ln, "fonksiyon '}' ile kapanmadi: " + fsrc.back().name);
  }

  // ---- sabit/dogrudan operand yardimcilari ----
  bool litBits(const std::string& tok, uint64_t& bits) {
    int64_t iv; double fv;
    if (parse_int(tok, iv)) { bits = uint64_t(iv); return true; }
    if (parse_float(tok, fv)) { bits = bits_of(fv); return true; }
    auto d = dataAddr.find(tok);
    if (d != dataAddr.end()) { bits = uint64_t(d->second); return true; }
    return false;
  }
  bool immOf(const std::string& t, int64_t& v) {   // 32 bite sigan tamsayi (veya data adresi)?
    if (parse_int(t, v)) return v >= INT32_MIN && v <= INT32_MAX;
    auto d = dataAddr.find(t);
    if (d != dataAddr.end()) { v = d->second; return v <= INT32_MAX; }
    return false;
  }

  // Son operand 32 bit sabitse dogrudan-operand komutuna cevir; degismeli islemlerde sabit basta ise yer degistir.
  void rewriteImm(NStmt& n) {
    const std::string m = n.mn;
    if ((m == "gt" || m == "ge" || m == "fgt" || m == "fge") && n.a.size() != 3) err(n.line, m + " 3 operand ister");
    if (n.a.size() != 3) return;
    int64_t v = 0, v1 = 0;
    bool i2 = immOf(n.a[2], v);
    bool i1 = !i2 && immOf(n.a[1], v1);
    auto use2 = [&](const char* nm) { n.mn = nm; };
    auto swap12 = [&](const char* nm) { std::swap(n.a[1], n.a[2]); n.mn = nm; };
    if (m == "add" || m == "mul" || m == "and" || m == "eq" || m == "ne") {
      const char* im = m == "add" ? "addi" : m == "mul" ? "muli" : m == "and" ? "andi" : m == "eq" ? "eqi" : "nei";
      if (i2) use2(im); else if (i1) swap12(im);
    } else if (m == "sub") {
      if (i2 && v != INT32_MIN) { n.mn = "addi"; n.a[2] = std::to_string(-v); }
    } else if (m == "lt" || m == "le" || m == "gt" || m == "ge") {
      bool lt = m == "lt", le = m == "le", gt = m == "gt";
      if (i2) use2(lt ? "lti" : le ? "lei" : gt ? "gti" : "gei");
      else if (i1) swap12(lt ? "gti" : le ? "gei" : gt ? "lti" : "lei");
      else if (gt || m == "ge") { std::swap(n.a[1], n.a[2]); n.mn = gt ? "lt" : "le"; }
    } else if (m == "shl" || m == "shr" || m == "sar") {
      if (i2) use2(m == "shl" ? "shli" : m == "shr" ? "shri" : "sari");
    } else if (m == "div" || m == "mod") {
      if (i2 && v != 0 && v != -1) use2(m == "div" ? "divi" : "modi");
    } else if (m == "fgt" || m == "fge") {
      std::swap(n.a[1], n.a[2]); n.mn = m == "fgt" ? "flt" : "fle";
    }
  }

  template <class R, class W>
  void regAccess(const Ins& in, R onRead, W onWrite) {
    if (in.op == OP_CALL) { size_t np = fsrc[in.a].params.size(); for (size_t j = 0; j < np; j++) onRead(in.b + int(j)); onWrite(in.b); return; }
    if (in.op == OP_HOST) { int ar = kHostInfo[in.a].arity; for (int j = 0; j < ar; j++) onRead(in.b + j); onWrite(in.b); return; }
    const OpInfo& oi = kOpInfo[in.op];
    for (int j = 0; j < oi.nops; j++) {
      int32_t v = j == 0 ? in.a : j == 1 ? in.b : in.c; char k = oi.kinds[j];
      if (k == 'r' || k == 'f' || k == 's') onRead(v); else if (k == 'd') onWrite(v);
    }
  }

  // Davranisi koruyan yerel sadelestirmeler (yazmac sayimlari bu islevin tamami uzerinden):
  //  A) tek yazilan/tek okunan gecici, hemen cagri penceresine kopyalaniyorsa dogrudan pencereye yaz
  //  B) "mov x, W" ardindan "mov W, x"  -> ikincisi gereksiz
  //  C) hic okunmayan "mov x, W" (kullanilmayan cagri sonucu) -> sil
  void peephole(Func& f, int nNamed, int w) {
    std::vector<Ins>& C = f.code; const size_t M = C.size();
    std::vector<char> tgt(M + 1, 0), del(M, 0);
    for (const Ins& in : C) { const OpInfo& oi = kOpInfo[in.op]; for (int j = 0; j < oi.nops; j++) if (oi.kinds[j] == 'l') tgt[size_t(j == 0 ? in.a : j == 1 ? in.b : in.c)] = 1; }
    std::vector<int> rd(f.nregs, 0), wr(f.nregs, 0);
    for (const Ins& in : C) regAccess(in, [&](int r) { rd[size_t(r)]++; }, [&](int r) { wr[size_t(r)]++; });
    auto stops = [](uint8_t op) { return op == OP_JMP || op == OP_JZ || op == OP_JNZ || op == OP_RET || op == OP_HALT || op == OP_CALL || op == OP_HOST; };
    auto refs = [&](const Ins& in, int r) { bool x = false; regAccess(in, [&](int q) { if (q == r) x = true; }, [&](int q) { if (q == r) x = true; }); return x; };
    for (size_t u = 0; u < M; u++) {   // A
      const Ins m = C[u];
      if (del[u] || m.op != OP_MOV || m.a < w || m.b >= nNamed || m.b < int(f.nparams) || rd[size_t(m.b)] != 1 || wr[size_t(m.b)] != 1 || tgt[u]) continue;
      const int t = m.b, slot = m.a; long d = -1;
      for (long p = long(u) - 1; p >= 0; p--) {
        if (tgt[size_t(p) + 1]) break;
        if (del[size_t(p)]) continue;
        const Ins& q = C[size_t(p)];
        bool writesT = false; regAccess(q, [](int) {}, [&](int r) { if (r == t) writesT = true; });
        if (writesT) { d = p; break; }
        if (stops(q.op) || refs(q, slot)) break;
      }
      if (d < 0) continue;
      C[size_t(d)].a = slot; del[u] = 1; rd[size_t(t)] = wr[size_t(t)] = 0;
    }
    for (size_t p = 0; p + 1 < M; p++) {   // B
      if (del[p]) continue;
      size_t q = p + 1; bool bad = tgt[q];
      while (q < M && del[q]) { q++; if (q < M && tgt[q]) bad = true; }
      if (q >= M || bad) continue;
      const Ins &a = C[p], &b = C[q];
      if (a.op == OP_MOV && b.op == OP_MOV && a.b >= w && a.a < nNamed && b.a == a.b && b.b == a.a) { del[q] = 1; rd[size_t(a.a)]--; }
    }
    for (size_t p = 0; p < M; p++)   // C (ve kendine kopya)
      if (!del[p] && C[p].op == OP_MOV && (C[p].a == C[p].b || (C[p].b >= w && C[p].a < nNamed && rd[size_t(C[p].a)] == 0))) del[p] = 1;
    std::vector<int32_t> idx(M + 1); int32_t k = 0;
    for (size_t p = 0; p < M; p++) { idx[p] = k; if (!del[p]) k++; }
    idx[M] = k;
    std::vector<Ins> out; out.reserve(size_t(k));
    for (size_t p = 0; p < M; p++) {
      if (del[p]) continue;
      Ins in = C[p]; const OpInfo& oi = kOpInfo[in.op];
      for (int j = 0; j < oi.nops; j++) if (oi.kinds[j] == 'l') { int32_t& fld = j == 0 ? in.a : j == 1 ? in.b : in.c; fld = idx[size_t(fld)]; }
      out.push_back(in);
    }
    C.swap(out);
  }

  // ---- bir fonksiyonun cozumlenmesi ve uretimi ----
  Func build(const FuncSrc& fs) {
    std::map<std::string, int> regs;
    auto addReg = [&](const std::string& n, int line) {
      if (!is_ident(n)) err(line, "gecersiz yazmac adi: " + n);
      if (funcIdx.count(n) || dataAddr.count(n) || host_index(n) >= 0 || op_index(n) >= 0) err(line, "ad cakisiyor (fonksiyon/data/host/komut ile): " + n);
      if (!regs.count(n)) { int i = int(regs.size()); regs[n] = i; }
    };
    for (auto& p : fs.params) {
      if (regs.count(p)) err(fs.line, "parametre tekrarlandi: " + p);
      addReg(p, fs.line);
    }

    // gecis 1: etiketler, dogrudan-operand donusumu, hedef yazmaclar
    std::vector<NStmt> ir;
    std::map<std::string, int> labels;
    for (const Stmt& st : fs.body) {
      const std::string& m0 = st.tok[0];
      if (m0.back() == ':') {
        if (st.tok.size() != 1) err(st.line, "etiket satirinda baska ifade olmamali");
        std::string l = m0.substr(0, m0.size() - 1);
        if (!is_ident(l)) err(st.line, "gecersiz etiket: " + l);
        if (labels.count(l)) err(st.line, "etiket tekrarlandi: " + l);
        labels[l] = int(ir.size()); continue;
      }
      NStmt n{st.line, m0, std::vector<std::string>(st.tok.begin() + 1, st.tok.end())};
      rewriteImm(n);
      int op = op_index(n.mn);
      if (op < 0) err(st.line, "bilinmeyen komut: " + n.mn);
      if (op == OP_LDK) err(st.line, "ldk metinde kullanilamaz (sabitleri yukleyici uretir)");
      const OpInfo& oi = kOpInfo[op];
      if (op == OP_CALL) {
        if (n.a.empty()) err(st.line, "call eksik");
        if (!funcIdx.count(n.a[0])) {                 // call d, f, args...
          if (n.a.size() < 2 || !funcIdx.count(n.a[1])) err(st.line, "call [d,] fonksiyon, argumanlar... ; fonksiyon bulunamadi");
          addReg(n.a[0], st.line);
        }
      } else if (op == OP_HOST) {
        if (n.a.empty()) err(st.line, "host eksik");
        if (host_index(n.a[0]) < 0) {
          if (n.a.size() < 2 || host_index(n.a[1]) < 0) err(st.line, "host [d,] ad, argumanlar... ; bilinmeyen host islevi");
          addReg(n.a[0], st.line);
        }
      } else {
        if (int(n.a.size()) != oi.nops) err(st.line, std::string(oi.name) + " " + std::to_string(oi.nops) + " operand ister");
        for (int j = 0; j < oi.nops; j++) if (oi.kinds[j] == 'd') addReg(n.a[j], st.line);
      }
      ir.push_back(std::move(n));
    }
    auto terminator = [](const std::string& m) { return m == "jmp" || m == "ret" || m == "halt"; };
    bool labelAtEnd = false;
    for (auto& kv : labels) if (kv.second == int(ir.size())) labelAtEnd = true;
    if (ir.empty() || labelAtEnd || !terminator(ir.back().mn)) ir.push_back(NStmt{fs.line, "ret", {"0"}});

    // gecis 2: operand cozumleme
    std::vector<uint64_t> lits; std::map<uint64_t, int> litIdx;
    auto lit = [&](uint64_t b) {
      auto it = litIdx.find(b);
      int i;
      if (it != litIdx.end()) i = it->second; else { i = int(lits.size()); lits.push_back(b); litIdx[b] = i; }
      Opd o; o.t = Opd::LIT; o.v = i; return o;
    };
    auto src = [&](const std::string& tok, char kind, int line) -> Opd {
      int64_t iv; double fv;
      if (parse_int(tok, iv)) {
        if (kind == 'f') err(line, "ondalik sabit bekleniyor (ornek 2.0): " + tok);
        return lit(uint64_t(iv));
      }
      if (parse_float(tok, fv)) {
        if (kind == 'r') err(line, "tamsayi sabit bekleniyor: " + tok);
        return lit(bits_of(fv));
      }
      if (is_ident(tok)) {
        auto it = regs.find(tok);
        if (it != regs.end()) { Opd o; o.t = Opd::REG; o.v = it->second; return o; }
        auto d = dataAddr.find(tok);
        if (d != dataAddr.end()) {
          if (kind == 'f') err(line, "data adresi ondalik islemde kullanilamaz: " + tok);
          return lit(uint64_t(d->second));
        }
        err(line, "tanimsiz ad (hic yazilmamis yazmac?): " + tok);
      }
      err(line, "gecersiz operand: " + tok);
    };
    auto dst = [&](const std::string& tok, int line) -> Opd {
      auto it = regs.find(tok);
      if (it == regs.end()) err(line, "hedef yazmac bulunamadi: " + tok);
      Opd o; o.t = Opd::REG; o.v = it->second; return o;
    };
    auto constOpd = [](uint64_t bits) { Opd o; o.t = Opd::CONST; o.v = int64_t(bits); return o; };

    std::vector<IIns> code; int maxWin = 1;
    for (const NStmt& n : ir) {
      IIns I; I.line = n.line; I.op = op_index(n.mn);
      const OpInfo& oi = kOpInfo[I.op];
      uint64_t lb = 0;
      if (I.op == OP_CALL || I.op == OP_HOST) {
        size_t k = 0;
        bool isCall = I.op == OP_CALL;
        bool direct = isCall ? funcIdx.count(n.a[0]) > 0 : host_index(n.a[0]) >= 0;
        if (!direct) { I.has_dest = true; I.dest = dst(n.a[0], n.line); k = 1; }
        Opd t; t.t = isCall ? Opd::FUN : Opd::HOST; t.v = isCall ? funcIdx[n.a[k]] : host_index(n.a[k]);
        I.o[0] = t; k++;
        int want = isCall ? int(fsrc[t.v].params.size()) : kHostInfo[t.v].arity;
        int got = int(n.a.size() - k);
        if (got != want) err(n.line, std::string(oi.name) + ": " + std::to_string(want) + " arguman bekleniyor, " + std::to_string(got) + " verildi");
        for (; k < n.a.size(); k++) I.args.push_back(litBits(n.a[k], lb) ? constOpd(lb) : src(n.a[k], 's', n.line));
        maxWin = std::max(maxWin, std::max(1, got));
      } else if (I.op == OP_MOV && litBits(n.a[1], lb)) {   // mov d, sabit -> dogrudan yukle
        I.op = OP_LDI; I.o[0] = dst(n.a[0], n.line); I.o[1] = constOpd(lb);
      } else if (I.op == OP_RET && litBits(n.a[0], lb)) {   // ret sabit -> yalnizca bu yolda yukle
        I.o[0] = constOpd(lb);
      } else {
        for (int j = 0; j < oi.nops; j++) {
          char kd = oi.kinds[j];
          if (kd == 'd') I.o[j] = dst(n.a[j], n.line);
          else if (kd == 'r' || kd == 'f' || kd == 's') I.o[j] = src(n.a[j], kd, n.line);
          else if (kd == 'i') {
            int64_t iv; Opd o; o.t = Opd::IMM;
            if (parse_int(n.a[j], iv)) o.v = iv;
            else if (dataAddr.count(n.a[j])) o.v = dataAddr[n.a[j]];
            else err(n.line, "dogrudan tamsayi bekleniyor: " + n.a[j]);
            if (o.v < INT32_MIN || o.v > INT32_MAX) err(n.line, "dogrudan deger 32 bit disinda");
            I.o[j] = o;
          } else if (kd == 'l') {
            auto it = labels.find(n.a[j]);
            if (it == labels.end()) err(n.line, "etiket bulunamadi: " + n.a[j]);
            Opd o; o.t = Opd::LAB; o.v = it->second; I.o[j] = o;
          }
        }
      }
      code.push_back(std::move(I));
    }

    // yerlesim
    int nNamed = int(regs.size()), nLits = int(lits.size());
    int w = nNamed + nLits;
    uint64_t nregs = uint64_t(w) + uint64_t(maxWin);
    if (nregs > Limits::kMaxRegs) err(fs.line, "fonksiyon cok fazla yazmac kullaniyor: " + fs.name);
    Func f; f.name = fs.name; f.nparams = uint32_t(fs.params.size()); f.nregs = uint32_t(nregs);
    auto reg = [&](const Opd& o) -> int32_t { return o.t == Opd::LIT ? int32_t(nNamed + o.v) : int32_t(o.v); };
    auto emitConst = [&](int32_t dest, int64_t v) {
      Ins in{}; in.a = dest;
      if (v >= INT32_MIN && v <= INT32_MAX) { in.op = OP_LDI; in.b = int32_t(v); }
      else { in.op = OP_LDK; in.b = int32_t(konst(uint64_t(v))); }
      f.code.push_back(in);
    };

    for (int i = 0; i < nLits; i++) emitConst(nNamed + i, int64_t(lits[size_t(i)]));   // on hazirlik: sabit yazmaclari
    size_t pre = f.code.size();
    auto sizeOf = [](const IIns& I) -> size_t {
      if (I.op == OP_CALL || I.op == OP_HOST) return I.args.size() + 1 + (I.has_dest ? 1 : 0);
      if (I.op == OP_RET && I.o[0].t == Opd::CONST) return 2;
      return 1;
    };
    std::vector<size_t> pcOf(code.size() + 1);
    size_t pc = pre;
    for (size_t i = 0; i < code.size(); i++) { pcOf[i] = pc; pc += sizeOf(code[i]); }
    pcOf[code.size()] = pc;
    if (pc > Limits::kMaxCode) err(fs.line, "fonksiyon cok uzun: " + fs.name);

    for (const IIns& I : code) {
      if (I.op == OP_CALL || I.op == OP_HOST) {
        for (size_t j = 0; j < I.args.size(); j++) {
          if (I.args[j].t == Opd::CONST) emitConst(w + int32_t(j), I.args[j].v);
          else { Ins m{}; m.op = OP_MOV; m.a = w + int32_t(j); m.b = reg(I.args[j]); f.code.push_back(m); }
        }
        Ins c{}; c.op = uint8_t(I.op); c.a = int32_t(I.o[0].v); c.b = w; f.code.push_back(c);
        if (I.has_dest) { Ins m{}; m.op = OP_MOV; m.a = reg(I.dest); m.b = w; f.code.push_back(m); }
        continue;
      }
      if (I.op == OP_LDI) { emitConst(reg(I.o[0]), I.o[1].v); continue; }
      if (I.op == OP_RET && I.o[0].t == Opd::CONST) {
        emitConst(w, I.o[0].v);
        Ins r{}; r.op = OP_RET; r.a = w; f.code.push_back(r);
        continue;
      }
      Ins in{}; in.op = uint8_t(I.op);
      const OpInfo& oi = kOpInfo[I.op];
      for (int j = 0; j < oi.nops; j++) {
        int32_t v = 0; const Opd& o = I.o[j];
        switch (o.t) {
          case Opd::REG: case Opd::LIT: v = reg(o); break;
          case Opd::IMM: v = int32_t(o.v); break;
          case Opd::LAB: v = int32_t(pcOf[size_t(o.v)]); break;
          default: break;
        }
        (j == 0 ? in.a : j == 1 ? in.b : in.c) = v;
      }
      f.code.push_back(in);
    }
    peephole(f, nNamed, w);
    return f;
  }

  Program finish() {
    int64_t off = 1 + int64_t(prog.data.size());
    for (const std::string& nm : spaceOrder) { dataAddr[nm] = off; off += spaceReq[nm]; prog.bss += uint64_t(spaceReq[nm]); }
    if (prog.bss > Limits::kMaxMemWords) throw PmError("space toplami cok buyuk");
    for (const FuncSrc& fs : fsrc) prog.funcs.push_back(build(fs));
    auto it = funcIdx.find(entryName);
    if (it == funcIdx.end()) err(entryLine, "giris fonksiyonu bulunamadi: " + entryName);
    prog.entry = uint32_t(it->second);
    prog.has_names = true;
    validate_program(prog);
    analyze_program(prog);
    return std::move(prog);
  }
};

}  // namespace

Program assemble(const std::string& text) {
  Asm a;
  a.parse(text);
  return a.finish();
}
