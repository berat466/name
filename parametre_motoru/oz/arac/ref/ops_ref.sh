#!/bin/bash
# ops.h -> OPCOUNT/OPNAMES/NOPS/KIND sozlugu. Referans (awk): opstab.pma ile karsilastirilir.
awk '/^  X\(/{ s=$0; sub(/^  X\(/,"",s); split(s,a,"\""); names=names a[2] "\0"; k=$0; sub(/.*"[a-zA-Z0-9]*", *[0-9] *, *"/,"",k); sub(/".*/,"",k); nops=nops " " length(k);
  for(i=1;i<=3;i++){c=substr(k,i,1); v=0; if(c=="d")v=1;else if(c=="r")v=2;else if(c=="f")v=3;else if(c=="s")v=4;else if(c=="i")v=5;else if(c=="k")v=6;else if(c=="l")v=7;else if(c=="F")v=8;else if(c=="b")v=9;else if(c=="h")v=10; kd=kd " " v}; n++; nl[n]=a[2] }
  END{ print "OPCOUNT " n; s=""; for(i=1;i<=n;i++){ for(j=1;j<=length(nl[i]);j++) s=s " " (index("abcdefghijklmnopqrstuvwxyz",substr(nl[i],j,1)) ? 96+index("abcdefghijklmnopqrstuvwxyz",substr(nl[i],j,1)) : (index("0123456789",substr(nl[i],j,1))? 47+index("0123456789",substr(nl[i],j,1)) : 0)); s=s " 0" }
       h="putc puti putf getc present poll frame"; m=split(h,hh," "); for(i=1;i<=m;i++){ for(j=1;j<=length(hh[i]);j++) s=s " " (96+index("abcdefghijklmnopqrstuvwxyz",substr(hh[i],j,1))); s=s " 0" }
       print "OPNAMES" s; print "NOPS" nops; print "KIND" kd }' "$1"
