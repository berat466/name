// x86-64 (Linux) makine kodu uretici + yurutucu. Sablonlar anlam.h tablosundan (DATA genisletmesi) okunur;
// yorumlayiciyla AYNI anlam ve AYNI yakit/tuzak kurallari. Yazmaclar bellekte (R[]) kalir; gecici degerler islemci yazmaclarinda.
// Sabit yazmaclar: rbx=R  rbp=Ctx*  r12=cagri derinligi  r13=yakit  r14=bellek sozcuk sayisi  r15=bellek tabani.
#include "program.h"
#if defined(__x86_64__) && defined(__linux__)
#include <cstddef>
#include <sys/mman.h>
#include "host.h"
#define PM_SEM_TABLES
#include "anlam.h"

namespace {
enum { RAX = 0, RCX = 1, RDX = 2, RBX = 3, RSP = 4, RBP = 5, RSI = 6, RDI = 7, R8 = 8, R9 = 9, R10 = 10, R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15 };
const int TREG[5] = {0, RSI, R8, R9, R10};   // T1..T4
enum { CC_B = 2, CC_AE = 3, CC_E = 4, CC_NE = 5, CC_A = 7, CC_L = 0xC, CC_GE = 0xD, CC_LE = 0xE, CC_G = 0xF };

struct Ctx {
  uint64_t saved_rsp, new_sp, stack_end, heap, mem_words, fuel, fuel_out, trap, result;
  uint64_t* mem; IO* io;
};
enum { O_SAVED = 0, O_NEWSP = 8, O_STACKEND = 16, O_HEAP = 24, O_MEMW = 32, O_FUEL = 40, O_FUELOUT = 48, O_TRAP = 56, O_RESULT = 64, O_MEM = 72, O_IO = 80 };
static_assert(offsetof(Ctx, trap) == O_TRAP && offsetof(Ctx, io) == O_IO && offsetof(Ctx, result) == O_RESULT, "Ctx duzeni");

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
  void mov_rr(int dst, int src) { rr(0x89, dst, src); }
  void load(int dst, int base, int32_t disp) { mem(0x8B, dst, base, disp); }
  void store(int base, int32_t disp, int src) { mem(0x89, src, base, disp); }
  void lea(int dst, int base, int32_t disp) { mem(0x8D, dst, base, disp); }
  void mov_imm32(int dst, int32_t v) { rex(0, dst); u8(0xC7); u8(uint8_t(0xC0 | (dst & 7))); u32(uint32_t(v)); }
  void mov_imm64(int dst, uint64_t v) { rex(0, dst); u8(uint8_t(0xB8 + (dst & 7))); u64(v); }
  void alu_rr(uint8_t opc, int rm, int reg) { rr(opc, rm, reg); }   // 01 add, 09 or, 21 and, 29 sub, 31 xor, 39 cmp, 85 test (rm op= reg)
  void alu_ri(int ext, int rm, int32_t v) { rex(0, rm); u8(0x81); u8(uint8_t(0xC0 | (ext << 3) | (rm & 7))); u32(uint32_t(v)); }   // 0 add 5 sub 7 cmp
  void imul(int dst, int src) { rex(dst, src); u8(0x0F); u8(0xAF); u8(uint8_t(0xC0 | ((dst & 7) << 3) | (src & 7))); }
  void shift_cl(int ext, int rm) { rex(0, rm); u8(0xD3); u8(uint8_t(0xC0 | (ext << 3) | (rm & 7))); }   // 4 shl 5 shr 7 sar
  void shr_imm(int rm, uint8_t n) { rex(0, rm); u8(0xC1); u8(uint8_t(0xC0 | (5 << 3) | (rm & 7))); u8(n); }
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
};

