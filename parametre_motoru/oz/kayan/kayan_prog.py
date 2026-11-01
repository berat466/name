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
print('\n'.join(o))
