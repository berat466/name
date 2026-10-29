// Motorun KENDI vm.cpp'sini kullanan sure olcucu: dosyayi yukler, N kez calistirir, en iyi sureyi basar
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "program.h"
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
  }
  printf("%.4f", best);
}
