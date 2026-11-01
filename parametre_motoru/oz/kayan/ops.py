#!/usr/bin/env python3
"""Yazilim kayan nokta islem mikro-programi ureticileri (GELISTIRME araci; cikti: komut tablosu SEM mikro-programlari)."""
from sf import G, F128, F80
def raw_sign_bit(fmt): return 15 if fmt['ex'] else 63
def shr_sticky(g, v, cnt, st, maxn):
    """v (2 sozcuk) sag kaydir 'cnt' kez (degisken cnt, 0..maxn), dusen bitler st'ye YA da"""
    g.e('lit t13 %d' % maxn, 'lpb')
    g.ld('t1', cnt); g.e('lit t2 %d' % maxn, 'alu_sub t3 t2 t13', 'cmp_gt t4 t1 t3')   # t4 = yap
    g.ld('t5', v, 0); g.ld('t6', v, 1); g.ld('t7', st)
    g.e('lit t8 1', 'alu_and t9 t5 t8', 'alu_and t9 t9 t4', 'alu_or t7 t7 t9')
    g.e('lit t9 63', 'alu_shl t10 t6 t9', 'alu_shr t11 t5 t8', 'alu_or t11 t11 t10', 'alu_shr t10 t6 t8')
    g.e('sel t5 t4 t11 t5', 'sel t6 t4 t10 t6')
    g.st(v, 't5', 0); g.st(v, 't6', 1); g.st(st, 't7')
    g.e('lit t12 1', 'alu_sub t13 t13 t12', 'lpe t13')
def addsub(fmt, sub):
    g = G()
    g.unpack('A', fmt, 'b'); g.unpack('B', fmt, 'c')
    if sub: g.ld('t1', 'B_s'); g.e('lit t2 1', 'alu_xor t1 t1 t2'); g.st('B_s', 't1')
    # siralama: buyuk (BG) ve kucuk (SM) buyukluk
    g.ld('t1', 'A_E'); g.ld('t2', 'B_E'); g.e('cmp_gt t10 t1 t2', 'cmp_eq t11 t1 t2')
    g.ltu2('t9', 'B_m', 'A_m'); g.e('alu_and t11 t11 t9', 'alu_or t10 t10 t11'); g.st('agt', 't10')
    for n in ('s', 'E'):
        g.ld('t1', 'A_' + n); g.ld('t2', 'B_' + n); g.ld('t3', 'agt'); g.e('sel t4 t3 t1 t2', 'sel t5 t3 t2 t1'); g.st('BG_' + n, 't4'); g.st('SM_' + n, 't5')
    g.ld('t9', 'agt'); g.sel2('BG_m', 't9', 'A_m', 'B_m'); g.ld('t9', 'agt'); g.sel2('SM_m', 't9', 'B_m', 'A_m')
    # kaydirma sayisi = min(d+1, 130)
    g.ld('t1', 'BG_E'); g.ld('t2', 'SM_E'); g.e('alu_sub t3 t1 t2', 'lit t4 1', 'alu_add t3 t3 t4', 'lit t5 130', 'cmp_gt t6 t3 t5', 'sel t3 t6 t5 t3'); g.st('cnt', 't3')
    g.e('lit t1 0'); g.st('st', 't1')
    g.shr1('BG_m'); shr_sticky(g, 'SM_m', 'cnt', 'st', 130)
    # toplam / fark
    g.ld('t1', 'BG_s'); g.ld('t2', 'SM_s'); g.e('alu_xor t3 t1 t2'); g.st('diff', 't3')
    g.add2('V_sum', 'BG_m', 'SM_m'); g.sub2('V_dif', 'BG_m', 'SM_m')
    # fark: sticky varsa 1 cikar
    g.ld('t1', 'V_dif', 0); g.ld('t2', 'V_dif', 1); g.ld('t3', 'st'); g.e('cmp_ltu t4 t1 t3', 'alu_sub t1 t1 t3', 'alu_sub t2 t2 t4'); g.st('V_dif', 't1', 0); g.st('V_dif', 't2', 1)
    g.ld('t9', 'diff'); g.sel2('R_m', 't9', 'V_dif', 'V_sum')
    g.ld('t1', 'BG_s'); g.st('R_s', 't1'); g.ld('t1', 'BG_E'); g.e('lit t2 1', 'alu_add t1 t1 t2'); g.st('R_E', 't1')
    g.ld('t1', 'st'); g.st('R_st', 't1')
    g.normalize('R')
    g.nz2('t9', 'R_m'); g.e('lit t1 1', 'alu_xor t9 t9 t1'); g.st('f_sz', 't9')
    g.pack(fmt, 'R', 'W')
    # ozel durumlar
    for X in 'AB':
        g.flag('f_n' + X, X, 3); g.flag('f_i' + X, X, 2); g.flag('f_z' + X, X, 0)
    g.orf('f_nn', 'f_nA', 'f_nB'); g.orf('f_ii', 'f_iA', 'f_iB'); g.andf('f_zz', 'f_zA', 'f_zB')
    g.ld('t1', 'A_s'); g.ld('t2', 'B_s'); g.e('alu_xor t3 t1 t2'); g.st('f_dif', 't3'); g.andf('f_cf', 'f_iA', 'f_iB'); g.andf('f_cf', 'f_cf', 'f_dif')
    g.mk_raw('V_a', 'A'); g.flip_raw('V_b', fmt, 'B', None) if sub else g.mk_raw('V_b', 'B')
    g.ld('t1', 'A_s'); g.ld('t2', 'B_s'); g.e('alu_and t3 t1 t2'); g.st('s_zz', 't3'); g.mk_zero('V_zz', fmt, 's_zz')
    g.ld('t1', 'A_s'); g.ld('t2', 'B_s'); g.ld('t3', 'f_iA'); g.e('sel t4 t3 t1 t2'); g.st('s_inf', 't4'); g.mk_inf('V_inf', fmt, 's_inf')
    g.mk_defnan('V_dn', fmt); g.c2('V_z0', 0, 0)
    g.mk_quiet('V_qa', fmt, 'A'); g.mk_quiet('V_qb', fmt, 'B'); g.ld('t9', 'f_nA'); g.sel2('V_n', 't9', 'V_qa', 'V_qb')
    g.chain('W', [('f_sz', 'V_z0'), ('f_zB', 'V_a'), ('f_zA', 'V_b'), ('f_zz', 'V_zz'), ('f_ii', 'V_inf'), ('f_cf', 'V_dn'), ('f_nn', 'V_n')])
    g.emit_wide()
    return g
