// KOMUT ANLAM TABLOSU -- tek kaynak.
// Her komutun anlami kucuk "mikro-islem" dizisiyle yazilir. Ayni tablodan:
//   (1) yorumlayicinin islemcileri (vm.cpp, INTERP genisletmesi, derleme aninda)
//   (2) x86-64 makine kodu uretecinin sablonlari (native_x64.cpp, DATA genisletmesi)
// uretilir. Tabloda olmayan komutlar (ondalik islemler) yalnizca yorumlayicida elle yazilidir.
//
// Mikro-islemler (T1..T4 gecici 64 bit degerler; a,b,c komutun operand alanlari):
//   RD(t,x)   T_t = yazmac[x]            IM(t,x) T_t = isaret-genisletilmis dogrudan x     LK(t,x) T_t = sabit havuzu[x]
//   ALU(op,t,t1,t2)  op: ADD SUB MUL AND OR XOR SHL SHR SAR (kaydirma miktari 63 ile maskelenir)
//   CMP(cc,t,t1,t2)  cc: EQ NE LT LE GT GE (isaretli) -> 0/1        UN(op,t,t1) op: NEG NOT
//   DIVS/MODS(t,t1,t2) sifira bolme tuzagi, bolen -1 ozel durum      DIVN/MODN(t,t1,t2) bolen sifir/-1 degil (dogrulayici garantiler)
//   WR(x,t)   yazmac[x] = T_t            LD(t,ta,x) T_t = bellek[T_ta + x]    ST(tv,ta,x) bellek[T_ta + x] = T_tv    (sinir disi -> OOB)
//   BR(x) BRZ(t,x) BRNZ(t,x)  etiket x'e atla (geri atlama = yakit kontrol noktasi)
//   CALLM(f,b) RETM(t) HALTM() HOSTM(h,b) ALLOCM(t,tn)
// YAKIT KURALI: kontrol noktasi = her CALL, her alinan GERI atlama (hedef <= mevcut komut), her HOST. Her biri 1 yakit;
// yakit 0 iken kontrol noktasina gelinirse TRAP FUEL. Komut sayisina bagli degildir.
#pragma once
#include <cstdint>
#include <cstring>
#include "ops.h"

enum AluK : uint8_t { ALUK_ADD, ALUK_SUB, ALUK_MUL, ALUK_AND, ALUK_OR, ALUK_XOR, ALUK_SHL, ALUK_SHR, ALUK_SAR };
enum CmpK : uint8_t { CMPK_EQ, CMPK_NE, CMPK_LT, CMPK_LE, CMPK_GT, CMPK_GE, CMPK_LTU, CMPK_LEU };
enum UnK : uint8_t { UNK_NEG, UNK_NOT };
enum FaluK : uint8_t { FALUK_ADD, FALUK_SUB, FALUK_MUL, FALUK_DIV };
enum FunK : uint8_t { FUNK_NEG, FUNK_SQRT };
enum FcmpK : uint8_t { FCMPK_EQ, FCMPK_LT, FCMPK_LE };
enum MicroKind : uint8_t { MK_END, MK_RD, MK_IM, MK_LK, MK_ALU, MK_CMP, MK_UN, MK_DIVS, MK_MODS, MK_DIVN, MK_MODN, MK_WR,
                           MK_LD, MK_ST, MK_BR, MK_BRZ, MK_BRNZ, MK_CALLM, MK_RETM, MK_HALTM, MK_HOSTM, MK_ALLOCM, MK_FALU, MK_FUN, MK_FCMP, MK_CVTIF, MK_CVTFI,
                           MK_SX, MK_ZX, MK_DIVU, MK_MODU, MK_LIT, MK_U2F, MK_F2U, MK_RNDF, MK_I2FN, MK_BSWAP, MK_W128, MK_WFP, MK_WCX };   // MK_SX.. : Plan Bolum 01 (tamsayi turleri); makine kodu uretecinde (native) DESTEKLENMEZ -> yorumlayici
struct MicroOp { MicroKind k; uint8_t a, b, c, d; };
#define OPD_a 0
#define OPD_b 1
#define OPD_c 2

