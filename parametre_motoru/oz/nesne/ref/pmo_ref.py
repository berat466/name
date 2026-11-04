#!/usr/bin/env python3
# PMO1 okunabilir gorunum (PMV) BAGIMSIZ REFERANSI: oz/arac/gorunum.pmb (PMA araci) bu betikle bayt-bayt karsilastirilir.
# Kullanim:  pmo_ref.py gor  SOZLUK.txt NESNE.pmo [ADLAR.txt]  -> gorunum (stdout)
#            pmo_ref.py yaz  SOZLUK.txt GORUNUM.pmv           -> PMO1 (stdout)
#            pmo_ref.py adlar GORUNUM.pmv                     -> islev adlari (satir satir)
# SOZLUK.txt: `komut @@tab` ciktisi (OPNAMES NOPS KIND KFLAGS HOSTCOUNT OPCOUNT ...).  Hata: "HATA: kod" (stdout ilk satir), cikis kodu 1.
import sys, re

S1 = 1845493760
S2 = 1879048192
MAGIC = 1347243825
MAXKEY = 4096
MAXREL = 65536
MAXTOK = 524288
MAXFN = 4096

class Err(Exception):
    def __init__(self, code): self.code = code

def die(code): raise Err(code)

def ident_ok(s): return re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*', s) is not None

# ---------------------------------------------------------------- sozluk
class Dic:
    def __init__(self, text):
        d = {}
        for ln in text.split('\n'):
            p = ln.split()
            if not p: continue
            try: d[p[0]] = [int(x) for x in p[1:] if re.fullmatch(r'-?[0-9]+', x)]
            except ValueError: pass
        for k in ('OPCOUNT', 'OPNAMES', 'NOPS', 'KIND', 'KFLAGS', 'HOSTCOUNT'):
            if k not in d: die(2)
        self.nop = d['OPCOUNT'][0]; self.nhost = d['HOSTCOUNT'][0]
        names, cur = [], ''
        for c in d['OPNAMES']:
            if c == 0: names.append(cur); cur = ''
            else: cur += chr(c)
        if len(names) < self.nop + self.nhost or len(d['NOPS']) < self.nop or len(d['KIND']) < 3 * self.nop: die(2)
        self.ops = names[:self.nop]; self.hosts = names[self.nop:self.nop + self.nhost]
        self.nops = d['NOPS'][:self.nop]
        self.kinds = [d['KIND'][3 * i:3 * i + 3] for i in range(self.nop)]
        self.kfl = d['KFLAGS']
        self.opid = {n: i for i, n in reversed(list(enumerate(self.ops)))}
        self.hostid = {n: i for i, n in reversed(list(enumerate(self.hosts)))}
    def flags(self, kind): return self.kfl[kind] if 0 <= kind < len(self.kfl) else 0

# sinif: 'r' yazmac, 'i' dogrudan sayi, 'l' etiket, 'F' islev, 'k' sabit havuzu, 'h' host, 'n' duz sayi
def opclass(dic, op, j):
    f = dic.flags(dic.kinds[op][j])
    if f & 3: return 'r'
    if f & 4: return 'i'
    if f & 8: return 'l'
    if f & 16: return 'F'
    if f & 32: return 'k'
    if f & 128: return 'h'
    return 'n'

# ---------------------------------------------------------------- yardimcilar
def u64(v): return v & 0xFFFFFFFFFFFFFFFF
def s64(v):
    v &= 0xFFFFFFFFFFFFFFFF
    return v - (1 << 64) if v >> 63 else v

def parse_int(tok, unsigned=False, ecs=30, ecr=31):
    # kanonik ondalik: -?(0|[1-9][0-9]*); 64 bit araligi
    if not re.fullmatch(r'-?(0|[1-9][0-9]*)', tok) or tok == '-0': die(ecs)
    v = int(tok)
    if unsigned:
        if v < 0 or v >= 1 << 64: die(ecr)
    elif v < -(1 << 63) or v >= 1 << 63: die(ecr)
    return v

