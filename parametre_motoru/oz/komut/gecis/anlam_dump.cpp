// TEK SEFERLIK GECIS + TEST ORAKLI: src/anlam.h'deki komut mikro-programlarini (C++ referansi) okunabilir "sem" satirlari olarak dokup
// kanonik sayisal tablodaki SEM bolumuyle karsilastirmak icin. Derleme: g++ -std=c++17 -I src oz/komut/gecis/anlam_dump.cpp
#include <cstdio>
#include <cstdint>
#include <cstring>
#define PM_SEM_TABLES
#include "../../../src/anlam.h"
static const char* ALU[] = {"add", "sub", "mul", "and", "or", "xor", "shl", "shr", "sar"};
static const char* CMP[] = {"eq", "ne", "lt", "le", "gt", "ge"};
static const char* UN[] = {"neg", "not"};
static const char* FALU[] = {"add", "sub", "mul", "div"};
static const char* FUN[] = {"neg", "sqrt"};
static const char* FCMP[] = {"eq", "lt", "le"};
static void opd(int x) { putchar("abc"[x]); }
int main() {
  static const char* names[] = {
#define X(E, N, C, K) #E,
    PM_OPS(X)
#undef X
  };
  for (int op = 0; op < OP_COUNT; op++) {
    const MicroOp* m = sem_of(op);
    if (!m) continue;
    printf("sem %s ", names[op]);
    bool first = true;
    for (; m->k != MK_END; m++) {
      if (!first) printf(" ; "); first = false;
      switch (m->k) {
        case MK_RD: printf("rd t%d ", m->a); opd(m->b); break;
        case MK_IM: printf("im t%d ", m->a); opd(m->b); break;
        case MK_LK: printf("lk t%d ", m->a); opd(m->b); break;
        case MK_ALU: printf("alu_%s t%d t%d t%d", ALU[m->a], m->b, m->c, m->d); break;
        case MK_CMP: printf("cmp_%s t%d t%d t%d", CMP[m->a], m->b, m->c, m->d); break;
        case MK_UN: printf("un_%s t%d t%d", UN[m->a], m->b, m->c); break;
        case MK_DIVS: printf("divs t%d t%d t%d", m->a, m->b, m->c); break;
        case MK_MODS: printf("mods t%d t%d t%d", m->a, m->b, m->c); break;
        case MK_DIVN: printf("divn t%d t%d t%d", m->a, m->b, m->c); break;
        case MK_MODN: printf("modn t%d t%d t%d", m->a, m->b, m->c); break;
        case MK_WR: printf("wr "); opd(m->a); printf(" t%d", m->b); break;
        case MK_LD: printf("ld t%d t%d ", m->a, m->b); opd(m->c); break;
        case MK_ST: printf("st t%d t%d ", m->a, m->b); opd(m->c); break;
        case MK_BR: printf("br "); opd(m->a); break;
        case MK_BRZ: printf("brz t%d ", m->a); opd(m->b); break;
        case MK_BRNZ: printf("brnz t%d ", m->a); opd(m->b); break;
        case MK_CALLM: printf("call "); opd(m->a); putchar(' '); opd(m->b); break;
        case MK_RETM: printf("ret t%d", m->a); break;
        case MK_HALTM: printf("halt"); break;
        case MK_HOSTM: printf("host "); opd(m->a); putchar(' '); opd(m->b); break;
        case MK_ALLOCM: printf("alloc t%d t%d", m->a, m->b); break;
        case MK_FALU: printf("f%s t%d t%d t%d", FALU[m->a], m->b, m->c, m->d); break;
        case MK_FUN: printf("f%s t%d t%d", FUN[m->a], m->b, m->c); break;
        case MK_FCMP: printf("f%s t%d t%d t%d", FCMP[m->a], m->b, m->c, m->d); break;
        case MK_CVTIF: printf("i2f t%d t%d", m->a, m->b); break;
        case MK_CVTFI: printf("f2i t%d t%d", m->a, m->b); break;
        default: printf("BILINMEYEN%d", m->k);
      }
    }
    putchar('\n');
  }
  return 0;
}
