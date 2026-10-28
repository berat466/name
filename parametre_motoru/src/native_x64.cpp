// x86-64 (Linux) makine kodu uretici + yurutucu. Sablonlar anlam.h tablosundan (DATA genisletmesi) okunur;
// yorumlayiciyla AYNI anlam, AYNI yakit/tuzak kurallari.
//
// Yazmac dagitimi: bir islevde sik kullanilan VM yazmaclari CPU yazmaclarinda tutulur (yalniz program "kesin atama" ispatliysa,
// cunku o zaman cagri sonrasi pencere yazmaclari okunmaz). Cagri penceresi (>= en kucuk cagri tabani) hep bellekte kalir.
// Sabit yazmaclar: rbx = VM yazmac penceresi R, r12 = cagri derinligi, r13 = yakit, r15 = bellek tabani.
// Gecici: rax rcx rdx. Dagitim havuzu: rsi rdi r8 r9 r10 r11 r14 rbp (her islev kullandigini kendisi saklar/geri yukler).
#include "program.h"
#if defined(__x86_64__) && defined(__linux__)
#include <algorithm>
#include <cstddef>
#include <sys/mman.h>
#include "host.h"
#define PM_SEM_TABLES
#include "anlam.h"

namespace {
enum { RAX = 0, RCX = 1, RDX = 2, RBX = 3, RSP = 4, RBP = 5, RSI = 6, RDI = 7, R8 = 8, R9 = 9, R10 = 10, R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15 };
const int POOL[8] = {RSI, RDI, R8, R9, R10, R11, R14, RBP};
bool caller_saved(int r) { return r == RSI || r == RDI || (r >= R8 && r <= R11); }
enum { CC_B = 2, CC_AE = 3, CC_E = 4, CC_NE = 5, CC_A = 7, CC_L = 0xC, CC_GE = 0xD, CC_LE = 0xE, CC_G = 0xF };
int inv_cc(int cc) { return cc ^ 1; }

struct Ctx {
  uint64_t saved_rsp, heap, fuel_out, trap, result;
  IO* io;
};

// Deger konumu: islemci yazmaci, VM yazmaci bellegi [rbx+d], 32 bit sabit, 64 bit sabit
enum VK { V_REG, V_MEM, V_IMM, V_IMM64 };
struct V { VK k = V_IMM; int r = 0; int32_t d = 0; int64_t imm = 0; };
V vreg(int r) { V v; v.k = V_REG; v.r = r; return v; }
V vmem(int32_t d) { V v; v.k = V_MEM; v.d = d; return v; }
V vimm(int64_t x) { V v; if (x >= INT32_MIN && x <= INT32_MAX) { v.k = V_IMM; v.imm = x; } else { v.k = V_IMM64; v.imm = x; } return v; }
bool is_const(const V& v) { return v.k == V_IMM || v.k == V_IMM64; }

struct Label { long pos = -1; std::vector<size_t> fix; };

struct X {
  std::vector<uint8_t> b;
  void u8(uint8_t v) { b.push_back(v); }
  void u32(uint32_t v) { for (int i = 0; i < 4; i++) u8(uint8_t(v >> (8 * i))); }
  void u64(uint64_t v) { for (int i = 0; i < 8; i++) u8(uint8_t(v >> (8 * i))); }
  void rex(int reg, int rm) { u8(uint8_t(0x48 | ((reg >> 3) & 1) << 2 | ((rm >> 3) & 1))); }
  void rr(uint8_t opc, int rm, int reg) { rex(reg, rm); u8(opc); u8(uint8_t(0xC0 | ((reg & 7) << 3) | (rm & 7))); }
  void mem(uint8_t opc, int reg, int base, int32_t disp) {
    rex(reg, base); u8(opc); u8(uint8_t(0x80 | ((reg & 7) << 3) | (base & 7)));
    if ((base & 7) == 4) u8(0x24);
    u32(uint32_t(disp));
  }
  void mem_idx(uint8_t opc, int reg, int base, int idx) {   // [base + idx*8], base&7 != 5
    u8(uint8_t(0x48 | ((reg >> 3) & 1) << 2 | ((idx >> 3) & 1) << 1 | ((base >> 3) & 1)));
    u8(opc); u8(uint8_t(((reg & 7) << 3) | 4)); u8(uint8_t((3 << 6) | ((idx & 7) << 3) | (base & 7)));
  }
  void store_idx_imm(int base, int idx, int32_t imm) {   // mov qword [base+idx*8], imm32
    u8(uint8_t(0x48 | ((idx >> 3) & 1) << 1 | ((base >> 3) & 1)));
    u8(0xC7); u8(0x04); u8(uint8_t((3 << 6) | ((idx & 7) << 3) | (base & 7))); u32(uint32_t(imm));
  }
  // --- r/m (islemci yazmaci ya da VM yazmac bellegi) operandli komutlar
  void rm_op(uint8_t opc, int reg, const V& m) { if (m.k == V_REG) rr(opc, m.r, reg); else mem(opc, reg, RBX, m.d); }
  void rm_op2(uint8_t o1, uint8_t o2, int reg, const V& m) {
    if (m.k == V_REG) { rex(reg, m.r); u8(o1); u8(o2); u8(uint8_t(0xC0 | ((reg & 7) << 3) | (m.r & 7))); }
    else { rex(reg, RBX); u8(o1); u8(o2); u8(uint8_t(0x80 | ((reg & 7) << 3) | 3)); u32(uint32_t(m.d)); }
  }
  void rm_imm(int ext, const V& m, int32_t imm) {   // 81 /ext id
    if (m.k == V_REG) { rex(0, m.r); u8(0x81); u8(uint8_t(0xC0 | (ext << 3) | (m.r & 7))); }
    else mem(0x81, ext, RBX, m.d);
    u32(uint32_t(imm));
  }
  void mov_rm_imm(const V& m, int32_t imm) {   // C7 /0 id
    if (m.k == V_REG) { rex(0, m.r); u8(0xC7); u8(uint8_t(0xC0 | (m.r & 7))); }
    else mem(0xC7, 0, RBX, m.d);
    u32(uint32_t(imm));
  }
  void imul_imm(int dst, const V& m, int32_t imm) {   // 69 /r id
    if (m.k == V_REG) { rex(dst, m.r); u8(0x69); u8(uint8_t(0xC0 | ((dst & 7) << 3) | (m.r & 7))); }
    else mem(0x69, dst, RBX, m.d);
    u32(uint32_t(imm));
  }
  void mov_rr(int dst, int src) { if (dst != src) rr(0x89, dst, src); }
  void load(int dst, int base, int32_t disp) { mem(0x8B, dst, base, disp); }
  void store(int base, int32_t disp, int src) { mem(0x89, src, base, disp); }
  void lea(int dst, int base, int32_t disp) { mem(0x8D, dst, base, disp); }
  void mov_imm32(int dst, int32_t v) { rex(0, dst); u8(0xC7); u8(uint8_t(0xC0 | (dst & 7))); u32(uint32_t(v)); }
  void mov_imm64(int dst, uint64_t v) { rex(0, dst); u8(uint8_t(0xB8 + (dst & 7))); u64(v); }
  void alu_rr(uint8_t opc, int rm, int reg) { rr(opc, rm, reg); }   // 01 add, 09 or, 21 and, 29 sub, 31 xor, 39 cmp, 85 test
  void alu_ri(int ext, int rm, int32_t v) { rm_imm(ext, vreg(rm), v); }
  void shift_cl(int ext, int rm) { rex(0, rm); u8(0xD3); u8(uint8_t(0xC0 | (ext << 3) | (rm & 7))); }   // 4 shl 5 shr 7 sar
  void shift_imm(int ext, int rm, uint8_t n) { rex(0, rm); u8(0xC1); u8(uint8_t(0xC0 | (ext << 3) | (rm & 7))); u8(n); }
  void unary(int ext, int rm) { rex(0, rm); u8(0xF7); u8(uint8_t(0xC0 | (ext << 3) | (rm & 7))); }   // 2 not 3 neg 7 idiv
  void incdec(int ext, int rm) { rex(0, rm); u8(0xFF); u8(uint8_t(0xC0 | (ext << 3) | (rm & 7))); }   // 0 inc 1 dec
  void cqo() { u8(0x48); u8(0x99); }
  void setcc_al(int cc) { u8(0x0F); u8(uint8_t(0x90 | cc)); u8(0xC0); }
  void movzx_al(int dst) { rex(dst, 0); u8(0x0F); u8(0xB6); u8(uint8_t(0xC0 | ((dst & 7) << 3))); }
  void push(int r) { if (r >= 8) u8(0x41); u8(uint8_t(0x50 + (r & 7))); }
  void pop(int r) { if (r >= 8) u8(0x41); u8(uint8_t(0x58 + (r & 7))); }
  void ret() { u8(0xC3); }
  void call_rax() { u8(0xFF); u8(0xD0); }
  void rep_stosq() { u8(0xF3); u8(0x48); u8(0xAB); }
  void xor_eax() { u8(0x31); u8(0xC0); }
  void rel32(Label& l) {
    if (l.pos >= 0) u32(uint32_t(int32_t(l.pos - long(b.size() + 4))));
    else { l.fix.push_back(b.size()); u32(0); }
  }
  void jmp(Label& l) { u8(0xE9); rel32(l); }
  void jcc(int cc, Label& l) { u8(0x0F); u8(uint8_t(0x80 | cc)); rel32(l); }
  void call(Label& l) { u8(0xE8); rel32(l); }
  void bind(Label& l) {
    l.pos = long(b.size());
    for (size_t f : l.fix) { int32_t d = int32_t(l.pos - long(f + 4)); for (int i = 0; i < 4; i++) b[f + size_t(i)] = uint8_t(uint32_t(d) >> (8 * i)); }
    l.fix.clear();
  }
  // --- V yardimcilari
  void load_to(int dst, const V& v) {
    switch (v.k) {
      case V_REG: mov_rr(dst, v.r); break;
      case V_MEM: load(dst, RBX, v.d); break;
      case V_IMM: mov_imm32(dst, int32_t(v.imm)); break;
      case V_IMM64: mov_imm64(dst, uint64_t(v.imm)); break;
    }
  }
  void store_to(const V& dst, const V& src) {   // dst: REG ya da MEM
    if (dst.k == V_REG) { load_to(dst.r, src); return; }
    if (src.k == V_MEM) { if (src.d == dst.d) return; load_to(RAX, src); store(RBX, dst.d, RAX); }
    else if (src.k == V_REG) store(RBX, dst.d, src.r);
    else if (src.k == V_IMM) mov_rm_imm(dst, int32_t(src.imm));
    else { mov_imm64(RAX, uint64_t(src.imm)); store(RBX, dst.d, RAX); }
  }
  void cmp_flags(V A, V B) {   // bayraklar: A - B
    if (is_const(A)) { load_to(RAX, A); A = vreg(RAX); }
    if (B.k == V_IMM64) { load_to(RCX, B); B = vreg(RCX); }
    if (A.k == V_MEM && B.k == V_MEM) { load_to(RAX, A); A = vreg(RAX); }
    if (B.k == V_IMM) rm_imm(7, A, int32_t(B.imm));
    else if (A.k == V_REG) rm_op(0x3B, A.r, B);
    else rm_op(0x39, B.r, A);
  }
  V emit_alu(int op, V A, V B, int hint) {
    bool comm = op == ALUK_ADD || op == ALUK_MUL || op == ALUK_AND || op == ALUK_OR || op == ALUK_XOR;
    int D = hint >= 0 ? hint : RAX;
    if (B.k == V_REG && B.r == D && !(A.k == V_REG && A.r == D)) { if (comm) std::swap(A, B); else D = RAX; }
    if (op == ALUK_SHL || op == ALUK_SHR || op == ALUK_SAR) {
      int ext = op == ALUK_SHL ? 4 : op == ALUK_SHR ? 5 : 7;
      if (B.k == V_IMM || B.k == V_IMM64) { if (!(A.k == V_REG && A.r == D)) load_to(D, A); shift_imm(ext, D, uint8_t(B.imm & 63)); }
      else { load_to(RCX, B); if (!(A.k == V_REG && A.r == D)) load_to(D, A); shift_cl(ext, D); }
      return vreg(D);
    }
    if (B.k == V_IMM64) { load_to(RCX, B); B = vreg(RCX); }
    if (!(A.k == V_REG && A.r == D)) load_to(D, A);
    switch (op) {
      case ALUK_ADD: if (B.k == V_IMM) alu_ri(0, D, int32_t(B.imm)); else rm_op(0x03, D, B); break;
      case ALUK_SUB: if (B.k == V_IMM) alu_ri(5, D, int32_t(B.imm)); else rm_op(0x2B, D, B); break;
      case ALUK_AND: if (B.k == V_IMM) alu_ri(4, D, int32_t(B.imm)); else rm_op(0x23, D, B); break;
      case ALUK_OR:  if (B.k == V_IMM) alu_ri(1, D, int32_t(B.imm)); else rm_op(0x0B, D, B); break;
      case ALUK_XOR: if (B.k == V_IMM) alu_ri(6, D, int32_t(B.imm)); else rm_op(0x33, D, B); break;
      case ALUK_MUL: if (B.k == V_IMM) imul_imm(D, vreg(D), int32_t(B.imm)); else rm_op2(0x0F, 0xAF, D, B); break;
    }
    return vreg(D);
  }
};

void host_thunk(uint64_t* w, Ctx* c, int id) { host_call(*c->io, id, w); }
inline int32_t fld(const Ins& in, int x) { return x == 0 ? in.a : x == 1 ? in.b : in.c; }

template <class R, class W>
void reg_access(const Program& P, const Ins& in, R rd, W wr) {
  if (in.op == OP_CALL) { size_t np = P.funcs[size_t(in.a)].nparams; for (size_t j = 0; j < np; j++) rd(in.b + int(j)); wr(in.b); return; }
  if (in.op == OP_HOST) { int ar = kHostInfo[in.a].arity; for (int j = 0; j < ar; j++) rd(in.b + j); wr(in.b); return; }
  const OpInfo& oi = kOpInfo[in.op];
  for (int j = 0; j < oi.nops; j++) {
    int32_t v = fld(in, j); char k = oi.kinds[j];
    if (k == 'r' || k == 'f' || k == 's') rd(v); else if (k == 'd') wr(v);
  }
}
bool is_cmp_op(int op) {
  return op == OP_EQ || op == OP_NE || op == OP_LT || op == OP_LE || op == OP_EQI || op == OP_NEI || op == OP_LTI || op == OP_LEI || op == OP_GTI || op == OP_GEI;
}
}  // namespace