# ---------------------------------------------------------------- gor: PMO1 -> gorunum
def gor(dic, pmo_text, adlar=None):
    spaces = []
    if adlar is not None: adlar, spaces = adlar
    T = []
    for x in pmo_text.split():
        if not re.fullmatch(r'-?(0|[1-9][0-9]*)', x) or x == '-0': die(3)
        v = int(x)
        if v < -(1 << 63) or v >= 1 << 64: die(3)
        T.append(v)
    if len(T) < 4: die(3)
    if T[0] != MAGIC or T[1] != 1: die(4)
    nk, nb = T[2], T[3]
    if nk < 0 or nk > MAXKEY or nb < 0 or nb > MAXREL: die(5)
    p = 4
    keys = []
    for i in range(nk):
        if p + 3 > len(T): die(3)
        kd, base, ln = T[p:p + 3]; p += 3
        if ln < 1 or ln > 64 or p + ln > len(T): die(3)
        try: nm = ''.join(chr(c) for c in T[p:p + ln])
        except (ValueError, OverflowError): die(7)
        p += ln
        if not ident_ok(nm) or any(c < 32 or c > 126 for c in map(ord, nm)): die(7)
        if kd == 0:
            if base != S1 + i: die(6)
        elif kd == 1:
            if base != 1: die(6)
        else: die(6)
        keys.append((kd, nm))
    if len({n for _, n in keys}) != nk: die(7)
    if 'bss' in {n for _, n in keys}: die(7)
    rel = []
    for i in range(nb):
        if p + 3 > len(T): die(3)
        rel.append(tuple(T[p:p + 3])); p += 3
    L = T[p:]
    if len(L) > MAXTOK: die(5)
    reloc = {}                                   # konum -> {anahtar: katsayi}
    for ps, k, c in rel:
        if ps < 0 or ps >= len(L) or k < 0 or k >= nk or c == 0: die(8)
        if keys[k][0] != 1: die(9)               # skaler anahtarlarin bagil kaydi gorunumde yok (yer tutucu mekanizmasi)
        reloc.setdefault(ps, {})
        if k in reloc[ps]: die(9)
        reloc[ps][k] = c
    # ---- listeyi coz (konum izleyerek)
    pos = [0]
    def nxt(u=False):
        if pos[0] >= len(L): die(3)
        v = L[pos[0]]; pos[0] += 1
        if v >= 1 << 63 and not u: die(12)        # 2^63 ve ustu yalniz sabit havuzunda gecerli
        return v, pos[0] - 1
    def sentinel(v):
        # (tur, anahtar) ya da None. skaler: S1+no ya da -(S1+no); dizi: S2+no*65536
        for sg, w in ((1, v), (-1, -v)):
            d = w - S1
            if 0 <= d < nk and keys[d][0] == 0: return ('s', d, sg)
        d = v - S2
        if d >= 0 and d % 65536 == 0 and d // 65536 < nk and keys[d // 65536][0] == 1: return ('a', d // 65536, 1)
        return None
    used = set()
    flag, _ = nxt(); mem, _ = nxt(); entry, _ = nxt()
    if flag not in (0, 1) or mem < 0 or entry < 0: die(12)
    ncon, _ = nxt()
    if ncon < 0 or ncon > MAXTOK: die(12)
    consts = []
    for _ in range(ncon):
        v, ps = nxt(True)
        if ps in reloc or v < 0: die(8 if ps in reloc else 12)
        consts.append(v)
    ndata_pos = pos[0]
    ndata, _ = nxt()
    if ndata < 0 or ndata > MAXTOK: die(12)
    data = []                                     # ('a', k) | ('w', v, pos)
    heads = {}
    for _ in range(ndata):
        v, ps = nxt()
        if ps in reloc: die(8)
        sn = sentinel(v)
        if sn and sn[0] == 'a':
            if sn[1] in heads: die(11)
            heads[sn[1]] = len(data); data.append(('a', sn[1]))
        else: data.append(('w', v))
    bss, _ = nxt()
    nf, _ = nxt()
    if bss < 0 or nf < 0 or nf > MAXTOK: die(12)
    if nf > MAXFN: die(5)
    # veri sayisi konumundaki bagil kayitlar: tam olarak basliklar (katsayi = tekrar) olmali
    want = {}
    for kk in heads: want[kk] = 1
    if reloc.get(ndata_pos, {}) != want: die(8)
    if want: used.add(ndata_pos)
    # basliklar sirasi
    hlist = [(i, d[1]) for i, d in enumerate(data) if d[0] == 'a']
    def pre(i): return {k: 1 for (j, k) in hlist if j < i}
    # ---- islevler
    funcs = []
    for _ in range(nf):
        np_, _ = nxt(); nr, _ = nxt(); nc, _ = nxt()
        if np_ < 0 or nr < 0 or nc < 0 or nc > MAXTOK: die(12)
        code = []
        for _ in range(nc):
            op, ps0 = nxt()
            if op < 0 or op >= dic.nop: die(12)
            if ps0 in reloc: die(8)
            opnd = []
            for j in range(dic.nops[op]):
                v, ps = nxt(); opnd.append((v, ps))
                if not (-(1 << 31) <= v < (1 << 31)) and ps not in reloc: die(12)
            code.append((op, opnd))
        funcs.append((np_, nr, code))
    names = [None] * nf
    if flag == 1:
        for i in range(nf):
            ln, _ = nxt()
            if ln < 1 or ln > 256: die(13)
            nm = ''
            for _ in range(ln):
                c, _ = nxt()
                if c < 33 or c > 126: die(13)
                nm += chr(c)
            if not ident_ok(nm): die(13)
            names[i] = nm
    elif adlar:
        if len(adlar) != nf: die(14)
        for i, nm in enumerate(adlar):
            if not ident_ok(nm): die(14)
            names[i] = nm
    if pos[0] != len(L): die(3)
    for i in range(nf):
        if names[i] is None: names[i] = 'f%d' % i
    if len(set(names)) != nf: die(14)
    nd_base = len(data)
    prev = -1
    for (nm, so) in spaces:
        if not ident_ok(nm) or nm == 'bss' or nm in {n for _, n in keys} or so <= prev or so > bss: die(14)
        prev = so
    if len({n for n, _ in spaces}) != len(spaces): die(14)
    # ---- yazim
    def numtok(v):                                # duz sayi alani: yer tutucu ise $ANAHTAR
        sn = sentinel(v)
        if sn is None: return str(v)
        if sn[0] == 's': return ('-' if sn[2] < 0 else '') + '$' + keys[sn[1]][1]
        die(10)
    def immtok(v, ps):
        if ps in reloc:
            used.add(ps)
            rs = reloc[ps]
            if sentinel(v) is not None: die(9)
            # bagil kayit -> baglayici ifadesi
            allh = {k: 1 for (_, k) in hlist}
            if v >= nd_base + 1 and rs == allh:
                off = v - nd_base - 1
                best = None
                for (nm, so) in spaces:
                    if so <= off: best = (nm, so)
                if best: return '&' + best[0] + (('+%d' % (off - best[1])) if off - best[1] else '')
                return '&bss' + (('+%d' % off) if off else '')
            for (i, k) in hlist:
                if v >= i + 1 and rs == pre(i):
                    return '&' + keys[k][1] + (('+%d' % (v - i - 1)) if v - i - 1 else '')
            for (i, k) in hlist:
                if v >= i + 2 and rs == {**pre(i), k: 1}:
                    return '~' + keys[k][1] + (('+%d' % (v - i - 2)) if v - i - 2 else '')
            return '%d{%s}' % (v, '|'.join('%s*%d' % (keys[k][1], c) for k, c in sorted(rs.items())))
        return numtok(v)
    out = []
    def finish():
        if set(reloc) != used: die(8)
    out.append('; PMO1 gorunumu (gor): sayisal nesnenin okunabilir hali; yaz ile sayilara donusur')
    out.append('pmo 1')
    out.append('flags %d' % flag)
    out.append('memory %d' % mem)
    out.append('entry ' + (names[entry] if 0 <= entry < nf else '#%d' % entry))
    for kd, nm in keys: out.append('key %s %s' % (nm, 'scalar' if kd == 0 else 'array'))
    def consttok(v):
        sn = sentinel(v) if 0 <= v < 1 << 63 else None
        if sn is None: return str(v)
        if sn[0] == 's' and sn[2] == 1: return '$' + keys[sn[1]][1]
        die(10)
    out.append('const' + ''.join(' ' + consttok(v) for v in consts))
    out.append('data')
    run = []
    def flush():
        for i in range(0, len(run), 16):
            ch = run[i:i + 16]
            ln = '  words ' + ' '.join(numtok(v) for v in ch)
            if all(v == 0 or 32 <= v <= 126 for v in ch) and any(v for v in ch):
                ln += '   ; "' + ''.join('\\0' if v == 0 else chr(v) for v in ch).replace('\n', '') + '"'
            out.append(ln)
        run.clear()
    for d in data:
        if d[0] == 'a': flush(); out.append('  array ' + keys[d[1]][1])
        else: run.append(d[1])
    flush()
    out.append('bss %d' % bss)
    for (nm, so) in spaces: out.append('space %s %d' % (nm, so))
    for fi, (np_, nr, code) in enumerate(funcs):
        out.append('')
        out.append('func %s %d %d' % (names[fi], np_, nr))
        tg = set()
        for (op, opnd) in code:
            for j, (v, ps) in enumerate(opnd):
                if opclass(dic, op, j) == 'l' and 0 <= v <= len(code): tg.add(v)
        for i in range(len(code) + 1):
            if i in tg: out.append('L%d:' % i)
            if i == len(code): break
            op, opnd = code[i]
            parts = []
            for j, (v, ps) in enumerate(opnd):
                c = opclass(dic, op, j)
                if c != 'i' and ps in reloc: die(8)
                if c == 'r': parts.append('r%d' % v)
                elif c == 'i': parts.append(immtok(v, ps))
                elif c == 'l': parts.append('L%d' % v if 0 <= v <= len(code) else '@%d' % v)
                elif c == 'F': parts.append(names[v] if 0 <= v < nf else '#%d' % v)
                elif c == 'k': parts.append('c%d' % v)
                elif c == 'h': parts.append(dic.hosts[v] if 0 <= v < len(dic.hosts) else str(v))
                else: parts.append(str(v))
                if c != 'i' and sentinel(v) is not None: die(10)
            out.append('  ' + dic.ops[op] + ((' ' + ', '.join(parts)) if parts else ''))
        out.append('end')
    finish()
    return '\n'.join(out) + '\n'