def special_head(g, fmt):
    """A,B acilmis olmali: sinif bayraklari + sessiz NaN adaylari"""
    for X in 'AB':
        g.flag('f_n' + X, X, 3); g.flag('f_i' + X, X, 2); g.flag('f_z' + X, X, 0)
    g.orf('f_nn', 'f_nA', 'f_nB'); g.orf('f_ii', 'f_iA', 'f_iB'); g.orf('f_zo', 'f_zA', 'f_zB')
    g.mk_defnan('V_dn', fmt)
    g.mk_quiet('V_qa', fmt, 'A'); g.mk_quiet('V_qb', fmt, 'B'); g.ld('t9', 'f_nA'); g.sel2('V_n', 't9', 'V_qa', 'V_qb')
def prod(g, X, Y, out='r'):
    """128x128 -> 256 bit isaretsiz carpim: X (2 sozcuk) * Y (2 sozcuk) -> out0..out3 (1 sozcuk degiskenler); t1..t12 kullanir"""
    g.ld('t1', X, 0); g.ld('t2', X, 1); g.ld('t3', Y, 0); g.ld('t4', Y, 1)
    g.e('alu_mul t5 t1 t3', 'mulhu t6 t1 t3'); g.st(out + '0', 't5')
    g.e('alu_mul t7 t1 t4', 'mulhu t8 t1 t4', 'alu_mul t9 t2 t3', 'mulhu t10 t2 t3', 'alu_mul t11 t2 t4', 'mulhu t12 t2 t4')
    g.e('alu_add t1 t6 t7', 'cmp_ltu t2 t1 t6', 'alu_add t1 t1 t9', 'cmp_ltu t3 t1 t9', 'alu_add t2 t2 t3'); g.st(out + '1', 't1')
    g.e('alu_add t1 t8 t10', 'cmp_ltu t3 t1 t8', 'alu_add t1 t1 t11', 'cmp_ltu t4 t1 t11', 'alu_add t3 t3 t4', 'alu_add t1 t1 t2', 'cmp_ltu t4 t1 t2', 'alu_add t3 t3 t4'); g.st(out + '2', 't1')
    g.e('alu_add t12 t12 t3'); g.st(out + '3', 't12')
