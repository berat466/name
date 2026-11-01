#!/usr/bin/env python3
"""dogrula_kayan.sh icin PMA test programi uretici (GELISTIRME araci). Kullanim: kayan_prog.py <tur> <F:f80|f128> <N> <oracle>  -> stdout PMA
tur: bin (cift: add sub mul div eq ne lt le) | un (tekli: neg abs sqrt tobool, tamsayi/dar kayan donusumleri) | cd (double -> wide) | ci (tamsayi -> wide)"""
import sys, subprocess
kind, F, n, oracle = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
f = F; o = []
def ora(*a): return subprocess.run([oracle] + [str(x) for x in a], capture_output=True, check=True).stdout.decode().strip()
sfx = '80' if F == 'f80' else '128'
def W(r): o.extend(['    lo64_%s t, %s' % (f, r), '    host puti, t', '    host putc, 32', '    hi64_%s t, %s' % (f, r), '    host puti, t', '    host putc, 32'])
def Wf(r, g):   # baska genis kayan nokta (oteki bicim)
    o.extend(['    lo64_%s t, %s' % (g, r), '    host puti, t', '    host putc, 32', '    hi64_%s t, %s' % (g, r), '    host puti, t', '    host putc, 32'])
def W128(r, signed):
    ty = 'i128' if signed else 'u128'
    o.extend(['    trunc_%s_%s t, %s' % (ty, 'i64' if signed else 'u64', r), '    host puti, t', '    host putc, 32', '    hi64_%s t, %s' % (ty, r), '    host puti, t', '    host putc, 32'])
def N(c): o.extend(['    host puti, %s' % c, '    host putc, 32'])
other = 'f128' if F == 'f80' else 'f80'
if kind in ('bin', 'un'):
    veri = ora('veri' + sfx, n); nv = n
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % nv, '    jnz z, done', '    muli a, i, 2', '    addi a, a, tab', '    ld xl, a, 0', '    ld xh, a, 1', '    pack_%s x, xl, xh' % f]
    if kind == 'bin':
        o += ['    mov j, 0', 'lj: gei z, j, %d' % nv, '    jnz z, nj', '    muli b, j, 2', '    addi b, b, tab', '    ld yl, b, 0', '    ld yh, b, 1', '    pack_%s y, yl, yh' % f]
        for op in ('add', 'sub', 'mul', 'div'): o.append('    %s_%s r, x, y' % (op, f)); W('r')
        for op in ('eq', 'ne', 'lt', 'le'): o.append('    %s_%s c, x, y' % (op, f)); N('c')
        o += ['    host putc, 10', '    addi j, j, 1', '    jmp lj', 'nj: addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
    else:
        for op in ('neg', 'abs', 'sqrt'): o.append('    %s_%s r, x' % (op, f)); W('r')
        o.append('    tobool_%s c, x' % f); N('c')
        o.append('    cv_%s_i64 c, x' % f); N('c'); o.append('    cv_%s_u64 c, x' % f); N('c')
        o.append('    cv_%s_i128 q, x' % f); W128('q', True); o.append('    cv_%s_u128 q, x' % f); W128('q', False)
        for nb in (8, 16, 32):
            for sg in 'iu': o.append('    cv_%s_%s%d c, x' % (f, sg, nb)); N('c')
        for g in ('f64', 'f32', 'f16', 'bf16'): o.append('    cv_%s_%s c, x' % (f, g)); N('c')
        o.append('    cv_%s_%s r, x' % (f, other)); Wf('r', other)
        o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
elif kind == 'cd':
    veri = ora('dbl', n)
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % n, '    jnz z, done', '    addi a, i, tab', '    ld d, a, 0', '    cv_f64_%s r, d' % f]
    W('r'); o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
elif kind == 'ci':
    veri = ora('int', n)
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % n, '    jnz z, done', '    muli a, i, 3', '    addi a, a, tab', '    ld b, a, 0', '    ld lo, a, 1', '    ld hi, a, 2',
          '    cv_i64_%s r, b' % f]
    W('r'); o.append('    cv_u64_%s r, b' % f); W('r')
    o += ['    pack_i128 q, lo, hi', '    cv_i128_%s r, q' % f]; W('r')
    o += ['    pack_u128 q, lo, hi', '    cv_u128_%s r, q' % f]; W('r')
    o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']