# ---------------------------------------------------------------- yaz: gorunum -> PMO1
def strip_comment(ln):
    i = ln.find(';')
    return ln if i < 0 else ln[:i]

def yaz(dic, view):
    lines = [strip_comment(l).replace(',', ' ').split() for l in view.split('\n')]
    lines = [l for l in lines if l]
    i = 0
    def need(kw):
        nonlocal i
        if i >= len(lines) or lines[i][0] != kw: die(40)
        r = lines[i]; i += 1; return r
    r = need('pmo')
    if len(r) != 2 or r[1] != '1': die(41)
    r = need('flags')
    if len(r) != 2 or r[1] not in ('0', '1'): die(42)
    flag = int(r[1])
    r = need('memory')
    if len(r) != 2: die(43)
    mem = parse_int(r[1], True)
    r = need('entry')
    if len(r) != 2: die(44)
    entry_tok = r[1]
    keys = []                                    # (ad, tur)
    kidx = {}
    while i < len(lines) and lines[i][0] == 'key':
        r = lines[i]; i += 1
        if len(r) != 3 or not ident_ok(r[1]) or r[2] not in ('scalar', 'array') or r[1] == 'bss' or r[1] in kidx: die(45)
        if len(r[1]) > 64: die(45)
        kidx[r[1]] = len(keys); keys.append((r[1], 0 if r[2] == 'scalar' else 1))
    nk = len(keys)
    if nk > MAXKEY: die(5)
    def sentinel_v(v):
        for w in (v, -v):
            d = w - S1
            if 0 <= d < nk and keys[d][1] == 0: return True
        d = v - S2
        return d >= 0 and d % 65536 == 0 and d // 65536 < nk and keys[d // 65536][1] == 1
    def ref_val(tok):                            # $ANAHTAR | -$ANAHTAR | sayi  -> deger (duz alan)
        if tok.startswith('$') or tok.startswith('-$'):
            neg = tok[0] == '-'
            nm = tok[2:] if neg else tok[1:]
            if nm not in kidx or keys[kidx[nm]][1] != 0: die(46)
            v = S1 + kidx[nm]
            return -v if neg else v
        v = parse_int(tok)
        if sentinel_v(v): die(47)
        return v
    r = need('const')
    consts = []
    for t in r[1:]:
        if t.startswith('$') or t.startswith('-$'):
            v = ref_val(t)
            if v < 0: die(46)
        else:
            v = parse_int(t, True)
            if sentinel_v(v): die(47)
        consts.append(v)
    need('data')
    data = []                                    # ('a', k) | ('w', v)
    hpos = {}
    while i < len(lines) and lines[i][0] in ('array', 'words'):
        r = lines[i]; i += 1
        if r[0] == 'array':
            if len(r) != 2 or r[1] not in kidx or keys[kidx[r[1]]][1] != 1 or kidx[r[1]] in hpos: die(48)
            hpos[kidx[r[1]]] = len(data); data.append(('a', kidx[r[1]]))
        else:
            for t in r[1:]: data.append(('w', ref_val(t)))
    nd_base = len(data)
    r = need('bss')
    if len(r) != 2: die(49)
    bss = parse_int(r[1], True)
    spaces = {}                                  # ad -> bss ofseti
    while i < len(lines) and lines[i][0] == 'space':
        r = lines[i]; i += 1
        if len(r) != 3 or not ident_ok(r[1]) or r[1] == 'bss' or r[1] in kidx or r[1] in spaces: die(66)
        so = parse_int(r[2], True)
        if so > bss: die(66)
        spaces[r[1]] = so
    hlist = [(j, d[1]) for j, d in enumerate(data) if d[0] == 'a']
    def pre_of(j): return {k: 1 for (jj, k) in hlist if jj < j}
    # ---- islevler (ilk gecis: adlar, etiketler)
    funcs = []                                   # (ad, np, nr, [(etiket|None satirlari...)])
    while i < len(lines) and lines[i][0] == 'func':
        r = lines[i]; i += 1
        if len(r) != 4 or not ident_ok(r[1]): die(50)
        fname = r[1]; np_ = parse_int(r[2], True); nr = parse_int(r[3], True)
        body = []
        while True:
            if i >= len(lines): die(51)
            r = lines[i]; i += 1
            if r[0] == 'end':
                if len(r) != 1: die(51)
                break
            if len(r) == 1 and r[0].endswith(':'):
                body.append(('L', r[0][:-1])); continue
            body.append(('I', r))
        funcs.append((fname, np_, nr, body))
    if i != len(lines): die(52)
    fnames = [f[0] for f in funcs]
    if len(set(fnames)) != len(fnames): die(53)
    fidx = {n: j for j, n in enumerate(fnames)}
    nf = len(funcs)
    if entry_tok.startswith('#'): entry = parse_int(entry_tok[1:], True)
    elif entry_tok in fidx: entry = fidx[entry_tok]
    else: die(54)
    # ---- uretim
    L = []                                       # listeye giren sayilar (satir yapisi ayrica)
    rel = []                                     # (konum, anahtar, katsayi)
    lines_out = []
    cur = []
    def put(v): L.append(v); cur.append(str(v))
    def endline():
        lines_out.append(' '.join(cur)); cur.clear()
    put(flag); put(mem); put(entry); endline()
    put(len(consts))
    for c in consts: put(c)
    endline()
    ndpos = len(L)
    put(len(data))
    for k in hpos: rel.append((ndpos, k, 1))
    for d in data:
        if d[0] == 'a': put(S2 + d[1] * 65536)
        else: put(d[1])
    endline()
    put(bss); endline()
    put(nf); endline()
    def imm(tok):
        # $K | -$K | sayi | &K[+n] | ~K[+n] | &bss[+n] | n{K*c ...}
        m = re.fullmatch(r'(-?\$[A-Za-z_][A-Za-z0-9_]*|-?[0-9]+)', tok)
        if m: return ref_val(tok), {}
        m = re.fullmatch(r'([&~])([A-Za-z_][A-Za-z0-9_]*)(?:([+-])([0-9]+))?', tok)
        if m:
            sg, nm, so, nn = m.groups(); off = parse_int(nn) if nn else 0
            if so == '-': off = -off
            if sg == '&' and nm == 'bss': return nd_base + 1 + off, {k: 1 for (_, k) in hlist}
            if sg == '&' and nm in spaces: return nd_base + 1 + spaces[nm] + off, {k: 1 for (_, k) in hlist}
            if nm not in kidx or kidx[nm] not in hpos: die(55)
            j = hpos[kidx[nm]]
            if sg == '&': return j + 1 + off, pre_of(j)
            return j + 2 + off, {**pre_of(j), kidx[nm]: 1}
        m = re.fullmatch(r'(-?[0-9]+)\{([^}]*)\}', tok)
        if m:
            v = parse_int(m.group(1)); rs = {}
            for it in m.group(2).split('|'):
                mm = re.fullmatch(r'([A-Za-z_][A-Za-z0-9_]*)\*(-?[1-9][0-9]*)', it)
                if not mm or mm.group(1) not in kidx or keys[kidx[mm.group(1)]][1] != 1 or kidx[mm.group(1)] in rs: die(56)
                rs[kidx[mm.group(1)]] = parse_int(mm.group(2), False, 56, 56)
            if sentinel_v(v): die(47)
            return v, rs
        die(57)
    for (nm, np_, nr, body) in funcs:
        # etiket tablosu
        labels = {}; n = 0
        for kind, x in body:
            if kind == 'L':
                if x in labels or not ident_ok(x): die(58)
                labels[x] = n
            else: n += 1
        put(np_); put(nr); put(n); endline()
        for kind, x in body:
            if kind == 'L': continue
            mn = x[0]
            if mn not in dic.opid: die(59)
            op = dic.opid[mn]
            if len(x) - 1 != dic.nops[op]: die(60)
            put(op)
            for j, t in enumerate(x[1:]):
                c = opclass(dic, op, j)
                ps = len(L)
                if c == 'r':
                    if not re.fullmatch(r'r-?[0-9]+', t): die(61)
                    v = parse_int(t[1:])
                elif c == 'i':
                    v, rs = imm(t)
                    for k in sorted(rs): pass
                    for k, cf in rs.items(): rel.append((ps, k, cf))
                elif c == 'l':
                    if t.startswith('@'): v = parse_int(t[1:])
                    elif t in labels: v = labels[t]
                    else: die(62)
                elif c == 'F':
                    if t.startswith('#'): v = parse_int(t[1:])
                    elif t in fidx: v = fidx[t]
                    else: die(63)
                elif c == 'k':
                    if not re.fullmatch(r'c-?[0-9]+', t): die(64)
                    v = parse_int(t[1:])
                elif c == 'h':
                    v = dic.hostid[t] if t in dic.hostid else parse_int(t)
                else: v = parse_int(t)
                if c != 'i' and sentinel_v(v): die(47)
                if not (-(1 << 31) <= v < (1 << 31)): die(65)
                put(v)
            endline()
    if flag == 1:
        for (nm, _, _, _) in funcs:
            if len(nm) > 256: die(67)
            put(len(nm))
            for ch in nm: put(ord(ch))
            endline()
    if len(L) > MAXTOK: die(5)
    # bagil kayitlar: anahtar-ana, konum artan
    rel.sort(key=lambda r: (r[1], r[0]))
    if len(rel) > MAXREL: die(5)
    out = ['%d 1 %d %d' % (MAGIC, nk, len(rel))]
    for nm, kd in keys:
        out.append('%d %d %d %s' % (kd, 1 if kd == 1 else S1 + kidx[nm], len(nm), ' '.join(str(ord(c)) for c in nm)))
    for ps, k, c in rel: out.append('%d %d %d' % (ps, k, c))
    return '\n'.join(out) + '\n' + '\n'.join(lines_out) + '\n'