def mul(fmt):
    g = G()
    g.unpack('A', fmt, 'b'); g.unpack('B', fmt, 'c')
    prod(g, 'A_m', 'B_m')
    # sticky = (r1|r0) != 0
    g.ld('t1', 'r1'); g.ld('t2', 'r0'); g.e('alu_or t1 t1 t2', 'lit t2 0', 'cmp_ne t1 t1 t2'); g.st('R_st', 't1')
    # tek adim normalizasyon: r3 msb 0 ise 1 sola
    g.ld('t1', 'r2'); g.ld('t2', 'r3'); g.e('lit t3 63', 'alu_shr t4 t2 t3', 'lit t5 1', 'alu_xor t4 t4 t5')   # t4 = kaydir
    g.e('alu_shl t6 t2 t5', 'alu_shr t7 t1 t3', 'alu_or t6 t6 t7', 'alu_shl t8 t1 t5', 'sel t2 t4 t6 t2', 'sel t1 t4 t8 t1'); g.st('R_m', 't1', 0); g.st('R_m', 't2', 1)
    g.ld('t1', 'A_E'); g.ld('t2', 'B_E'); g.e('alu_add t1 t1 t2', 'lit t3 1', 'alu_add t1 t1 t3', 'alu_sub t1 t1 t4'); g.st('R_E', 't1')
    g.ld('t1', 'A_s'); g.ld('t2', 'B_s'); g.e('alu_xor t1 t1 t2'); g.st('R_s', 't1')
    g.pack(fmt, 'R', 'W')
    special_head(g, fmt)
    g.andf('f_x', 'f_ii', 'f_zo')   # sonsuz * sifir
    g.mk_inf('V_inf', fmt, 'R_s'); g.mk_zero('V_zz', fmt, 'R_s')
    g.chain('W', [('f_zo', 'V_zz'), ('f_ii', 'V_inf'), ('f_x', 'V_dn'), ('f_nn', 'V_n')])
    g.emit_wide()
    return g
def div(fmt):
    g = G()
    g.unpack('A', fmt, 'b'); g.unpack('B', fmt, 'c')
    g.copy2('V_r', 'A_m')
    g.ltu2('t9', 'A_m', 'B_m'); g.e('lit t1 1', 'alu_xor t9 t9 t1'); g.st('q0', 't9')    # q0 = A_m >= B_m
    g.sub2('V_t', 'V_r', 'B_m'); g.ld('t9', 'q0'); g.sel2('V_r', 't9', 'V_t', 'V_r')
    g.ld('t1', 'q0'); g.st('R_m', 't1', 0); g.e('lit t1 0'); g.st('R_m', 't1', 1)
    g.e('lit t13 127', 'lpb')
    g.ld('t1', 'V_r', 0); g.ld('t2', 'V_r', 1); g.ld('t3', 'B_m', 0); g.ld('t4', 'B_m', 1)
    g.e('lit t5 63', 'alu_shr t6 t2 t5', 'alu_shr t7 t1 t5', 'lit t5 1', 'alu_shl t2 t2 t5', 'alu_or t2 t2 t7', 'alu_shl t1 t1 t5')   # t6 = tasan bit ; R <<= 1
    g.e('alu_or t10 t1 t1', 'alu_or t11 t2 t2')   # t10,t11 = R<<1 (yedek)
    g.e('cmp_ltu t7 t2 t4', 'cmp_eq t8 t2 t4', 'cmp_ltu t9 t1 t3', 'alu_and t8 t8 t9', 'alu_or t7 t7 t8', 'alu_xor t7 t7 t5', 'alu_or t7 t7 t6')   # t7 = yap (R<<1 >= D veya tasma)
    g.e('cmp_ltu t8 t1 t3', 'alu_sub t1 t1 t3', 'alu_sub t2 t2 t4', 'alu_sub t2 t2 t8')   # R - D
    g.e('sel t3 t7 t1 t10', 'sel t4 t7 t2 t11'); g.st('V_r', 't3', 0); g.st('V_r', 't4', 1)
    g.ld('t1', 'R_m', 0); g.ld('t2', 'R_m', 1)
    g.e('lit t5 1', 'lit t8 63', 'alu_shr t9 t1 t8', 'alu_shl t2 t2 t5', 'alu_or t2 t2 t9', 'alu_shl t1 t1 t5', 'alu_or t1 t1 t7'); g.st('R_m', 't1', 0); g.st('R_m', 't2', 1)
    g.e('lit t12 1', 'alu_sub t13 t13 t12', 'lpe t13')
    g.nz2('t9', 'V_r'); g.st('R_st', 't9')
    # q0 = 0 ise 1 sola, E = Ea - Eb - !q0
    g.ld('t4', 'q0'); g.e('lit t5 1', 'alu_xor t4 t4 t5'); g.ld('t1', 'R_m', 0); g.ld('t2', 'R_m', 1)
    g.e('lit t8 63', 'alu_shr t9 t1 t8', 'alu_shl t6 t2 t5', 'alu_or t6 t6 t9', 'alu_shl t7 t1 t5', 'sel t2 t4 t6 t2', 'sel t1 t4 t7 t1'); g.st('R_m', 't1', 0); g.st('R_m', 't2', 1)
    g.ld('t1', 'A_E'); g.ld('t2', 'B_E'); g.e('alu_sub t1 t1 t2', 'alu_sub t1 t1 t4'); g.st('R_E', 't1')
    g.ld('t1', 'A_s'); g.ld('t2', 'B_s'); g.e('alu_xor t1 t1 t2'); g.st('R_s', 't1')
    g.pack(fmt, 'R', 'W')
    special_head(g, fmt)
    g.andf('f_x', 'f_iA', 'f_iB'); g.andf('f_x2', 'f_zA', 'f_zB'); g.orf('f_x', 'f_x', 'f_x2')   # inf/inf , 0/0 -> NaN
    g.mk_inf('V_inf', fmt, 'R_s'); g.mk_zero('V_zz', fmt, 'R_s')
    g.chain('W', [('f_zA', 'V_zz'), ('f_iB', 'V_zz'), ('f_zB', 'V_inf'), ('f_iA', 'V_inf'), ('f_x', 'V_dn'), ('f_nn', 'V_n')])
    g.emit_wide()
    return g
