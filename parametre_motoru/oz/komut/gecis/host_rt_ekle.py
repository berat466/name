#!/usr/bin/env python3
"""Bolum 04 duzeltmesi (host putf yazmac ezme): calisma-zamani islevine (RT) yonlenen host komutlari (putf, present, poll, frame) icin iki ozel sablon ekler.
KOK NEDEN: host -> RT CALL, RT islevinin cercevesini PENCERE TABANINDAN baslatiyordu; RT islevi (ve cagirdigi alt islevler) cagiranin pencere ustundeki CANLI yazmaclarini eziyordu
(referans pm/svm/native host cagrisinda yazmac ezmez). Yalniz RT cercevesi kadar kaydetmek yetmez (alt cagrilar daha derine iner).
DUZELTME: RT cercevesi CAGIRANIN CERCEVE SONUNDA baslar (yeni simge CURNR8 = gecerli islevin yazmac sayisi*8); en cok 3 arguman pencereden RT r0..r2'ye kopyalanir,
sonuc pencere[0]'a yazilir (referans anlami: HER host pencere[0]'a bir sonuc yazar; sonucsuz hostlarda 0 -- host_putc/puti sablonlari da `xor eax,eax; mov [pencere],rax` yapar). Boylece hicbir canli yazmac ezilmez.
  host_rt  : sonucsuz (putf: pencere[0] = 0)      host_rtr : sonuclu (present, poll, frame: pencere[0] = RT r0)
okunabilir gorunum stdin -> stdout."""
import sys
L = sys.stdin.read().split('\n')
if any(l.startswith('tpl sp ') and ' host_rt ' in l for l in L): sys.exit('host_rt_ekle: tablo zaten genisletilmis')
sp = max(i for i, l in enumerate(L) if l.startswith('tpl sp '))
spid = int(L[sp].split()[2]) + 1
tk = max(i for i, l in enumerate(L) if l.startswith('token 292 '))
PRE = '4D 85 F6 0F 84 REL_FUEL 49 FF CE 48 8D 83 CURNR8 48 C7 C1 STKEND 48 29 C1 48 81 F9 NREGS8 0F 82 REL_STACK 49 81 FD 00 00 01 00 0F 83 REL_DEPTH 49 FF C5'
ARGS = '48 8B 8B R_B 48 89 08 48 8B 8B R_B8 48 89 48 08 48 8B 8B R_B16 48 89 48 10'
CALL = '48 8D B8 NP8 B9 CNT 48 89 C3 31 C0 F3 48 AB E8 REL_FA 49 FF CD 48 8B 03 48 81 EB CURNR8'
a = ' '.join([PRE, ARGS, CALL, '31 C0 48 89 83 R_B'])
b = ' '.join([PRE, ARGS, CALL, '48 89 83 R_B'])
out = []
for i, l in enumerate(L):
    out.append(l)
    if i == sp: out += ['tpl sp %d host_rt : %s' % (spid, a), 'tpl sp %d host_rtr : %s' % (spid + 1, b)]
    if i == tk: out += ['token 293 CURNR8 1']
print('\n'.join(out), end='')