void host_thunk(uint64_t* w, Ctx* c, int id) { host_call(*c->io, id, w); }
inline int32_t fld(const Ins& in, int x) { return x == 0 ? in.a : x == 1 ? in.b : in.c; }
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

  // ---- kod uretimi ----
  X a;
  std::vector<Label> fl(P.funcs.size());
  Label lEpi, lDiv0, lOob, lStack, lDepth, lFuel, lAlloc;
  // trampolin: (uint64_t* R, Ctx* ctx)
  a.push(RBX); a.push(RBP); a.push(R12); a.push(R13); a.push(R14); a.push(R15);
  a.mov_rr(RBP, RSI);
  a.store(RBP, O_SAVED, RSP);
  a.load(RSP, RBP, O_NEWSP);
  a.alu_rr(0x31, R12, R12);
  a.load(R13, RBP, O_FUEL); a.load(R14, RBP, O_MEMW); a.load(R15, RBP, O_MEM);
  a.mov_rr(RBX, RDI);
  a.call(fl[P.entry]);
  a.load(RAX, RBX, 0); a.store(RBP, O_RESULT, RAX);
  a.bind(lEpi);
  a.store(RBP, O_FUELOUT, R13);
  a.load(RSP, RBP, O_SAVED);
  a.pop(R15); a.pop(R14); a.pop(R13); a.pop(R12); a.pop(RBP); a.pop(RBX); a.ret();
  struct TS { Label* l; int code; };
  for (TS t : {TS{&lDiv0, T_DIV0}, TS{&lOob, T_OOB}, TS{&lStack, T_STACK}, TS{&lDepth, T_DEPTH}, TS{&lFuel, T_FUEL}, TS{&lAlloc, T_ALLOC}}) {
    a.bind(*t.l); if (t.code == T_FUEL) a.alu_rr(0x31, R13, R13);   /* sub borcu r13 sardi: yakit 0 kalmali */
    a.mov_imm32(RAX, t.code); a.store(RBP, O_TRAP, RAX); a.jmp(lEpi);
  }
  auto checkpoint = [&]() { a.alu_ri(5, R13, 1); a.jcc(CC_B, lFuel); };   // r13==0 ise borc -> TRAP; degilse r13--

  for (size_t fi = 0; fi < P.funcs.size(); fi++) {
    const Func& f = P.funcs[fi];
    std::vector<Label> pcl(f.code.size());
    a.bind(fl[fi]);
    a.push(RBX); a.mov_rr(RBX, RDI);
    for (size_t pc = 0; pc < f.code.size(); pc++) {
      const Ins& in = f.code[pc];
      a.bind(pcl[pc]);
      for (const MicroOp* m = sem_of(in.op); m->k != MK_END; m++) {
        auto T = [&](int t) { return TREG[t]; };
        switch (m->k) {
          case MK_RD: a.load(T(m->a), RBX, 8 * fld(in, m->b)); break;
          case MK_WR: a.store(RBX, 8 * fld(in, m->a), T(m->b)); break;
          case MK_IM: a.mov_imm32(T(m->a), fld(in, m->b)); break;
          case MK_LK: a.mov_imm64(T(m->a), P.consts[size_t(fld(in, m->b))]); break;
          case MK_ALU: {
            int d = T(m->b), x = T(m->c), y = T(m->d);
            switch (m->a) {
              case ALUK_ADD: a.mov_rr(d, x); a.alu_rr(0x01, d, y); break;
              case ALUK_SUB: a.mov_rr(d, x); a.alu_rr(0x29, d, y); break;
              case ALUK_AND: a.mov_rr(d, x); a.alu_rr(0x21, d, y); break;
              case ALUK_OR:  a.mov_rr(d, x); a.alu_rr(0x09, d, y); break;
              case ALUK_XOR: a.mov_rr(d, x); a.alu_rr(0x31, d, y); break;
              case ALUK_MUL: a.mov_rr(d, x); a.imul(d, y); break;
              case ALUK_SHL: a.mov_rr(RCX, y); a.mov_rr(d, x); a.shift_cl(4, d); break;
              case ALUK_SHR: a.mov_rr(RCX, y); a.mov_rr(d, x); a.shift_cl(5, d); break;
              case ALUK_SAR: a.mov_rr(RCX, y); a.mov_rr(d, x); a.shift_cl(7, d); break;
            }
            break;
          }
          case MK_CMP: {
            int d = T(m->b), x = T(m->c), y = T(m->d);
            static const int cc[] = {CC_E, CC_NE, CC_L, CC_LE, CC_G, CC_GE};
            a.alu_rr(0x39, x, y); a.setcc_al(cc[m->a]); a.movzx_al(d);
            break;
          }
          case MK_UN: { int d = T(m->b), x = T(m->c); a.mov_rr(d, x); a.unary(m->a == UNK_NEG ? 3 : 2, d); break; }
          case MK_DIVS: case MK_MODS: {
            bool isDiv = m->k == MK_DIVS; int d = T(m->a), x = T(m->b), y = T(m->c);
            Label lNeg, lDone;
            a.mov_rr(RCX, y); a.alu_rr(0x85, RCX, RCX); a.jcc(CC_E, lDiv0);
            a.alu_ri(7, RCX, -1); a.jcc(CC_E, lNeg);
            a.mov_rr(RAX, x); a.cqo(); a.unary(7, RCX); a.mov_rr(d, isDiv ? RAX : RDX); a.jmp(lDone);
            a.bind(lNeg);
            if (isDiv) { a.mov_rr(RAX, x); a.unary(3, RAX); a.mov_rr(d, RAX); } else a.mov_imm32(d, 0);
            a.bind(lDone);
            break;
          }
          case MK_DIVN: case MK_MODN: {
            bool isDiv = m->k == MK_DIVN; int d = T(m->a), x = T(m->b), y = T(m->c);
            a.mov_rr(RCX, y); a.mov_rr(RAX, x); a.cqo(); a.unary(7, RCX); a.mov_rr(d, isDiv ? RAX : RDX);
            break;
          }
          case MK_LD: case MK_ST: {
            bool isLd = m->k == MK_LD;
            int tv = T(m->a), ta = T(m->b);
            a.mov_rr(RAX, ta); a.alu_ri(0, RAX, fld(in, m->c)); a.alu_rr(0x39, RAX, R14); a.jcc(CC_AE, lOob);
            if (isLd) a.mem_idx(0x8B, tv, R15, RAX); else a.mem_idx(0x89, tv, R15, RAX);
            break;
          }
          case MK_BR: {
            int tgt = fld(in, m->a);
            if (tgt <= int(pc)) checkpoint();
            a.jmp(pcl[size_t(tgt)]);
            break;
          }
          case MK_BRZ: case MK_BRNZ: {
            int t = T(m->a), tgt = fld(in, m->b);
            bool z = m->k == MK_BRZ;
            a.alu_rr(0x85, t, t);
            if (tgt > int(pc)) a.jcc(z ? CC_E : CC_NE, pcl[size_t(tgt)]);
            else { Label skip; a.jcc(z ? CC_NE : CC_E, skip); checkpoint(); a.jmp(pcl[size_t(tgt)]); a.bind(skip); }
            break;
          }
          case MK_CALLM: {
            const Func& g = P.funcs[size_t(fld(in, m->a))]; int base = fld(in, m->b);
            checkpoint();
            a.lea(RDI, RBX, 8 * base);
            a.load(RAX, RBP, O_STACKEND); a.alu_rr(0x29, RAX, RDI); a.shr_imm(RAX, 3); a.alu_ri(7, RAX, int32_t(g.nregs)); a.jcc(CC_B, lStack);
            a.alu_ri(7, R12, int32_t(cfg.max_frames)); a.jcc(CC_AE, lDepth); a.incdec(0, R12);
            if (!P.zero_free && g.nregs > g.nparams) {
              a.lea(RDI, RDI, 8 * int32_t(g.nparams)); a.mov_imm32(RCX, int32_t(g.nregs - g.nparams)); a.xor_eax(); a.rep_stosq();
              a.lea(RDI, RBX, 8 * base);
            }
            a.call(fl[size_t(fld(in, m->a))]);
            a.incdec(1, R12);
            break;
          }
          case MK_RETM: a.mov_rr(RAX, T(m->a)); a.store(RBX, 0, RAX); a.pop(RBX); a.ret(); break;
          case MK_HALTM: a.jmp(lEpi); break;
          case MK_HOSTM: {
            checkpoint();
            a.lea(RDI, RBX, 8 * fld(in, m->b)); a.mov_rr(RSI, RBP); a.mov_imm32(RDX, fld(in, m->a));
            a.mov_imm64(RAX, reinterpret_cast<uint64_t>(&host_thunk)); a.call_rax();
            break;
          }
          case MK_ALLOCM: {
            int d = T(m->a), n = T(m->b);
            a.mov_rr(RAX, n); a.load(RCX, RBP, O_HEAP); a.mov_rr(RDX, R14); a.alu_rr(0x29, RDX, RCX);
            a.alu_rr(0x39, RAX, RDX); a.jcc(CC_A, lAlloc);
            a.mov_rr(d, RCX); a.alu_rr(0x01, RCX, RAX); a.store(RBP, O_HEAP, RCX);
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
  if (code == MAP_FAILED) throw PmError("native: yurutulebilir bellek ayrilamadi");
  memcpy(code, a.b.data(), a.b.size());
  if (mprotect(code, csize, PROT_READ | PROT_EXEC) != 0) { munmap(code, csize); throw PmError("native: mprotect basarisiz"); }
  size_t ssize = (cfg.max_frames * 16 + (256 << 10) + 4095) & ~size_t(4095);
  void* nstack = mmap(nullptr, ssize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (nstack == MAP_FAILED) { munmap(code, csize); throw PmError("native: yigin ayrilamadi"); }
  Ctx ctx{};
  ctx.new_sp = (reinterpret_cast<uint64_t>(nstack) + ssize) & ~uint64_t(15);
  ctx.stack_end = reinterpret_cast<uint64_t>(stack.data() + stack.size());
  ctx.heap = 1 + P.data.size() + P.bss; ctx.mem_words = mem_words; ctx.fuel = cfg.fuel;
  ctx.mem = mem.data(); ctx.io = &io;
  reinterpret_cast<void (*)(uint64_t*, Ctx*)>(code)(stack.data(), &ctx);
  munmap(nstack, ssize); munmap(code, csize);
  res.trap = Trap(ctx.trap); res.value = int64_t(ctx.result); res.fuel_used = cfg.fuel - ctx.fuel_out;
  return res;
}
#else
bool native_available() { return false; }
bool native_supported(const Program&) { return false; }
RunResult run_program_native(const Program&, IO&, const RunConfig&) { throw PmError("native: bu platformda makine kodu uretimi yok (yalniz Linux x86-64)"); }
#endif