def sqrt(fmt):
    g = G()
    g.unpack('A', fmt, 'b')
    # e0 = E & 1 ; k = E >> 1 (aritmetik) ; Y = M << (127 + e0) (256 bit: y0..y3)
    g.ld('t1', 'A_E'); g.e('lit t2 1', 'alu_and t3 t1 t2', 'alu_sar t4 t1 t2'); g.st('e0', 't3'); g.st('R_E', 't4')
    g.ld('t1', 'A_m', 0); g.ld('t2', 'A_m', 1); g.ld('t3', 'e0'); g.e('lit t5 63', 'lit t6 1', 'lit t7 0')
    g.e('alu_shl t8 t1 t5', 'sel t8 t3 t7 t8'); g.st('y1', 't8')                                   # y1 = e0 ? 0 : m0<<63
    g.e('alu_shr t8 t1 t6', 'alu_shl t9 t2 t5', 'alu_or t8 t8 t9', 'sel t8 t3 t1 t8'); g.st('y2', 't8')   # y2 = e0 ? m0 : (m0>>1)|(m1<<63)
    g.e('alu_shr t8 t2 t6', 'sel t8 t3 t2 t8'); g.st('y3', 't8')                                   # y3 = e0 ? m1 : m1>>1
    g.st('y0', 't7')
    g.c2('R_m', 0, 0)
    def le_cmp(dst):   # dst = (p <= Y)  ; p0..p3, y0..y3 ; t1..t12
        g.ld('t1', 'p3'); g.ld('t2', 'y3'); g.e('cmp_ltu t3 t1 t2', 'cmp_ltu t4 t2 t1', 'alu_or t5 t3 t4')   # t3 p<y , t4 y<p , t5 ayri
        g.ld('t1', 'p2'); g.ld('t2', 'y2'); g.e('cmp_ltu t6 t1 t2', 'cmp_ltu t7 t2 t1', 'alu_or t8 t6 t7')
        g.e('lit t9 1', 'alu_xor t10 t5 t9', 'alu_and t6 t6 t10', 'alu_and t7 t7 t10', 'alu_or t3 t3 t6', 'alu_or t4 t4 t7', 'alu_or t5 t5 t8')
        g.ld('t1', 'p1'); g.ld('t2', 'y1'); g.e('cmp_ltu t6 t1 t2', 'cmp_ltu t7 t2 t1', 'alu_or t8 t6 t7')
        g.e('lit t9 1', 'alu_xor t10 t5 t9', 'alu_and t6 t6 t10', 'alu_and t7 t7 t10', 'alu_or t3 t3 t6', 'alu_or t4 t4 t7', 'alu_or t5 t5 t8')
        g.ld('t1', 'p0'); g.ld('t2', 'y0'); g.e('cmp_ltu t6 t1 t2', 'cmp_ltu t7 t2 t1', 'alu_or t8 t6 t7')
        g.e('lit t9 1', 'alu_xor t10 t5 t9', 'alu_and t6 t6 t10', 'alu_and t7 t7 t10', 'alu_or t3 t3 t6', 'alu_or t4 t4 t7')
        g.e('lit t9 1', 'alu_xor %s t4 t9' % dst)   # p <= Y  <=>  !(Y < p)
    g.e('lit t13 128', 'lpb')
    g.e('lit t1 1', 'alu_sub t2 t13 t1', 'lit t3 63', 'alu_and t4 t2 t3', 'alu_shl t5 t1 t4', 'lit t6 64', 'cmp_ge t7 t2 t6', 'lit t8 0', 'sel t9 t7 t8 t5', 'sel t10 t7 t5 t8')   # t9 = bit alt, t10 = bit ust
    g.st('b0', 't9'); g.st('b1', 't10')
    g.ld('t1', 'R_m', 0); g.ld('t2', 'R_m', 1); g.ld('t3', 'b0'); g.ld('t4', 'b1'); g.e('alu_or t1 t1 t3', 'alu_or t2 t2 t4'); g.st('V_tr', 't1', 0); g.st('V_tr', 't2', 1)
    prod(g, 'V_tr', 'V_tr', 'p')
    le_cmp('t11')
    g.ld('t1', 'V_tr', 0); g.ld('t2', 'V_tr', 1); g.ld('t3', 'R_m', 0); g.ld('t4', 'R_m', 1); g.e('sel t3 t11 t1 t3', 'sel t4 t11 t2 t4'); g.st('R_m', 't3', 0); g.st('R_m', 't4', 1)
    g.e('lit t12 1', 'alu_sub t13 t13 t12', 'lpe t13')
    prod(g, 'R_m', 'R_m', 'p')
    g.ld('t1', 'p0'); g.ld('t2', 'y0'); g.e('alu_xor t3 t1 t2'); g.ld('t1', 'p1'); g.ld('t2', 'y1'); g.e('alu_xor t4 t1 t2', 'alu_or t3 t3 t4'); g.ld('t1', 'p2'); g.ld('t2', 'y2'); g.e('alu_xor t4 t1 t2', 'alu_or t3 t3 t4')
    g.ld('t1', 'p3'); g.ld('t2', 'y3'); g.e('alu_xor t4 t1 t2', 'alu_or t3 t3 t4', 'lit t5 0', 'cmp_ne t3 t3 t5'); g.st('R_st', 't3')
    g.e('lit t1 0'); g.st('R_s', 't1')
    g.pack(fmt, 'R', 'W')
    g.flag('f_nA', 'A', 3); g.flag('f_iA', 'A', 2); g.flag('f_zA', 'A', 0)
    g.ld('t1', 'A_s'); g.ld('t2', 'f_zA'); g.e('lit t3 1', 'alu_xor t2 t2 t3', 'alu_and t1 t1 t2'); g.st('f_neg', 't1')
    g.mk_raw('V_a', 'A'); g.mk_defnan('V_dn', fmt); g.mk_quiet('V_qa', fmt, 'A')
    g.chain('W', [('f_iA', 'V_a'), ('f_zA', 'V_a'), ('f_neg', 'V_dn'), ('f_nA', 'V_qa')])
    g.emit_wide()
    return g
