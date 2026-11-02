#!/usr/bin/env python3
"""Sabotaj (test-testi) -- sayisal PMX1 derleyicisi: (a) KURAL TABLOSU sayilari bozulur (yeniden uretim yok), (b) ifx.cx gelistirme gorunumu bozulup ifx.pmb yeniden uretilir.
Her mutantla dogrula_ifx.sh calistirilir; mutant YAKALANMALIDIR. Kullanim: sabotaj_ifx.py"""
import os, sys, subprocess, tempfile, shutil, concurrent.futures
D = os.path.dirname(os.path.abspath(__file__)); R = os.path.abspath(D + '/../../..'); PM = R + '/pm'
TAB = open(R + '/oz/komut/komutlar.pmk').read().split(); SRC = open(D + '/ifx.cx').read()
def sw(old, new):   # tablodaki dizge (uzunluk onekli) degistir; tum olusumlar
    a = [len(old)] + [ord(c) for c in old]; b = [len(new)] + [ord(c) for c in new]; v = [int(x) for x in TAB]; n = 0; i = 0; out = []
    while i < len(v):
        if v[i:i + len(a)] == a: out += b; i += len(a); n += 1
        else: out.append(v[i]); i += 1
    assert n, old; return out
T = [  # (ad, tur, ...) tablo mutantlari
 ('tablo: int sext32->zext32', 'tab', 'sext32', 'zext32'), ('tablo: uchar zext8->sext8', 'tab', 'zext8', 'sext8'), ('tablo: >> sar->shr', 'tab', 'sar', 'shr'),
 ('tablo: div->mod', 'tab', 'div', 'mod'), ('tablo: fadd->fsub', 'tab', 'fadd', 'fsub'), ('tablo: lt->le', 'tab', 'lt', 'le'), ('tablo: ltu->leu', 'tab', 'ltu', 'leu'),
 ('tablo: mul->add', 'tab', 'mul', 'add'), ('tablo: xor->and', 'tab', 'xor', 'and'), ('tablo: shl->shr', 'tab', 'shl', 'shr'), ('tablo: divu->div', 'tab', 'divu', 'div'),
 ('tablo: i2f->u2f', 'tab', 'i2f', 'u2f'), ('tablo: f2i->f2u', 'tab', 'f2i', 'f2u'), ('tablo: neg->not', 'tab', 'neg', 'not'), ('tablo: rnd_f32 -> fneg', 'tab', 'rnd_f32', 'fneg_f32'),
]
S = [  # kaynak mutantlari
 ('u64 bolme isaretsiz secimi', 'if (when == 1 && uns64(t)) p = pu;', 'if (0) p = pu;'), ('isaretsiz kaydirma secimi', 'if (when == 2 && sg_of(t) == 0) p = pu;', 'if (0) p = pu;'),
 ('terfi siniri', 'if (is_int(t) && rank_of(t) < 4)', 'if (is_int(t) && rank_of(t) < 3)'), ('64 bit donusum kisayolu', '      else if (bits_of(to) == 64) need = 0;\n', ''),
 ('sabit isaret genisletme', 'if (sg_of(t)) return (x << sh) >> sh;', 'if (sg_of(t)) return x;'), ('dizi sinir denetimi', 'ins3("ltu", c, i, imm(0, cnt));', 'ins3("leu", c, i, imm(0, cnt));'),
 ('null denetimi', 'void nullchk(V p) {\n  if (dry) return;', 'void nullchk(V p) {\n  if (1) return;'), ('kisa devre', '  if (isand) w("  jz "); else w("  jnz ");', '  if (isand) w("  jnz "); else w("  jz ");'),
 ('sonek artirma', '  if (post) return old;', '  if (post) return nv;'), ('atama sirasi', '  V r = gen(kid(n, 1)); V lv = lval_check(gen(kid(n, 0)), "=");', '  V lv = lval_check(gen(kid(n, 0)), "="); V r = gen(kid(n, 1));'),
 ('isaretsiz karsilastirma takasi', 'if (sw_u) { V x = l; l = r; r = x; }', 'if (0) { V x = l; l = r; r = x; }'), ('isaretci farki', '      if (w != 1) { long long q = newreg(); ins3("div", q, mkv(0, 0, d), imm(0, w)); d = q; }', ''),
 ('lvalue ?:', 'if (a0r.cat == 1 && b0r.cat == 1 && a0r.ty == b0r.ty && !is_arr(a0r.ty)) {', 'if (0) {'), ('kaydirma siniri', 'if (r.cv >= bits_of(lt)) die(', 'if (0) die('),
 ('char sabiti isareti', 'return imm(mkt(Y_CHAR, -1, 0), fold(nk_i[n], mkt(Y_CHAR, -1, 0)));', 'return imm(mkt(Y_CHAR, -1, 0), nk_i[n]);'), ('bool donusumu', "  if (tk[t] == Y_BOOL) { V m = mat(v); long long d = newreg(); ins3(\"ne\", d, m, imm(t, 0)); return mkv(t, 0, d); }", "  if (tk[t] == Y_BOOL) { return v; }"),
 ('uac isaretli/isaretsiz', '  if (rank_of(u) >= rank_of(s)) return u;\n  if (bits_of(s) > bits_of(u)) return s;', '  if (rank_of(u) >= rank_of(s)) return s;\n  if (bits_of(s) > bits_of(u)) return s;'), ('global ilklendirme', '    if (init >= 0) { int base = nreg; long long a = newreg();', '    if (0) { int base = nreg; long long a = newreg();'),
 ('dize verisi sonlandirici', 'w("0\\n"); i = i + 1; }\n  i = 0; while (i < ndat)', 'w("1\\n"); i = i + 1; }\n  i = 0; while (i < ndat)'), ('dizi ilklendirme sifirlama', '    zero_fill(addr, words_of(T));\n    int i = 0; while (i < nk_n[init])', '    int i = 0; while (i < nk_n[init])'),
]
def run(tab, ifxcx):
    w = tempfile.mkdtemp(prefix='sabifx'); pm = w + '/ifx.pmb'
    if ifxcx is not None:
        open(w + '/ifx.cx', 'w').write(ifxcx)
        r = subprocess.run([PM, 'ifade', w + '/ifx.cx', '-o', pm, '--strip'], capture_output=True)
        if r.returncode: shutil.rmtree(w); return 'DERLENEMEDI'
    else: shutil.copy(D + '/ifx.pmb', pm)
    tp = w + '/tab.pmk'; open(tp, 'w').write(' '.join(map(str, tab)))
    t = subprocess.run(['bash', D + '/dogrula_ifx.sh', PM], capture_output=True, text=True, env=dict(os.environ, SABOTAJ='1', IFXB=pm, TAB=tp, SVM='0', LIMS='60'), cwd=D)
    shutil.rmtree(w, ignore_errors=True); return 'YAKALANDI' if t.returncode != 0 else 'HAYATTA'
def job(m):
    if m[1] == 'tab': return (m[0], run(sw(m[2], m[3]), None))
    if SRC.count(m[1]) != 1: return (m[0], 'HATA eski metin %d kez' % SRC.count(m[1]))
    return (m[0], run([int(x) for x in TAB], SRC.replace(m[1], m[2])))
jobs = [m for m in T] + [(a, b, c) for (a, b, c) in S]
with concurrent.futures.ThreadPoolExecutor(max_workers=max(2, os.cpu_count() or 2)) as ex: res = list(ex.map(job, jobs))
bad = [r for r in res if r[1] != 'YAKALANDI']
for n, st in bad: print('%s: %s' % (st, n))
print('ifx sabotaj: %d mutant (%d tablo + %d kaynak), %d yakalandi, %d yakalanmadi' % (len(res), len(T), len(S), len(res) - len(bad), len(bad)))
sys.exit(1 if bad else 0)
