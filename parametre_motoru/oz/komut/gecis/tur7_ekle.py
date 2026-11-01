#!/usr/bin/env python3
# Plan Bolum 01 (genis operand): x86-64 sablonlari. OPW_Fk simgesi = alan k'nin gosterdigi operand yazmaci + (alan k+1) literal sozcuk indeksi, 8 ile carpilmis (32 bit).
#   rdw to,op,n   : mov rax,[rbx+OPW_F1] ; mov [rsp+TMP_F0],rax
#   wrw op,n,ti   : mov rax,[rsp+TMP_F2] ; mov [rbx+OPW_F0],rax
#   mulhu to,ti,ti: mov rax,[rsp+TMP_F1] ; mul qword [rsp+TMP_F2] ; mov [rsp+TMP_F0],rdx
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('token 444 ') for l in L): sys.exit('tur7_ekle: tablo zaten genisletilmis')
tl = max(i for i, l in enumerate(L) if l.startswith('token '))
pl = max(i for i, l in enumerate(L) if l.startswith('tpl prim '))
tok = ['token %d OPW_F%d 4' % (444 + k, k) for k in range(4)]
tpl = ['tpl prim rdw : 48 8B 83 OPW_F1 48 89 44 24 TMP_F0', 'tpl prim wrw : 48 8B 44 24 TMP_F2 48 89 83 OPW_F0', 'tpl prim mulhu : 48 8B 44 24 TMP_F1 48 F7 64 24 TMP_F2 48 89 54 24 TMP_F0']
ins = {tl: tok, pl: tpl}
out = []
for i, l in enumerate(L):
    out.append(l)
    if i in ins: out += ins[i]
print('\n'.join(out), end='')
