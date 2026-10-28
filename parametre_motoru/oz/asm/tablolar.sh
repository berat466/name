#!/bin/bash
# ops.h'den komut/host ad tablolari (NUL ile ayrilmis), NOPS ve KIND sayi tablolari; yalnizca bicim donusumu.
cd "$(dirname "$0")"
awk '/^  X\(/{ s=$0; sub(/^  X\(/,"",s); split(s,a,"\""); names=names a[2] "\\0" }
  END{ names=names "putc\\0puti\\0putf\\0getc\\0present\\0poll\\0frame\\0"; print names }' ../../src/ops.h | sed 's/\\\\0/\\0/g' | tr -d '\n' > /tmp/opnames.txt
printf '%b' "$(cat /tmp/opnames.txt)" | od -An -v -tu1 | tr -s ' \n' ' ' > /tmp/opnames.num
echo "data OPNAMES = $(cat /tmp/opnames.num)"
awk '/^  X\(/{ k=$0; sub(/.*"[a-zA-Z0-9]*", *[0-9] *, *"/,"",k); sub(/".*/,"",k); nops=nops " " length(k);
  for(i=1;i<=3;i++){c=substr(k,i,1); v=0; if(c=="d")v=1;else if(c=="r")v=2;else if(c=="f")v=3;else if(c=="s")v=4;else if(c=="i")v=5;else if(c=="k")v=6;else if(c=="l")v=7;else if(c=="F")v=8;else if(c=="b")v=9;else if(c=="h")v=10; kd=kd " " v} }
  END{print "data NOPS =" nops; print "data KIND =" kd}' ../../src/ops.h
echo "data ARITY = 1 1 1 0 3 0 0"
