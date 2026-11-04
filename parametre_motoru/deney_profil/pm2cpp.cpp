// DENEY: Program -> C++ (AOT). Yazmaclar bellekte kalir (R[]), anlam VM ile ayni; yakit/derinlik/yigin denetimi YOK.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>
#include "program.h"
int main(int argc, char** argv) {
  std::ifstream f(argv[1]); std::string txt((std::istreambuf_iterator<char>(f)), {});
  Program p = assemble(txt);
  bool restrictR = argc > 2 && std::string(argv[2]).find("restrict") != std::string::npos; bool fuel = argc > 2 && std::string(argv[2]).find("fuel") != std::string::npos;
  printf("#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n#include <cstring>\n#include <cmath>\n#include <vector>\n");
  printf("typedef uint64_t u64; typedef int64_t i64;\n");
  printf("static inline double F(u64 x){double d; memcpy(&d,&x,8); return d;}\nstatic inline u64 U(double d){u64 x; memcpy(&x,&d,8); return x;}\n");
  printf("static i64 f2i(double d){ if(d!=d) return 0; if(d>=9223372036854775807.0) return INT64_MAX; if(d<=-9223372036854775808.0) return INT64_MIN; return (i64)d; }\n");
  printf("static u64 FUEL = 4000000000ULL;\n#define BURN do { if (__builtin_expect(FUEL == 0, 0)) trap(\"FUEL\"); --FUEL; } while (0)\n"); printf("static u64* MEM; static u64 MEMW=%llu, HEAP; static u64 STACK[1<<20];\n", (unsigned long long)p.mem_words);
  printf("[[noreturn]] static void trap(const char* n){ fflush(stdout); fprintf(stderr,\"TRAP: %%s\\n\",n); exit(2); }\n");
  printf("static const i64 DATA[] = {0");
  for (int64_t d : p.data) printf(",%lldLL", (long long)d);
  printf("};\n");
  printf("static const u64 KONST[] = {0");
  for (uint64_t k : p.consts) printf(",%lluULL", (unsigned long long)k);
  printf("};\n");
  const char* RR = restrictR ? "u64* __restrict R" : "u64* R";
  for (size_t i = 0; i < p.funcs.size(); i++) printf("static void f%zu(%s);\n", i, RR);
  for (size_t fi = 0; fi < p.funcs.size(); fi++) {
    const Func& f = p.funcs[fi];
    std::set<int> targets;
    for (const Ins& in : f.code) { const OpInfo& oi = kOpInfo[in.op]; for (int j = 0; j < oi.nops; j++) if (oi.kinds[j] == 'l') targets.insert(j == 0 ? in.a : j == 1 ? in.b : in.c); }
    printf("static void f%zu(%s) {\n", fi, RR);
    if (fuel) printf("  BURN;\n");
    if (f.nregs > f.nparams) printf("  memset(R+%u, 0, %u);\n", f.nparams, (f.nregs - f.nparams) * 8);
    for (size_t pc = 0; pc < f.code.size(); pc++) {
      const Ins& in = f.code[pc];
      if (targets.count(int(pc))) printf("L%zu:;\n", pc);
      int a = in.a, b = in.b, c = in.c;
      switch (in.op) {
        case OP_MOV: printf("  R[%d]=R[%d];\n", a, b); break;
        case OP_LDI: printf("  R[%d]=(u64)(i64)%d;\n", a, b); break;
        case OP_LDK: printf("  R[%d]=KONST[%d];\n", a, b + 1); break;
        case OP_ADD: printf("  R[%d]=R[%d]+R[%d];\n", a, b, c); break;
        case OP_SUB: printf("  R[%d]=R[%d]-R[%d];\n", a, b, c); break;
        case OP_MUL: printf("  R[%d]=R[%d]*R[%d];\n", a, b, c); break;
        case OP_DIV: printf("  { i64 x=(i64)R[%d], y=(i64)R[%d]; if(y==0) trap(\"DIV0\"); R[%d]=(y==-1)?0-(u64)x:(u64)(x/y); }\n", b, c, a); break;
        case OP_MOD: printf("  { i64 x=(i64)R[%d], y=(i64)R[%d]; if(y==0) trap(\"DIV0\"); R[%d]=(y==-1)?0:(u64)(x%%y); }\n", b, c, a); break;
        case OP_AND: printf("  R[%d]=R[%d]&R[%d];\n", a, b, c); break;
        case OP_OR:  printf("  R[%d]=R[%d]|R[%d];\n", a, b, c); break;
        case OP_XOR: printf("  R[%d]=R[%d]^R[%d];\n", a, b, c); break;
        case OP_SHL: printf("  R[%d]=R[%d]<<(R[%d]&63);\n", a, b, c); break;
        case OP_SHR: printf("  R[%d]=R[%d]>>(R[%d]&63);\n", a, b, c); break;
        case OP_SAR: printf("  R[%d]=(u64)((i64)R[%d]>>(R[%d]&63));\n", a, b, c); break;
        case OP_EQ: printf("  R[%d]=R[%d]==R[%d];\n", a, b, c); break;
        case OP_NE: printf("  R[%d]=R[%d]!=R[%d];\n", a, b, c); break;
        case OP_LT: printf("  R[%d]=(i64)R[%d]<(i64)R[%d];\n", a, b, c); break;
        case OP_LE: printf("  R[%d]=(i64)R[%d]<=(i64)R[%d];\n", a, b, c); break;
        case OP_NEG: printf("  R[%d]=0-R[%d];\n", a, b); break;
        case OP_NOT: printf("  R[%d]=~R[%d];\n", a, b); break;
        case OP_FADD: printf("  R[%d]=U(F(R[%d])+F(R[%d]));\n", a, b, c); break;
        case OP_FSUB: printf("  R[%d]=U(F(R[%d])-F(R[%d]));\n", a, b, c); break;
        case OP_FMUL: printf("  R[%d]=U(F(R[%d])*F(R[%d]));\n", a, b, c); break;
        case OP_FDIV: printf("  R[%d]=U(F(R[%d])/F(R[%d]));\n", a, b, c); break;
        case OP_FNEG: printf("  R[%d]=U(-F(R[%d]));\n", a, b); break;
        case OP_FSQRT: printf("  R[%d]=U(std::sqrt(F(R[%d])));\n", a, b); break;
        case OP_FEQ: printf("  R[%d]=F(R[%d])==F(R[%d]);\n", a, b, c); break;
        case OP_FLT: printf("  R[%d]=F(R[%d])<F(R[%d]);\n", a, b, c); break;
        case OP_FLE: printf("  R[%d]=F(R[%d])<=F(R[%d]);\n", a, b, c); break;
        case OP_I2F: printf("  R[%d]=U((double)(i64)R[%d]);\n", a, b); break;
        case OP_F2I: printf("  R[%d]=(u64)f2i(F(R[%d]));\n", a, b); break;
        case OP_JMP: if (fuel && a <= int(pc)) printf("  BURN;\n"); printf("  goto L%d;\n", a); break;
        case OP_JZ: if (fuel && b <= int(pc)) printf("  if(R[%d]==0){BURN; goto L%d;}\n", a, b); else printf("  if(R[%d]==0) goto L%d;\n", a, b); break;
        case OP_JNZ: if (fuel && b <= int(pc)) printf("  if(R[%d]!=0){BURN; goto L%d;}\n", a, b); else printf("  if(R[%d]!=0) goto L%d;\n", a, b); break;
        case OP_CALL: printf("  f%d(R+%d);\n", a, b); break;
        case OP_RET: printf("  R[0]=R[%d]; return;\n", a); break;
        case OP_HALT: printf("  fflush(stdout); exit(0);\n"); break;
        case OP_LD: printf("  { u64 ad=R[%d]+(u64)(i64)%d; if(ad>=MEMW) trap(\"OOB\"); R[%d]=MEM[ad]; }\n", b, c, a); break;
        case OP_ST: printf("  { u64 ad=R[%d]+(u64)(i64)%d; if(ad>=MEMW) trap(\"OOB\"); MEM[ad]=R[%d]; }\n", b, c, a); break;
        case OP_ALLOC: printf("  { i64 n=(i64)R[%d]; if(n<0||(u64)n>MEMW-HEAP) trap(\"ALLOC\"); R[%d]=HEAP; HEAP+=(u64)n; }\n", b, a); break;
        case OP_HOST:
          if (a == H_PUTC) printf("  putchar((int)(R[%d]&255)); R[%d]=0;\n", b, b);
          else if (a == H_PUTI) printf("  printf(\"%%lld\", (long long)R[%d]); R[%d]=0;\n", b, b);
          else if (a == H_PUTF) printf("  printf(\"%%.15g\", F(R[%d])); R[%d]=0;\n", b, b);
          else printf("  { int ch=getchar(); R[%d]=(u64)(i64)ch; }\n", b);
          break;
      }
    }
    printf("}\n");
  }
  printf("int main(){ MEM=(u64*)calloc(MEMW,8); for(size_t i=1;i<sizeof(DATA)/8;i++) MEM[i]=(u64)DATA[i]; HEAP=1+%zu+%llu;\n", p.data.size(), (unsigned long long)p.bss);
  printf("  f%u(STACK); fflush(stdout); return 0; }\n", p.entry);
}
