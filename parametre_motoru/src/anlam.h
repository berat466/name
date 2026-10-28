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
#include "ops.h"

enum AluK : uint8_t { ALUK_ADD, ALUK_SUB, ALUK_MUL, ALUK_AND, ALUK_OR, ALUK_XOR, ALUK_SHL, ALUK_SHR, ALUK_SAR };
enum CmpK : uint8_t { CMPK_EQ, CMPK_NE, CMPK_LT, CMPK_LE, CMPK_GT, CMPK_GE };
enum UnK : uint8_t { UNK_NEG, UNK_NOT };
enum MicroKind : uint8_t { MK_END, MK_RD, MK_IM, MK_LK, MK_ALU, MK_CMP, MK_UN, MK_DIVS, MK_MODS, MK_DIVN, MK_MODN, MK_WR,
                           MK_LD, MK_ST, MK_BR, MK_BRZ, MK_BRNZ, MK_CALLM, MK_RETM, MK_HALTM, MK_HOSTM, MK_ALLOCM };
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

#define PM_SEM_OPS(X) \
  X(MOV) X(LDI) X(LDK) X(ADD) X(SUB) X(MUL) X(AND) X(OR) X(XOR) X(SHL) X(SHR) X(SAR) X(EQ) X(NE) X(LT) X(LE) \
  X(NEG) X(NOT) X(DIV) X(MOD) X(ADDI) X(MULI) X(ANDI) X(SHLI) X(SHRI) X(SARI) X(DIVI) X(MODI) \
  X(EQI) X(NEI) X(LTI) X(LEI) X(GTI) X(GEI) X(JMP) X(JZ) X(JNZ) X(CALL) X(RET) X(HALT) X(LD) X(ST) X(ALLOC) X(HOST)

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
#endif
