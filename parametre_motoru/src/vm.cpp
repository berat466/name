// Yorumlayici: yazmac makinesi, pencereli cagri, yakit kontrol noktalari, sinirli bellek. Tuzaklar belirlidir.
// Tum komut anlamlari anlam.h tablosundan uretilir (INTERP genisletmesi).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "program.h"
#include "host.h"

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
// uint64 donusumu (Plan Bolum 01): NaN ve <= -1 -> 0, >= 2^64 -> UINT64_MAX (doyurmali), aksi halde sifira dogru yuvarlama
uint64_t f2u(double d) {
  if (d != d || d <= -1.0) return 0;
  if (d >= 18446744073709551616.0) return UINT64_MAX;
  return uint64_t(d);
}
struct Frame { const Ins* ret_ip; uint64_t* caller_R; uint32_t caller_func; };
}  // namespace

#define PM_SEM_INTERP
#include "anlam.h"

#define TRAP(t) do { trap = (t); goto done; } while (0)
#define CHECKPOINT() do { if (fuel == 0) TRAP(T_FUEL); --fuel; } while (0)

RunResult run_program(const Program& P, IO& io, const RunConfig& cfg) {
  RunResult res;
  const uint64_t mem_words = std::min<uint64_t>(P.mem_words, cfg.max_mem_words);
  if (mem_words < 1 + P.data.size() + P.bss) { res.trap = T_ALLOC; return res; }
  std::vector<uint64_t> mem(mem_words, 0);
  io.mem = mem.data(); io.mem_words = mem_words;
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
#ifdef PM_STEPS
  uint64_t steps = 0;
#endif

  for (;;) {
#ifdef PM_STEPS
    ++steps;
#endif
    const Ins in = *ip;
    switch (in.op) {
#define X(N) case OP_##N: { [[maybe_unused]] uint64_t T1 = 0, T2 = 0, T3 = 0, T4 = 0; SEM_##N(INTERP) } break;
      PM_SEM_OPS(X)
#undef X
      default: TRAP(T_OOB);   // dogrulamadan sonra ulasilamaz
    }
    ++ip;
  }
done:
  res.trap = trap; res.value = result; res.fuel_used = cfg.fuel - fuel; res.func = cur;
  res.pc = uint32_t(ip - fn->code.data());
#ifdef PM_STEPS
  res.steps = steps;
#endif
  return res;
}