// ---------------- anlam tanimlari ----------------
#define SEM_MOV(M)  M##_RD(1,b) M##_WR(a,1)
#define SEM_LDI(M)  M##_IM(1,b) M##_WR(a,1)
#define SEM_LDK(M)  M##_LK(1,b) M##_WR(a,1)
#define SEM_ADD(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_WR(a,3)
#define SEM_SUB(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_WR(a,3)
#define SEM_MUL(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_WR(a,3)
#define SEM_AND(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(AND,3,1,2) M##_WR(a,3)
#define SEM_OR(M)   M##_RD(1,b) M##_RD(2,c) M##_ALU(OR,3,1,2) M##_WR(a,3)
#define SEM_XOR(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(XOR,3,1,2) M##_WR(a,3)
#define SEM_SHL(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_WR(a,3)
#define SEM_SHR(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(SHR,3,1,2) M##_WR(a,3)
#define SEM_SAR(M)  M##_RD(1,b) M##_RD(2,c) M##_ALU(SAR,3,1,2) M##_WR(a,3)
#define SEM_EQ(M)   M##_RD(1,b) M##_RD(2,c) M##_CMP(EQ,3,1,2) M##_WR(a,3)
#define SEM_NE(M)   M##_RD(1,b) M##_RD(2,c) M##_CMP(NE,3,1,2) M##_WR(a,3)
#define SEM_LT(M)   M##_RD(1,b) M##_RD(2,c) M##_CMP(LT,3,1,2) M##_WR(a,3)
#define SEM_LE(M)   M##_RD(1,b) M##_RD(2,c) M##_CMP(LE,3,1,2) M##_WR(a,3)
#define SEM_NEG(M)  M##_RD(1,b) M##_UN(NEG,2,1) M##_WR(a,2)
#define SEM_NOT(M)  M##_RD(1,b) M##_UN(NOT,2,1) M##_WR(a,2)
#define SEM_DIV(M)  M##_RD(1,b) M##_RD(2,c) M##_DIVS(3,1,2) M##_WR(a,3)
#define SEM_MOD(M)  M##_RD(1,b) M##_RD(2,c) M##_MODS(3,1,2) M##_WR(a,3)
#define SEM_ADDI(M) M##_RD(1,b) M##_IM(2,c) M##_ALU(ADD,3,1,2) M##_WR(a,3)
#define SEM_MULI(M) M##_RD(1,b) M##_IM(2,c) M##_ALU(MUL,3,1,2) M##_WR(a,3)
#define SEM_ANDI(M) M##_RD(1,b) M##_IM(2,c) M##_ALU(AND,3,1,2) M##_WR(a,3)
#define SEM_SHLI(M) M##_RD(1,b) M##_IM(2,c) M##_ALU(SHL,3,1,2) M##_WR(a,3)
#define SEM_SHRI(M) M##_RD(1,b) M##_IM(2,c) M##_ALU(SHR,3,1,2) M##_WR(a,3)
#define SEM_SARI(M) M##_RD(1,b) M##_IM(2,c) M##_ALU(SAR,3,1,2) M##_WR(a,3)
#define SEM_DIVI(M) M##_RD(1,b) M##_IM(2,c) M##_DIVN(3,1,2) M##_WR(a,3)
#define SEM_MODI(M) M##_RD(1,b) M##_IM(2,c) M##_MODN(3,1,2) M##_WR(a,3)
#define SEM_EQI(M)  M##_RD(1,b) M##_IM(2,c) M##_CMP(EQ,3,1,2) M##_WR(a,3)
#define SEM_NEI(M)  M##_RD(1,b) M##_IM(2,c) M##_CMP(NE,3,1,2) M##_WR(a,3)
#define SEM_LTI(M)  M##_RD(1,b) M##_IM(2,c) M##_CMP(LT,3,1,2) M##_WR(a,3)
#define SEM_LEI(M)  M##_RD(1,b) M##_IM(2,c) M##_CMP(LE,3,1,2) M##_WR(a,3)
#define SEM_GTI(M)  M##_RD(1,b) M##_IM(2,c) M##_CMP(GT,3,1,2) M##_WR(a,3)
#define SEM_GEI(M)  M##_RD(1,b) M##_IM(2,c) M##_CMP(GE,3,1,2) M##_WR(a,3)
#define SEM_FADD(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(ADD,3,1,2) M##_WR(a,3)
#define SEM_FSUB(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(SUB,3,1,2) M##_WR(a,3)
#define SEM_FMUL(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(MUL,3,1,2) M##_WR(a,3)
#define SEM_FDIV(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(DIV,3,1,2) M##_WR(a,3)
#define SEM_FNEG(M)  M##_RD(1,b) M##_FUN(NEG,2,1) M##_WR(a,2)
#define SEM_FSQRT(M) M##_RD(1,b) M##_FUN(SQRT,2,1) M##_WR(a,2)
#define SEM_FEQ(M)  M##_RD(1,b) M##_RD(2,c) M##_FCMP(EQ,3,1,2) M##_WR(a,3)
#define SEM_FLT(M)  M##_RD(1,b) M##_RD(2,c) M##_FCMP(LT,3,1,2) M##_WR(a,3)
#define SEM_FLE(M)  M##_RD(1,b) M##_RD(2,c) M##_FCMP(LE,3,1,2) M##_WR(a,3)
#define SEM_I2F(M)  M##_RD(1,b) M##_CVTIF(2,1) M##_WR(a,2)
#define SEM_F2I(M)  M##_RD(1,b) M##_CVTFI(2,1) M##_WR(a,2)
#define SEM_JMP(M)  M##_BR(a)
#define SEM_JZ(M)   M##_RD(1,a) M##_BRZ(1,b)
#define SEM_JNZ(M)  M##_RD(1,a) M##_BRNZ(1,b)
#define SEM_CALL(M) M##_CALLM(a,b)
#define SEM_RET(M)  M##_RD(1,a) M##_RETM(1)
#define SEM_HALT(M) M##_HALTM()
#define SEM_LD(M)   M##_RD(1,b) M##_LD(2,1,c) M##_WR(a,2)
#define SEM_ST(M)   M##_RD(1,a) M##_RD(2,b) M##_ST(1,2,c)
#define SEM_ALLOC(M) M##_RD(1,b) M##_ALLOCM(2,1) M##_WR(a,2)
#define SEM_HOST(M) M##_HOSTM(a,b)
// Plan Bolum 01: tamsayi turleri (isaret/sifir genisletme M##_SX/ZX, isaretsiz karsilastirma/bolme). Yorumlayicida calisir; native uretecte desteklenmez.
#define SEM_SEXT8(M) M##_RD(1,b) M##_SX(8,2,1) M##_WR(a,2)
#define SEM_SEXT16(M) M##_RD(1,b) M##_SX(16,2,1) M##_WR(a,2)
#define SEM_SEXT32(M) M##_RD(1,b) M##_SX(32,2,1) M##_WR(a,2)
#define SEM_ZEXT8(M) M##_RD(1,b) M##_ZX(8,2,1) M##_WR(a,2)
#define SEM_ZEXT16(M) M##_RD(1,b) M##_ZX(16,2,1) M##_WR(a,2)
#define SEM_ZEXT32(M) M##_RD(1,b) M##_ZX(32,2,1) M##_WR(a,2)
#define SEM_ADD_I8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_SX(8,4,3) M##_WR(a,4)
#define SEM_SUB_I8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_SX(8,4,3) M##_WR(a,4)
#define SEM_MUL_I8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_SX(8,4,3) M##_WR(a,4)
#define SEM_SHL_I8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_SX(8,4,3) M##_WR(a,4)
#define SEM_DIV_I8(M) M##_RD(1,b) M##_RD(2,c) M##_DIVS(3,1,2) M##_SX(8,4,3) M##_WR(a,4)
#define SEM_MOD_I8(M) M##_RD(1,b) M##_RD(2,c) M##_MODS(3,1,2) M##_SX(8,4,3) M##_WR(a,4)
#define SEM_NEG_I8(M) M##_RD(1,b) M##_UN(NEG,2,1) M##_SX(8,3,2) M##_WR(a,3)
#define SEM_ADD_I16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_SX(16,4,3) M##_WR(a,4)
#define SEM_SUB_I16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_SX(16,4,3) M##_WR(a,4)
#define SEM_MUL_I16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_SX(16,4,3) M##_WR(a,4)
#define SEM_SHL_I16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_SX(16,4,3) M##_WR(a,4)
#define SEM_DIV_I16(M) M##_RD(1,b) M##_RD(2,c) M##_DIVS(3,1,2) M##_SX(16,4,3) M##_WR(a,4)
#define SEM_MOD_I16(M) M##_RD(1,b) M##_RD(2,c) M##_MODS(3,1,2) M##_SX(16,4,3) M##_WR(a,4)
#define SEM_NEG_I16(M) M##_RD(1,b) M##_UN(NEG,2,1) M##_SX(16,3,2) M##_WR(a,3)
#define SEM_ADD_I32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_SX(32,4,3) M##_WR(a,4)
#define SEM_SUB_I32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_SX(32,4,3) M##_WR(a,4)
#define SEM_MUL_I32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_SX(32,4,3) M##_WR(a,4)
#define SEM_SHL_I32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_SX(32,4,3) M##_WR(a,4)
#define SEM_DIV_I32(M) M##_RD(1,b) M##_RD(2,c) M##_DIVS(3,1,2) M##_SX(32,4,3) M##_WR(a,4)
#define SEM_MOD_I32(M) M##_RD(1,b) M##_RD(2,c) M##_MODS(3,1,2) M##_SX(32,4,3) M##_WR(a,4)
#define SEM_NEG_I32(M) M##_RD(1,b) M##_UN(NEG,2,1) M##_SX(32,3,2) M##_WR(a,3)
#define SEM_ADD_U8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_ZX(8,4,3) M##_WR(a,4)
#define SEM_SUB_U8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_ZX(8,4,3) M##_WR(a,4)
#define SEM_MUL_U8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_ZX(8,4,3) M##_WR(a,4)
#define SEM_SHL_U8(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_ZX(8,4,3) M##_WR(a,4)
#define SEM_NEG_U8(M) M##_RD(1,b) M##_UN(NEG,2,1) M##_ZX(8,3,2) M##_WR(a,3)
#define SEM_NOT_U8(M) M##_RD(1,b) M##_UN(NOT,2,1) M##_ZX(8,3,2) M##_WR(a,3)
#define SEM_ADD_U16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_ZX(16,4,3) M##_WR(a,4)
#define SEM_SUB_U16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_ZX(16,4,3) M##_WR(a,4)
#define SEM_MUL_U16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_ZX(16,4,3) M##_WR(a,4)
#define SEM_SHL_U16(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_ZX(16,4,3) M##_WR(a,4)
#define SEM_NEG_U16(M) M##_RD(1,b) M##_UN(NEG,2,1) M##_ZX(16,3,2) M##_WR(a,3)
#define SEM_NOT_U16(M) M##_RD(1,b) M##_UN(NOT,2,1) M##_ZX(16,3,2) M##_WR(a,3)
#define SEM_ADD_U32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(ADD,3,1,2) M##_ZX(32,4,3) M##_WR(a,4)
#define SEM_SUB_U32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SUB,3,1,2) M##_ZX(32,4,3) M##_WR(a,4)
#define SEM_MUL_U32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(MUL,3,1,2) M##_ZX(32,4,3) M##_WR(a,4)
#define SEM_SHL_U32(M) M##_RD(1,b) M##_RD(2,c) M##_ALU(SHL,3,1,2) M##_ZX(32,4,3) M##_WR(a,4)
#define SEM_NEG_U32(M) M##_RD(1,b) M##_UN(NEG,2,1) M##_ZX(32,3,2) M##_WR(a,3)
#define SEM_NOT_U32(M) M##_RD(1,b) M##_UN(NOT,2,1) M##_ZX(32,3,2) M##_WR(a,3)
#define SEM_DIVU(M) M##_RD(1,b) M##_RD(2,c) M##_DIVU(3,1,2) M##_WR(a,3)
#define SEM_MODU(M) M##_RD(1,b) M##_RD(2,c) M##_MODU(3,1,2) M##_WR(a,3)
#define SEM_LTU(M) M##_RD(1,b) M##_RD(2,c) M##_CMP(LTU,3,1,2) M##_WR(a,3)
#define SEM_LEU(M) M##_RD(1,b) M##_RD(2,c) M##_CMP(LEU,3,1,2) M##_WR(a,3)
#define SEM_U2F(M) M##_RD(1,b) M##_U2F(2,1) M##_WR(a,2)
#define SEM_F2U(M) M##_RD(1,b) M##_F2U(2,1) M##_WR(a,2)
// Plan Bolum 01: bool / nullptr_t (mantiksal komutlar; yeni mikro-islem yok, yalniz LIT sabiti)
#define SEM_CAST(M) M##_RD(1,b) M##_WR(a,1)
#define SEM_TOBOOL(M) M##_RD(1,b) M##_LIT(2,0) M##_CMP(NE,3,1,2) M##_WR(a,3)
#define SEM_BNOT(M) M##_RD(1,b) M##_LIT(2,0) M##_CMP(EQ,3,1,2) M##_WR(a,3)
#define SEM_BAND(M) M##_RD(1,b) M##_RD(2,c) M##_LIT(3,0) M##_CMP(NE,4,1,3) M##_CMP(NE,3,2,3) M##_ALU(AND,4,4,3) M##_WR(a,4)
#define SEM_BOR(M) M##_RD(1,b) M##_RD(2,c) M##_LIT(3,0) M##_CMP(NE,4,1,3) M##_CMP(NE,3,2,3) M##_ALU(OR,4,4,3) M##_WR(a,4)
#define SEM_BXOR(M) M##_RD(1,b) M##_RD(2,c) M##_LIT(3,0) M##_CMP(NE,4,1,3) M##_CMP(NE,3,2,3) M##_ALU(XOR,4,4,3) M##_WR(a,4)
#define SEM_NULLB(M) M##_RD(1,b) M##_LIT(2,0) M##_WR(a,2)