# ======================= donusumler ve karsilastirmalar =======================
from sf import F64
def nanconv(g, name, src, dst, X):
    """X (acilmis, ham sozcukler X_rl/X_rh) NaN -> dst bicimi sessiz NaN (kesir yuku kaydirilir, isaret korunur) ; name: 2 sozcuk degisken"""
    W = {'f64': 52, 'f80': 63, 'f128': 112}; ws, wd = W[src['name']], W[dst['name']]
    g.ld('t1', X + '_rl'); g.ld('t2', X + '_rh')
    if src['name'] == 'f128': g.c('t3', (1 << 48) - 1); g.e('alu_and t2 t2 t3')
    elif src['name'] == 'f80': g.c('t3', (1 << 63) - 1); g.e('alu_and t1 t1 t3', 'lit t2 0')
    else: g.c('t3', (1 << 52) - 1); g.e('alu_and t1 t1 t3', 'lit t2 0')
    k = wd - ws
    if k > 0: g.e('lit t3 %d' % k, 'lit t4 %d' % (64 - k), 'alu_shl t2 t2 t3', 'alu_shr t5 t1 t4', 'alu_or t2 t2 t5', 'alu_shl t1 t1 t3')
    elif k < 0: g.e('lit t3 %d' % -k, 'lit t4 %d' % (64 + k), 'alu_shr t1 t1 t3', 'alu_shl t5 t2 t4', 'alu_or t1 t1 t5', 'alu_shr t2 t2 t3')
    qb = wd - 1   # sessiz bit konumu
    if qb >= 64: g.e('lit t3 1', 'lit t4 %d' % (qb - 64), 'alu_shl t3 t3 t4', 'alu_or t2 t2 t3')
    else: g.e('lit t3 1', 'lit t4 %d' % qb, 'alu_shl t3 t3 t4', 'alu_or t1 t1 t3')
    g.ld('t5', X + '_s')
    if dst['name'] == 'f128': g.e('lit t6 63', 'alu_shl t5 t5 t6', 'alu_or t2 t2 t5'); g.c('t6', 0x7FFF << 48); g.e('alu_or t2 t2 t6')
    elif dst['name'] == 'f80': g.e('lit t6 15', 'alu_shl t5 t5 t6', 'alu_or t2 t2 t5', 'lit t6 32767', 'alu_or t2 t2 t6'); g.c('t6', 1 << 63); g.e('alu_or t1 t1 t6')
    else: g.e('lit t6 63', 'alu_shl t5 t5 t6', 'alu_or t1 t1 t5'); g.c('t6', 0x7FF << 52); g.e('alu_or t1 t1 t6', 'lit t2 0')
    g.st(name, 't1', 0); g.st(name, 't2', 1)
