// DENEY kopyasi (motorun kendisi degil): vm.cpp ile ayni anlam, olcum icin ac/kapa edilebilir mekanizmalar.
//  -DPM_CGOTO  hesaplanmis goto dagitimi      -DPM_NOFUEL  yakit sayaci yok
//  -DPM_NOZERO cagri sirasinda yazmac sifirlama yok   -DPM_NOCHK  yigin/derinlik denetimi yok
//  -DPM_NOMEMCHK ld/st sinir denetimi yok     -DPM_HIST  komut histogrami
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "program.h"
const char* kTrapName[T_COUNT] = {"OK", "DIV0", "OOB", "STACK", "DEPTH", "FUEL", "ALLOC"};
uint64_t g_hist[OP_COUNT];
namespace {
inline double asF(uint64_t x) { double d; memcpy(&d, &x, 8); return d; }
inline uint64_t fromF(double d) { uint64_t x; memcpy(&x, &d, 8); return x; }
inline int64_t asI(uint64_t x) { return int64_t(x); }
int64_t f2i(double d) {
  if (d != d) return 0;
  if (d >= 9223372036854775807.0) return INT64_MAX;
  if (d <= -9223372036854775808.0) return INT64_MIN;
  return int64_t(d);
}
struct Frame { const Ins* ret_ip; uint64_t* caller_R; uint32_t caller_func; };
}
#ifdef PM_NOFUEL
#define FUELCHK
#else
#define FUELCHK do { if (fuel == 0) { trap = T_FUEL; goto done; } --fuel; } while (0)
#endif
#ifdef PM_HIST
#define HISTCNT g_hist[in.op]++;
#else
#define HISTCNT
#endif
#ifdef PM_NOMEMCHK
#define MEMCHK(addr)
#else
#define MEMCHK(addr) if ((addr) >= mem_words) TRAP(T_OOB)
#endif
#define TRAP(t) do { trap = (t); goto done; } while (0)
#ifdef PM_CGOTO
#define DISPATCH do { FUELCHK; in = *ip; HISTCNT goto *tbl[in.op]; } while (0)
#define OPC(n) L_##n:
#define ADVANCE do { ++ip; DISPATCH; } while (0)
#define JUMPED DISPATCH
#else
#define OPC(n) case OP_##n:
#define ADVANCE break
#define JUMPED continue
#endif