// Plan Bolum 01: dar kayan nokta (f32/f16/bf16) komutlari; RNDF = bicim izgarasina yuvarla, I2FN = tamsayi -> bicim (fmt 0..2 isaretli, 4..6 isaretsiz)
#define SEM_RND_F32(M) M##_RD(1,b) M##_RNDF(0,2,1) M##_WR(a,2)
#define SEM_ADD_F32(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(ADD,3,1,2) M##_RNDF(0,4,3) M##_WR(a,4)
#define SEM_SUB_F32(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(SUB,3,1,2) M##_RNDF(0,4,3) M##_WR(a,4)
#define SEM_MUL_F32(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(MUL,3,1,2) M##_RNDF(0,4,3) M##_WR(a,4)
#define SEM_DIV_F32(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(DIV,3,1,2) M##_RNDF(0,4,3) M##_WR(a,4)
#define SEM_SQRT_F32(M) M##_RD(1,b) M##_FUN(SQRT,2,1) M##_RNDF(0,3,2) M##_WR(a,3)
#define SEM_I2F_F32(M) M##_RD(1,b) M##_I2FN(0,2,1) M##_WR(a,2)
#define SEM_U2F_F32(M) M##_RD(1,b) M##_I2FN(4,2,1) M##_WR(a,2)
#define SEM_RND_F16(M) M##_RD(1,b) M##_RNDF(1,2,1) M##_WR(a,2)
#define SEM_ADD_F16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(ADD,3,1,2) M##_RNDF(1,4,3) M##_WR(a,4)
#define SEM_SUB_F16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(SUB,3,1,2) M##_RNDF(1,4,3) M##_WR(a,4)
#define SEM_MUL_F16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(MUL,3,1,2) M##_RNDF(1,4,3) M##_WR(a,4)
#define SEM_DIV_F16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(DIV,3,1,2) M##_RNDF(1,4,3) M##_WR(a,4)
#define SEM_SQRT_F16(M) M##_RD(1,b) M##_FUN(SQRT,2,1) M##_RNDF(1,3,2) M##_WR(a,3)
#define SEM_I2F_F16(M) M##_RD(1,b) M##_I2FN(1,2,1) M##_WR(a,2)
#define SEM_U2F_F16(M) M##_RD(1,b) M##_I2FN(5,2,1) M##_WR(a,2)
#define SEM_RND_BF16(M) M##_RD(1,b) M##_RNDF(2,2,1) M##_WR(a,2)
#define SEM_ADD_BF16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(ADD,3,1,2) M##_RNDF(2,4,3) M##_WR(a,4)
#define SEM_SUB_BF16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(SUB,3,1,2) M##_RNDF(2,4,3) M##_WR(a,4)
#define SEM_MUL_BF16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(MUL,3,1,2) M##_RNDF(2,4,3) M##_WR(a,4)
#define SEM_DIV_BF16(M) M##_RD(1,b) M##_RD(2,c) M##_FALU(DIV,3,1,2) M##_RNDF(2,4,3) M##_WR(a,4)
#define SEM_SQRT_BF16(M) M##_RD(1,b) M##_FUN(SQRT,2,1) M##_RNDF(2,3,2) M##_WR(a,3)
#define SEM_I2F_BF16(M) M##_RD(1,b) M##_I2FN(2,2,1) M##_WR(a,2)
#define SEM_U2F_BF16(M) M##_RD(1,b) M##_I2FN(6,2,1) M##_WR(a,2)
#define SEM_F2I_I8(M) M##_RD(1,b) M##_CVTFI(2,1) M##_SX(8,3,2) M##_WR(a,3)
#define SEM_F2I_U8(M) M##_RD(1,b) M##_CVTFI(2,1) M##_ZX(8,3,2) M##_WR(a,3)
#define SEM_F2I_I16(M) M##_RD(1,b) M##_CVTFI(2,1) M##_SX(16,3,2) M##_WR(a,3)
#define SEM_F2I_U16(M) M##_RD(1,b) M##_CVTFI(2,1) M##_ZX(16,3,2) M##_WR(a,3)
#define SEM_F2I_I32(M) M##_RD(1,b) M##_CVTFI(2,1) M##_SX(32,3,2) M##_WR(a,3)
#define SEM_F2I_U32(M) M##_RD(1,b) M##_CVTFI(2,1) M##_ZX(32,3,2) M##_WR(a,3)
#define SEM_FTOBOOL(M) M##_RD(1,b) M##_LIT(2,0) M##_FCMP(EQ,3,1,2) M##_LIT(4,0) M##_CMP(EQ,3,3,4) M##_WR(a,3)

