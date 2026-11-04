#!/bin/bash
# pick_isa.sh <bin_dizini> : bu makinede guvenle calisacak ISA surumunu yazdirir (avx512|avx2). Ikili ayrica kendi CPU korumasina sahiptir (cikis kodu 97).
BIN="$1"; ISA=$(cat "$BIN/ISA_SECILEN.txt" 2>/dev/null | tr -d '\r\n'); [ -z "$ISA" ] && ISA=avx2
if [ "$ISA" = avx512 ]; then
  if grep -qiE "avx512f" /proc/cpuinfo 2>/dev/null && grep -qiE "avx512dq" /proc/cpuinfo && grep -qiE "avx512bw" /proc/cpuinfo && grep -qiE "avx512vl" /proc/cpuinfo && grep -qiE "avx512cd" /proc/cpuinfo; then echo avx512; else
    # /proc/cpuinfo guvenilir degilse (MSYS2): ikilinin yerlesik korumasina guven -- 'derleme makinesi probu' avx512 dediyse zaten destekliyordur
    [ -r /proc/cpuinfo ] && grep -qi flags /proc/cpuinfo && echo avx2 || echo avx512; fi
else echo avx2; fi
