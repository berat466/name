#!/usr/bin/env python3
"""Bolum 02 SABOTAJ (test-testi): src/ifade.cpp ve src/ifade_ops_gen.h icindeki anlamsal kararlari bilerek bozar (mutant), her mutantla dogrula_ifade.sh calistirilir;
mutant YAKALANMALIDIR (en az bir test basarisiz). Hayatta kalan = test bosluğu ya da gerekceli denk mutant (DENK listesi, gerekceyle).
Kullanim: sabotaj_ifade.py [pm_dizini_kok]   (varsayilan: ../..)"""
import sys, os, subprocess, tempfile, shutil, concurrent.futures, re
D = os.path.dirname(os.path.abspath(__file__)); R = os.path.abspath(D + '/../..')
CPP, HDR = 'src/ifade.cpp', 'src/ifade_ops_gen.h'
M = [  # (dosya, eski, yeni, aciklama)
 (CPP, 'E((isAnd ? "jz " : "jnz ") + res', 'E((isAnd ? "jnz " : "jz ") + res', '&&/|| kisa devre dali ters'),
 (CPP, 'if (op == "+") out = int64_t(ux + uy); else if (op == "-")', 'if (op == "+") out = int64_t(ux - uy); else if (op == "-")', 'sabit katlama + yerine -'),
 (CPP, 'case Y_INT: d = R(); E("sext32 " + d + ", " + r); return d;', 'case Y_INT: d = R(); E("zext32 " + d + ", " + r); return d;', 'int normallestirme zext'),
 (CPP, 'bool u = (isUnsigned(t) && bitsOf(t) == 64) || isPtr(t); string d = R();', 'bool u = (isUnsigned(t) && bitsOf(t) == 64); string d = R();', 'isaretci karsilastirmasi isaretli'),
 (CPP, 'if (ru >= rs) return u;', 'if (ru > rs) return u;', 'olagan aritmetik donusum: esit derece isaretsiz'),
 (CPP, 'if (k >= Y_BOOL && k <= Y_USHORT) return tInt; return t;', 'if (k >= Y_BOOL && k < Y_USHORT) return tInt; return t;', 'tamsayi terfisi ushort disarida'),
 (CPP, 'E(string(isSigned(t) ? "sar " : "shr ")', 'E(string(isSigned(t) ? "shr " : "sar ")', 'sag kaydirma isaret'),
 (CPP, 'if (post) { res = old; res.cat = C_PR;', 'if (post) { res = nv; res.cat = C_PR;', 'sonek ++ yeni degeri dondurur'),
 (CPP, '    V Rv = gen(x.kid[1]); V Lv = gen(x.kid[0]);\n    if (op == "=") {', '    V Lv = gen(x.kid[0]); V Rv = gen(x.kid[1]);\n    if (op == "=") {', 'atama: sol operand once'),
 (CPP, 'E("jnz " + c + ", " + ok); trapNow(); LAB(ok);\n      V o = v; o.ty = tgtT;', 'E("jz " + c + ", " + ok); trapNow(); LAB(ok);\n      V o = v; o.ty = tgtT;', 'dynamic_cast<T&> basarisizlik kosulu ters'),
 (CPP, 'int64_t fa = alignOf(f.ty); off = (off + fa - 1) / fa * fa;', 'int64_t fa = alignOf(f.ty); off = off;', 'ABI dizilimi: uye hizalamasi'),
 (CPP, 'c.size = std::max<int64_t>(1, (off + al - 1) / al * al);', 'c.size = std::max<int64_t>(1, off);', 'ABI dizilimi: kuyruk dolgusu'),
 (CPP, 'off = 8; al = 8; w = 1;', 'off = 4; al = 8; w = 1;', 'vptr boyutu'),
 (CPP, 'if (!f.noex) g->mayThrow = true;', 'if (false) g->mayThrow = true;', 'noexcept: cagri analizi'),
 (CPP, 'if (op == "<") E("eq " + d + ", " + v + ", -1");', 'if (op == "<") E("eq " + d + ", " + v + ", 0");', 'siralama < 0'),
 (CPP, 'E("add " + res + ", " + r0 + ", " + u2);', 'E("add " + res + ", " + r0 + ", " + r0);', '<=> unordered kodu'),
 (CPP, 'if (allowReversed && ops.size() == 2) {', 'if (false && allowReversed && ops.size() == 2) {', 'ters cevrilmis operator adayi'),
 (CPP, 'if ((ty(fe).c && !ty(te).c) || (ty(fe).v && !ty(te).v)) return true;', 'if (false) return true;', 'const kaldirma denetimi'),
 (CPP, 'if (ty(v.ty).c) err("E_CONST", what + ": const nesneye atama");', 'if (false) err("E_CONST", what + ": const nesneye atama");', 'const atama reddi'),
 (CPP, 'E("ltu " + c + ", " + i.reg + ", " + num(cnt)); E("jnz " + c + ", " + ok);', 'E("leu " + c + ", " + i.reg + ", " + num(cnt)); E("jnz " + c + ", " + ok);', 'dizi sinir denetimi bir fazla'),
 (CPP, "if (base == 10 && digits.size() > 1 && digits[0] == '0') { base = 8; oct = true; }", 'if (false) { base = 8; oct = true; }', 'sekizlik sabit'),
 (CPP, 'b.s += t.s; if (b.udl.empty()) b.udl = t.udl; return;', 'out.push_back(t); return;', 'bitisik dize birlestirme'),
 (CPP, 'if (a.cat == C_LV && rr) err("E_BIND", "rvalue referansi lvalue\'ya baglanamaz");', 'if (false) err("E_BIND", "rvalue referansi lvalue\'ya baglanamaz");', 'rvalue ref lvalue baglama reddi'),
 (CPP, 'bool narrow = false;\n    if (isFlt(f) && isInt(to)) narrow = true;', 'bool narrow = false;\n    if (isFlt(f) && isInt(to)) narrow = false;', 'daraltici donusum reddi'),
 (CPP, 'if (r.cval >= bitsOf(t)) err("E_SHIFT"', 'if (false) err("E_SHIFT"', 'asiri kaydirma reddi'),
 (CPP, 'r.cat = o.cat == C_LV ? C_LV : C_XV;', 'r.cat = C_LV;', 'prvalue nesnenin ueyesi xvalue'),
 (CPP, 'r.cval = c.cval ? a1.cval : b1.cval; }', 'r.cval = c.cval ? b1.cval : a1.cval; }', '?: sabit katlama'),
 (CPP, 'if (unq(v.ty) != tu && !(isCls(unq(v.ty))', 'if (false && unq(v.ty) != tu && !(isCls(unq(v.ty))', 'referans donus turu denetimi'),
 (CPP, 'bool lv = A0.cat == C_LV && B0.cat == C_LV && ty(A0.ty).c == ty(B0.ty).c;', 'bool lv = false;', '?: sinif lvalue'),
 (CPP, 'if (isCls(t)) return C[size_t(ty(t).cls)].id; return 1000 + t;', 'if (isCls(t)) return C[size_t(ty(t).cls)].id; return 1000;', 'typeid: tur kimligi'),
 (CPP, 'E("call " + c + ", isa" + num(dc) + ", " + id); E("jz " + c + ", " + end);', 'E("call " + c + ", isa" + num(dc) + ", " + id); E("jnz " + c + ", " + end);', 'dynamic_cast<T*> kosulu ters'),
 (CPP, 'for (auto& e : xe) for (auto& f : ye) if (!e.key.empty() && e.key == f.key && (e.rw == 2 || f.rw == 2))', 'for (auto& e : xe) for (auto& f : ye) if (false && !e.key.empty() && e.key == f.key && (e.rw == 2 || f.rw == 2))', 'siralanmamis degistirme denetimi'),
 (CPP, 'if (v.ci) { r.reg = num(v.cval); r.ci = true;', 'if (false && v.ci) { r.reg = num(v.cval); r.ci = true;', 'sabit yayilimi'),
 (CPP, 'if (fs == ts && tb >= fb) needs = false;', 'if (tb >= fb) needs = false;', 'isaretli->isaretsiz genisleme'),
 (CPP, 'if (fk == Y_NULLPTR) return 2;\n      if (v && isInt(from) && v->ci && v->cval == 0 && v->nullLit) return 2;', 'if (fk == Y_NULLPTR) return 2;\n      if (v && isInt(from) && v->ci) return 2;', 'null isaretci sabiti: yalniz 0 degeri'),
 (CPP, 'if (tk == Y_INT && fk >= Y_BOOL && fk <= Y_USHORT) return 1;', 'if (tk == Y_INT && fk >= Y_BOOL && fk <= Y_USHORT) return 2;', 'terfi sirasi'),
 (CPP, 'if (a1 <= b1) ', 'if (a1 <= b1) ', 'sabit'),
 (CPP, 'string off = i.reg; if (w != 1) { off = R(); E("mul " + off + ", " + i.reg + ", " + num(w)); }', 'string off = i.reg;', 'dizi indeksi eleman genisligi'),
 (CPP, 'if (w != 1) { string q = R(); E("div " + q + ", " + d + ", " + num(w)); d = q; }', 'if (false) { string q = R(); E("div " + q + ", " + d + ", " + num(w)); d = q; }', 'isaretci farki eleman genisligi'),
 (CPP, 'if (!rr) {\n          if (!tc && !lv) return rk;', 'if (!rr) {\n          if (false) return rk;', 'T& rvalue baglama'),
 (CPP, 'if (isCls(fe) && isCls(te) && derivedFrom(ty(fe).cls, ty(te).cls) && (!a1.c || b1.c) && (!a1.v || b1.v)) return 2;', 'if (isCls(fe) && isCls(te) && (!a1.c || b1.c) && (!a1.v || b1.v)) return 2;', 'turemis->taban isaretci yonu'),
 (CPP, 'if (e == "=") ', 'if (e == "=") ', 'sabit'),
 (CPP, 'if (c.ci && a1.ci && b1.ci)', 'if (false && c.ci && a1.ci && b1.ci)', '?: sabit'),
 (CPP, 'case Y_LDOUBLE: return 16; default: return sizeOf(t); }', 'case Y_LDOUBLE: return 8; default: return sizeOf(t); }', 'alignof long double'),
 (CPP, 'case Y_LDOUBLE: return 16; case Y_ARR:', 'case Y_LDOUBLE: return 8; case Y_ARR:', 'sizeof long double'),
 (HDR, 'X(1, 12, 0, "+")', 'X(1, 13, 0, "+")', 'oncelik: + * ile ayni'),
 (HDR, 'X(1, 11, 0, "<<")', 'X(1, 9, 0, "<<")', 'oncelik: << iliskisel ile ayni'),
 (HDR, 'X(1, 8, 0, "==")', 'X(1, 9, 0, "==")', 'oncelik: == < ile ayni'),
 (HDR, 'X(1, 7, 0, "&")', 'X(1, 8, 0, "&")', 'oncelik: & == ile ayni'),
 (HDR, 'X(1, 5, 0, "|")', 'X(1, 6, 0, "|")', 'oncelik: | ^ ile ayni'),
 (HDR, 'X(1, 3, 0, "||")', 'X(1, 4, 0, "||")', 'oncelik: || && ile ayni'),
 (HDR, 'X(1, 10, 0, "<=>")', 'X(1, 9, 0, "<=>")', 'oncelik: <=> iliskisel ile ayni'),
]
# gerekceli denk mutantlar (davranisi degistirmeyen): acıklama anahtari -> neden
DENK = {'sabit': 'metni degistirmeyen yer tutucu'}
M = [m for m in M if m[3] != 'sabit']
T = tempfile.mkdtemp(prefix='sabifade'); S = T + '/src'; shutil.copytree(R + '/src', S)
names = ['main', 'asm', 'ikili', 'vm', 'dump', 'native_x64', 'sablon', 'yapisal', 'liste']
def cc(args): return subprocess.run(['g++', '-std=c++17', '-O1', '-w', '-fno-fast-math', '-ffp-contract=off'] + args, capture_output=True, text=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
    rs = list(ex.map(lambda n: cc(['-c', S + '/%s.cpp' % n, '-o', T + '/%s.o' % n]), names))
if any(r.returncode for r in rs): print('temel nesneler derlenemedi'); sys.exit(2)
def mutant(i):
    f, old, new, ac = M[i]; w = T + '/m%d' % i; os.makedirs(w)
    src = open(R + '/' + f).read()
    if src.count(old) != 1: return (i, 'HATA', 'eski metin %d kez' % src.count(old))
    shutil.copytree(S, w + '/src'); open(w + '/' + f, 'w').write(src.replace(old, new))
    r = cc(['-I', w + '/src', '-c', w + '/src/ifade.cpp', '-o', w + '/ifade.o'])
    if r.returncode: return (i, 'DERLENEMEDI', r.stderr[:200])
    l = subprocess.run(['g++', '-o', w + '/pm'] + [T + '/%s.o' % n for n in names] + [w + '/ifade.o', '-lquadmath'], capture_output=True, text=True)
    if l.returncode: return (i, 'DERLENEMEDI', l.stderr[:200])
    env = dict(os.environ, SVM='0', LIMS='60')
    t = subprocess.run(['bash', D + '/dogrula_ifade.sh', w + '/pm', '0'], capture_output=True, text=True, env=env, cwd=D)
    shutil.rmtree(w, ignore_errors=True)
    return (i, 'YAKALANDI' if t.returncode != 0 else 'HAYATTA', '')
with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, (os.cpu_count() or 2) // 2)) as ex: res = list(ex.map(mutant, range(len(M))))
shutil.rmtree(T, ignore_errors=True)
cau = sum(1 for r in res if r[1] == 'YAKALANDI'); bad = [r for r in res if r[1] != 'YAKALANDI']
for i, st, msg in bad: print('%s: %s -- %s %s' % (st, M[i][3], M[i][1][:70].replace('\n', ' '), msg))
print('ifade sabotaj: %d mutant, %d yakalandi, %d yakalanmadi' % (len(M), cau, len(bad)))
sys.exit(1 if bad else 0)