// Plan Bolum 01: bayt sirasi (BSWAP: isaretsiz, sifir genisletilmis)
#define SEM_BSWAP16(M) M##_RD(1,b) M##_BSWAP(16,2,1) M##_WR(a,2)
#define SEM_BSWAP32(M) M##_RD(1,b) M##_BSWAP(32,2,1) M##_WR(a,2)
#define SEM_BSWAP64(M) M##_RD(1,b) M##_BSWAP(64,2,1) M##_WR(a,2)

// Plan Bolum 01: 128 bit komutlari (genis operandlar)
#define SEM_MOV_I128(M) M##_W128(W128_MOV,0,0)
#define SEM_LDI_I128(M) M##_W128(W128_LDI,0,0)
#define SEM_ADD_I128(M) M##_W128(W128_ADD,0,0)
#define SEM_SUB_I128(M) M##_W128(W128_SUB,0,0)
#define SEM_MUL_I128(M) M##_W128(W128_MUL,0,0)
#define SEM_NEG_I128(M) M##_W128(W128_NEG,0,0)
#define SEM_NOT_I128(M) M##_W128(W128_NOT,0,0)
#define SEM_AND_I128(M) M##_W128(W128_AND,0,0)
#define SEM_OR_I128(M) M##_W128(W128_OR,0,0)
#define SEM_XOR_I128(M) M##_W128(W128_XOR,0,0)
#define SEM_SHL_I128(M) M##_W128(W128_SHL,0,0)
#define SEM_MOV_U128(M) M##_W128(W128_MOV,0,0)
#define SEM_LDI_U128(M) M##_W128(W128_LDI,0,0)
#define SEM_ADD_U128(M) M##_W128(W128_ADD,0,0)
#define SEM_SUB_U128(M) M##_W128(W128_SUB,0,0)
#define SEM_MUL_U128(M) M##_W128(W128_MUL,0,0)
#define SEM_NEG_U128(M) M##_W128(W128_NEG,0,0)
#define SEM_NOT_U128(M) M##_W128(W128_NOT,0,0)
#define SEM_AND_U128(M) M##_W128(W128_AND,0,0)
#define SEM_OR_U128(M) M##_W128(W128_OR,0,0)
#define SEM_XOR_U128(M) M##_W128(W128_XOR,0,0)
#define SEM_SHL_U128(M) M##_W128(W128_SHL,0,0)
#define SEM_SHR_U128(M) M##_W128(W128_SHR,0,0)
#define SEM_SAR_I128(M) M##_W128(W128_SAR,0,0)
#define SEM_EQ_128(M) M##_W128(W128_EQ,0,0)
#define SEM_NE_128(M) M##_W128(W128_NE,0,0)
#define SEM_LT_I128(M) M##_W128(W128_LTS,0,0)
#define SEM_LE_I128(M) M##_W128(W128_LES,0,0)
#define SEM_LT_U128(M) M##_W128(W128_LTU,0,0)
#define SEM_LE_U128(M) M##_W128(W128_LEU,0,0)
#define SEM_SEXT64_I128(M) M##_W128(W128_SEXT,0,0)
#define SEM_ZEXT64_I128(M) M##_W128(W128_ZEXT,0,0)
#define SEM_TOBOOL_I128(M) M##_W128(W128_TOBOOL,0,0)
#define SEM_TRUNC_I128_I8(M) M##_W128(W128_TRUNC,8,1)
#define SEM_TRUNC_I128_I16(M) M##_W128(W128_TRUNC,16,1)
#define SEM_TRUNC_I128_I32(M) M##_W128(W128_TRUNC,32,1)
#define SEM_TRUNC_I128_U8(M) M##_W128(W128_TRUNC,8,0)
#define SEM_TRUNC_I128_U16(M) M##_W128(W128_TRUNC,16,0)
#define SEM_TRUNC_I128_U32(M) M##_W128(W128_TRUNC,32,0)
#define SEM_TRUNC_I128_I64(M) M##_W128(W128_TRUNC,64,0)
#define SEM_TRUNC_I128_U64(M) M##_W128(W128_TRUNC,64,0)
#define SEM_SEXT64_U128(M) M##_W128(W128_SEXT,0,0)
#define SEM_ZEXT64_U128(M) M##_W128(W128_ZEXT,0,0)
#define SEM_TOBOOL_U128(M) M##_W128(W128_TOBOOL,0,0)
#define SEM_TRUNC_U128_I8(M) M##_W128(W128_TRUNC,8,1)
#define SEM_TRUNC_U128_I16(M) M##_W128(W128_TRUNC,16,1)
#define SEM_TRUNC_U128_I32(M) M##_W128(W128_TRUNC,32,1)
#define SEM_TRUNC_U128_U8(M) M##_W128(W128_TRUNC,8,0)
#define SEM_TRUNC_U128_U16(M) M##_W128(W128_TRUNC,16,0)
#define SEM_TRUNC_U128_U32(M) M##_W128(W128_TRUNC,32,0)
#define SEM_TRUNC_U128_I64(M) M##_W128(W128_TRUNC,64,0)
#define SEM_TRUNC_U128_U64(M) M##_W128(W128_TRUNC,64,0)
#define SEM_CAST_I128_U128(M) M##_W128(W128_MOV,0,0)
#define SEM_CAST_U128_I128(M) M##_W128(W128_MOV,0,0)
#define SEM_HI64_I128(M) M##_W128(W128_HI,0,0)
#define SEM_PACK_I128(M) M##_W128(W128_PACK,0,0)
#define SEM_HI64_U128(M) M##_W128(W128_HI,0,0)
#define SEM_PACK_U128(M) M##_W128(W128_PACK,0,0)
#define SEM_DIV_I128(M) M##_W128(W128_DIVS,0,0)
#define SEM_MOD_I128(M) M##_W128(W128_MODS,0,0)
#define SEM_DIV_U128(M) M##_W128(W128_DIVU,0,0)
#define SEM_MOD_U128(M) M##_W128(W128_MODU,0,0)