if kind in ('cbin', 'cun'):
    C = F; big = C == 'c160'; sfx = C[1:]
    veri = ora('cveri' + sfx, n)
    def getc(x, ofs):   # n. deger: data[(i*stride)+ofs..]
        pass
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % n, '    jnz z, done']
    def load(v, idx):
        if not big: return ['    muli a, %s, 2' % idx, '    addi a, a, tab', '    ld %sr, a, 0' % v, '    ld %si, a, 1' % v, '    mk_%s %s, %sr, %si' % (C, v, v, v)]
        return ['    muli a, %s, 4' % idx, '    addi a, a, tab', '    ld %sl0, a, 0' % v, '    ld %sh0, a, 1' % v, '    ld %sl1, a, 2' % v, '    ld %sh1, a, 3' % v,
                '    pack_f80 %sr, %sl0, %sh0' % (v, v, v), '    pack_f80 %si, %sl1, %sh1' % (v, v, v), '    mk_c160 %s, %sr, %si' % (v, v, v)]
    def outc(r):   # karmasik r: gercek ve sanal parcalarini yaz
        for part in ('real', 'imag'):
            if big: o.append('    %s_c160 pp, %s' % (part, r)); Wf('pp', 'f80')
            else: o.append('    %s_%s pp, %s' % (part, C, r)); N('pp')
    o += load('x', 'i')
    if kind == 'cbin':
        o += ['    mov j, 0', 'lj: gei z, j, %d' % n, '    jnz z, nj'] + load('y', 'j')
        for op in ('add', 'sub', 'mul', 'div'): o.append('    %s_%s r, x, y' % (op, C)); outc('r')
        for op in ('eq', 'ne'): o.append('    %s_%s c, x, y' % (op, C)); N('c')
        o += ['    host putc, 10', '    addi j, j, 1', '    jmp lj', 'nj: addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
    else:
        for op in ('neg', 'conj'): o.append('    %s_%s r, x' % (op, C)); outc('r')
        o.append('    tobool_%s c, x' % C); N('c'); outc('x')
        o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
if kind == 'ccv':
    veri = ora('ccveri', n)
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % n, '    jnz z, done', '    muli a, i, 6', '    addi a, a, tab',
          '    ld re, a, 0', '    ld im, a, 1', '    ld l0, a, 2', '    ld h0, a, 3', '    ld l1, a, 4', '    ld h1, a, 5', '    mk_c128 s128, re, im', '    pack_f80 fr, l0, h0', '    pack_f80 fi, l1, h1', '    mk_c160 s160, fr, fi']
    def p2(r, t): o.extend(['    real_%s pp, %s' % (t, r), '    host puti, pp', '    host putc, 32', '    imag_%s pp, %s' % (t, r), '    host puti, pp', '    host putc, 32'])
    def p160(r): 
        for part in ('real', 'imag'): o.append('    %s_c160 pp, %s' % (part, r)); Wf('pp', 'f80')
    o += ['    cv_c128_c64 a64, s128']; p2('a64', 'c64')
    o += ['    cv_c160_c128 b128, s160']; p2('b128', 'c128')
    o += ['    cv_c160_c64 c64v, s160']; p2('c64v', 'c64')
    o += ['    cv_c128_c160 d160, s128']; p160('d160')
    o += ['    cv_c64_c160 e160, a64']; p160('e160')
    o += ['    cv_c64_c128 f128, a64']; p2('f128', 'c128')
    o += ['    cv_f64_c64 g64, re']; p2('g64', 'c64')
    o += ['    cv_f64_c128 h128, re']; p2('h128', 'c128')
    o += ['    cv_f64_c160 k160, re']; p160('k160')
    o += ['    cv_f80_c64 m64, fr']; p2('m64', 'c64')
    o += ['    cv_f80_c128 q128, fr']; p2('q128', 'c128')
    o += ['    cv_f80_c160 t160, fr']; p160('t160')
    o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
if kind == 'sq':
    veri = ora('sqveri' + sfx, n)
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % n, '    jnz z, done', '    muli a, i, 2', '    addi a, a, tab', '    ld xl, a, 0', '    ld xh, a, 1', '    pack_%s x, xl, xh' % f, '    sqrt_%s r, x' % f]
    W('r'); o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
if kind == 'ni':
    veri = ora('niveri', n)
    o += ['memory 65536', 'data tab = ' + veri, 'func main() {', '    mov i, 0', 'li: gei z, i, %d' % n, '    jnz z, done', '    muli a, i, 3', '    addi a, a, tab', '    ld lo, a, 0', '    ld hi, a, 1', '    ld d, a, 2', '    pack_i128 qs, lo, hi', '    pack_u128 qu, lo, hi']
    for ty, v in (('i128', 'qs'), ('u128', 'qu')):
        for nar in ('f64', 'f32', 'f16', 'bf16'): o.append('    cv_%s_%s c, %s' % (ty, nar, v)); N('c')
    for ty in ('i128', 'u128'): o.append('    cv_f64_%s r, d' % ty); W128('r', ty == 'i128')
    o += ['    host putc, 10', '    addi i, i, 1', '    jmp li', 'done: halt', '}', 'entry main']
print('\n'.join(o))
