// Host islevleri: yorumlayici ve makine kodu yurutucusu AYNI kodu kullanir (tek kaynak).
#pragma once
#include <cstdio>
#include <cstring>
#include "program.h"
inline void host_call(IO& io, int id, uint64_t* w) {
  switch (id) {
    case H_PUTC: io.out.push_back(char(w[0] & 255)); w[0] = 0; break;
    case H_PUTI: io.out += std::to_string(int64_t(w[0])); w[0] = 0; break;
    case H_PUTF: { double d; memcpy(&d, &w[0], 8); char buf[64]; snprintf(buf, sizeof buf, "%.15g", d); io.out += buf; w[0] = 0; break; }
    case H_GETC: { int ch = io.in ? fgetc(io.in) : -1; w[0] = uint64_t(int64_t(ch)); break; }
  }
}