#include "anlam_wfp_gen.h"
#include "anlam_wcx_gen.h"
#include "anlam_wi_gen.h"
#define PM_SEM_OPS(X) PM_SEM_OPS_WFP(X) PM_SEM_OPS_WCX(X) PM_SEM_OPS_WI(X) \
  X(MOV) X(LDI) X(LDK) X(ADD) X(SUB) X(MUL) X(AND) X(OR) X(XOR) X(SHL) X(SHR) X(SAR) X(EQ) X(NE) X(LT) X(LE) \
  X(NEG) X(NOT) X(DIV) X(MOD) X(ADDI) X(MULI) X(ANDI) X(SHLI) X(SHRI) X(SARI) X(DIVI) X(MODI) \
  X(EQI) X(NEI) X(LTI) X(LEI) X(GTI) X(GEI) X(JMP) X(JZ) X(JNZ) X(CALL) X(RET) X(HALT) X(LD) X(ST) X(ALLOC) X(HOST) \
  X(FADD) X(FSUB) X(FMUL) X(FDIV) X(FNEG) X(FSQRT) X(FEQ) X(FLT) X(FLE) X(I2F) X(F2I) \
  X(SEXT8) X(SEXT16) X(SEXT32) X(ZEXT8) X(ZEXT16) X(ZEXT32) X(ADD_I8) X(SUB_I8) X(MUL_I8) X(SHL_I8) X(DIV_I8) X(MOD_I8) X(NEG_I8) X(ADD_I16) X(SUB_I16) X(MUL_I16) X(SHL_I16) X(DIV_I16) X(MOD_I16) X(NEG_I16) X(ADD_I32) X(SUB_I32) X(MUL_I32) X(SHL_I32) X(DIV_I32) X(MOD_I32) X(NEG_I32) X(ADD_U8) X(SUB_U8) X(MUL_U8) X(SHL_U8) X(NEG_U8) X(NOT_U8) X(ADD_U16) X(SUB_U16) X(MUL_U16) X(SHL_U16) X(NEG_U16) X(NOT_U16) X(ADD_U32) X(SUB_U32) X(MUL_U32) X(SHL_U32) X(NEG_U32) X(NOT_U32) X(DIVU) X(MODU) X(LTU) X(LEU) X(U2F) X(F2U) \
  X(TOBOOL) X(BNOT) X(BAND) X(BOR) X(BXOR) X(NULLB) \
  X(RND_F32) X(ADD_F32) X(SUB_F32) X(MUL_F32) X(DIV_F32) X(SQRT_F32) X(I2F_F32) X(U2F_F32) X(RND_F16) X(ADD_F16) X(SUB_F16) X(MUL_F16) X(DIV_F16) X(SQRT_F16) X(I2F_F16) X(U2F_F16) X(RND_BF16) X(ADD_BF16) X(SUB_BF16) X(MUL_BF16) X(DIV_BF16) X(SQRT_BF16) X(I2F_BF16) X(U2F_BF16) X(F2I_I8) X(F2I_U8) X(F2I_I16) X(F2I_U16) X(F2I_I32) X(F2I_U32) X(FTOBOOL) \
  X(BSWAP16) X(BSWAP32) X(BSWAP64) X(CAST) \
  X(MOV_I128) X(LDI_I128) X(ADD_I128) X(SUB_I128) X(MUL_I128) X(NEG_I128) X(NOT_I128) X(AND_I128) X(OR_I128) X(XOR_I128) X(SHL_I128) X(MOV_U128) X(LDI_U128) X(ADD_U128) X(SUB_U128) X(MUL_U128) X(NEG_U128) X(NOT_U128) X(AND_U128) X(OR_U128) X(XOR_U128) X(SHL_U128) X(SHR_U128) X(SAR_I128) X(EQ_128) X(NE_128) X(LT_I128) X(LE_I128) X(LT_U128) X(LE_U128) X(SEXT64_I128) X(ZEXT64_I128) X(TOBOOL_I128) X(TRUNC_I128_I8) X(TRUNC_I128_I16) X(TRUNC_I128_I32) X(TRUNC_I128_U8) X(TRUNC_I128_U16) X(TRUNC_I128_U32) X(TRUNC_I128_I64) X(TRUNC_I128_U64) X(SEXT64_U128) X(ZEXT64_U128) X(TOBOOL_U128) X(TRUNC_U128_I8) X(TRUNC_U128_I16) X(TRUNC_U128_I32) X(TRUNC_U128_U8) X(TRUNC_U128_U16) X(TRUNC_U128_U32) X(TRUNC_U128_I64) X(TRUNC_U128_U64) X(CAST_I128_U128) X(CAST_U128_I128) X(HI64_I128) X(PACK_I128) X(HI64_U128) X(PACK_U128) X(DIV_I128) X(MOD_I128) X(DIV_U128) X(MOD_U128)


