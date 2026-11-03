#!/usr/bin/env python3
"""Plan Bolum 04 (04.010 CALLI, 04.030 TCALL): komut tablosuna (okunabilir gorunum, stdin -> stdout) dolayli cagri ve kuyruk cagrisi ekler.
  calli F, b, r : dolayli cagri. F = PROTOTIP islev (imza: parametre sayisi + tur notu; tipdogr/valid CALL gibi denetler), b = pencere tabani, r = islev indeksini tutan yazmac.
      r (isaretsiz) >= islev sayisi ise TRAP OOB; hedefin parametre sayisi prototipinkine esit degilse TRAP OOB (imza uyumsuzlugu); sonrasi CALL ile ayni (yakit, STACK, DEPTH, yerelleri sifirla).
  tcall F, b    : kuyruk cagrisi. Pencere argumanlari cercevenin tabanina tasinir, ayni cerceve F'e devredilir (derinlik artmaz, ret adresi korunur); yakit CALL gibi harcanir; STACK denetimi F'in yazmac sayisiyla.
  SEM: calli a b c ; tcall a b        PRIM calli op,op,op / tcall op,op (akis: ext 1=2)
  x86 (ELF): calli = islev basina 128 baytlik 'calli_stub' tablosuna (STUBS) dolayli cagri; stub imza/STACK/DEPTH denetimi, yerel sifirlama ve girise jmp yapar. tcall = argumanlari tasi + yerelleri sifirla + jmp.
  Yeni sablon simgeleri: NFUNCS (islev sayisi), STUBS (stub tablosu sanal adresi)."""
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' CALLI ' in l for l in L): sys.exit('cagri_ekle: tablo zaten genisletilmis')
def last(pfx): return max(i for i, l in enumerate(L) if l.startswith(pfx))
ol = last('op '); oid = int(L[ol].split()[1]) + 1
pl = last('prim '); pid = int(L[pl].split()[1]) + 1
sl = last('sem ')
tl = last('tpl prim ')
ot = max(i for i, l in enumerate(L) if l.startswith('tpl op '))
sp = max(i for i, l in enumerate(L) if l.startswith('tpl sp '))
spid = int(L[sp].split()[2]) + 1
tk = max(i for i, l in enumerate(L) if l.startswith('token 290 '))
FUEL = '4D 85 F6 0F 84 REL_FUEL 49 FF CE'
calli = FUEL + ' 48 8B 83 R_C 48 3D NFUNCS 0F 83 REL_OOB BA NP8 48 C1 E0 07 48 05 STUBS 48 81 C3 R_B FF D0 49 FF CD 48 81 EB R_B'
stub = '81 FA NP8 0F 85 REL_OOB 48 C7 C1 STKEND 48 29 D9 48 81 F9 NREGS8 0F 82 REL_STACK 49 81 FD 00 00 01 00 0F 83 REL_DEPTH 49 FF C5 48 8D BB NP8 B9 CNT 31 C0 F3 48 AB E9 REL_FA'
tcall = FUEL + ' 48 C7 C1 STKEND 48 29 D9 48 81 F9 NREGS8 0F 82 REL_STACK 48 8D B3 R_B 48 89 DF B9 NP8 C1 E9 03 F3 48 A5 48 8D BB NP8 B9 CNT 31 C0 F3 48 AB E9 REL_FA'
calli_p = FUEL + ' 48 8B 83 OPR_F2 48 3D NFUNCS 0F 83 REL_OOB BA FNP8_F0 48 C1 E0 07 48 05 STUBS 48 81 C3 OPR_F1 FF D0 49 FF CD 48 81 EB OPR_F1'
tcall_p = FUEL + ' 48 C7 C1 STKEND 48 29 D9 48 81 F9 FNR8_F0 0F 82 REL_STACK 48 8D B3 OPR_F1 48 89 DF B9 FNP8_F0 C1 E9 03 F3 48 A5 48 8D BB FNP8_F0 B9 FCNT_F0 31 C0 F3 48 AB E9 FREL_F0'
ins = {
  ol: ['op %d CALLI calli F,b,r' % oid, 'op %d TCALL tcall F,b' % (oid + 1)],
  pl: ['prim %d calli op,op,op ext 1=2' % pid, 'prim %d tcall op,op ext 1=2' % (pid + 1)],
  sl: ['sem CALLI calli a b c', 'sem TCALL tcall a b'],
  ot: ['tpl op CALLI : ' + calli, 'tpl op TCALL : ' + tcall],
  tl: ['tpl prim calli : ' + calli_p, 'tpl prim tcall : ' + tcall_p],
  sp: ['tpl sp %d calli_stub : %s' % (spid, stub)],
  tk: ['token 291 NFUNCS 1', 'token 292 STUBS 1'],
}
# deger simgeleri 291 (NFUNCS) ve 292 (STUBS): gen2 val32 k=35,36
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
