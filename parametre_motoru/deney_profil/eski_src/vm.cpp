// Yorumlayici: yazmac makinesi, pencereli cagri, yakit (adim butcesi), sinirli bellek. Tuzaklar belirlidir (tanimsiz davranis yok).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "program.h"

const char* kTrapName[T_COUNT] = {"OK", "DIV0", "OOB", "STACK", "DEPTH", "FUEL", "ALLOC"};

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
}  // namespace

RunResult run_program(const Program& P, IO& io, const RunConfig& cfg) {
  RunResult res;
  const uint64_t mem_words = std::min<uint64_t>(P.mem_words, cfg.max_mem_words);
  if (mem_words < 1 + P.data.size() + P.bss) { res.trap = T_ALLOC; return res; }
  std::vector<uint64_t> mem(mem_words, 0);
  for (size_t i = 0; i < P.data.size(); i++) mem[1 + i] = uint64_t(P.data[i]);
  uint64_t heap = 1 + P.data.size() + P.bss;
  std::vector<uint64_t> stack(cfg.stack_words, 0);
  std::vector<Frame> frames(cfg.max_frames);
  size_t depth = 0;

  uint64_t* const stack_end = stack.data() + stack.size();
  uint32_t cur = P.entry;
  const Func* fn = &P.funcs[cur];
  if (fn->nregs > stack.size()) { res.trap = T_STACK; return res; }
  uint64_t* R = stack.data();
  const Ins* ip = fn->code.data();
  uint64_t fuel = cfg.fuel;
  Trap trap = T_OK;
  int64_t result = 0;

#define TRAP(t) do { trap = (t); goto done; } while (0)
  for (;;) {
    if (fuel == 0) TRAP(T_FUEL);
    --fuel;
    const Ins in = *ip;
    switch (in.op) {
      case OP_MOV: R[in.a] = R[in.b]; break;
      case OP_LDI: R[in.a] = uint64_t(int64_t(in.b)); break;
      case OP_LDK: R[in.a] = P.consts[in.b]; break;
      case OP_ADD: R[in.a] = R[in.b] + R[in.c]; break;
      case OP_SUB: R[in.a] = R[in.b] - R[in.c]; break;
      case OP_MUL: R[in.a] = R[in.b] * R[in.c]; break;
      case OP_DIV: {
        int64_t x = asI(R[in.b]), y = asI(R[in.c]);
        if (y == 0) TRAP(T_DIV0);
        R[in.a] = (y == -1) ? uint64_t(0) - uint64_t(x) : uint64_t(x / y);
        break;
      }
      case OP_MOD: {
        int64_t x = asI(R[in.b]), y = asI(R[in.c]);
        if (y == 0) TRAP(T_DIV0);
        R[in.a] = (y == -1) ? 0 : uint64_t(x % y);
        break;
      }
      case OP_AND: R[in.a] = R[in.b] & R[in.c]; break;
      case OP_OR:  R[in.a] = R[in.b] | R[in.c]; break;
      case OP_XOR: R[in.a] = R[in.b] ^ R[in.c]; break;
      case OP_SHL: R[in.a] = R[in.b] << (R[in.c] & 63); break;
      case OP_SHR: R[in.a] = R[in.b] >> (R[in.c] & 63); break;
      case OP_SAR: R[in.a] = uint64_t(asI(R[in.b]) >> (R[in.c] & 63)); break;
      case OP_EQ: R[in.a] = R[in.b] == R[in.c]; break;
      case OP_NE: R[in.a] = R[in.b] != R[in.c]; break;
      case OP_LT: R[in.a] = asI(R[in.b]) < asI(R[in.c]); break;
      case OP_LE: R[in.a] = asI(R[in.b]) <= asI(R[in.c]); break;
      case OP_NEG: R[in.a] = uint64_t(0) - R[in.b]; break;
      case OP_NOT: R[in.a] = ~R[in.b]; break;
      case OP_FADD: R[in.a] = fromF(asF(R[in.b]) + asF(R[in.c])); break;
      case OP_FSUB: R[in.a] = fromF(asF(R[in.b]) - asF(R[in.c])); break;
      case OP_FMUL: R[in.a] = fromF(asF(R[in.b]) * asF(R[in.c])); break;
      case OP_FDIV: R[in.a] = fromF(asF(R[in.b]) / asF(R[in.c])); break;
      case OP_FNEG: R[in.a] = fromF(-asF(R[in.b])); break;
      case OP_FSQRT: R[in.a] = fromF(std::sqrt(asF(R[in.b]))); break;
      case OP_FEQ: R[in.a] = asF(R[in.b]) == asF(R[in.c]); break;
      case OP_FLT: R[in.a] = asF(R[in.b]) < asF(R[in.c]); break;
      case OP_FLE: R[in.a] = asF(R[in.b]) <= asF(R[in.c]); break;
      case OP_I2F: R[in.a] = fromF(double(asI(R[in.b]))); break;
      case OP_F2I: R[in.a] = uint64_t(f2i(asF(R[in.b]))); break;
      case OP_JMP: ip = fn->code.data() + in.a; continue;
      case OP_JZ:  if (R[in.a] == 0) { ip = fn->code.data() + in.b; continue; } break;
      case OP_JNZ: if (R[in.a] != 0) { ip = fn->code.data() + in.b; continue; } break;
      case OP_CALL: {
        const Func& g = P.funcs[in.a];
        uint64_t* nr = R + in.b;
        if (size_t(stack_end - nr) < g.nregs) TRAP(T_STACK);
        if (depth >= frames.size()) TRAP(T_DEPTH);
        frames[depth++] = Frame{ip + 1, R, cur};
        memset(nr + g.nparams, 0, size_t(g.nregs - g.nparams) * 8);
        R = nr; cur = in.a; fn = &g; ip = g.code.data();
        continue;
      }
      case OP_RET: {
        uint64_t v = R[in.a];
        if (depth == 0) { result = asI(v); goto done; }
        R[0] = v;
        const Frame& fr = frames[--depth];
        ip = fr.ret_ip; R = fr.caller_R; cur = fr.caller_func; fn = &P.funcs[cur];
        continue;
      }
      case OP_HALT: result = 0; goto done;
      case OP_LD: {
        uint64_t addr = R[in.b] + uint64_t(int64_t(in.c));
        if (addr >= mem_words) TRAP(T_OOB);
        R[in.a] = mem[addr]; break;
      }
      case OP_ST: {
        uint64_t addr = R[in.b] + uint64_t(int64_t(in.c));
        if (addr >= mem_words) TRAP(T_OOB);
        mem[addr] = R[in.a]; break;
      }
      case OP_ALLOC: {
        int64_t n = asI(R[in.b]);
        if (n < 0 || uint64_t(n) > mem_words - heap) TRAP(T_ALLOC);
        R[in.a] = heap; heap += uint64_t(n); break;
      }
      case OP_HOST: {
        uint64_t* w = R + in.b;
        switch (in.a) {
          case H_PUTC: io.out.push_back(char(w[0] & 255)); w[0] = 0; break;
          case H_PUTI: io.out += std::to_string(asI(w[0])); w[0] = 0; break;
          case H_PUTF: { char buf[64]; snprintf(buf, sizeof buf, "%.15g", asF(w[0])); io.out += buf; w[0] = 0; break; }
          case H_GETC: { int ch = io.in ? fgetc(io.in) : -1; w[0] = uint64_t(int64_t(ch)); break; }
        }
        break;
      }
      default: TRAP(T_OOB);   // dogrulamadan sonra ulasilamaz
    }
    ++ip;
  }
done:
  res.trap = trap; res.value = result; res.steps = cfg.fuel - fuel; res.func = cur;
  res.pc = uint32_t(ip - fn->code.data());
  return res;
}
