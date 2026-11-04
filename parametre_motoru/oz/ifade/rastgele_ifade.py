#!/usr/bin/env python3
"""Rastgele (tanimsiz davranissiz) tamsayi/kayan nokta ifade ureteci -- Bolum 02.
Kullanim: rastgele_ifade.py N CIKTI_DIZINI [tohum]
Her test: tipli rastgele ifadeler (ikili/tekli/kosul/mantiksal/donusum/virgul/birlesik atama); ifadenin TURU uretecte izlenir (olagan aritmetik donusumler), boylece pi/pd dogru secilir.
Tanimsiz davranis (isaretli tasma, gecersiz kaydirma, sifira bolme, aralik disi f->i) UBSan ile g++ tarafindan ELENIR: yalniz temiz calisan testler yazilir.
Beklenen cikti g++'dan gelir (dogrula_ifade.sh); bu betik yalniz temiz kaynaklari uretir."""
import re, sys, os, random, subprocess, tempfile, concurrent.futures
N = int(sys.argv[1]); OUT = sys.argv[2]; SEED = int(sys.argv[3]) if len(sys.argv) > 3 else random.randrange(1 << 30)
D = os.path.dirname(os.path.abspath(__file__))
# tur: ad, genislik, isaretli, kayan
T = {'bool': (1, False, False), 'char': (8, True, False), 'uchar': (8, False, False), 'short': (16, True, False), 'ushort': (16, False, False), 'int': (32, True, False),
     'uint': (32, False, False), 'long': (64, True, False), 'ulong': (64, False, False), 'float': (32, True, True), 'double': (64, True, True)}
CN = {'bool': 'bool', 'char': 'char', 'uchar': 'unsigned char', 'short': 'short', 'ushort': 'unsigned short', 'int': 'int', 'uint': 'unsigned', 'long': 'long long', 'ulong': 'unsigned long long', 'float': 'float', 'double': 'double'}
INTS = [t for t in T if not T[t][2]]
def promote(t): return 'int' if T[t][2] is False and T[t][0] < 32 else t
def uac(a, b):
    if T[a][2] or T[b][2]:
        if 'double' in (a, b): return 'double'
        return 'float'
    a, b = promote(a), promote(b)
    if a == b: return a
    sa, sb = T[a][1], T[b][1]
    if sa == sb: return a if T[a][0] >= T[b][0] else b
    u, s = (b, a) if sa else (a, b)
    if T[u][0] >= T[s][0]: return u
    if T[s][0] > T[u][0]: return s
    return u
class G:
    def __init__(s, rnd): s.r = rnd; s.vars = {}
    def lit(s, t):
        r = s.r
        if T[t][2]:
            v = r.choice(['0.5', '1.25', '2.0', '3.75', '100.0', '0.1', '7.0', '1e3', '0.0', '15.5'])
            return (v + 'f', 'float') if t == 'float' else (v, 'double')
        if t == 'bool': return (r.choice(['true', 'false']), 'bool')
        v = r.choice([0, 1, 2, 3, 5, 7, 10, 31, 100, 127, 255])
        return (str(v), 'int')
    def expr(s, d):
        r = s.r
        if d <= 0 or r.random() < 0.18:
            if r.random() < 0.7:
                v = r.choice(list(s.vars)); return (v, s.vars[v])
            return s.lit(r.choice(list(T)))
        k = r.random()
        if k < 0.50:
            op = r.choice(['+', '-', '*', '/', '%', '&', '|', '^', '<<', '>>', '==', '!=', '<', '>', '<=', '>=', '+', '-', '*'])
            a, ta = s.expr(d - 1); b, tb = s.expr(d - 1)
            if op in ('/', '%'):
                if T[tb][2]:
                    if op == '%': op = '*'
                    else: b = '(%s*%s+1.0)' % (b, b); tb = uac(tb, 'double')
                else: b = '(%s | 1)' % b; tb = uac(tb, 'int')
                if op == '%' and T[ta][2]: op = '+'
            if op in ('%', '&', '|', '^', '<<', '>>') and (T[ta][2] or T[tb][2]):
                # kayan operandlari tamsayiya indir
                if T[ta][2]: a = '(int)(%s)' % a; ta = 'int'
                if T[tb][2]: b = '(int)(%s)' % b; tb = 'int'
            if op in ('<<', '>>'):
                lt = promote(ta); b = '((%s) & %d)' % (b, T[lt][0] - 1); return ('(%s %s %s)' % (a, op, b), lt)
            if op in ('==', '!=', '<', '>', '<=', '>='): return ('(%s %s %s)' % (a, op, b), 'bool')
            return ('(%s %s %s)' % (a, op, b), uac(ta, tb))
        if k < 0.60:
            a, ta = s.expr(d - 1); op = r.choice(['-', '+', '!', '~'])
            if op == '~' and T[ta][2]: op = '-'
            if op == '!': return ('(!%s)' % a, 'bool')
            return ('(%s%s)' % (op, a), promote(ta) if not T[ta][2] else ta)
        if k < 0.72:
            c, tc = s.expr(d - 1); a, ta = s.expr(d - 1); b, tb = s.expr(d - 1)
            return ('(%s ? %s : %s)' % (c, a, b), uac(ta, tb) if not (ta == tb) else ta)
        if k < 0.80:
            a, ta = s.expr(d - 1); b, tb = s.expr(d - 1); op = r.choice(['&&', '||'])
            return ('(%s %s %s)' % (a, op, b), 'bool')
        if k < 0.92:
            a, ta = s.expr(d - 1); t = r.choice(list(T))
            if t == 'bool': return ('(bool)(%s)' % a, 'bool')
            if T[ta][2] and not T[t][2] and not re.search(r'v\d', a): t = 'double'   # degiskensiz sabit kayan nokta ifadesinin tamsayiya donusumu: g++ derleme zamaninda katlar (aralik disi = tanimsiz davranis, UBSan gormez)
            if r.random() < 0.5: return ('static_cast<%s>(%s)' % (CN[t], a), t)
            return ('(%s)(%s)' % (CN[t], a), t)
        a, ta = s.expr(d - 1); b, tb = s.expr(d - 1)
        return ('(%s, %s)' % (a, b), tb)