// Plan Bolum 01: dar kayan nokta bicimleri f32 / f16 / bf16 (deger double olarak tam saklanir; her sonuc bicimin izgarasina yuvarlanir). Bu C++ gercekleme TAMSAYI algoritmasidir
// (mikro-program sihirli-toplama yontemi ya da derleyici donusumu degil): en yakina, esitlikte cifte; tasma -> sonsuz; NaN -> sessiz NaN (yuk kisaltilir).
#include <cmath>
struct PmFmt { int mb, emin, emax; };
static const PmFmt PM_FMT[3] = {{23, -126, 127}, {10, -14, 15}, {7, -126, 127}};
// m * 2^E degerini (m > 0) izgaraya yuvarla; sonuc buyuklugu (tasmada sonsuz)
static inline double pm_round_core(unsigned __int128 m, int E, const PmFmt& f) {
  int msb = 127; while (msb >= 0 && !((m >> msb) & 1)) msb--;
  int q = msb + E - f.mb; if (q < f.emin - f.mb) q = f.emin - f.mb;
  unsigned __int128 r;
  if (E >= q) r = m << (E - q);
  else { int sh = q - E; if (sh > 127) r = 0; else { r = m >> sh; unsigned __int128 rem = m & ((((unsigned __int128)1) << sh) - 1), half = ((unsigned __int128)1) << (sh - 1);
      if (rem > half || (rem == half && (r & 1))) r++; } }
  if (r == 0) return 0.0;
  double v = std::ldexp((double)(uint64_t)r, q);          // r <= 2^(mb+1): double'da tam
  if (v >= std::ldexp(1.0, f.emax + 1)) return HUGE_VAL;
  return v;
}
static inline double pm_rndfmt(double x, int fmt) {
  const PmFmt& f = PM_FMT[fmt]; uint64_t b; std::memcpy(&b, &x, 8);
  uint64_t sign = b & (1ULL << 63), ab = b ^ sign; int e = int(ab >> 52); uint64_t mant = ab & ((1ULL << 52) - 1);
  auto mk = [&](uint64_t v) { double d; std::memcpy(&d, &v, 8); return d; };
  if (e == 2047) { if (mant == 0) return x; int s = 52 - f.mb; return mk(sign | (2047ULL << 52) | ((mant >> s) << s) | (1ULL << 51)); }
  if (ab == 0) return x;
  uint64_t m = e ? (mant | (1ULL << 52)) : mant; int E = (e ? e : 1) - 1023 - 52;
  double r = pm_round_core(m, E, f); return mk(sign | [&] { uint64_t t; std::memcpy(&t, &r, 8); return t; }());
}
static inline double pm_i2fmt(uint64_t x, bool isSigned, int fmt) {
  bool neg = isSigned && int64_t(x) < 0; uint64_t a = neg ? (0 - x) : x;
  if (a == 0) return 0.0;
  double r = pm_round_core(a, 0, PM_FMT[fmt]); return neg ? -r : r;
}

enum W128K { W128_DIVS, W128_MODS, W128_DIVU, W128_MODU, W128_HI, W128_PACK, W128_MOV, W128_LDI, W128_ADD, W128_SUB, W128_MUL, W128_NEG, W128_NOT, W128_AND, W128_OR, W128_XOR, W128_SHL, W128_SHR, W128_SAR, W128_EQ, W128_NE, W128_LTS, W128_LES, W128_LTU, W128_LEU, W128_SEXT, W128_ZEXT, W128_TOBOOL, W128_TRUNC };
// Plan Bolum 01: 128 bit tamsayilar (genis operand: r, r+1 = dusuk, yuksek sozcuk). C++ gercekleme DERLEYICININ unsigned __int128 turuyle (mikro-program degil): bagimsiz referans.
static inline int pm_w128(int code, int p1, int p2, uint64_t* R, int a, int b, int c) {
  typedef unsigned __int128 U; typedef __int128 S;
  auto L = [&](int r) -> U { return (U(R[r + 1]) << 64) | R[r]; };
  auto W = [&](int r, U v) { R[r] = uint64_t(v); R[r + 1] = uint64_t(v >> 64); };
  switch (code) {
    case W128_DIVS: case W128_MODS: {   // isaretli bolme: buyukluk uzerinden; INT128_MIN / -1 sarar (motor tanimi); sifira bolme -> tuzak
      U y = L(c); if (y == 0) return 1; U x = L(b); bool nx = S(x) < 0, ny = S(y) < 0;
      U ax = nx ? U(0) - x : x, ay = ny ? U(0) - y : y; U q = ax / ay, r = ax % ay;
      if (nx != ny) q = U(0) - q; if (nx) r = U(0) - r;
      W(a, code == W128_DIVS ? q : r); break; }
    case W128_DIVU: case W128_MODU: { U y = L(c); if (y == 0) return 1; U x = L(b); W(a, code == W128_DIVU ? x / y : x % y); break; }
    case W128_HI: R[a] = R[b + 1]; break;
    case W128_PACK: R[a] = R[b]; R[a + 1] = R[c]; break;
    case W128_MOV: W(a, L(b)); break;
    case W128_LDI: W(a, U(S(int64_t(int32_t(b))))); break;
    case W128_ADD: W(a, L(b) + L(c)); break;
    case W128_SUB: W(a, L(b) - L(c)); break;
    case W128_MUL: W(a, L(b) * L(c)); break;
    case W128_NEG: W(a, U(0) - L(b)); break;
    case W128_NOT: W(a, ~L(b)); break;
    case W128_AND: W(a, L(b) & L(c)); break;
    case W128_OR: W(a, L(b) | L(c)); break;
    case W128_XOR: W(a, L(b) ^ L(c)); break;
    case W128_SHL: W(a, L(b) << (R[c] & 127)); break;
    case W128_SHR: W(a, L(b) >> (R[c] & 127)); break;
    case W128_SAR: W(a, U(S(L(b)) >> (R[c] & 127))); break;
    case W128_EQ: R[a] = L(b) == L(c); break;
    case W128_NE: R[a] = L(b) != L(c); break;
    case W128_LTS: R[a] = S(L(b)) < S(L(c)); break;
    case W128_LES: R[a] = S(L(b)) <= S(L(c)); break;
    case W128_LTU: R[a] = L(b) < L(c); break;
    case W128_LEU: R[a] = L(b) <= L(c); break;
    case W128_SEXT: W(a, U(S(int64_t(R[b])))); break;
    case W128_ZEXT: W(a, U(R[b])); break;
    case W128_TOBOOL: R[a] = L(b) != 0; break;
    case W128_TRUNC: {   // p1 = bit (8/16/32/64), p2 = isaretli
      uint64_t v = R[b];
      if (p1 == 8) v = p2 ? uint64_t(int64_t(int8_t(v))) : uint64_t(uint8_t(v));
      else if (p1 == 16) v = p2 ? uint64_t(int64_t(int16_t(v))) : uint64_t(uint16_t(v));
      else if (p1 == 32) v = p2 ? uint64_t(int64_t(int32_t(v))) : uint64_t(uint32_t(v));
      R[a] = v; break; }
  }
  return 0;
}

