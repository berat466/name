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
 ('tablo: f128 add->sub', 'tab', 'add_f128', 'sub_f128'), ('tablo: f128 cv_i64->cv_u64', 'tab', 'cv_i64_f128', 'cv_u64_f128'), ('tablo: f128 lt->le', 'tab', 'lt_f128', 'le_f128'), ('tablo: f128 cv_f128_f64->cv_f128_f32', 'tab', 'cv_f128_f64', 'cv_f128_f32'),
 ('tablo: int sext32->zext32', 'tab', 'sext32', 'zext32'), ('tablo: uchar zext8->sext8', 'tab', 'zext8', 'sext8'), ('tablo: >> sar->shr', 'tab', 'sar', 'shr'),
 ('tablo: div->mod', 'tab', 'div', 'mod'), ('tablo: fadd->fsub', 'tab', 'fadd', 'fsub'), ('tablo: lt->le', 'tab', 'lt', 'le'), ('tablo: ltu->leu', 'tab', 'ltu', 'leu'),
 ('tablo: mul->add', 'tab', 'mul', 'add'), ('tablo: xor->and', 'tab', 'xor', 'and'), ('tablo: shl->shr', 'tab', 'shl', 'shr'), ('tablo: divu->div', 'tab', 'divu', 'div'),
 ('tablo: i2f->u2f', 'tab', 'i2f', 'u2f'), ('tablo: f2i->f2u', 'tab', 'f2i', 'f2u'), ('tablo: neg->not', 'tab', 'neg', 'not'), ('tablo: rnd_f32 -> fneg', 'tab', 'rnd_f32', 'fneg_f32'),
]
S = [  # kaynak mutantlari (her satir bir mutant)
 ('u64 bolme isaretsiz secimi', 'if (when == 1 && uns64(t)) p = pu;', 'if (0) p = pu;'),
 ('isaretsiz kaydirma secimi', 'if (when == 2 && sg_of(t) == 0) p = pu;', 'if (0) p = pu;'),
 ('terfi siniri', 'if (is_int(t) && rank_of(t) < 4)', 'if (is_int(t) && rank_of(t) < 3)'),
 ('64 bit donusum kisayolu', '      else if (bits_of(to) == 64) need = 0;\n', ''),
 ('sabit isaret genisletme', 'if (sg_of(t)) return (x << sh) >> sh;', 'if (sg_of(t)) return x;'),
 ('dizi sinir denetimi', 'ins3("ltu", c, i, imm(0, cnt));', 'ins3("leu", c, i, imm(0, cnt));'),
 ('null denetimi', 'e_jcc("jnz", m, ok); trapnow(); wl(ok); w(":\\n");\n}\n', 'e_jcc("jnz", m, ok); wl(ok); w(":\\n");\n}\n'),
 ('kisa devre', 'e_jcc(isand ? "jz" : "jnz", mkv(0, 0, res), end);', 'e_jcc(isand ? "jnz" : "jz", mkv(0, 0, res), end);'),
 ('sonek artirma', '  if (post) { res = old; res.cat = 0;', '  if (0) { res = old; res.cat = 0;'),
 ('atama sirasi', '  V R = gen(kid(n, 1)); V Lv = gen(kid(n, 0));', '  V Lv = gen(kid(n, 0)); V R = gen(kid(n, 1));'),
 ('isaretsiz karsilastirma takasi', 'if (sw_u) { V x = l; l = r; r = x; }', 'if (0) { V x = l; l = r; r = x; }'),
 ('isaretci farki', 'if (wd != 1) { long long q = newreg(); ins3("div", q, mkv(0, 0, d), imm(0, wd)); d = q; }', ''),
 ('lvalue ?:', '  if (a0.cat == 1 && b0.cat == 1 && at == bt && is_arr(at) == 0', '  if (0 && a0.cat == 1 && b0.cat == 1 && at == bt && is_arr(at) == 0'),
 ('kaydirma siniri', 'if (r.cv >= bits_of(lt)) die(', 'if (0) die('),
 ('char sabiti isareti', 'return imm(mkt(Y_CHAR, 0 - 1, 0), fold(nk_i[n], mkt(Y_CHAR, 0 - 1, 0))); }', 'return imm(mkt(Y_CHAR, 0 - 1, 0), nk_i[n]); }'),
 ('bool donusumu', '  if (tk[t] == Y_BOOL) { V m = mat(v); long long d = newreg(); ins3("ne", d, m, imm(t, 0)); return mkv(t, 0, d); }', '  if (tk[t] == Y_BOOL) { return v; }'),
 ('uac isaretli/isaretsiz', '  if (rank_of(u) >= rank_of(s)) return u;\n  if (bits_of(s) > bits_of(u)) return s;', '  if (rank_of(u) >= rank_of(s)) return s;\n  if (bits_of(s) > bits_of(u)) return s;'),
 ('global ilklendirme', 'if (nk_n[init] == 0) { zero_n(addr, words_of(T)); return; }', 'if (nk_n[init] == 0) { return; }'),
 ('dize verisi sonlandirici', 'j = j + 1; } w("0\\n"); i = i + 1;', 'j = j + 1; } w("1\\n"); i = i + 1;'),
 ('istisna: cagri sonrasi denetim', 'if (exc_on && fn_has(fi, F_NOEX) == 0) exc_check();', 'if (0) exc_check();'),
 ('istisna: catch eslesmesi', 'note_win(1); e_jcc("jnz", mkv(0, 0, r), hl[i]);', 'note_win(1); e_jcc("jz", mkv(0, 0, r), hl[i]);'),
 ('istisna: skaler tur anahtari', 'long long kv = e_movi(0, type_id_of(Tb)); long long cb', 'long long kv = e_movi(0, type_id_of(Tb) + 1); long long cb'),
 ('istisna: noexcept terminate', 'if (cur_fn < 0 || fn_has(cur_fn, F_NOEX) || is_main_fn(cur_fn)) { trapnow(); return; }', 'if (cur_fn < 0 || is_main_fn(cur_fn)) { trapnow(); return; }'),
 ('istisna: main terminate', 'if (cur_fn < 0 || fn_has(cur_fn, F_NOEX) || is_main_fn(cur_fn)) { trapnow(); return; }', 'if (cur_fn < 0 || fn_has(cur_fn, F_NOEX)) { trapnow(); return; }'),
 ('istisna: handler girisinde __ex temizleme', '{ V ea = cell_addr("__ex"); e_sti(0, ea, 0); }', ''),
 ('istisna: bad_cast', 'throw_named("std::bad_cast"); wl(ok)', 'trapnow(); wl(ok)'),
 ('istisna: bad_typeid', 'if (n == tid_node && exc_on) nullchk_exc', 'if (0) nullchk_exc'),
 ('istisna: yeniden firlatma denetimi', 'V c = cell_ld("__cx", 0); int ok = newlab(); e_jcc("jnz", c, ok); trapnow();', 'V c = cell_ld("__cx", 0); int ok = newlab(); e_jcc("jz", c, ok); trapnow();'),
 ('istisna: break islenen-istisna geri yukleme', 'die("E_STMT", "\'break\' dongu disinda"); hs_restore_to(loop_hs[nloop - 1]);', 'die("E_STMT", "\'break\' dongu disinda");'),
 ('istisna: return islenen-istisna geri yukleme', 'void emit_ret(V v) {\n  hs_restore_to(0);\n', 'void emit_ret(V v) {\n'),
 ('istisna: catch(...) son olmali', 'if (nk_k[kid(c, 0)] == N_NONE) { if (i != nh - 1) die(', 'if (nk_k[kid(c, 0)] == N_NONE) { if (0) die('),
 ('istisna: eslesmeyen iletimi', '  if (all == 0) emit_propagate();', '  if (all == 0) { }'),
 ('istisna: sinif kopyasi', 'else if (is_cls(T)) { if (cl_bi[tcl[T]]) copy_n(hv, v, 0, 1); else copy_obj(hv, v, tcl[T], 1); }\n  else { V sv = rval(v);', 'else if (is_cls(T)) { }\n  else { V sv = rval(v);'),
 ('bellek gorunumu: normalize okuma', 'if (pun_on && is_int(r.ty) && tk[r.ty] != Y_BOOL', 'if (0 && is_int(r.ty) && tk[r.ty] != Y_BOOL'),
 ('bellek gorunumu: boyut uyumu', 'size_of(a) == size_of(b)) return 1;', '1) return 1;'),
 ('bellek gorunumu: referans uyumu', 'if (view_compat(vt, tgt) == 0) die(', 'if (0) die('),
 ('tip notu: cagri penceresi', 'if (win_max < 1) win_max = 1; int q = 0; while (q < win_max)', 'if (win_max < 1) win_max = 1; int q = 0; while (q < win_max - 1)'),
 ('tip notu: bildirim turu', 'pm_add(1); pm_add(1); pm_add(1); pm_add(1); pm_add(1); pm_add(3); pm_add(0); pm_add(0);', 'pm_add(1); pm_add(1); pm_add(1); pm_add(1); pm_add(1); pm_add(1); pm_add(0); pm_add(0);'),
 ('bellek: donus reddi', 'if (pb_.bp) die(', 'if (0) die('),
 ('bellek: atama reddi', 'else die("E_REINTERP", "bayt gorunumu yok: uyumsuz gosterimli isaretci yalniz yerel degiskene atanabilir"); }', 'else { } }'),
 ('oz-barindirma: operator adi onegi', 'while (k < 8) { if (chars[s_off[nm] + k] != o[k]) return 0; k = k + 1; }', 'while (k < 8) { k = k + 1; }'),
 ('oz-barindirma: dize tablosu siniri', 'int str_node[8000];', 'int str_node[600];'),
 ('bayt: okuma kaydirma', 'long long t = newreg(); ins3("sar", t, mkv(0, 0, w), mkv(0, 0, v.bkr));', 'long long t = newreg(); ins3("sar", t, mkv(0, 0, w), imm(0, 0));'),
 ('bayt: okuma maskesi', 'long long m = newreg(); ins3("and", m, mkv(0, 0, t), imm(0, 255));', 'long long m = newreg(); ins3("and", m, mkv(0, 0, t), imm(0, 65535));'),
 ('bayt: bayt indeksi', 'ins3("mod", ky, mkv(0, 0, p2), imm(0, S));', 'ins3("mod", ky, mkv(0, 0, p2), imm(0, 2));'),
 ('bayt: yazma maskesi', 'long long inv = newreg(); ins3("xor", inv, mkv(0, 0, msk), imm(0, 0 - 1));', 'long long inv = newreg(); ins3("xor", inv, mkv(0, 0, msk), imm(0, 0));'),
 ('bayt: geri donus bolme', 'ins3("div", q, cast_k(mat(r), 0), imm(0, r.bp))', 'ins3("add", q, cast_k(mat(r), 0), imm(0, 0))'),
 ('bayt: taint boyutu', 'bk.ty = T; bk.bp = S; return bk;', 'bk.ty = T; bk.bp = 8; return bk;'),
 ('bayt: referans reddi', '  if (is_ref(P)) no_bytelv(a);\n', ''),
 ('bayt: bilesik atama reddi', '  need_modifiable(Lv); no_bytelv(Lv);\n', '  need_modifiable(Lv);\n'),
 ('bayt: adres alma reddi', '    no_bytelv(v);\n', ''),
 ('bellek gorunumu: dolayli erisim (bayt)', 'if (v.bp) return byte_lv(v, imm(mkt(Y_INT, 0 - 1, 0), 0), te[v.ty]);', 'if (0) return byte_lv(v, imm(mkt(Y_INT, 0 - 1, 0), 0), te[v.ty]);'),
 ('bellek gorunumu: indeks (bayt)', 'if (bp.bp) return byte_lv(bp, i, et);', 'if (0) return byte_lv(bp, i, et);'),
 ('bellek gorunumu: aritmetikte leke korunumu', 'V rr = mkv(p.ty, 0, d); rr.bp = p.bp; return rr;', 'V rr = mkv(p.ty, 0, d); rr.bp = 0; return rr;'),
 ('bellek gorunumu: degisken lekesi', 'tnt = pt_.bp;', 'tnt = 0;'),
 ('bellek: arguman reddi', 'if (a.bp) die("E_REINTERP", "bayt adresli isaretci islev argumani olamaz (calisan islev sozcuk adresli isaretci bekler)");', ''),
 ('f128: ad kaymasi', 'long long nmw(int b80, int f) { return names[b80 + f * 20]; }', 'long long nmw(int b80, int f) { return names[b80 + f * 19]; }'),
 ('f128: cift yazmac PMT1 turu', 'pm_add(pf ? 13 : 11); pm_add(12);', 'pm_add(11); pm_add(12);'),
 ('f128: ortak tur', 'int f = wfmt(l.ty) || wfmt(r.ty); l = to_ld(l, f);', 'int f = wfmt(l.ty); l = to_ld(l, f);'),
 ('f128: uac', '  if (tk[x] == 24 || tk[y] == 24) return mkt(24, 0 - 1, 0);\n', ''),
 ('f128: daraltma sirasi', 'else if (ff && tf) narrow = frank(to) < frank(f);', 'else if (ff && tf) narrow = 0;'),
 ('f128: f80<->f128 donusumu', 'ins2t(names[tf ? NM_CV80_128 : NM_CV128_80], d, pv);', 'ins2t(names[tf ? NM_CV128_80 : NM_CV80_128], d, pv);'),
 ('f128: cift yazmac turu', 'int pkind(long long reg) { if (pair_fmt[(reg - 100000) / 2]) return 13; return 11; }', 'int pkind(long long reg) { return 11; }'),
 ('f128: bit-tam sabit', 'if (is_ld(t)) return ld_const(nk_i[n], nk_d[n], wfmt(t));', 'if (is_ld(t)) return ld_const(nk_i[n], nk_d[n], 0);'),
 ('f128: yazdirma bicimi', 'int pf = fn_bi[fi] == 6;', 'int pf = 0;'),
 ('b03 cj: && sol operand', 'ma = cj(kid(n, 0), mid, fl, mid); place(mid);', 'ma = cj(kid(n, 0), fl, mid, mid); place(mid);'),
 ('b03 cj: || sol operand', 'ma = cj(kid(n, 0), tl, mid, mid); place(mid);', 'ma = cj(kid(n, 0), mid, tl, mid); place(mid);'),
 ('b03 cj: !', 'int m = cj(kid(n, 0), fl, tl, nxt);', 'int m = cj(kid(n, 0), tl, fl, nxt);'),
 ('b03 cj: sabit dogru', 'if (c.cv != 0) { if (nxt != tl) jmpl(tl); return 1; }', 'if (c.cv == 0) { if (nxt != tl) jmpl(tl); return 1; }'),
 ('b03 switch: aralik ust siniri', 'V t2 = binop("<=", cc, imm(T, sw_hi[e]));', 'V t2 = binop("<", cc, imm(T, sw_hi[e]));'),
 ('b03 switch: cakisma', 'if (sw_lo[sw_ord[i]] <= sw_hi[sw_ord[i - 1]]) {', 'if (sw_lo[sw_ord[i]] < sw_hi[sw_ord[i - 1]]) {'),
 ('b03 switch: default', 'int end = newlab(); int deflab = end; if (defi >= 0) deflab = sw_lab[defi];', 'int end = newlab(); int deflab = end;'),
 ('b03 switch: continue', 'loop_cont[nloop] = 0 - 1; loop_kind[nloop] = 1;', 'loop_cont[nloop] = 0 - 1; loop_kind[nloop] = 0;'),
 ('b03 switch: tablo esigi', 'if (use_jtab && cnt >= 4 &&', 'if (use_jtab && cnt >= 400 &&'),
 ('b03 switch: tablo dizini', 'V idx = binop("-", sw_c, imm(T, mn));', 'V idx = binop("-", sw_c, imm(T, mn + 1));'),
 ('b03 switch: ilklendirme atlama', 'if (vinit[j]) { curline = nk_line[n]; die("E_JUMPINIT", "case etiketi', 'if (0) { curline = nk_line[n]; die("E_JUMPINIT", "case etiketi'),
 ('b03 goto: ilklendirme atlama', 'if (in_snap(lsn[i], gsn, gn) == 0) die(', 'if (0) die('),
 ('b03 goto: try icine', 'if (pg_try[g] < try_depth) die(', 'if (0) die('),
 ('b03 erisilebilirlik: donus eksik', 'if (co_mode == 0 && reach && is_void(rt) == 0', 'if (0 && reach && is_void(rt) == 0'),
 ('b03 erisilebilirlik: while sonrasi', 'jmpl(top); place(end); reach = 0; if (r0 && ((m & 2) || loop_bs[nloop])) reach = 1; return;\n  }\n  if (k == N_DO)', 'jmpl(top); place(end); reach = 0; if (0) reach = 1; return;\n  }\n  if (k == N_DO)'),
 ('b03 if constexpr: dal', 'int taken = 2; if (cc_.cv != 0) taken = 1;', 'int taken = 1; if (cc_.cv != 0) taken = 2;'),
 ('b03 range-for: dizi sonu', 'int cntn = mknode(N_INT, tn[rt], 0 - 1, line, 0, 0, 0, 0);', 'int cntn = mknode(N_INT, tn[rt] - 1, 0 - 1, line, 0, 0, 0, 0);'),
 ('b03 match: aralik alt sinir', 'mkbin(">=", subj, kid(pn, 0), line)', 'mkbin(">", subj, kid(pn, 0), line)'),
 ('b03 match: secenek', 'pat_test(alt, subj, nf); jmpl(ok); place(nf);', 'pat_test(alt, subj, nf); jmpl(fl); place(nf);'),
 ('b03 korutin: devam noktasi', 'ins3("eq", ck, mkv(0, 0, st), imm(0, q + 1));', 'ins3("eq", ck, mkv(0, 0, st), imm(0, q + 2));'),
 ('b03 korutin: next/await', 'ins3("eq", c2, mkv(0, 0, r), imm(0, 2)); e_jcc("jnz", mkv(0, 0, c2), top);', 'ins3("eq", c2, mkv(0, 0, r), imm(0, 3)); e_jcc("jnz", mkv(0, 0, c2), top);'),
 ('b03 sabit okuma', 'if (v.ci && v.cat == 1 && is_int(unq(v.ty))) {', 'if (0 && v.ci && v.cat == 1 && is_int(unq(v.ty))) {'),
 ('b03 sabit &&', 'if (isand && a.cv == 0) { int sv = may_throw;', 'if (isand && a.cv != 0) { int sv = may_throw;'),
 ('b03 sabit karsilastirma', 'int res = 0; if (streq(tok, "==")) res = a == b;', 'int res = 0; if (streq(tok, "==")) res = a != b;'),
 ('b03 do-while sonu', 'int m = cj(kid(n, 1), top, end, end); nreg = base; place(end);', 'int m = cj(kid(n, 1), end, top, end); nreg = base; place(end);'),
 ('b03 try erisilebilirligi', 'if (reach) tacc = 1;', ''),
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
ONLY = os.environ.get('ONLY')
if ONLY: jobs = [m for m in jobs if ONLY in m[0]]
with concurrent.futures.ThreadPoolExecutor(max_workers=max(2, os.cpu_count() or 2)) as ex: res = list(ex.map(job, jobs))
bad = [r for r in res if r[1] != 'YAKALANDI']
for n, st in bad: print('%s: %s' % (st, n))
nt = len([m for m in jobs if m[1] == 'tab']); print('ifx sabotaj: %d mutant (%d tablo + %d kaynak), %d yakalandi, %d yakalanmadi' % (len(res), nt, len(res) - nt, len(res) - len(bad), len(bad)))
sys.exit(1 if bad else 0)
