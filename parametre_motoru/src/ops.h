// Parametre Motoru -- komut kumesi (tek kaynak: ayristirici, ikili kodlama, dokum ve yorumlayici buradan okur)
#pragma once
#include <cstdint>

// KOMUT VE HOST TANIMLARI burada DEGIL: kanonik sayisal tablodan (oz/komut/komutlar.pmk) uretilen ops_gen.h icindedir (PM_OPS, PM_HOSTS).
// Bu dosya yalniz C++ tarafi yapilarini (Ins, Limits, enum/yardimci tipler) tutar.
#include "ops_gen.h"

enum Op : uint16_t {
#define X(E, N, C, K) OP_##E,
  PM_OPS(X)
#undef X
  OP_COUNT
};

struct OpInfo { const char* name; int nops; const char* kinds; };
// Operand turu karakterleri (PMK1 KIND bolumu): d hedef, r/f/s kaynak yazmac; GENIS operandlar: q (2 sozcuk kaynak), Q (2 sozcuk hedef), o (4 sozcuk kaynak), O (4 sozcuk hedef).
// Genis operand = r, r+1, .. r+w-1 ardisik yazmaclar (dusuk sozcuk once). Genislikler KIND bolumu ext 3 ile ayni olmak zorundadir (dogrula_komut.sh denetler).
inline int kind_width(char k) { return (k == 'q' || k == 'Q') ? 2 : (k == 'o' || k == 'O') ? 4 : 1; }
inline bool kind_src(char k) { return k == 'r' || k == 'f' || k == 's' || k == 'q' || k == 'o'; }
inline bool kind_dst(char k) { return k == 'd' || k == 'Q' || k == 'O'; }
extern const OpInfo kOpInfo[OP_COUNT];

// Host islevleri (sistem arayuzu). Argumanlar pencere yazmaclarinda, sonuc pencerenin ilk yazmacinda.
enum HostId : uint8_t {
#define X(E, N, A) H_##E,
  PM_HOSTS(X)
#undef X
  H_COUNT
};
struct HostInfo { const char* name; int arity; };
extern const HostInfo kHostInfo[H_COUNT];

struct Ins { uint16_t op; int32_t a, b, c; };   // bellek ici cozulmus hal

struct Limits {
  static constexpr uint32_t kMaxFuncs = 65535;
  static constexpr uint32_t kMaxRegs  = 65535;
  static constexpr uint32_t kMaxCode  = 1u << 22;
  static constexpr uint64_t kMaxMemWords = 1ull << 27;
};