def copy_to_R(g, X='A'):
    for n in ('s', 'E'): g.ld('t1', X + '_' + n); g.st('R_' + n, 't1')
    g.copy2('R_m', X + '_m'); g.e('lit t1 0'); g.st('R_st', 't1')
def flags(g, X='A'):
    g.flag('f_n' + X, X, 3); g.flag('f_i' + X, X, 2); g.flag('f_z' + X, X, 0)
def w2w(src, dst, odd=False, final='wide'):
    """src bicimi (f64/f80/f128) -> dst bicimi (f64/f80/f128)"""
    g = G(); g.unpack('A', src, 'b'); copy_to_R(g); g.pack(dst, 'R', 'W', odd=odd); flags(g)
    g.mk_zero('V_z', dst, 'A_s'); g.mk_inf('V_i', dst, 'A_s'); nanconv(g, 'V_n', src, dst, 'A')
    g.chain('W', [('f_zA', 'V_z'), ('f_iA', 'V_i'), ('f_nA', 'V_n')])
    if final == 'wide': g.emit_wide()
    elif final == 'one': g.emit_one()
    return g
def w2n(src, nar, rnd_sem):
    """genis -> dar (f32/f16/bf16): tek yuvarlama = tek'e yuvarlayarak f64'e, sonra tablodaki RND_<dar> mikro-programi (cifte yuvarlama olmaz)"""
    g = w2w(src, F64, odd=True, final='none')
    g.ld('t1', 'Z', 0); g.st('NV', 't1')
    ops_ = rnd_sem.split(' ; '); assert ops_[0] == 'rd t1 b' and ops_[-1] == 'wr a t3', ops_[0] + ' ... ' + ops_[-1]
    g.e('spld t1 %d' % g.var('NV')); g.e(*ops_[1:])
    return g
def d2w(dst): return w2w(F64, dst)
def cmp_(fmt, kind):
    g = G(); g.unpack('A', fmt, 'b'); g.unpack('B', fmt, 'c'); flags(g, 'A'); flags(g, 'B')
    g.orf('f_nn', 'f_nA', 'f_nB'); g.andf('f_zz', 'f_zA', 'f_zB')
    # isaret (sifirda 0)
    for X in 'AB': g.ld('t1', X + '_s'); g.ld('t2', 'f_z' + X); g.e('lit t3 1', 'alu_xor t2 t2 t3', 'alu_and t1 t1 t2'); g.st('s' + X, 't1')
    g.ld('t1', 'A_c'); g.ld('t2', 'B_c'); g.e('cmp_ltu t3 t1 t2', 'cmp_eq t4 t1 t2', 'lit t5 1', 'cmp_eq t6 t1 t5', 'alu_and t6 t6 t4'); g.st('c_lt', 't3'); g.st('c_eq', 't4'); g.st('c_f', 't6')   # c_f: ikisi de sonlu sifirdan farkli
    g.ld('t1', 'A_E'); g.ld('t2', 'B_E'); g.e('cmp_lt t3 t1 t2', 'cmp_eq t4 t1 t2'); g.st('e_lt', 't3'); g.st('e_eq', 't4')
    g.ltu2('t7', 'A_m', 'B_m'); g.st('m_lt', 't7'); g.ld('t1', 'A_m', 0); g.ld('t2', 'B_m', 0); g.ld('t3', 'A_m', 1); g.ld('t4', 'B_m', 1); g.e('alu_xor t1 t1 t2', 'alu_xor t3 t3 t4', 'alu_or t1 t1 t3', 'lit t2 0', 'cmp_eq t1 t1 t2'); g.st('m_eq', 't1')
    # buyukluk: ml = c_lt | c_f & (e_lt | e_eq & m_lt) ; me = c_eq & (!c_f | e_eq & m_eq)
    g.ld('t1', 'e_eq'); g.ld('t2', 'm_lt'); g.e('alu_and t1 t1 t2'); g.ld('t3', 'e_lt'); g.e('alu_or t1 t1 t3'); g.ld('t4', 'c_f'); g.e('alu_and t1 t1 t4'); g.ld('t5', 'c_lt'); g.e('alu_or t1 t1 t5'); g.st('ml', 't1')
    g.ld('t1', 'e_eq'); g.ld('t2', 'm_eq'); g.e('alu_and t1 t1 t2'); g.ld('t4', 'c_f'); g.e('lit t5 1', 'alu_xor t4 t4 t5', 'alu_or t1 t1 t4'); g.ld('t6', 'c_eq'); g.e('alu_and t1 t1 t6'); g.st('me', 't1')
    # esit: !nan & (sifir&sifir | isaret ayni & me)
    g.ld('t1', 'sA'); g.ld('t2', 'sB'); g.e('alu_xor t3 t1 t2', 'lit t4 1', 'alu_xor t3 t3 t4'); g.ld('t5', 'me'); g.e('alu_and t3 t3 t5'); g.ld('t6', 'f_zz'); g.e('alu_or t3 t3 t6'); g.ld('t7', 'f_nn'); g.e('alu_xor t7 t7 t4', 'alu_and t3 t3 t7'); g.st('r_eq', 't3')
    # kucuk: !nan & ( sA&!sB | !sA&!sB&ml | sA&sB&mg ) ; mg = !ml & !me
    g.ld('t1', 'sA'); g.ld('t2', 'sB'); g.e('lit t4 1', 'alu_xor t5 t1 t4', 'alu_xor t6 t2 t4')   # t5 = !sA , t6 = !sB
    g.e('alu_and t7 t1 t6')                                                                    # sA & !sB
    g.ld('t8', 'ml'); g.e('alu_and t9 t5 t6', 'alu_and t9 t9 t8', 'alu_or t7 t7 t9')
    g.ld('t3', 'me'); g.e('alu_or t3 t3 t8', 'alu_xor t3 t3 t4', 'alu_and t3 t3 t1', 'alu_and t3 t3 t2', 'alu_or t7 t7 t3')   # sA&sB&mg
    g.ld('t9', 'f_nn'); g.e('alu_xor t9 t9 t4', 'alu_and t7 t7 t9'); g.st('r_lt', 't7')
    g.ld('t1', 'r_eq'); g.ld('t2', 'r_lt')
    if kind == 'eq': pass
    elif kind == 'ne': g.e('lit t3 1', 'alu_xor t1 t1 t3')
    elif kind == 'lt': g.e('alu_or t1 t2 t2')
    else: g.e('alu_or t1 t1 t2')
    g.e('wr a t1')
    return g
