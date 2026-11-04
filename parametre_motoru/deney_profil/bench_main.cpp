// deney surucusu: dosyayi yukler, N kez calistirir, en iyi sureyi basar
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "program.h"
extern uint64_t g_hist[OP_COUNT];
int main(int argc, char** argv) {
  std::ifstream f(argv[1]); std::string txt((std::istreambuf_iterator<char>(f)), {});
  Program p = assemble(txt);
  int reps = argc > 2 ? atoi(argv[2]) : 5;
  RunConfig cfg; cfg.fuel = ~0ull >> 1;
  double best = 1e9; RunResult r;
  for (int i = 0; i < reps; i++) {
    IO io; auto t0 = std::chrono::steady_clock::now();
    r = run_program(p, io, cfg);
    best = std::min(best, std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    if (i == 0) fputs(io.out.c_str(), stdout);
  }
  printf("adim=%llu  en_iyi=%.4f s  ns/komut=%.3f\n", (unsigned long long)r.steps, best, best * 1e9 / r.steps);
  if (getenv("HIST")) {
    uint64_t tot = 0; for (int i = 0; i < OP_COUNT; i++) tot += g_hist[i];
    for (int i = 0; i < OP_COUNT; i++) if (g_hist[i]) printf("  %-6s %12llu  %5.1f%%\n", kOpInfo[i].name, (unsigned long long)g_hist[i], 100.0 * g_hist[i] / tot);
  }
}