def gen_test(seed):
    rnd = random.Random(seed); g = G(rnd); L = ['// otomatik uretildi (rastgele_ifade.py, tohum %d)' % seed, 'int main() {']
    for i, t in enumerate(rnd.sample(list(T), 8) + rnd.choice([['int'], ['uint'], ['long']])):
        v = 'v%d' % i; g.vars[v] = t; lt = g.lit(t)
        init = lt[0]
        if t in ('int', 'long', 'short', 'char') and rnd.random() < 0.5: init = '-' + init if not T[t][2] else init
        if T[t][2] is False and t not in ('bool',) and rnd.random() < 0.4: init = rnd.choice(['127', '255', '65535', '2147483647', '200', '1000'])
        L.append('  %s %s = %s;' % (CN[t], v, init if t != 'bool' else lt[0]))
    for _ in range(26):
        k = rnd.random()
        if k < 0.78:
            e, t = g.expr(rnd.choice([2, 3, 4]))
            L.append('  %s' % ('pd(%s);' % e if T[t][2] else 'pi(%s);' % e))
        elif k < 0.90:
            v = rnd.choice(list(g.vars)); op = rnd.choice(['+=', '-=', '*=', '|=', '&=', '^=', '=']); e, t = g.expr(2)
            tv = g.vars[v]
            if op in ('|=', '&=', '^=') and (T[tv][2] or T[t][2]): op = '+='
            if T[tv][2] is False and T[t][2]:
                e = '(int)(%s)' % e
            if op == '*=' and T[tv][2] is False and T[tv][1]: op = '|='   # isaretli tasmayi azalt
            L.append('  %s %s %s; %s' % (v, op, e, 'pd(%s);' % v if T[tv][2] else 'pi(%s);' % v))
        else:
            v = rnd.choice(list(g.vars)); tv = g.vars[v]
            if tv == 'bool': continue
            L.append('  %s' % rnd.choice(['pi(%s++);', 'pi(++%s);', 'pi(%s--);', 'pi(--%s);']).replace('pi', 'pd' if T[tv][2] else 'pi') % v)
    L.append('  return 0;'); L.append('}')
    return '\n'.join(L) + '\n'
def temiz(src):
    with tempfile.TemporaryDirectory() as td:
        f = td + '/t.cx'; open(f, 'w').write(src)
        c = subprocess.run(['g++', '-std=c++20', '-O0', '-w', '-fsanitize=signed-integer-overflow,shift,integer-divide-by-zero,float-cast-overflow,undefined', '-fno-sanitize-recover=all', '-include', D + '/oracle.h', '-x', 'c++', '-o', td + '/b', f], capture_output=True)
        if c.returncode: return False
        try: r = subprocess.run([td + '/b'], capture_output=True, timeout=10)
        except subprocess.TimeoutExpired: return False
        if r.returncode or b'nan' in r.stdout or b'inf' in r.stdout: return False   # yazdirma bicimleri uygulamalar arasi ayni olmayabilir
        return True
os.makedirs(OUT, exist_ok=True)
def work(i):
    for k in range(40):
        seed = SEED * 1000 + i * 50 + k; src = gen_test(seed)
        if temiz(src): open('%s/r%03d.cx' % (OUT, i), 'w').write(src); return 1
    return 0
with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex: ok = sum(ex.map(work, range(N)))
print('rastgele ifade: %d/%d test uretildi (tohum %d)' % (ok, N, SEED))
