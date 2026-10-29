// URETILDI -- ELLE DUZENLEMEYIN. Kaynak: oz/komut/komutlar.pmk (kanonik sayisal komut tablosu); uretici: komut @@opsh
// X(ENUM, "ad", operand_sayisi, "turler")   turler: d r f s i k l F b h (bkz. PMK1 KIND bolumu)
#pragma once
#define PM_OPS(X) \
  X(MOV, "mov", 2, "ds") \
  X(LDI, "ldi", 2, "di") \
  X(LDK, "ldk", 2, "dk") \
  X(ADD, "add", 3, "drr") \
  X(SUB, "sub", 3, "drr") \
  X(MUL, "mul", 3, "drr") \
  X(DIV, "div", 3, "drr") \
  X(MOD, "mod", 3, "drr") \
  X(AND, "and", 3, "drr") \
  X(OR, "or", 3, "drr") \
  X(XOR, "xor", 3, "drr") \
  X(SHL, "shl", 3, "drr") \
  X(SHR, "shr", 3, "drr") \
  X(SAR, "sar", 3, "drr") \
  X(EQ, "eq", 3, "drr") \
  X(NE, "ne", 3, "drr") \
  X(LT, "lt", 3, "drr") \
  X(LE, "le", 3, "drr") \
  X(NEG, "neg", 2, "dr") \
  X(NOT, "not", 2, "dr") \
  X(FADD, "fadd", 3, "dff") \
  X(FSUB, "fsub", 3, "dff") \
  X(FMUL, "fmul", 3, "dff") \
  X(FDIV, "fdiv", 3, "dff") \
  X(FNEG, "fneg", 2, "df") \
  X(FSQRT, "fsqrt", 2, "df") \
  X(FEQ, "feq", 3, "dff") \
  X(FLT, "flt", 3, "dff") \
  X(FLE, "fle", 3, "dff") \
  X(I2F, "i2f", 2, "dr") \
  X(F2I, "f2i", 2, "df") \
  X(JMP, "jmp", 1, "l") \
  X(JZ, "jz", 2, "rl") \
  X(JNZ, "jnz", 2, "rl") \
  X(CALL, "call", 2, "Fb") \
  X(RET, "ret", 1, "s") \
  X(HALT, "halt", 0, "") \
  X(LD, "ld", 3, "dri") \
  X(ST, "st", 3, "sri") \
  X(ALLOC, "alloc", 2, "dr") \
  X(HOST, "host", 2, "hb") \
  X(ADDI, "addi", 3, "dri") \
  X(MULI, "muli", 3, "dri") \
  X(ANDI, "andi", 3, "dri") \
  X(SHLI, "shli", 3, "dri") \
  X(SHRI, "shri", 3, "dri") \
  X(SARI, "sari", 3, "dri") \
  X(DIVI, "divi", 3, "dri") \
  X(MODI, "modi", 3, "dri") \
  X(EQI, "eqi", 3, "dri") \
  X(NEI, "nei", 3, "dri") \
  X(LTI, "lti", 3, "dri") \
  X(LEI, "lei", 3, "dri") \
  X(GTI, "gti", 3, "dri") \
  X(GEI, "gei", 3, "dri")

// X(ENUM, "ad", arguman_sayisi)  host islevleri
#define PM_HOSTS(X) \
  X(PUTC, "putc", 1) \
  X(PUTI, "puti", 1) \
  X(PUTF, "putf", 1) \
  X(GETC, "getc", 0) \
  X(PRESENT, "present", 3) \
  X(POLL, "poll", 0) \
  X(FRAME, "frame", 0)
