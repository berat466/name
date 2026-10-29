#!/usr/bin/env python3
# TEK SEFERLIK GECIS: eski sablonc ciktisi (TPLS/TPL/TS_*) -> PMK1 bolum 8 (TOKEN) ve 9 (TEMPLATE) "section" satirlari (okunabilir gorunum bicimi, komut @@yaz ile sayisal tabloya islenir).
# Kullanim: sablon_to_pmk.py sablonc_ciktisi.txt nops > bolumler.txt
import sys
ref = open(sys.argv[1]).read().split('\n'); nops = int(sys.argv[2])
tpls = None; tpl = None
for l in ref:
    w = l.split()
    if w and w[0] == 'TPLS': tpls = list(map(int, w[1:]))
    elif w and w[0] == 'TPL': tpl = list(map(int, w[1:]))
IMMN = "R_A R_B R_C I_A I_B I_C K_B MEMW STKEND NREGS8 NP8 CNT MSG_DIV0 MSG_OOB MSG_STACK MSG_DEPTH MSG_ALLOC RSBASE MBASE HEAP0 DATAIMG ND HDR_ENTRY FILESZ MEMSZ FUEL0 MSG_FUEL LIMRT RTW RTNEG STK4 R_B8 R_B16 RTIMG RTNW".split()
RELN = "REL_DIV0 REL_OOB REL_STACK REL_DEPTH REL_ALLOC REL_PUTI REL_PUTC REL_TA REL_TB REL_FA REL_EXIT REL_ENTRY REL_FUEL REL_EXITC REL_RTEND".split()
MICN = "TMP_F0 TMP_F1 TMP_F2 TMP_F3 OPR_F0 OPR_F1 OPR_F2 OPR_F3 OPV_F0 OPV_F1 OPV_F2 OPV_F3 KAD_F0 KAD_F1 KAD_F2 KAD_F3 LIT_F0 LIT_F1 LIT_F2 LIT_F3".split()
SPEC = "host_putc host_puti host_putf host_getc host_present host_poll host_frame prologue exit0 trap_div0 trap_oob trap_stack trap_depth trap_alloc header jmp_back jz_back jnz_back trap_fuel ldp stp call_p host_sysw exitc exitc_rt prologue_rt".split()
ns = len(SPEC)
def ch(s): return [ord(c) for c in s]
def rec(body): return [len(body)] + body
toks = [(256 + i, n, 1) for i, n in enumerate(IMMN)] + [(300 + i, n, 2) for i, n in enumerate(RELN)] + [(320, "IMM8C", 3)] + [(400 + i, n, 4) for i, n in enumerate(MICN)]
s8 = [len(toks)]
for code, name, cls in toks: s8 += rec([code, len(name)] + ch(name) + [cls, 0])
recs = []
def tokens_at(slot):
    st = tpls[slot]
    if st < 0: return None
    out = []; i = st
    while tpl[i] != -1: out.append(tpl[i]); i += 1
    return out
for slot in range(len(tpls)):
    t = tokens_at(slot)
    if slot < nops:
        if t is None: continue
        recs.append(rec([0, slot, 0, len(t)] + t + [0]))
    elif slot < nops + ns:
        k = slot - nops; name = SPEC[k]
        recs.append(rec([1, k, len(name)] + ch(name) + ([-1] if t is None else [len(t)] + t) + [0]))
    else:
        pid = slot - nops - ns
        if pid == 0 or t is None: continue
        recs.append(rec([2, pid, 0, len(t)] + t + [0]))
s9 = [len(recs)]
for r in recs: s9 += r
print('section 8 1 1 ' + ' '.join(map(str, s8)))
print('section 9 1 1 ' + ' '.join(map(str, s9)))
