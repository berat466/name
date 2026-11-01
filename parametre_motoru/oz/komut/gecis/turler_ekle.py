#!/usr/bin/env python3
"""TYPES bolumu (PMK1 bolum 10) uretici: tablo okunabilir gorunumune 'section 10 ...' satirini ekler/degistirir (tek seferlik gecis betigi; tablonun kendisi kanonik kaynak kalir).
Limit degerleri IEEE-754 / ikili tamsayi bicim kurallarindan ANALITIK hesaplanir (derleyici kullanilmaz); dogrula_turler.sh bunlari C++ std::numeric_limits ile karsilastirir.
Kullanim: turler_ekle.py komutlar.pmk komut.pmb pm  > yeni.gor (stdout: okunabilir gorunum)"""
import sys, subprocess
tab, komut, pm = sys.argv[1:4]
def run(mode, data):
    return subprocess.run([pm, 'run', komut], input=('@@%s\n' % mode).encode() + data, capture_output=True, check=True).stdout
gor = run('gor', open(tab, 'rb').read()).decode().splitlines()
M64 = (1 << 64) - 1
def sw(v):  # 64 bit isaretli sozcuk
    v &= M64
    return v - (1 << 64) if v >> 63 else v
def pair(v):  # 128 bitlik desen -> (lo, hi)
    return [sw(v), sw(v >> 64)]
def intlim(bits, signed):
    if signed:
        mx = (1 << (bits - 1)) - 1; mn = -(1 << (bits - 1))
    else:
        mx = (1 << bits) - 1; mn = 0
    mn &= (1 << 128) - 1 if bits > 64 else M64
    # iki sozcuk: alt, ust (ust = isaret genislemesi)
    def two(v, neg):
        if bits > 64: return pair(v)
        return [sw(v), -1 if neg else 0]
    return two(mn, signed and mn != 0), two(mx, False)
def flim(eb, mb, explicit):
    bias = (1 << (eb - 1)) - 1
    sign = 1 << (eb + mb)
    if not explicit:
        mx = ((1 << eb) - 2) << mb | ((1 << mb) - 1); tiny = 1 << mb; eps = (bias - mb) << mb; dmin = 1
    else:  # 80 bit genisletilmis: mb = 64 (tam sayi biti dahil)
        mx = (((1 << eb) - 2) << mb) | ((1 << mb) - 1); tiny = (1 << mb) | (1 << (mb - 1)); eps = ((bias - (mb - 1)) << mb) | (1 << (mb - 1)); dmin = 1
        tiny = (1 << mb) | (1 << (mb - 1)) if False else (1 << mb) | (1 << (mb - 1))
        tiny = (1 << mb) | (1 << (mb - 1))
    return [mx | sign, mx, tiny, eps, dmin]
