#!/usr/bin/env python3
"""Plan Bolum 03 (03.009 atlama tablosu): komut tablosuna (okunabilir gorunum, stdin -> stdout) JTAB komutu + BRX mikro-islemi ekler.
  jtab r, n : r dizini [0,n) icindeyse sonraki komut + r'e, degilse sonraki komut + n'e atlar; ardindaki n adet ILERI `jmp` merdiveni hedefleri tasir (dizin gecersizse merdivenin ardina duser).
  SEM: rd t1 a ; im t2 b ; brx t1 t2     PRIM brx ti,ti (akis: ext 1=2)     x86: dizin >= n (isaretsiz) ise n'e kenetlenir; 5 baytlik `jmp rel32` merdivenine hesaplanmis atlama (lea rax,[rax+rax*4]; lea rcx,[rip+5]; add; jmp rax).
Not: merdiven girdileri ileri `jmp` olmalidir (sabit 5 bayt: geri atlama yakit kontrol noktasi ekler). Kod degisikligi yok: svm ve ELF uretici tablodan calisir; C++ referans icin src/anlam.h'ta BRX makrolari vardir."""
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('op ') and ' JTAB ' in l for l in L): sys.exit('denetim_ekle: tablo zaten genisletilmis')
def last(pfx): return max(i for i, l in enumerate(L) if l.startswith(pfx))
ol = last('op '); oid = int(L[ol].split()[1]) + 1
pl = last('prim '); pid = int(L[pl].split()[1]) + 1
sl = last('sem ')
tl = last('tpl prim ')
newop = ['op %d JTAB jtab r,i' % oid]
newprim = ['prim %d brx ti,ti ext 1=2' % pid]
newsem = ['sem JTAB rd t1 a ; im t2 b ; brx t1 t2']
newtpl_op = ['tpl op JTAB : 48 8B 83 R_A 48 3D I_B 72 07 48 C7 C0 I_B 48 8D 04 80 48 8D 0D 05 00 00 00 48 01 C8 FF E0']
newtpl_prim = ['tpl prim brx : 48 8B 44 24 TMP_F0 48 8B 4C 24 TMP_F1 48 39 C8 72 03 48 89 C8 48 8D 04 80 48 8D 0D 05 00 00 00 48 01 C8 FF E0']
# op-sablonu: son 'tpl op ' satirindan sonra
ot = max(i for i, l in enumerate(L) if l.startswith('tpl op '))
ins = {ol: newop, pl: newprim, sl: newsem, ot: newtpl_op, tl: newtpl_prim}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
