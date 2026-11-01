#!/usr/bin/env python3
"""Mikro-program yorumlayicisi (GELISTIRME araci): komut tablosundaki SEM mikro-programlarinin (rd wr rdw wrw im lit alu_* cmp_* un_* sel mulhu divu lpb lpe spld spst sx*/zx* ...)
anlamini Python'da uygular. Yalniz uretecin dogrulugunu hizlica sinamak icindir; motorun yurutme yolu degildir (motor: svm / gen2 ELF / C++ pm)."""
M64 = (1 << 64) - 1
import struct
def f_of(b): return struct.unpack('<d', struct.pack('<Q', b & ((1 << 64) - 1)))[0]
def b_of(f): return struct.unpack('<Q', struct.pack('<d', f))[0]
def s64(v): v &= M64; return v - (1 << 64) if v >> 63 else v
class Trap(Exception): pass
def calistir(ops, operand_regs, a=0, b=0, c=0, nregs=64, adim_siniri=3000000):
    """ops: mikro-islem metinleri; operand_regs: yazmac dizisi (liste, degistirilir); a,b,c: operand yuvalari (yazmac numaralari)"""
    T = [0] * 32; SC = [0] * 256; OP = {'a': a, 'b': b, 'c': c}
    prog = [o.split() for o in ops]
    i = 0; lpk = 0; adim = 0
    R = operand_regs
    while i < len(prog):
        adim += 1
        if adim > adim_siniri: raise Trap('adim siniri')
        p = prog[i]; i += 1; n = p[0]
        def t(x): return int(x[1:])
        if n == 'rd': T[t(p[1])] = R[OP[p[2]]] & M64
        elif n == 'wr': R[OP[p[1]]] = T[t(p[2])] & M64
        elif n == 'rdw': T[t(p[1])] = R[OP[p[2]] + int(p[3])] & M64
        elif n == 'wrw': R[OP[p[1]] + int(p[2])] = T[t(p[3])] & M64
        elif n == 'im': T[t(p[1])] = OP[p[2]] & M64
        elif n == 'lit': T[t(p[1])] = int(p[2]) & M64
        elif n == 'lpb': lpk = i
        elif n == 'lpe':
            if T[t(p[1])] != 0: i = lpk
        elif n == 'spld': T[t(p[1])] = SC[int(p[2])]
        elif n == 'spst': SC[int(p[1])] = T[t(p[2])]
        elif n.startswith('alu_'):
            x, y = T[t(p[2])], T[t(p[3])]; k = n[4:]
            if k == 'add': v = x + y
            elif k == 'sub': v = x - y
            elif k == 'mul': v = x * y
            elif k == 'and': v = x & y
            elif k == 'or': v = x | y
            elif k == 'xor': v = x ^ y
            elif k == 'shl': v = x << (y & 63)
            elif k == 'shr': v = x >> (y & 63)
            elif k == 'sar': v = s64(x) >> (y & 63)
            else: raise Exception(n)
            T[t(p[1])] = v & M64
        elif n.startswith('cmp_'):
            x, y = T[t(p[2])], T[t(p[3])]; k = n[4:]
            if k == 'eq': v = x == y
            elif k == 'ne': v = x != y
            elif k == 'lt': v = s64(x) < s64(y)
            elif k == 'le': v = s64(x) <= s64(y)
            elif k == 'gt': v = s64(x) > s64(y)
            elif k == 'ge': v = s64(x) >= s64(y)
            elif k == 'ltu': v = x < y
            elif k == 'leu': v = x <= y
            else: raise Exception(n)
            T[t(p[1])] = int(v)
        elif n in ('fadd', 'fsub', 'fmul', 'fdiv', 'feq', 'flt', 'fle'):
            import math
            x, y = f_of(T[t(p[2])]), f_of(T[t(p[3])])
            if n == 'fadd': T[t(p[1])] = b_of(x + y)
            elif n == 'fsub': T[t(p[1])] = b_of(x - y)
            elif n == 'fmul': T[t(p[1])] = b_of(x * y)
            elif n == 'fdiv': T[t(p[1])] = b_of(x / y)
            elif n == 'feq': T[t(p[1])] = int(x == y)
            elif n == 'flt': T[t(p[1])] = int(x < y)
            else: T[t(p[1])] = int(x <= y)
        elif n == 'fneg': T[t(p[1])] = T[t(p[2])] ^ (1 << 63)
        elif n == 'i2f': T[t(p[1])] = b_of(float(s64(T[t(p[2])])))
        elif n == 'f2i':
            x = f_of(T[t(p[2])])
            try: T[t(p[1])] = int(x) & M64 if abs(x) < 2.0 ** 63 else 1 << 63
            except (ValueError, OverflowError): T[t(p[1])] = 1 << 63
        elif n == 'un_neg': T[t(p[1])] = (-T[t(p[2])]) & M64
        elif n == 'un_not': T[t(p[1])] = (~T[t(p[2])]) & M64
        elif n == 'sel': T[t(p[1])] = T[t(p[3])] if T[t(p[2])] else T[t(p[4])]
        elif n == 'mulhu': T[t(p[1])] = (T[t(p[2])] * T[t(p[3])]) >> 64
        elif n == 'divu':
            if T[t(p[3])] == 0: raise Trap('DIV0')
            T[t(p[1])] = T[t(p[2])] // T[t(p[3])]
        elif n == 'modu':
            if T[t(p[3])] == 0: raise Trap('DIV0')
            T[t(p[1])] = T[t(p[2])] % T[t(p[3])]
        elif n in ('sx8', 'sx16', 'sx32', 'zx8', 'zx16', 'zx32'):
            w = int(n[2:]); x = T[t(p[2])] & ((1 << w) - 1)
            if n[0] == 's' and x >> (w - 1): x -= 1 << w
            T[t(p[1])] = x & M64
        else: raise Exception('bilinmeyen mikro-islem: ' + n)
    return R