def neg_abs(fmt, kind):
    g = G(); hi = 15 if fmt['ex'] else 63
    g.e('rdw t1 b 0', 'rdw t2 b 1', 'lit t3 1', 'lit t4 %d' % hi, 'alu_shl t3 t3 t4')
    if kind == 'neg': g.e('alu_xor t2 t2 t3')
    else: g.e('un_not t3 t3', 'alu_and t2 t2 t3')
    g.e('wrw a 0 t1', 'wrw a 1 t2')
    return g
def tobool(fmt):
    g = G(); g.unpack('A', fmt, 'b'); g.flag('f_zA', 'A', 0); g.ld('t1', 'f_zA'); g.e('lit t2 1', 'alu_xor t1 t1 t2', 'wr a t1')
    return g
def i2w(fmt, signed):
    """slot b: 64 bit tamsayi (isaretli ya da isaretsiz) -> fmt"""
    g = G(); g.e('rd t1 b')
    if signed: g.e('lit t2 63', 'alu_shr t3 t1 t2', 'un_neg t4 t3', 'alu_xor t1 t1 t4', 'alu_add t1 t1 t3')   # t3 = isaret ; t1 = |x|
    else: g.e('lit t3 0')
    g.st('R_s', 't3'); g.e('lit t5 0'); g.st('R_m', 't5', 0); g.st('R_m', 't1', 1); g.e('lit t6 63'); g.st('R_E', 't6'); g.e('lit t5 0'); g.st('R_st', 't5')
    g.e('alu_or t7 t1 t1', 'lit t8 0', 'cmp_eq t7 t7 t8'); g.st('f_zA', 't7')
    g.normalize('R'); g.pack(fmt, 'R', 'W')
    g.c2('V_z', 0, 0)
    g.chain('W', [('f_zA', 'V_z')]); g.emit_wide()
    return g