bool native_available() { return true; }
bool native_supported(const Program& p) {
  for (const Func& f : p.funcs) for (const Ins& in : f.code) if (!sem_of(in.op)) return false;
  return true;
}

RunResult run_program_native(const Program& P, IO& io, const RunConfig& cfg) {
  RunResult res;
  const uint64_t mem_words = std::min<uint64_t>(P.mem_words, cfg.max_mem_words);
  if (mem_words < 1 + P.data.size() + P.bss) { res.trap = T_ALLOC; return res; }
  std::vector<uint64_t> mem(mem_words, 0);
  for (size_t i = 0; i < P.data.size(); i++) mem[1 + i] = uint64_t(P.data[i]);
  std::vector<uint64_t> stack(cfg.stack_words, 0);
  if (P.funcs[P.entry].nregs > stack.size()) { res.trap = T_STACK; return res; }

  size_t ssize = (cfg.max_frames * 96 + (256 << 10) + 4095) & ~size_t(4095);
  void* nstack = mmap(nullptr, ssize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (nstack == MAP_FAILED) throw PmError("native: yigin ayrilamadi");
  Ctx ctx{};
  ctx.heap = 1 + P.data.size() + P.bss; ctx.io = &io;
  const uint64_t new_sp = (reinterpret_cast<uint64_t>(nstack) + ssize) & ~uint64_t(15);
  const uint64_t stack_end = reinterpret_cast<uint64_t>(stack.data() + stack.size());
  auto A_ = [](const void* p) { return reinterpret_cast<uint64_t>(p); };

  // ---- kod uretimi ----
  X a;
  std::vector<Label> fl(P.funcs.size());
  Label lEpi, lDiv0, lOob, lStack, lDepth, lFuel, lAlloc;
  // trampolin: (uint64_t* R)
  a.push(RBX); a.push(RBP); a.push(R12); a.push(R13); a.push(R14); a.push(R15);
  a.mov_imm64(RAX, A_(&ctx.saved_rsp)); a.store(RAX, 0, RSP);
  a.mov_imm64(RSP, new_sp);
  a.alu_rr(0x31, R12, R12);
  a.mov_imm64(R13, cfg.fuel); a.mov_imm64(R15, A_(mem.data()));
  a.mov_rr(RBX, RDI);
  a.call(fl[P.entry]);
  a.load(RAX, RBX, 0); a.mov_imm64(RDX, A_(&ctx.result)); a.store(RDX, 0, RAX);
  a.bind(lEpi);
  a.mov_imm64(RAX, A_(&ctx.fuel_out)); a.store(RAX, 0, R13);
  a.mov_imm64(RAX, A_(&ctx.saved_rsp)); a.load(RSP, RAX, 0);
  a.pop(R15); a.pop(R14); a.pop(R13); a.pop(R12); a.pop(RBP); a.pop(RBX); a.ret();
  struct TS { Label* l; int code; };
  for (TS t : {TS{&lDiv0, T_DIV0}, TS{&lOob, T_OOB}, TS{&lStack, T_STACK}, TS{&lDepth, T_DEPTH}, TS{&lFuel, T_FUEL}, TS{&lAlloc, T_ALLOC}}) {
    a.bind(*t.l); if (t.code == T_FUEL) a.alu_rr(0x31, R13, R13);   // sub borcu r13'u sardi: yakit 0 kalmali
    a.mov_imm64(RDX, A_(&ctx.trap)); a.mem(0xC7, 0, RDX, 0); a.u32(uint32_t(t.code)); a.jmp(lEpi);
  }
  auto checkpoint = [&]() { a.alu_ri(5, R13, 1); a.jcc(CC_B, lFuel); };   // r13==0 ise borc -> TRAP; degilse r13--

  for (size_t fi = 0; fi < P.funcs.size(); fi++) {
    const Func& f = P.funcs[fi];
    const size_t M = f.code.size();
    // ---- islev analizi: okuma/yazma sayilari, hedefler, donguler, yazmaca alma ----
    std::vector<int> rd(f.nregs, 0), wr(f.nregs, 0), wgt(f.nregs, 0);
    std::vector<char> tgt(M + 1, 0), inloop(M, 0);
    int limit = int(f.nregs);
    for (size_t pc = 0; pc < M; pc++) {
      const Ins& in = f.code[pc];
      if (in.op == OP_CALL || in.op == OP_HOST) limit = std::min(limit, int(in.b));
      const OpInfo& oi = kOpInfo[in.op];
      for (int j = 0; j < oi.nops; j++) if (oi.kinds[j] == 'l') {
        int t = fld(in, j); tgt[size_t(t)] = 1;
        if (t <= int(pc)) for (int p = t; p <= int(pc); p++) inloop[size_t(p)] = 1;
      }
    }
    for (size_t pc = 0; pc < M; pc++) {
      int w = inloop[pc] ? 8 : 1;
      reg_access(P, f.code[pc], [&](int r) { rd[size_t(r)]++; wgt[size_t(r)] += w; }, [&](int r) { wr[size_t(r)]++; wgt[size_t(r)] += w; });
    }
    std::vector<int> preg(f.nregs, -1), used;
    if (P.zero_free) {
      std::vector<int> cand;
      for (int r = 0; r < limit; r++) if (wgt[size_t(r)] > 0 && wgt[size_t(r)] >= cfg.native_promote) cand.push_back(r);
      std::stable_sort(cand.begin(), cand.end(), [&](int x, int y) { return wgt[size_t(x)] > wgt[size_t(y)]; });
      for (size_t i = 0; i < cand.size() && i < 8; i++) { preg[size_t(cand[i])] = POOL[i]; used.push_back(POOL[i]); }
    }
    const bool pad = used.size() % 2 == 0;
    auto vmloc = [&](int x) { return preg[size_t(x)] >= 0 ? vreg(preg[size_t(x)]) : vmem(8 * x); };
    auto emit_ret = [&]() { if (pad) a.alu_ri(0, RSP, 8); for (size_t i = used.size(); i-- > 0;) a.pop(used[i]); a.ret(); };

    std::vector<Label> pcl(M);
    a.bind(fl[fi]);
    for (int p : used) a.push(p);
    if (pad) a.alu_ri(5, RSP, 8);
    for (uint32_t r = 0; r < f.nparams; r++) if (preg[r] >= 0) a.load(preg[r], RBX, int32_t(8 * r));

    for (size_t pc = 0; pc < M; pc++) {
      const Ins& in = f.code[pc];
      a.bind(pcl[pc]);
      const MicroOp* sem = sem_of(in.op);
      // karsilastir + hemen ardindan jz/jnz (karsilastirma sonucu baska yerde okunmuyorsa): birlestir
      bool fuse = false;
      // Birlestirme karsilastirma sonucunu yazmaz; bu yalniz "kesin atama" ispatliysa guvenli (aksi halde cagri penceresi
      // ustuste binen programlarda cagiran, cagrilanin yazmacini okuyabilir).
      if (P.zero_free && is_cmp_op(in.op) && pc + 1 < M && !tgt[pc + 1]) {
        const Ins& nx = f.code[pc + 1];
        fuse = (nx.op == OP_JZ || nx.op == OP_JNZ) && nx.a == in.a && rd[size_t(in.a)] == 1;
      }
      V tv[5];
      bool done = false;
      for (size_t mi = 0; sem[mi].k != MK_END && !done; mi++) {
        const MicroOp& m = sem[mi];
        const MicroOp& nxt = sem[mi + 1];
        // sonraki mikro-islem bu sonucu bir VM yazmacina yaziyorsa hedef bilgisi
        int hint = -1; bool wrNext = false; V wdst;
        if (nxt.k == MK_WR && nxt.b == (m.k == MK_ALU || m.k == MK_CMP ? m.b : m.k == MK_LD ? m.a : 255)) {
          wrNext = true; wdst = vmloc(fld(in, nxt.a));
          if (wdst.k == V_REG) hint = wdst.r;
        }
        switch (m.k) {
          case MK_RD: tv[m.a] = vmloc(fld(in, m.b)); break;
          case MK_IM: tv[m.a] = vimm(fld(in, m.b)); break;
          case MK_LK: tv[m.a] = vimm(int64_t(P.consts[size_t(fld(in, m.b))])); break;
          case MK_WR: a.store_to(vmloc(fld(in, m.a)), tv[m.b]); break;
          case MK_ALU: {
            V A = tv[m.c], B = tv[m.d];
            // yerinde guncelleme: bellek hedefi = bellek kaynagi (add [m], x)
            if (wrNext && wdst.k == V_MEM && A.k == V_MEM && A.d == wdst.d && m.a <= ALUK_XOR && m.a != ALUK_MUL && (B.k == V_REG || B.k == V_IMM)) {
              static const int ext[] = {0, 5, 0, 4, 1, 6};   // ADD SUB MUL AND OR XOR
              static const uint8_t opc[] = {0x01, 0x29, 0, 0x21, 0x09, 0x31};
              if (B.k == V_IMM) a.rm_imm(ext[m.a], A, int32_t(B.imm)); else a.rm_op(opc[m.a], B.r, A);
              mi++; break;   // WR atlandi
            }
            V r = a.emit_alu(m.a, A, B, hint);
            tv[m.b] = r;
            if (wrNext && r.k == V_REG && hint >= 0 && r.r == hint) mi++;
            break;
          }
          case MK_CMP: {
            static const int cc[] = {CC_E, CC_NE, CC_L, CC_LE, CC_G, CC_GE};
            a.cmp_flags(tv[m.c], tv[m.d]);
            if (fuse) {
              const Ins& nx = f.code[pc + 1];
              int target = nx.b; int cond = nx.op == OP_JZ ? inv_cc(cc[m.a]) : cc[m.a];
              if (target > int(pc + 1)) a.jcc(cond, pcl[size_t(target)]);
              else { Label skip; a.jcc(inv_cc(cond), skip); checkpoint(); a.jmp(pcl[size_t(target)]); a.bind(skip); }
              a.bind(pcl[pc + 1]);
              pc++; done = true; break;
            }
            int D = hint >= 0 ? hint : RAX;
            a.setcc_al(cc[m.a]); a.movzx_al(D);
            tv[m.b] = vreg(D);
            if (wrNext && hint >= 0) mi++;
            break;
          }
          case MK_UN: { a.load_to(RAX, tv[m.c]); a.unary(m.a == UNK_NEG ? 3 : 2, RAX); tv[m.b] = vreg(RAX); break; }
          case MK_DIVS: case MK_MODS: {
            bool isDiv = m.k == MK_DIVS; Label lNeg, lDone;
            a.load_to(RCX, tv[m.c]); a.alu_rr(0x85, RCX, RCX); a.jcc(CC_E, lDiv0);
            a.alu_ri(7, RCX, -1); a.jcc(CC_E, lNeg);
            a.load_to(RAX, tv[m.b]); a.cqo(); a.unary(7, RCX); a.jmp(lDone);
            a.bind(lNeg);
            if (isDiv) { a.load_to(RAX, tv[m.b]); a.unary(3, RAX); } else a.mov_imm32(RDX, 0);
            a.bind(lDone);
            tv[m.a] = vreg(isDiv ? RAX : RDX);
            break;
          }
          case MK_DIVN: case MK_MODN: {
            bool isDiv = m.k == MK_DIVN;
            a.load_to(RCX, tv[m.c]); a.load_to(RAX, tv[m.b]); a.cqo(); a.unary(7, RCX);
            tv[m.a] = vreg(isDiv ? RAX : RDX);
            break;
          }
          case MK_LD: case MK_ST: {
            bool isLd = m.k == MK_LD; int off = fld(in, m.c);
            a.load_to(RAX, tv[m.b]);
            if (off) a.alu_ri(0, RAX, off);
            if (mem_words <= uint64_t(INT32_MAX)) a.alu_ri(7, RAX, int32_t(mem_words)); else { a.mov_imm64(RCX, mem_words); a.alu_rr(0x39, RAX, RCX); }
            a.jcc(CC_AE, lOob);
            if (isLd) { int D = hint >= 0 ? hint : RAX; a.mem_idx(0x8B, D, R15, RAX); tv[m.a] = vreg(D); if (wrNext && hint >= 0) mi++; }
            else {
              V val = tv[m.a];
              if (val.k == V_IMM) a.store_idx_imm(R15, RAX, int32_t(val.imm));
              else if (val.k == V_REG) a.mem_idx(0x89, val.r, R15, RAX);
              else { a.load_to(RCX, val); a.mem_idx(0x89, RCX, R15, RAX); }
            }
            break;
          }
          case MK_BR: {
            int target = fld(in, m.a);
            if (target <= int(pc)) checkpoint();
            a.jmp(pcl[size_t(target)]);
            break;
          }
          case MK_BRZ: case MK_BRNZ: {
            V c = tv[m.a]; int target = fld(in, m.b); bool z = m.k == MK_BRZ;
            if (c.k == V_REG) a.alu_rr(0x85, c.r, c.r);
            else if (c.k == V_MEM) a.rm_imm(7, c, 0);
            else { a.load_to(RAX, c); a.alu_rr(0x85, RAX, RAX); }
            if (target > int(pc)) a.jcc(z ? CC_E : CC_NE, pcl[size_t(target)]);
            else { Label skip; a.jcc(z ? CC_NE : CC_E, skip); checkpoint(); a.jmp(pcl[size_t(target)]); a.bind(skip); }
            break;
          }
          case MK_CALLM: {
            const Func& g = P.funcs[size_t(fld(in, m.a))]; int base = fld(in, m.b);
            checkpoint();
            a.mov_imm64(RAX, stack_end); a.lea(RCX, RBX, 8 * base); a.alu_rr(0x29, RAX, RCX); a.shift_imm(5, RAX, 3);
            a.alu_ri(7, RAX, int32_t(g.nregs)); a.jcc(CC_B, lStack);
            a.alu_ri(7, R12, int32_t(cfg.max_frames)); a.jcc(CC_AE, lDepth); a.incdec(0, R12);
            if (!P.zero_free && g.nregs > g.nparams) {   // (bu kipte yazmaca alma yok: rdi serbest)
              a.lea(RDI, RBX, 8 * (base + int(g.nparams))); a.mov_imm32(RCX, int32_t(g.nregs - g.nparams)); a.xor_eax(); a.rep_stosq();
            }
            a.alu_ri(0, RBX, 8 * base);
            a.call(fl[size_t(fld(in, m.a))]);
            a.alu_ri(5, RBX, 8 * base);
            a.incdec(1, R12);
            break;
          }
          case MK_RETM: a.load_to(RAX, tv[m.a]); a.store(RBX, 0, RAX); emit_ret(); break;
          case MK_HALTM: a.jmp(lEpi); break;
          case MK_HOSTM: {
            checkpoint();
            std::vector<int> cs; for (int p : used) if (caller_saved(p)) cs.push_back(p);
            for (int p : cs) a.push(p);
            bool oddp = cs.size() % 2 == 1; if (oddp) a.alu_ri(5, RSP, 8);
            a.lea(RDI, RBX, 8 * fld(in, m.b)); a.mov_imm64(RSI, A_(&ctx)); a.mov_imm32(RDX, fld(in, m.a));
            a.mov_imm64(RAX, A_(reinterpret_cast<void*>(&host_thunk))); a.call_rax();
            if (oddp) a.alu_ri(0, RSP, 8);
            for (size_t i = cs.size(); i-- > 0;) a.pop(cs[i]);
            break;
          }
          case MK_ALLOCM: {
            a.load_to(RAX, tv[m.b]);
            a.mov_imm64(RDX, A_(&ctx.heap)); a.load(RCX, RDX, 0);
            a.mov_imm64(RDX, mem_words); a.alu_rr(0x29, RDX, RCX);
            a.alu_rr(0x39, RAX, RDX); a.jcc(CC_A, lAlloc);
            a.alu_rr(0x01, RAX, RCX);
            a.mov_imm64(RDX, A_(&ctx.heap)); a.store(RDX, 0, RAX);
            tv[m.a] = vreg(RCX);
            break;
          }
          default: break;
        }
      }
    }
  }
  // ---- calistir ----
  size_t csize = (a.b.size() + 4095) & ~size_t(4095);
  void* code = mmap(nullptr, csize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) { munmap(nstack, ssize); throw PmError("native: yurutulebilir bellek ayrilamadi"); }
  memcpy(code, a.b.data(), a.b.size());
  if (mprotect(code, csize, PROT_READ | PROT_EXEC) != 0) { munmap(code, csize); munmap(nstack, ssize); throw PmError("native: mprotect basarisiz"); }
  reinterpret_cast<void (*)(uint64_t*)>(code)(stack.data());
  munmap(nstack, ssize); munmap(code, csize);
  res.trap = Trap(ctx.trap); res.value = int64_t(ctx.result); res.fuel_used = cfg.fuel - ctx.fuel_out;
  return res;
}
#else
bool native_available() { return false; }
bool native_supported(const Program&) { return false; }
RunResult run_program_native(const Program&, IO&, const RunConfig&) { throw PmError("native: bu platformda makine kodu uretimi yok (yalniz Linux x86-64)"); }
#endif
