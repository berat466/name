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
  X(GEI, "gei", 3, "dri") \
  X(SEXT8, "sext8", 2, "dr") \
  X(SEXT16, "sext16", 2, "dr") \
  X(SEXT32, "sext32", 2, "dr") \
  X(ZEXT8, "zext8", 2, "dr") \
  X(ZEXT16, "zext16", 2, "dr") \
  X(ZEXT32, "zext32", 2, "dr") \
  X(ADD_I8, "add_i8", 3, "drr") \
  X(SUB_I8, "sub_i8", 3, "drr") \
  X(MUL_I8, "mul_i8", 3, "drr") \
  X(SHL_I8, "shl_i8", 3, "drr") \
  X(DIV_I8, "div_i8", 3, "drr") \
  X(MOD_I8, "mod_i8", 3, "drr") \
  X(NEG_I8, "neg_i8", 2, "dr") \
  X(ADD_I16, "add_i16", 3, "drr") \
  X(SUB_I16, "sub_i16", 3, "drr") \
  X(MUL_I16, "mul_i16", 3, "drr") \
  X(SHL_I16, "shl_i16", 3, "drr") \
  X(DIV_I16, "div_i16", 3, "drr") \
  X(MOD_I16, "mod_i16", 3, "drr") \
  X(NEG_I16, "neg_i16", 2, "dr") \
  X(ADD_I32, "add_i32", 3, "drr") \
  X(SUB_I32, "sub_i32", 3, "drr") \
  X(MUL_I32, "mul_i32", 3, "drr") \
  X(SHL_I32, "shl_i32", 3, "drr") \
  X(DIV_I32, "div_i32", 3, "drr") \
  X(MOD_I32, "mod_i32", 3, "drr") \
  X(NEG_I32, "neg_i32", 2, "dr") \
  X(ADD_U8, "add_u8", 3, "drr") \
  X(SUB_U8, "sub_u8", 3, "drr") \
  X(MUL_U8, "mul_u8", 3, "drr") \
  X(SHL_U8, "shl_u8", 3, "drr") \
  X(NEG_U8, "neg_u8", 2, "dr") \
  X(NOT_U8, "not_u8", 2, "dr") \
  X(ADD_U16, "add_u16", 3, "drr") \
  X(SUB_U16, "sub_u16", 3, "drr") \
  X(MUL_U16, "mul_u16", 3, "drr") \
  X(SHL_U16, "shl_u16", 3, "drr") \
  X(NEG_U16, "neg_u16", 2, "dr") \
  X(NOT_U16, "not_u16", 2, "dr") \
  X(ADD_U32, "add_u32", 3, "drr") \
  X(SUB_U32, "sub_u32", 3, "drr") \
  X(MUL_U32, "mul_u32", 3, "drr") \
  X(SHL_U32, "shl_u32", 3, "drr") \
  X(NEG_U32, "neg_u32", 2, "dr") \
  X(NOT_U32, "not_u32", 2, "dr") \
  X(DIVU, "divu", 3, "drr") \
  X(MODU, "modu", 3, "drr") \
  X(LTU, "ltu", 3, "drr") \
  X(LEU, "leu", 3, "drr") \
  X(U2F, "u2f", 2, "dr") \
  X(F2U, "f2u", 2, "dr")

// X(ENUM, "ad", arguman_sayisi)  host islevleri
#define PM_HOSTS(X) \
  X(PUTC, "putc", 1) \
  X(PUTI, "puti", 1) \
  X(PUTF, "putf", 1) \
  X(GETC, "getc", 0) \
  X(PRESENT, "present", 3) \
  X(POLL, "poll", 0) \
  X(FRAME, "frame", 0)