// ---------------- DATA genisletmesi: makine kodu uretecinin okudugu tablo ----------------
#define DATA_RD(t,x) {MK_RD, t, OPD_##x, 0, 0},
#define DATA_IM(t,x) {MK_IM, t, OPD_##x, 0, 0},
#define DATA_LK(t,x) {MK_LK, t, OPD_##x, 0, 0},
#define DATA_ALU(op,t,t1,t2) {MK_ALU, ALUK_##op, t, t1, t2},
#define DATA_CMP(cc,t,t1,t2) {MK_CMP, CMPK_##cc, t, t1, t2},
#define DATA_UN(op,t,t1) {MK_UN, UNK_##op, t, t1, 0},
#define DATA_DIVS(t,t1,t2) {MK_DIVS, t, t1, t2, 0},
#define DATA_MODS(t,t1,t2) {MK_MODS, t, t1, t2, 0},
#define DATA_DIVN(t,t1,t2) {MK_DIVN, t, t1, t2, 0},
#define DATA_MODN(t,t1,t2) {MK_MODN, t, t1, t2, 0},
#define DATA_WR(x,t) {MK_WR, OPD_##x, t, 0, 0},
#define DATA_LD(t,ta,x) {MK_LD, t, ta, OPD_##x, 0},
#define DATA_ST(tv,ta,x) {MK_ST, tv, ta, OPD_##x, 0},
#define DATA_BR(x) {MK_BR, OPD_##x, 0, 0, 0},
#define DATA_BRZ(t,x) {MK_BRZ, t, OPD_##x, 0, 0},
#define DATA_BRNZ(t,x) {MK_BRNZ, t, OPD_##x, 0, 0},
#define DATA_CALLM(f,b) {MK_CALLM, OPD_##f, OPD_##b, 0, 0},
#define DATA_RETM(t) {MK_RETM, t, 0, 0, 0},
#define DATA_HALTM() {MK_HALTM, 0, 0, 0, 0},
#define DATA_HOSTM(h,b) {MK_HOSTM, OPD_##h, OPD_##b, 0, 0},
#define DATA_ALLOCM(t,tn) {MK_ALLOCM, t, tn, 0, 0},

#include "anlam_wfp.h"
static inline uint64_t pm_f64_to_f32bits(uint64_t b) { double d; std::memcpy(&d, &b, 8); double r = pm_rndfmt(d, 0); uint64_t o; std::memcpy(&o, &r, 8); return o; }
#include "anlam_wcx.h"
#define DATA_FALU(op,t,t1,t2) {MK_FALU, FALUK_##op, t, t1, t2},
#define DATA_FUN(op,t,t1) {MK_FUN, FUNK_##op, t, t1, 0},
#define DATA_FCMP(cc,t,t1,t2) {MK_FCMP, FCMPK_##cc, t, t1, t2},
#define DATA_CVTIF(t,t1) {MK_CVTIF, t, t1, 0, 0},
#define DATA_CVTFI(t,t1) {MK_CVTFI, t, t1, 0, 0},
#define DATA_SX(w,t,t1) {MK_SX, w, t, t1, 0},
#define DATA_ZX(w,t,t1) {MK_ZX, w, t, t1, 0},
#define DATA_DIVU(t,t1,t2) {MK_DIVU, t, t1, t2, 0},
#define DATA_MODU(t,t1,t2) {MK_MODU, t, t1, t2, 0},
#define DATA_U2F(t,t1) {MK_U2F, t, t1, 0, 0},
#define DATA_F2U(t,t1) {MK_F2U, t, t1, 0, 0},
#define DATA_W128(kod,x1,x2) {MK_W128, kod, x1, x2, 0},
#define DATA_WFP(kod,x1,x2) {MK_WFP, kod, x1, x2, 0},
#define DATA_WCX(kod,x1,x2) {MK_WCX, kod, x1, x2, 0},
#define DATA_BSWAP(w,t,t1) {MK_BSWAP, w, t, t1, 0},
#define DATA_LIT(t,v) {MK_LIT, t, v, 0, 0},
#define DATA_RNDF(fmt,t,t1) {MK_RNDF, fmt, t, t1, 0},
#define DATA_I2FN(fmt,t,t1) {MK_I2FN, fmt, t, t1, 0},
#ifdef PM_SEM_TABLES
#define X(N) static const MicroOp sem_##N[] = { SEM_##N(DATA) {MK_END, 0, 0, 0, 0} };
PM_SEM_OPS(X)
#undef X
inline const MicroOp* sem_of(int op) {
  switch (op) {
#define X(N) case OP_##N: return sem_##N;
    PM_SEM_OPS(X)
#undef X
    default: return nullptr;
  }
}
#endif

// ---------------- INTERP genisletmesi: yorumlayicinin islemcileri ----------------
#ifdef PM_SEM_INTERP
#define ALU_ADD(x, y) ((x) + (y))
#define ALU_SUB(x, y) ((x) - (y))
#define ALU_MUL(x, y) ((x) * (y))
#define ALU_AND(x, y) ((x) & (y))
#define ALU_OR(x, y)  ((x) | (y))
#define ALU_XOR(x, y) ((x) ^ (y))
#define ALU_SHL(x, y) ((x) << ((y) & 63))
#define ALU_SHR(x, y) ((x) >> ((y) & 63))
#define ALU_SAR(x, y) (uint64_t(int64_t(x) >> ((y) & 63)))
#define CMP_EQ(x, y) ((x) == (y))
#define CMP_NE(x, y) ((x) != (y))
#define CMP_LT(x, y) (int64_t(x) < int64_t(y))
#define CMP_LE(x, y) (int64_t(x) <= int64_t(y))
#define CMP_GT(x, y) (int64_t(x) > int64_t(y))
#define CMP_GE(x, y) (int64_t(x) >= int64_t(y))
#define CMP_LTU(x, y) (uint64_t(x) < uint64_t(y))
#define CMP_LEU(x, y) (uint64_t(x) <= uint64_t(y))
#define UN_NEG(x) (uint64_t(0) - (x))
#define UN_NOT(x) (~(x))
#define INTERP_RD(t,x)  T##t = R[in.x];
#define INTERP_IM(t,x)  T##t = uint64_t(int64_t(in.x));
#define INTERP_LK(t,x)  T##t = P.consts[in.x];
#define INTERP_ALU(op,t,t1,t2) T##t = ALU_##op(T##t1, T##t2);
#define INTERP_CMP(cc,t,t1,t2) T##t = uint64_t(CMP_##cc(T##t1, T##t2));
#define INTERP_UN(op,t,t1) T##t = UN_##op(T##t1);
#define INTERP_DIVS(t,t1,t2) { int64_t X_ = int64_t(T##t1), Y_ = int64_t(T##t2); if (Y_ == 0) TRAP(T_DIV0); T##t = (Y_ == -1) ? uint64_t(0) - uint64_t(X_) : uint64_t(X_ / Y_); }
#define INTERP_MODS(t,t1,t2) { int64_t X_ = int64_t(T##t1), Y_ = int64_t(T##t2); if (Y_ == 0) TRAP(T_DIV0); T##t = (Y_ == -1) ? uint64_t(0) : uint64_t(X_ % Y_); }
#define INTERP_DIVN(t,t1,t2) T##t = uint64_t(int64_t(T##t1) / int64_t(T##t2));
#define INTERP_MODN(t,t1,t2) T##t = uint64_t(int64_t(T##t1) % int64_t(T##t2));
#define INTERP_WR(x,t)  R[in.x] = T##t;
#define INTERP_LD(t,ta,x)  { uint64_t ad_ = T##ta + uint64_t(int64_t(in.x)); if (ad_ >= mem_words) TRAP(T_OOB); T##t = mem[ad_]; }
#define INTERP_ST(tv,ta,x) { uint64_t ad_ = T##ta + uint64_t(int64_t(in.x)); if (ad_ >= mem_words) TRAP(T_OOB); mem[ad_] = T##tv; }
#define INTERP_BR(x)       { if (in.x <= int32_t(ip - fn->code.data())) CHECKPOINT(); ip = fn->code.data() + in.x; continue; }
#define INTERP_BRZ(t,x)    if (T##t == 0) { if (in.x <= int32_t(ip - fn->code.data())) CHECKPOINT(); ip = fn->code.data() + in.x; continue; }
#define INTERP_BRNZ(t,x)   if (T##t != 0) { if (in.x <= int32_t(ip - fn->code.data())) CHECKPOINT(); ip = fn->code.data() + in.x; continue; }
#define INTERP_CALLM(f,b) { \
    CHECKPOINT(); \
    const Func& g_ = P.funcs[in.f]; uint64_t* nr_ = R + in.b; \
    if (size_t(stack_end - nr_) < g_.nregs) TRAP(T_STACK); \
    if (depth >= frames.size()) TRAP(T_DEPTH); \
    frames[depth++] = Frame{ip + 1, R, cur}; \
    if (!P.zero_free) memset(nr_ + g_.nparams, 0, size_t(g_.nregs - g_.nparams) * 8); \
    R = nr_; cur = uint32_t(in.f); fn = &g_; ip = g_.code.data(); continue; }
