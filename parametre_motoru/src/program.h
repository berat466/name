#pragma once
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>
#include "ops.h"

struct Func {
  std::string name;               // sokulmus (strip) ikilide bos
  uint32_t nparams = 0, nregs = 0;
  std::vector<Ins> code;
};

struct Program {
  uint64_t mem_words = 65536;
  uint32_t entry = 0;
  std::vector<uint64_t> consts;
  std::vector<int64_t> data;      // adres 1'den baslayarak bellege yerlesir
  uint64_t bss = 0;               // veri sozcuklerinden sonra gelen sifir dolu alan (ikilide yalnizca sayi)
  std::vector<Func> funcs;
  bool has_names = false;
  bool zero_free = false;         // yukleyicinin ispatladigi: hicbir yazmac okunmadan once yazilmadan okunmuyor -> cagri sifirlamasi atlanabilir (ikiliye yazilmaz)
};

struct PmError : std::runtime_error { using std::runtime_error::runtime_error; };

// ikili.cpp
std::vector<uint8_t> encode_program(const Program& p, bool strip_names);
Program decode_program(const uint8_t* data, size_t n);   // cozer VE dogrular; hatada PmError
void validate_program(const Program& p);
void analyze_program(Program& p);   // kesin atama analizi -> p.zero_free

// asm.cpp
Program assemble(const std::string& text);

// dump.cpp
std::string disassemble(const Program& p);

// vm.cpp
enum Trap { T_OK = 0, T_DIV0, T_OOB, T_STACK, T_DEPTH, T_FUEL, T_ALLOC, T_COUNT };
extern const char* kTrapName[T_COUNT];
struct IO {
  std::string out;
  FILE* in = nullptr;   // getc buradan okur; null ise girdi yok (EOF)
};
struct RunConfig {
  uint64_t fuel = 4000000000ull;
  uint64_t max_mem_words = Limits::kMaxMemWords;
  size_t stack_words = 1u << 20;
  size_t max_frames = 1u << 16;
  int native_promote = 4;   // makine kodu: yazmaca alma esigi (agirlikli kullanim); 0 = hepsi, buyuk deger = hic
};
struct RunResult { Trap trap = T_OK; int64_t value = 0; uint64_t fuel_used = 0; uint64_t steps = 0; /* yalniz -DPM_STEPS */ uint32_t func = 0, pc = 0; };
RunResult run_program(const Program& p, IO& io, const RunConfig& cfg);
// native_x64.cpp: anlam tablosundan x86-64 makine kodu uretir ve calistirir (yorumlayiciyla ayni anlam / yakit / tuzak kurallari)
bool native_available();
bool native_supported(const Program& p);
RunResult run_program_native(const Program& p, IO& io, const RunConfig& cfg);
