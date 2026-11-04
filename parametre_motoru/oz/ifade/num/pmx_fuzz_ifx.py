#!/usr/bin/env python3
"""PMX1 sayi listesini rastgele bozar ve SAYISAL derleyiciye (ifx.elf) verir. Kosullar:
  (1) ifx.elf her zaman bitmeli (zaman asimi yok) ve sinyalle olmemeli (SIGSEGV/SIGKILL yok; bozuk girdi = temiz ret ya da motor TRAP'i);
  (2) cikis kodu 0 ise uretilen PMA asm.elf -> coz.elf -> valid.elf (yapisal) VE tipdogr (PMT1) tarafindan KABUL EDILMELI
      (bozuk girdi sessizce gecersiz program uretmemeli).
Kullanim: pmx_fuzz_ifx.py BAG_DIZINI IFX_ELF TIPDOGR_ELF TABLO IMZA PMX N TMP    Cikti: 0 ya da hata ozeti"""
import sys, random, subprocess
bag, ifx, tip, tab, imz, pmx, n, tmp = sys.argv[1:9]; n = int(n)
v = open(pmx).read().split(); r = random.Random(4242); bad = 0; first = ''
head = open(tab).read() + open(imz).read() + '\n'
def run(cmd, data, t=60):
    return subprocess.run(cmd, input=data, capture_output=True, timeout=t)
nacc = 0
for i in range(n):
    w = list(v)
    for _ in range(r.choice([1, 1, 2, 3])):
        k = r.randrange(3, len(w)); w[k] = r.choice([str(r.randrange(-3, 70)), str(r.randrange(0, len(w))), '-1', '999999999', str(1 << 40)])
    if r.random() < 0.15: w = w[:r.randrange(3, len(w))]
    data = (head + ' '.join(w) + '\n').encode()
    try: c = run([ifx], data)
    except subprocess.TimeoutExpired: bad += 1; first = first or 'ifx zaman asimi'; continue
    if c.returncode < 0 or c.returncode == 139: bad += 1; first = first or 'ifx sinyal rc=%d' % c.returncode; continue
    if c.returncode != 0 or b'@@HATA' in c.stdout: continue
    nacc += 1
    try:
        a = run([bag + '/asm.elf'], c.stdout)
        if a.returncode != 0 or not a.stdout: bad += 1; first = first or 'uretilen PMA assembler reddetti'; continue
        l = run([bag + '/coz.elf'], a.stdout).stdout
        vv = run([bag + '/valid.elf'], l).stdout.split(b'\n')[0]
        if not vv.startswith(b'1'): bad += 1; first = first or 'valid.elf uretilen programi reddetti'; continue
        t = b'\n'.join(x[len(';;PMT1 '):] for x in c.stdout.split(b'\n') if x.startswith(b';;PMT1'))
        tv = run([tip], b'@@IMZA\n' + open(imz, 'rb').read() + b'\n@@PROGRAM\n' + l + b'@@TIPLER\n' + t + b'\n').stdout.split(b'\n')[0]
        if not tv.startswith(b'1'): bad += 1; first = first or 'tipdogr uretilen programi reddetti'
    except subprocess.TimeoutExpired: bad += 1; first = first or 'zincir zaman asimi'
print(0 if bad == 0 else '%d gecersiz cikis (%s) [kabul edilen %d/%d]' % (bad, first, nacc, n))