#define INTERP_RETM(t) { \
    if (depth == 0) { result = int64_t(T##t); goto done; } \
    R[0] = T##t; const Frame& fr_ = frames[--depth]; \
    ip = fr_.ret_ip; R = fr_.caller_R; cur = fr_.caller_func; fn = &P.funcs[cur]; continue; }
#define INTERP_HALTM() { result = 0; goto done; }
#define INTERP_HOSTM(h,b)  { CHECKPOINT(); host_call(io, in.h, R + in.b); }
#define INTERP_ALLOCM(t,tn) { int64_t n_ = int64_t(T##tn); if (n_ < 0 || uint64_t(n_) > mem_words - heap) TRAP(T_ALLOC); T##t = heap; heap += uint64_t(n_); }
// NaN yayilimi TANIMLI: iki islenenden biri NaN ise sonuc ILK NaN islenenin sessizlestirilmis hali (yoksa ikincisi); NaN yoksa donanim sonucu (gecersiz islemde varsayilan NaN).
// Bu, x86-64 SSE davranisidir; C++ derleyicisi degisme-degisme (a+b -> b+a) yapsa da yorumlayici sonucu degismez (ELF ile bit-bit ayni kalir).
static inline double pm_fnan(double a, double b, double r) {
  if (r == r) return r;
  uint64_t u;
  if (a != a) { std::memcpy(&u, &a, 8); u |= 1ULL << 51; std::memcpy(&r, &u, 8); return r; }
  if (b != b) { std::memcpy(&u, &b, 8); u |= 1ULL << 51; std::memcpy(&r, &u, 8); return r; }
  return r;
}
#define FALU_ADD(x, y) pm_fnan(asF(x), asF(y), asF(x) + asF(y))
#define FALU_SUB(x, y) pm_fnan(asF(x), asF(y), asF(x) - asF(y))
#define FALU_MUL(x, y) pm_fnan(asF(x), asF(y), asF(x) * asF(y))
#define FALU_DIV(x, y) pm_fnan(asF(x), asF(y), asF(x) / asF(y))
#define FUN_NEG(x)  (-asF(x))
#define FUN_SQRT(x) (std::sqrt(asF(x)))
#define FCMP_EQ(x, y) (asF(x) == asF(y))
#define FCMP_LT(x, y) (asF(x) < asF(y))
#define FCMP_LE(x, y) (asF(x) <= asF(y))
#define INTERP_FALU(op,t,t1,t2) T##t = fromF(FALU_##op(T##t1, T##t2));
#define INTERP_FUN(op,t,t1)     T##t = fromF(FUN_##op(T##t1));
#define INTERP_FCMP(cc,t,t1,t2) T##t = uint64_t(FCMP_##cc(T##t1, T##t2));
#define INTERP_CVTIF(t,t1) T##t = fromF(double(int64_t(T##t1)));
#define INTERP_CVTFI(t,t1) T##t = uint64_t(f2i(asF(T##t1)));
#define INTERP_SX(w,t,t1) T##t = uint64_t(int64_t(int##w##_t(T##t1)));
#define INTERP_ZX(w,t,t1) T##t = uint64_t(uint##w##_t(T##t1));
#define INTERP_DIVU(t,t1,t2) { if (T##t2 == 0) TRAP(T_DIV0); T##t = T##t1 / T##t2; }
#define INTERP_MODU(t,t1,t2) { if (T##t2 == 0) TRAP(T_DIV0); T##t = T##t1 % T##t2; }
#define INTERP_U2F(t,t1) T##t = fromF(double(uint64_t(T##t1)));
#define INTERP_F2U(t,t1) T##t = f2u(asF(T##t1));
#define INTERP_W128(kod,x1,x2) { if (pm_w128(kod, x1, x2, R, in.a, in.b, in.c)) TRAP(T_DIV0); }
#define INTERP_WFP(kod,x1,x2) { pm_wfp(kod, x1, x2, R, in.a, in.b, in.c); }
#define INTERP_WCX(kod,x1,x2) { pm_wcx(kod, x1, x2, R, in.a, in.b, in.c); }
#define INTERP_BSWAP(w,t,t1) T##t = ((w) == 16 ? uint64_t(__builtin_bswap16(uint16_t(T##t1))) : (w) == 32 ? uint64_t(__builtin_bswap32(uint32_t(T##t1))) : __builtin_bswap64(T##t1));
#define INTERP_LIT(t,v) T##t = uint64_t(int64_t(v));
#define INTERP_RNDF(fmt,t,t1) T##t = fromF(pm_rndfmt(asF(T##t1), fmt));
#define INTERP_I2FN(fmt,t,t1) T##t = fromF(pm_i2fmt(T##t1, (fmt) < 4, (fmt) & 3));
#endif
