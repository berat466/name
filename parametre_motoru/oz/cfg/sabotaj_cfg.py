#!/usr/bin/env python3
"""Sabotaj (test-testi) -- CFG iyilestiricisi: cfgopt.cx kaynagi bozulur, ifx.elf + asm.elf ile (pm'siz) yeniden derlenir, dogrula_cfg.sh calistirilir; mutant YAKALANMALIDIR."""
import os, sys, subprocess, tempfile, shutil, concurrent.futures
D = os.path.dirname(os.path.abspath(__file__)); R = os.path.abspath(D + '/..'+'/..'); PM = R + '/pm'
SRC = open(D + '/cfgopt.cx').read(); TAB = R + '/oz/komut/komutlar.pmk'
S = [
 ('zincir kisaltma turu', 'flow(nt) == 1 && hop < 16', 'flow(nt) == 2 && hop < 16'),
 ('kosul katlama yonu', 'if (f == 2) taken = cval[r] == 0; else taken = cval[r] != 0;', 'if (f == 2) taken = cval[r] != 0; else taken = cval[r] == 0;'),
 ('sabit gecersiz kilma (mov)', 'cval[d] = cval[s]; cgen[d] = curgen; } else cgen[d] = 0; } }', 'cval[d] = cval[s]; cgen[d] = curgen; } } }'),
 ('sabit gecersiz kilma (hedef)', 'if (d + z >= 0 && d + z < 2048) cgen[d + z] = 0;', ''),
 ('etiket blok siniri', 'if (tcnt[i] > 0 || ladd[i]) curgen = curgen + 1;', ''),
 ('dal tersleme eslemesi', 'int inv = opBRZ; if (f == 2) inv = opBRNZ;', 'int inv = opBRZ; if (f == 3) inv = opBRNZ;'),
 ('sonraki komuta atlama', 'if (flow(i) == 1 && ladd[i] == 0 && target(i) == next_kept(i))', 'if (flow(i) == 1 && ladd[i] == 0 && target(i) == next_kept(i) + 1)'),
 ('erisilebilirlik: dal hedefi', 'else if (f == 2 || f == 3) { s0 = u + 1; s1 = target(u); }', 'else if (f == 2 || f == 3) { s0 = u + 1; }'),
 ('erisilebilirlik: jtab varsayilani', 'while (k <= n + 1) { int v = u + k;', 'while (k <= n) { int v = u + k;'),
 ('etiket yeniden eslemesi', 'setop(i, op_tg[iop[i]], nidx[t]);', 'setop(i, op_tg[iop[i]], t);'),
 ('olu yazma: okuma denetimi', 'if (rd_) go = 0;', 'if (0) go = 0;'),
 ('olu yazma: dal siniri', 'if (fj != 0) { go = 0; }   // dal/cagri/donus/host: sinir (tum yazmaclar canli varsay)', 'if (0) { go = 0; }'),
 ('hedef sayaci', 'if (f == 1 || f == 2 || f == 3) { int t = target(i); if (t >= 0 && t < ncode) tcnt[t] = tcnt[t] + 1; }', 'if (f == 1) { int t = target(i); if (t >= 0 && t < ncode) tcnt[t] = tcnt[t] + 1; }'),
 ('tersleme komut secimi', 'if (op_fl[o] == 2 && op_n[o] == 2 && opBRZ < 0) opBRZ = o; if (op_fl[o] == 3 && op_n[o] == 2 && opBRNZ < 0) opBRNZ = o;', 'if (op_fl[o] == 3 && op_n[o] == 2 && opBRZ < 0) opBRZ = o; if (op_fl[o] == 2 && op_n[o] == 2 && opBRNZ < 0) opBRNZ = o;'),
 ('merdiven korumasi', 'if (ladd[i] && nt <= i) { }', 'if (0) { }'),
 ('saf komut sinifi', 'if (fl == 0 && impure) fl = 9;', 'if (0) fl = 9;'),
 ('ldi sabiti', 'if (op_ldi[o]) { long long d = ia[i]; if (d >= 0 && d < 2048) { cgen[d] = curgen; cval[d] = ib[i]; } }', 'if (op_ldi[o]) { long long d = ia[i]; if (d >= 0 && d < 2048) { cgen[d] = curgen; cval[d] = ib[i] + 1; } }'),
 ('islev basligi', '__puti(np); __putc(32); __puti(fnregs); __putc(32); __puti(ncode);', '__puti(np); __putc(32); __puti(fnregs + 1); __putc(32); __puti(ncode);'),
]
def build(src, w):
    open(w + '/c.cx', 'w').write(src)
    r = subprocess.run([PM, 'ifade', w + '/c.cx', '--pmx1', w + '/c.pmx'], capture_output=True)
    if not os.path.exists(w + '/c.pmx'): return None
    imz = subprocess.run(['bash', '-c', '{ echo "@@imza"; cat %s; } | %s/komut.elf | tr "\\n" " "; echo' % (TAB, BAG)], capture_output=True).stdout
    inp = open(TAB, 'rb').read() + imz + open(w + '/c.pmx', 'rb').read()
    o = subprocess.run([BAG + '/../ifx.elf'], input=inp, capture_output=True)
    if b'@@HATA' in o.stdout or o.returncode: return None
    a = subprocess.run([BAG + '/asm.elf'], input=o.stdout, capture_output=True)
    if a.returncode or not a.stdout.startswith(b'PMB1'): return None
    open(w + '/c.pmb', 'wb').write(a.stdout); return w + '/c.pmb'
def job(m):
    w = tempfile.mkdtemp(prefix='sabcfg')
    if SRC.count(m[1]) != 1: shutil.rmtree(w); return (m[0], 'HATA eski metin %d kez' % SRC.count(m[1]))
    pmb = build(SRC.replace(m[1], m[2]), w)
    if pmb is None: shutil.rmtree(w); return (m[0], 'DERLENEMEDI')
    t = subprocess.run(['bash', D + '/dogrula_cfg.sh', PM], capture_output=True, text=True, env=dict(os.environ, SABOTAJ='1', CFGB=pmb, SVM='0', LIMS='60'), cwd=D)
    shutil.rmtree(w, ignore_errors=True); return (m[0], 'YAKALANDI' if t.returncode != 0 else 'HAYATTA')
BAGD = tempfile.mkdtemp(prefix='sabbag'); subprocess.run([R + '/oz/bag/kur.sh', PM, BAGD + '/bag'], capture_output=True); BAG = BAGD + '/bag'
# ifx.elf (pm'siz zincirin parcasi) bir kez uret
l = subprocess.run([PM, 'liste', R + '/oz/ifade/num/ifx.pmb'], capture_output=True).stdout
open(BAGD + '/ifx.l', 'wb').write(l); e = subprocess.run([BAG + '/gen2.elf'], input=l, capture_output=True).stdout; open(BAGD + '/ifx.elf', 'wb').write(e); os.chmod(BAGD + '/ifx.elf', 0o755)
BAG = BAGD + '/bag'
with concurrent.futures.ThreadPoolExecutor(max_workers=max(2, os.cpu_count() or 2)) as ex: res = list(ex.map(job, S))
bad = [r for r in res if r[1] != 'YAKALANDI']
for n, st in bad: print('%s: %s' % (st, n))
print('cfg sabotaj: %d mutant, %d yakalandi, %d yakalanmadi' % (len(res), len(res) - len(bad), len(bad)))
shutil.rmtree(BAGD, ignore_errors=True)
sys.exit(1 if bad else 0)
