// Parametre Motoru -- komut kumesi (tek kaynak: ayristirici, ikili kodlama, dokum ve yorumlayici buradan okur)
#pragma once
#include <cstdint>

// X(ENUM, "metin adi", operand sayisi, "operand turleri")
//  d: hedef yazmac   r: tamsayi kaynak (yazmac veya sabit)   f: ondalik kaynak   s: herhangi kaynak
//  i: dogrudan tamsayi   k: sabit havuzu indeksi   l: etiket (hedef komut)
//  F: fonksiyon indeksi   b: pencere taban yazmaci   h: host islev kimligi
#define PM_OPS(X)                      \
  X(MOV,  "mov",   2, "ds")            \
  X(LDI,  "ldi",   2, "di")            \
  X(LDK,  "ldk",   2, "dk")            \
  X(ADD,  "add",   3, "drr")           \
  X(SUB,  "sub",   3, "drr")           \
  X(MUL,  "mul",   3, "drr")           \
  X(DIV,  "div",   3, "drr")           \
  X(MOD,  "mod",   3, "drr")           \
  X(AND,  "and",   3, "drr")           \
  X(OR,   "or",    3, "drr")           \
  X(XOR,  "xor",   3, "drr")           \
  X(SHL,  "shl",   3, "drr")           \
  X(SHR,  "shr",   3, "drr")           \
  X(SAR,  "sar",   3, "drr")           \
  X(EQ,   "eq",    3, "drr")           \
  X(NE,   "ne",    3, "drr")           \
  X(LT,   "lt",    3, "drr")           \
  X(LE,   "le",    3, "drr")           \
  X(NEG,  "neg",   2, "dr")            \
  X(NOT,  "not",   2, "dr")            \
  X(FADD, "fadd",  3, "dff")           \
  X(FSUB, "fsub",  3, "dff")           \
  X(FMUL, "fmul",  3, "dff")           \
  X(FDIV, "fdiv",  3, "dff")           \
  X(FNEG, "fneg",  2, "df")            \
  X(FSQRT,"fsqrt", 2, "df")            \
  X(FEQ,  "feq",   3, "dff")           \
  X(FLT,  "flt",   3, "dff")           \
  X(FLE,  "fle",   3, "dff")           \
  X(I2F,  "i2f",   2, "dr")            \
  X(F2I,  "f2i",   2, "df")            \
  X(JMP,  "jmp",   1, "l")             \
  X(JZ,   "jz",    2, "rl")            \
  X(JNZ,  "jnz",   2, "rl")            \
  X(CALL, "call",  2, "Fb")            \
  X(RET,  "ret",   1, "s")             \
  X(HALT, "halt",  0, "")              \
  X(LD,   "ld",    3, "dri")           \
  X(ST,   "st",    3, "sri")           \
  X(ALLOC,"alloc", 2, "dr")            \
  X(HOST, "host",  2, "hb")

enum Op : uint8_t {
#define X(E, N, C, K) OP_##E,
  PM_OPS(X)
#undef X
  OP_COUNT
};

struct OpInfo { const char* name; int nops; const char* kinds; };
extern const OpInfo kOpInfo[OP_COUNT];

// Host islevleri (sistem arayuzu). Argumanlar pencere yazmaclarinda, sonuc pencerenin ilk yazmacinda.
enum HostId : uint8_t { H_PUTC = 0, H_PUTI = 1, H_PUTF = 2, H_GETC = 3, H_COUNT };
struct HostInfo { const char* name; int arity; };
extern const HostInfo kHostInfo[H_COUNT];

struct Ins { uint8_t op; int32_t a, b, c; };   // bellek ici cozulmus hal

struct Limits {
  static constexpr uint32_t kMaxFuncs = 65535;
  static constexpr uint32_t kMaxRegs  = 65535;
  static constexpr uint32_t kMaxCode  = 1u << 22;
  static constexpr uint64_t kMaxMemWords = 1ull << 27;
};
