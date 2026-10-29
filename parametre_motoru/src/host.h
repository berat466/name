// Host islevleri: yorumlayici ve makine kodu yurutucusu AYNI kodu kullanir (tek kaynak).
//   putc/puti/putf/getc : metin
//   present(ptr, w, h)  : uygulamanin bellegindeki ptr'den baslayan w*h sozcuk (her biri 0xRRGGBB) kareyi sunar.
//                         sonuc: 0 devam, 1 cik (max_kare doldu ya da cikis istendi), -1 gecersiz arguman (sunum yok)
//   poll()              : siradaki girdi olayi: (1<<32)|tus_kodu; yoksa 0. Olay, kare sayaci olayin karesine ulasinca teslim edilir.
//   frame()             : simdiye kadar sunulan kare sayisi
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
    case H_PRESENT: {
      const uint64_t ptr = w[0], W = w[1], H = w[2];
      if (W == 0 || H == 0 || W > 4096 || H > 4096 || ptr == 0 || ptr > io.mem_words || W * H > io.mem_words - ptr) { w[0] = ~uint64_t(0); break; }
      io.fw = uint32_t(W); io.fh = uint32_t(H); io.frame.resize(size_t(W * H));
      uint64_t h = 0xcbf29ce484222325ull;
      auto mix = [&](uint64_t b) { h ^= (b & 255); h *= 0x100000001b3ull; };
      mix(W); mix(W >> 8); mix(H); mix(H >> 8);
      for (uint64_t i = 0; i < W * H; i++) {
        uint32_t px = uint32_t(io.mem[ptr + i] & 0xFFFFFF);
        io.frame[size_t(i)] = px;
        mix(px); mix(px >> 8); mix(px >> 16);
      }
      io.last_hash = h; io.chain_hash = (io.chain_hash ^ h) * 0x100000001b3ull;
      io.frames++;
      w[0] = (io.quit || (io.max_frames && io.frames >= io.max_frames)) ? 1 : 0;
      break;
    }
    case H_POLL:
      if (io.ev_next < io.events.size() && io.events[io.ev_next].first <= io.frames) w[0] = (uint64_t(1) << 32) | io.events[io.ev_next++].second;
      else w[0] = 0;
      break;
    case H_FRAME: w[0] = io.frames; break;
  }
}