def adlar_of(view):
    lines = [strip_comment(l).replace(',', ' ').split() for l in view.split('\n')]
    o = ['func ' + l[1] for l in lines if l and l[0] == 'func' and len(l) == 4]
    o += ['space %s %s' % (l[1], l[2]) for l in lines if l and l[0] == 'space' and len(l) == 3]
    return o

def main(argv):
    try:
        if argv[1] == 'gor':
            dic = Dic(open(argv[2]).read())
            ad = None
            if len(argv) > 4:
                fn, sp = [], []
                for l in open(argv[4]).read().split('\n'):
                    p = l.split()
                    if not p: continue
                    if p[0] == 'func' and len(p) == 2: fn.append(p[1])
                    elif p[0] == 'space' and len(p) == 3:
                        if not re.fullmatch(r'(0|[1-9][0-9]*)', p[2]): die(14)
                        sp.append((p[1], int(p[2])))
                    else: die(14)
                ad = (fn, sp)
            sys.stdout.write(gor(dic, open(argv[3]).read(), ad))
        elif argv[1] == 'yaz':
            sys.stdout.write(yaz(Dic(open(argv[2]).read()), open(argv[3]).read()))
        elif argv[1] == 'adlar':
            sys.stdout.write(''.join(n + '\n' for n in adlar_of(open(argv[2]).read())))
        else: return 2
    except Err as e:
        print('HATA: %d' % e.code); return 1
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
