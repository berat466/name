#!/usr/bin/env python3
"""Bolum 04 duzeltme SABOTAJ (test-testi): korutin cerceve bellegi calisma zamani onsozunu (src/ifade.cpp coPrelude/instHandle: blok ayirici, serbest liste, sihir/nesil denetimi, tutamak nesli) bilerek bozar;
her mutantla dogrula_ifx.sh (mutant pm ile on yuz) calistirilir; mutant YAKALANMALIDIR. (Derleyici ifx.cx mutantlari sabotaj_ifx.py'dedir.) Kullanim: sabotaj_yasam.py"""
import sys, os, subprocess, tempfile, shutil, concurrent.futures
D = os.path.dirname(os.path.abspath(__file__)); R = os.path.abspath(D + '/../../..'); CPP = 'src/ifade.cpp'
M = [  # (eski, yeni, aciklama)
 ('if (b[1] != g) __trap(); return f; } ";', 'return f; } ";', 'cochk: nesil denetimi'),
 ('if (m - (m & 255) != " + magic + ") __trap(); if (b[1] != g) __trap(); return f; }', 'if (b[1] != g) __trap(); return f; }', 'cochk: sihir denetimi'),
 ('b[1] = b[1] + 1; b[0] = 0;', 'b[0] = 0;', 'ffree: nesil artisi'),
 ('__cofl[k] = (long long)b; } ";', '} ";', 'ffree: serbest listeye koy'),
 ('long long i = " + hdr + "; while (i < sz) { b[i] = 0; i = i + 1; } }', '}', 'falloc: yeniden kullanimda sifirlama'),
 ('__cofl[k] = b[" + hdr + "]; long long i', 'long long i', 'falloc: liste ilerletme'),
 ('long long* b = (long long*)__cofl[k]; if (b != nullptr) {', 'long long* b = (long long*)__cofl[k]; if (false) {', 'falloc: serbest listeden al'),
 ('while (sz < nw + " + hdr + ")', 'while (sz < nw)', 'falloc: baslik payi'),
 ('if (b[1] != g) return 0; return 1; } ";', 'return 1; } ";', 'covalid: nesil denetimi'),
 ('h.f = __co_frame(&pr); h.g = __co_gen(h.f); return h;', 'h.f = __co_frame(&pr); h.g = 0; return h;', 'tutamak: from_promise nesli'),
 ('static " + self + " from_address(void* a) { " + self + " h; h.f = a; h.g = __co_gen(a); return h; } " + arg', 'static " + self + " from_address(void* a) { " + self + " h; h.f = a; h.g = 0; return h; } " + arg', 'tutamak: from_address nesli'),
 ('h.f = f; h.g = g; return h; } };";', 'h.f = f; h.g = 0; return h; } };";', 'tutamak: donusum nesli'),
]
T = tempfile.mkdtemp(prefix='sabyasam'); S = T + '/src'; shutil.copytree(R + '/src', S)
names = ['main', 'asm', 'ikili', 'vm', 'dump', 'native_x64', 'sablon', 'yapisal', 'liste']
def cc(args, opt='-O1'): return subprocess.run(['g++', '-std=c++17', opt, '-w', '-fno-fast-math', '-ffp-contract=off'] + args, capture_output=True, text=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
    rs = list(ex.map(lambda n: cc(['-c', S + '/%s.cpp' % n, '-o', T + '/%s.o' % n]), names))
if any(r.returncode for r in rs): print('temel nesneler derlenemedi'); sys.exit(2)
def mutant(i):
    old, new, ac = M[i]; w = T + '/m%d' % i; os.makedirs(w)
    src = open(R + '/' + CPP).read()
    if src.count(old) != 1: return (i, 'HATA', 'eski metin %d kez' % src.count(old))
    shutil.copytree(S, w + '/src'); open(w + '/' + CPP, 'w').write(src.replace(old, new))
    r = cc(['-I', w + '/src', '-c', w + '/src/ifade.cpp', '-o', w + '/ifade.o'], '-O0')
    if r.returncode: return (i, 'DERLENEMEDI', r.stderr[:200])
    l = subprocess.run(['g++', '-o', w + '/pm'] + [T + '/%s.o' % n for n in names] + [w + '/ifade.o', '-lquadmath'], capture_output=True, text=True)
    if l.returncode: return (i, 'DERLENEMEDI', l.stderr[:200])
    log = w + '/log'
    t = subprocess.run([D + '/../../ilkhata.sh', log, 'bash', D + '/dogrula_ifx.sh', w + '/pm'], capture_output=True, text=True, env=dict(os.environ, SABOTAJ='1', SVM='0', LIMS='60'), cwd=D)
    shutil.rmtree(w, ignore_errors=True)
    return (i, 'YAKALANDI' if t.returncode != 0 else 'HAYATTA', '')
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as ex: res = list(ex.map(mutant, range(len(M))))
shutil.rmtree(T, ignore_errors=True)
bad = [r for r in res if r[1] != 'YAKALANDI']
for i, st, msg in bad: print('%s: %s -- %s %s' % (st, M[i][2], M[i][0][:70].replace('\n', ' '), msg))
print('yasam (onsoz) sabotaj: %d mutant, %d yakalandi, %d yakalanmadi' % (len(M), len(res) - len(bad), len(bad)))
sys.exit(1 if bad else 0)
