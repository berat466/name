// Okunabilir dokum: motorun gercekte calistirdigi tablo.
#include <cstdio>
#include <cstring>
#include "program.h"

std::string disassemble(const Program& p) {
  std::string o;
  char buf[160];
  snprintf(buf, sizeof buf, "; bellek=%llu sozcuk, sabit=%zu, veri=%zu, bss=%llu, giris=f%u\n", (unsigned long long)p.mem_words, p.consts.size(), p.data.size(), (unsigned long long)p.bss, p.entry);
  o += buf;
  for (size_t fi = 0; fi < p.funcs.size(); fi++) {
    const Func& f = p.funcs[fi];
    snprintf(buf, sizeof buf, "func f%zu %s (parametre=%u, yazmac=%u, komut=%zu)\n", fi, f.name.c_str(), f.nparams, f.nregs, f.code.size());
    o += buf;
    for (size_t pc = 0; pc < f.code.size(); pc++) {
      const Ins& in = f.code[pc]; const OpInfo& oi = kOpInfo[in.op];
      snprintf(buf, sizeof buf, "  %4zu  %-5s", pc, oi.name); o += buf;
      for (int j = 0; j < oi.nops; j++) {
        int32_t v = j == 0 ? in.a : j == 1 ? in.b : in.c;
        char k = oi.kinds[j];
        o += j ? ", " : " ";
        if (k == 'l') snprintf(buf, sizeof buf, "@%d", v);
        else if (k == 'i') snprintf(buf, sizeof buf, "%d", v);
        else if (k == 'F') snprintf(buf, sizeof buf, "f%d(%s)", v, size_t(v) < p.funcs.size() ? p.funcs[v].name.c_str() : "?");
        else if (k == 'k') { double d; memcpy(&d, &p.consts[v], 8); snprintf(buf, sizeof buf, "k%d(%lld|%g)", v, (long long)p.consts[v], d); }
        else if (k == 'h') snprintf(buf, sizeof buf, "%s", kHostInfo[v].name);
        else if (k == 'b') snprintf(buf, sizeof buf, "pencere r%d", v);
        else snprintf(buf, sizeof buf, "r%d", v);
        o += buf;
      }
      o += "\n";
    }
  }
  return o;
}