def i128_2w(fmt, signed):
    """slot b: 128 bit tamsayi (2 sozcuk) -> fmt"""
    g = G(); g.e('rdw t1 b 0', 'rdw t2 b 1')
    if signed:
        g.e('lit t3 63', 'alu_shr t3 t2 t3', 'un_neg t4 t3', 'alu_xor t1 t1 t4', 'alu_xor t2 t2 t4', 'alu_add t1 t1 t3', 'cmp_ltu t5 t1 t3', 'alu_add t2 t2 t5')
    else: g.e('lit t3 0')
    g.st('R_s', 't3'); g.st('R_m', 't1', 0); g.st('R_m', 't2', 1); g.e('lit t6 127'); g.st('R_E', 't6'); g.e('lit t5 0'); g.st('R_st', 't5')
    g.e('alu_or t7 t1 t2', 'lit t8 0', 'cmp_eq t7 t7 t8'); g.st('f_zA', 't7')
    g.normalize('R'); g.pack(fmt, 'R', 'W'); g.c2('V_z', 0, 0)
    g.chain('W', [('f_zA', 'V_z')]); g.emit_wide()
    return g
def w2i(fmt, bits, signed, narrow=None):
    """fmt -> 64 ya da 128 bit tamsayi (kes). gecersiz (NaN/sonsuz/aralik disi): isaretli -> 0x80..0 , isaretsiz -> 0"""
    g = G(); g.unpack('A', fmt, 'b'); flags(g)
    # tam sayi kismi: M >> (127 - E) , 0 <= E <= 127 ; E < 0 -> 0
    g.ld('t1', 'A_E'); g.e('lit t2 127', 'alu_sub t3 t2 t1', 'lit t4 0', 'cmp_lt t5 t1 t4', 'cmp_gt t6 t1 t2', 'sel t3 t5 t4 t3'); g.st('cnt', 't3'); g.st('e_neg', 't5'); g.st('e_big', 't6')
    g.copy2('V_x', 'A_m'); g.e('lit t1 0'); g.st('st', 't1')
    shr_sticky(g, 'V_x', 'cnt', 'st', 128)
    # E < 0 ise 0 (kaydirma 127 de 1 kalir: sifirla)
    g.ld('t5', 'e_neg'); g.c2('V_zero', 0, 0); g.sel2('V_x', 't5', 'V_zero', 'V_x')
    g.ld('t1', 'V_x', 0); g.ld('t2', 'V_x', 1); g.ld('t3', 'A_s'); g.e('un_neg t4 t3', 'alu_xor t1 t1 t4', 'alu_xor t2 t2 t4', 'alu_add t1 t1 t3', 'cmp_ltu t5 t1 t3', 'alu_add t2 t2 t5'); g.st('V_i', 't1', 0); g.st('V_i', 't2', 1)   # isaret uygulanmis
    # gecerlilik (t1 alt, t2 ust, t3 isaret)
    g.ld('t1', 'V_x', 0); g.ld('t2', 'V_x', 1); g.ld('t3', 'A_s'); g.e('lit t4 0', 'lit t5 1', 'lit t8 63', 'lit t9 1', 'alu_shl t9 t9 t8')   # t9 = 2^63
    g.e('alu_or t6 t1 t2', 'cmp_ne t6 t6 t4')   # t6 = buyukluk != 0
    if bits == 64 and signed:
        g.e('cmp_ne t7 t2 t4', 'alu_shr t10 t1 t8', 'cmp_eq t11 t1 t9', 'alu_and t11 t11 t3', 'alu_xor t11 t11 t5', 'alu_and t10 t10 t11', 'alu_or t7 t7 t10')
    elif bits == 64:
        g.e('cmp_ne t7 t2 t4', 'alu_and t10 t3 t6', 'alu_or t7 t7 t10')
    elif signed:
        g.e('alu_shr t10 t2 t8', 'cmp_eq t11 t2 t9', 'cmp_eq t12 t1 t4', 'alu_and t11 t11 t12', 'alu_and t11 t11 t3', 'alu_xor t11 t11 t5', 'alu_and t7 t10 t11')
    else:
        g.e('alu_and t7 t3 t6')
    g.ld('t6', 'e_big'); g.e('alu_or t7 t7 t6'); g.flag('f_n', 'A', 3); g.flag('f_i', 'A', 2); g.ld('t8', 'f_n'); g.ld('t9', 'f_i'); g.e('alu_or t7 t7 t8', 'alu_or t7 t7 t9')
    g.st('bad', 't7')
    if signed: g.c2('V_ind', 0, 1 << 63) if bits == 128 else g.c2('V_ind', 1 << 63, 0)
    else: g.c2('V_ind', 0, 0)
    g.ld('t9', 'bad'); g.sel2('Z', 't9', 'V_ind', 'V_i')
    if bits == 64:
        if narrow:   # (genislik, isaretli): once int64'e kes (gecersiz -> 0x80..0 / 0) sonra hedef araligina sar (motor tanimi)
            nb, ns = narrow; g.ld('t1', 'Z', 0); g.e('%s%d t1 t1' % ('sx' if ns else 'zx', nb), 'wr a t1')
        else: g.emit_one()
    else: g.emit_wide()
    return g