def name_words(n): return [len(n)] + [ord(c) for c in n]
# sinif: 1 tamsayi 2 bool 3 karakter 4 bayt 5 kayan 6 karmasik 7 nullptr
T = []  # (id, ad, sinif, bit, isaret, boyut, hiza, nsoz, eb, mb, explicit)
def add(ad, cls, bits, sg, size, al, nw, eb=0, mb=0, ex=False): T.append((len(T) + 1, ad, cls, bits, sg, size, al, nw, eb, mb, ex))
for b in (8, 16, 32, 64): add('i%d' % b, 1, b, 1, b // 8, b // 8, 1)
add('i128', 1, 128, 1, 16, 16, 2)
for b in (8, 16, 32, 64): add('u%d' % b, 1, b, 0, b // 8, b // 8, 1)
add('u128', 1, 128, 0, 16, 16, 2)
add('bool', 2, 1, 0, 1, 1, 1)
add('char', 3, 8, 1, 1, 1, 1); add('schar', 3, 8, 1, 1, 1, 1); add('uchar', 3, 8, 0, 1, 1, 1)
add('wchar', 3, 32, 1, 4, 4, 1); add('char8', 3, 8, 0, 1, 1, 1); add('char16', 3, 16, 0, 2, 2, 1); add('char32', 3, 32, 0, 4, 4, 1)
add('byte', 4, 8, 0, 1, 1, 1)
add('nullptr', 7, 64, 0, 8, 8, 1)
add('f16', 5, 16, 1, 2, 2, 1, 5, 10); add('bf16', 5, 16, 1, 2, 2, 1, 8, 7); add('f32', 5, 32, 1, 4, 4, 1, 8, 23); add('f64', 5, 64, 1, 8, 8, 1, 11, 52)
add('f80', 5, 80, 1, 16, 16, 2, 15, 64, True); add('f128', 5, 128, 1, 16, 16, 2, 15, 112)
add('c64', 6, 64, 1, 8, 4, 2, 8, 23); add('c128', 6, 128, 1, 16, 8, 2, 11, 52); add('c160', 6, 160, 1, 32, 16, 4, 15, 64, True)
rec = []
for (i, ad, cls, bits, sg, size, al, nw, eb, mb, ex) in T:
    if cls in (1, 2, 3, 4):
        mn, mx = intlim(bits, sg); lim = mn + mx + [0] * 6
    elif cls == 5:
        l = flim(eb, mb, ex); lim = []
        for v in l: lim += pair(v)
    else:
        lim = [0] * 10
    body = [1, i] + name_words(ad) + [cls, bits, sg, size, al, nw, 0, eb, mb] + lim + [0]
    rec.append([len(body)] + body)
# ABI 1 = Linux x86-64 (LP64): C tur adi -> tur
abi = {'char': 'char', 'signed char': 'schar', 'unsigned char': 'uchar', 'short': 'i16', 'unsigned short': 'u16', 'int': 'i32', 'unsigned int': 'u32',
       'long': 'i64', 'unsigned long': 'u64', 'long long': 'i64', 'unsigned long long': 'u64', 'float': 'f32', 'double': 'f64', 'long double': 'f80',
       'bool': 'bool', 'wchar_t': 'wchar', 'char8_t': 'char8', 'char16_t': 'char16', 'char32_t': 'char32', 'std::byte': 'byte', 'std::nullptr_t': 'nullptr',
       'void*': 'u64', 'size_t': 'u64', 'ptrdiff_t': 'i64', 'intptr_t': 'i64', 'uintptr_t': 'u64', 'int8_t': 'i8', 'int16_t': 'i16', 'int32_t': 'i32', 'int64_t': 'i64',
       'uint8_t': 'u8', 'uint16_t': 'u16', 'uint32_t': 'u32', 'uint64_t': 'u64', '__int128': 'i128', 'unsigned __int128': 'u128', '_Float16': 'f16', '__bf16': 'bf16',
       '__float128': 'f128', 'float _Complex': 'c64', 'double _Complex': 'c128', 'long double _Complex': 'c160'}
ids = {ad: i for (i, ad, *_r) in T}
for cn, tn in abi.items():
    body = [3, 1] + name_words(cn) + [ids[tn], 0]
    rec.append([len(body)] + body)
# ---- DONUSUM matrisi (tamsayi benzeri siniflar: tamsayi, bool, karakter, bayt; 128 bit ve kayan nokta sonraki adimlarda): her (kaynak,hedef) cifti icin komut ya da -1 (ozdes)
opid = {}
for l in gor:
    t = l.split()
    if len(t) >= 3 and t[0] == 'op': opid[t[2].lower()] = int(t[1])
byname = {t[1]: t for t in T}
def lims(t):
    _, ad, cls, bits, sg = t[:5]
    if cls == 2: return 0, 1
    return (-(1 << (bits - 1)), (1 << (bits - 1)) - 1) if sg else (0, (1 << bits) - 1)
def conv_op(f, t):
    if t[2] == 2: return -1 if f[2] == 2 else opid['tobool']
    if f[2] == 2: return -1
    tb, ts, fb, fs = t[3], t[4], f[3], f[4]
    if tb == 64: return -1
    if (fs == ts and fb <= tb) or (fs == 0 and ts == 1 and fb < tb): return -1
    return opid[('sext%d' if ts else 'zext%d') % tb]
conv = []
INTLIKE = (1, 2, 3, 4)
for f in T:
    for t in T:
        if f[2] not in INTLIKE or t[2] not in INTLIKE or f[3] > 64 or t[3] > 64: continue
        fm, fx = lims(f); tm, tx = lims(t)
        flags = 1 if (tm <= fm and fx <= tx) else 2
        body = [2, f[0], t[0], conv_op(f, t), flags, 0]
        conv.append([len(body)] + body)
# kayan nokta (f16 bf16 f32 f64; deger double olarak tam saklanir): tamsayi <-> kayan ve kayan <-> kayan
FL = [t for t in T if t[1] in ('f16', 'bf16', 'f32', 'f64')]
FMT = {'f16': (11, 5), 'bf16': (8, 8), 'f32': (24, 8), 'f64': (53, 11)}   # (anlamli bit sayisi, us biti)
def fl_exact(f, t):   # f -> t kayan: t'nin anlamli biti ve us araligi f'yi kapsiyor mu
    return FMT[t[1]][0] >= FMT[f[1]][0] and FMT[t[1]][1] >= FMT[f[1]][1]
for i_ in T:
    if i_[2] not in INTLIKE or i_[3] > 64: continue
    for f_ in FL:
        # tamsayi -> kayan
        u64src = (i_[2] != 2 and i_[4] == 0 and i_[3] == 64)
        o = (opid['u2f'] if u64src else opid['i2f']) if f_[1] == 'f64' else opid[('u2f_%s' if u64src else 'i2f_%s') % f_[1]]
        body = [2, i_[0], f_[0], o, 1 if (i_[3] < FMT[f_[1]][0] or (i_[2] == 2)) else 2, 0]
        conv.append([len(body)] + body)
        # kayan -> tamsayi
        if i_[2] == 2: o = opid['ftobool']
        elif i_[3] == 64: o = opid['f2u'] if i_[4] == 0 else opid['f2i']
        else: o = opid[('f2i_i%d' if i_[4] else 'f2i_u%d') % i_[3]]
        body = [2, f_[0], i_[0], o, 2, 0]
        conv.append([len(body)] + body)
for f_ in FL:
    for t_ in FL:
        o = -1 if (f_ is t_ or fl_exact(f_, t_)) else opid['rnd_' + t_[1]]
        body = [2, f_[0], t_[0], o, 1 if o == -1 else 2, 0]
        conv.append([len(body)] + body)

# ---- OPSIG (etiket 4) ve HOSTSIG (etiket 5): komutlarin program duzeyi tur imzalari. Alan kodlari (yuva a,b,c = OP bicimindeki operand sirasi):
#   0 yok/yazmac degil  >0 kesin tur kimligi  -1 INT1 (tamsayi benzeri 1 sozcuk)  -2 FLT1 (kayan 1 sozcuk)  -3 ANY1 (herhangi 1 sozcuk)  -4/-5/-6 a/b/c yuvasiyla AYNI tur
#   -7 ADDR (i64|u64)  -8 FREE (hedef: bildirilmis tur serbest, 1 sozcuk)  -9 INTDST (hedef tamsayi benzeri; akis izlemede i64)  -10 INT64 (i64|u64)
# kural: 1 = DONUSUM (b yuvasi turu -> a yuvasi turu icin TYPES'ta bu komutu (ya da CAST icin -1'i) kaydeden bir donusum olmali)
NONE, INT1, FLT1, ANY1, SAME_A, SAME_B, SAME_C, ADDR, FREE, INTDST, INT64 = 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10
tid = {t[1]: t[0] for t in T}
opfmt = {}
for l in gor:
    t = l.split()
    if len(t) >= 5 and t[0] == 'op': opfmt[t[2]] = t[4]
def sig(name, fmt):
    n = name.upper(); k = fmt.split(',') if fmt != '-' else []
    I = lambda x: tid[x]
    if n == 'MOV': return (SAME_B, ANY1, NONE, 0)
    if n == 'CAST': return (FREE, ANY1, NONE, 1)
    if n == 'LDI': return (INTDST, NONE, NONE, 0)
    if n == 'LDK': return (FREE, NONE, NONE, 0)
    if n in ('ADD', 'SUB', 'MUL', 'AND', 'OR', 'XOR'): return (SAME_B, INT64, SAME_B, 0)
    if n in ('DIV', 'MOD'): return (I('i64'), I('i64'), I('i64'), 0)
    if n == 'SHL': return (SAME_B, INT64, INT1, 0)
    if n == 'SHR': return (I('u64'), I('u64'), INT1, 0)
    if n == 'SAR': return (I('i64'), I('i64'), INT1, 0)
    if n in ('EQ', 'NE'): return (I('bool'), INT64, SAME_B, 0)
    if n in ('LT', 'LE'): return (I('bool'), I('i64'), I('i64'), 0)
    if n in ('NEG', 'NOT'): return (SAME_B, INT64, NONE, 0)
    if n in ('FADD', 'FSUB', 'FMUL', 'FDIV'): return (I('f64'), I('f64'), I('f64'), 0)
    if n in ('FNEG', 'FSQRT'): return (I('f64'), I('f64'), NONE, 0)
    if n in ('FEQ', 'FLT', 'FLE'): return (I('bool'), FLT1, SAME_B, 0)
    if n == 'I2F': return (I('f64'), I('i64'), NONE, 0)
    if n == 'F2I': return (I('i64'), I('f64'), NONE, 0)
    if n == 'JMP': return (NONE, NONE, NONE, 0)
    if n in ('JZ', 'JNZ'): return (INT1, NONE, NONE, 0)
    if n in ('CALL', 'RET', 'HALT', 'HOST'): return None   # ozel kurallar (denetleyicide)
    if n == 'LD': return (FREE, ADDR, NONE, 0)
    if n == 'ST': return (ANY1, ADDR, NONE, 0)
    if n == 'ALLOC': return (I('u64'), INT64, NONE, 0)
    if n in ('ADDI', 'MULI', 'ANDI', 'SHLI', 'SHRI', 'SARI'): return (SAME_B, INT64, NONE, 0)
    if n in ('DIVI', 'MODI'): return (I('i64'), I('i64'), NONE, 0)
    if n in ('EQI', 'NEI', 'LTI', 'LEI', 'GTI', 'GEI'): return (I('bool'), INT64, NONE, 0)
    if n in ('SEXT8', 'SEXT16', 'SEXT32'): return (I('i' + n[4:]), INT1, NONE, 1)
    if n in ('ZEXT8', 'ZEXT16', 'ZEXT32'): return (I('u' + n[4:]), INT1, NONE, 1)
    import re
    m = re.match(r'^(ADD|SUB|MUL|SHL|NEG|NOT|DIV|MOD)_([IU])(8|16|32)$', n)
    if m:
        ty = I(m.group(2).lower() + m.group(3))
        if m.group(1) in ('NEG', 'NOT'): return (ty, ty, NONE, 0)
        if m.group(1) == 'SHL': return (ty, ty, INT1, 0)
        return (ty, ty, ty, 0)
    if n in ('DIVU', 'MODU'): return (I('u64'), I('u64'), I('u64'), 0)
    if n in ('LTU', 'LEU'): return (I('bool'), I('u64'), I('u64'), 0)
    if n == 'U2F': return (I('f64'), I('u64'), NONE, 0)
    if n == 'F2U': return (I('u64'), I('f64'), NONE, 0)
    if n == 'TOBOOL': return (I('bool'), INT1, NONE, 1)
    if n == 'BNOT': return (I('bool'), INT1, NONE, 0)
    if n in ('BAND', 'BOR', 'BXOR'): return (I('bool'), INT1, INT1, 0)
    if n == 'NULLB': return (I('bool'), I('nullptr'), NONE, 0)
    m = re.match(r'^(RND|ADD|SUB|MUL|DIV|SQRT|I2F|U2F)_(F32|F16|BF16)$', n)
    if m:
        ty = I(m.group(2).lower())
        if m.group(1) == 'RND': return (ty, FLT1, NONE, 1)
        if m.group(1) in ('I2F', 'U2F'): return (ty, INT1, NONE, 1)
        if m.group(1) == 'SQRT': return (ty, ty, NONE, 0)
        return (ty, ty, ty, 0)
    m = re.match(r'^F2I_([IU])(8|16|32)$', n)
    if m: return (I(m.group(1).lower() + m.group(2)), FLT1, NONE, 1)
    if n == 'FTOBOOL': return (I('bool'), FLT1, NONE, 1)
    m = re.match(r'^BSWAP(16|32|64)$', n)
    if m: return (I('u' + m.group(1)), I('u' + m.group(1)), NONE, 0)
    return 'EKSIK'
sigrec = []; eksik = []
for l in gor:
    t = l.split()
    if len(t) >= 5 and t[0] == 'op':
        r = sig(t[2], t[4])
        if r == 'EKSIK': eksik.append(t[2]); continue
        if r is None: continue
        body = [4, int(t[1]), r[0], r[1], r[2], r[3], 0]
        sigrec.append([len(body)] + body)
if eksik: sys.exit('turler_ekle: imzasi tanimsiz komutlar: ' + ' '.join(eksik))
# HOSTSIG: [5, host_kimligi, donus_alani, arg0, arg1, arg2, ne]  (arg i = blok yazmaci b+i ; donus = b yazmaci)
HS = {'putc': (NONE, INT1, NONE, NONE), 'puti': (NONE, INT1, NONE, NONE), 'putf': (NONE, FLT1, NONE, NONE), 'getc': (I_ := tid['i64'], NONE, NONE, NONE),
      'present': (NONE, ADDR, INT1, INT1), 'poll': (tid['i64'], NONE, NONE, NONE), 'frame': (NONE, NONE, NONE, NONE)}
for l in gor:
    t = l.split()
    if len(t) >= 3 and t[0] == 'host' and t[2] in HS:
        r = HS[t[2]]; body = [5, int(t[1]), r[0], r[1], r[2], r[3], 0]
        sigrec.append([len(body)] + body)
# nullptr_t: bool'a (her zaman false) ve isaretsiz 64 bit gostericiye (ozdes)
nl = byname['nullptr']
for t in (byname['bool'], byname['u64']):
    body = [2, nl[0], t[0], opid['nullb'] if t[2] == 2 else -1, 1 if t[2] != 2 else 2, 0]
    conv.append([len(body)] + body)
words = [len(rec) + len(conv) + len(sigrec)]
for r in rec + conv + sigrec: words += r
line = 'section 10 0 1 ' + ' '.join(str(w) for w in words)
out = [l for l in gor if not l.startswith('section 10 ')] + [line]
print('\n'.join(out))