RunResult run_program(const Program& P, IO& io, const RunConfig& cfg) {
  RunResult res;
  const uint64_t mem_words = std::min<uint64_t>(P.mem_words, cfg.max_mem_words);
  std::vector<uint64_t> mem(mem_words, 0);
  for (size_t i = 0; i < P.data.size(); i++) mem[1 + i] = uint64_t(P.data[i]);
  uint64_t heap = 1 + P.data.size() + P.bss;
  std::vector<uint64_t> stack(cfg.stack_words, 0);
  std::vector<Frame> frames(cfg.max_frames);
  size_t depth = 0;
  uint64_t* const stack_end = stack.data() + stack.size();
  uint32_t cur = P.entry;
  const Func* fn = &P.funcs[cur];
  uint64_t* R = stack.data();
  const Ins* ip = fn->code.data();
  uint64_t fuel = cfg.fuel;
  Trap trap = T_OK;
  int64_t result = 0;
  Ins in;
#ifdef PM_CGOTO
  static const void* tbl[OP_COUNT] = {
#define X(E, N, C, K) &&L_##E,
  PM_OPS(X)
#undef X
  };
  DISPATCH;
  {
#else
  for (;;) {
    FUELCHK;
    in = *ip;
    HISTCNT
    switch (in.op) {
#endif
      OPC(MOV) R[in.a] = R[in.b]; ADVANCE;
      OPC(LDI) R[in.a] = uint64_t(int64_t(in.b)); ADVANCE;
      OPC(LDK) R[in.a] = P.consts[in.b]; ADVANCE;
      OPC(ADD) R[in.a] = R[in.b] + R[in.c]; ADVANCE;
      OPC(SUB) R[in.a] = R[in.b] - R[in.c]; ADVANCE;
      OPC(MUL) R[in.a] = R[in.b] * R[in.c]; ADVANCE;
      OPC(DIV) { int64_t x = asI(R[in.b]), y = asI(R[in.c]); if (y == 0) TRAP(T_DIV0); R[in.a] = (y == -1) ? uint64_t(0) - uint64_t(x) : uint64_t(x / y); ADVANCE; }
      OPC(MOD) { int64_t x = asI(R[in.b]), y = asI(R[in.c]); if (y == 0) TRAP(T_DIV0); R[in.a] = (y == -1) ? 0 : uint64_t(x % y); ADVANCE; }
      OPC(AND) R[in.a] = R[in.b] & R[in.c]; ADVANCE;
      OPC(OR)  R[in.a] = R[in.b] | R[in.c]; ADVANCE;
      OPC(XOR) R[in.a] = R[in.b] ^ R[in.c]; ADVANCE;
      OPC(SHL) R[in.a] = R[in.b] << (R[in.c] & 63); ADVANCE;
      OPC(SHR) R[in.a] = R[in.b] >> (R[in.c] & 63); ADVANCE;
      OPC(SAR) R[in.a] = uint64_t(asI(R[in.b]) >> (R[in.c] & 63)); ADVANCE;
      OPC(EQ) R[in.a] = R[in.b] == R[in.c]; ADVANCE;
      OPC(NE) R[in.a] = R[in.b] != R[in.c]; ADVANCE;
      OPC(LT) R[in.a] = asI(R[in.b]) < asI(R[in.c]); ADVANCE;
      OPC(LE) R[in.a] = asI(R[in.b]) <= asI(R[in.c]); ADVANCE;
      OPC(NEG) R[in.a] = uint64_t(0) - R[in.b]; ADVANCE;
      OPC(NOT) R[in.a] = ~R[in.b]; ADVANCE;
      OPC(FADD) R[in.a] = fromF(asF(R[in.b]) + asF(R[in.c])); ADVANCE;
      OPC(FSUB) R[in.a] = fromF(asF(R[in.b]) - asF(R[in.c])); ADVANCE;
      OPC(FMUL) R[in.a] = fromF(asF(R[in.b]) * asF(R[in.c])); ADVANCE;
      OPC(FDIV) R[in.a] = fromF(asF(R[in.b]) / asF(R[in.c])); ADVANCE;
      OPC(FNEG) R[in.a] = fromF(-asF(R[in.b])); ADVANCE;
      OPC(FSQRT) R[in.a] = fromF(std::sqrt(asF(R[in.b]))); ADVANCE;
      OPC(FEQ) R[in.a] = asF(R[in.b]) == asF(R[in.c]); ADVANCE;
      OPC(FLT) R[in.a] = asF(R[in.b]) < asF(R[in.c]); ADVANCE;
      OPC(FLE) R[in.a] = asF(R[in.b]) <= asF(R[in.c]); ADVANCE;
      OPC(I2F) R[in.a] = fromF(double(asI(R[in.b]))); ADVANCE;
      OPC(F2I) R[in.a] = uint64_t(f2i(asF(R[in.b]))); ADVANCE;
      OPC(JMP) ip = fn->code.data() + in.a; JUMPED;
      OPC(JZ)  if (R[in.a] == 0) { ip = fn->code.data() + in.b; JUMPED; } ADVANCE;
      OPC(JNZ) if (R[in.a] != 0) { ip = fn->code.data() + in.b; JUMPED; } ADVANCE;
      OPC(CALL) {
        const Func& g = P.funcs[in.a];
        uint64_t* nr = R + in.b;
#ifndef PM_NOCHK
        if (size_t(stack_end - nr) < g.nregs) TRAP(T_STACK);
        if (depth >= frames.size()) TRAP(T_DEPTH);
#endif
        frames[depth++] = Frame{ip + 1, R, cur};
#ifndef PM_NOZERO
        memset(nr + g.nparams, 0, size_t(g.nregs - g.nparams) * 8);
#endif
        R = nr; cur = in.a; fn = &g; ip = g.code.data();
        JUMPED;
      }
      OPC(RET) {
        uint64_t v = R[in.a];
        if (depth == 0) { result = asI(v); goto done; }
        R[0] = v;
        const Frame& fr = frames[--depth];
        ip = fr.ret_ip; R = fr.caller_R; cur = fr.caller_func; fn = &P.funcs[cur];
        JUMPED;
      }
      OPC(HALT) result = 0; goto done;
      OPC(LD) { uint64_t addr = R[in.b] + uint64_t(int64_t(in.c)); MEMCHK(addr); R[in.a] = mem[addr]; ADVANCE; }
      OPC(ST) { uint64_t addr = R[in.b] + uint64_t(int64_t(in.c)); MEMCHK(addr); mem[addr] = R[in.a]; ADVANCE; }
      OPC(ALLOC) { int64_t n = asI(R[in.b]); if (n < 0 || uint64_t(n) > mem_words - heap) TRAP(T_ALLOC); R[in.a] = heap; heap += uint64_t(n); ADVANCE; }
      OPC(HOST) {
        uint64_t* w = R + in.b;
        switch (in.a) {
          case H_PUTC: io.out.push_back(char(w[0] & 255)); w[0] = 0; break;
          case H_PUTI: io.out += std::to_string(asI(w[0])); w[0] = 0; break;
          case H_PUTF: { char buf[64]; snprintf(buf, sizeof buf, "%.15g", asF(w[0])); io.out += buf; w[0] = 0; break; }
          case H_GETC: { int ch = io.in ? fgetc(io.in) : -1; w[0] = uint64_t(int64_t(ch)); break; }
        }
        ADVANCE;
      }
#ifdef PM_CGOTO
  }
#else
      default: TRAP(T_OOB);
    }
    ++ip;
  }
#endif
done:
  res.trap = trap; res.value = result; res.steps = cfg.fuel - fuel;
  res.func = cur; res.pc = uint32_t(ip - fn->code.data());
  return res;
}
