#!/bin/bash
# oz/arac araclari (doldur, opstab, fmtc, sablonc) = onceki awk/sed uygulamalari (ref/ altinda REFERANS olarak tutulur): ayni cikti, ayni ret.
# Kullanim: dogrula_arac.sh [pm] [rastgele_sayi]  (araclar pm ile ya da ARAC=ELF dizini ile calistirilir)
PM=${1:-../../pm}; NR=${2:-150}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0; nrej=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
. ./arac.sh
nrm() { tr -s ' \n' ' ' < "$1" | sed 's/ $//'; }
# ---- 0) guncellik: oz/arac/*.pmb kaynaklariyla ayni mi
for t in doldur opstab fmtc sablonc; do $PM asm $t.pma -o $T/$t.pmb --strip > /dev/null 2>&1 && cmp -s $T/$t.pmb $t.pmb && pass || fail "$t.pmb guncel degil"; done
# ---- 1) opstab: gercek ops.h ve rastgele ops.h varyantlari
cmp_ops() { ./ref/ops_ref.sh "$2" > $T/ro.txt; arac opstab < "$2" > $T/mo.txt 2>/dev/null; [ "$(nrm $T/ro.txt)" = "$(nrm $T/mo.txt)" ] && pass || fail "opstab farkli: $1"; }
cmp_ops "ops.h" $R/src/ops.h
for s in $(seq 1 $NR); do awk -v seed=$s 'function r(n){x=(x*1664525+1013904223)%4294967296;return int(x/65536)%n}
  BEGIN{x=seed*7919+1; n=1+r(40); print "// rastgele ops.h"; print "#define PM_OPS(X)  \\"; for(i=0;i<n;i++){ nm=""; l=1+r(8); for(j=0;j<l;j++) nm=nm substr("abcdefghijklmnopqrstuvwxyz0123456789",1+r(36),1); no=r(4); k=""; for(j=0;j<no;j++) k=k substr("drfsiklFbh",1+r(10),1); printf "  X(E%d, \"%s\", %d, \"%s\")   \\\n", i, nm, no, k; if (r(7)==0) print "  // X(NOT, \"x\", 1, \"d\")" } print "enum Op { OP_COUNT };"}' > $T/ops_r.h; cmp_ops "rastgele-$s" $T/ops_r.h; done
# ---- 2) fmtc: gercek fmt.txt ve rastgele tablolar
cmp_fmt() { awk -f ref/fmt_ref.awk "$2" > $T/rf.txt; arac fmtc < "$2" > $T/mf.txt 2>/dev/null; [ "$(nrm $T/rf.txt)" = "$(nrm $T/mf.txt)" ] && pass || fail "fmtc farkli: $1"; }
cmp_fmt "fmt.txt" $D/../yap/fmt.txt
for s in $(seq 1 $NR); do awk -v seed=$s 'function r(n){x=(x*1664525+1013904223)%4294967296;return int(x/65536)%n}
  BEGIN{x=seed*7919+3; n=1+r(30); A=" !\"#$%&\x27()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; for(i=0;i<n;i++){ if(r(6)==0) print "# yorum " r(100); if(r(9)==0) print ""; nm=""; l=1+r(8); for(j=0;j<l;j++) nm=nm substr("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_",1+r(37),1); t=""; l=r(30); for(j=0;j<l;j++) t=t substr(A,1+r(length(A)),1); if (r(8)==0) print nm; else print nm " " t } }' > $T/fmt_r.txt; cmp_fmt "rastgele-$s" $T/fmt_r.txt; done
# ---- 3) sablonc: gercek sablon.txt ve rastgele sablon bildirimleri; reddedilen girdilerde ikisi de reddetmeli
cmp_sab() { { cat $R/src/ops.h; echo "@@SABLON"; cat "$2"; } > $T/sin.txt; awk -f ref/sablon_ref.awk $R/src/ops.h "$2" 2>/dev/null | sed -E 's/^data (TPLS|TPL) =/\1/' > $T/rs.txt; rr=${PIPESTATUS[0]}; arac sablonc < $T/sin.txt > $T/ms.txt 2>/dev/null; rm_=$?
  if [ $rr = 0 ]; then [ $rm_ = 0 ] && [ "$(nrm $T/rs.txt)" = "$(nrm $T/ms.txt)" ] && pass || fail "sablonc farkli: $1 (rr=$rr rm=$rm_)"; else [ $rm_ != 0 ] && { pass; nrej=$((nrej+1)); } || fail "sablonc: referans reddetti, arac kabul etti: $1"; fi; }
cmp_sab "sablon.txt" $R/oz/elf2/sablon.txt
for s in $(seq 1 $NR); do awk -v seed=$s 'function r(n){x=(x*1664525+1013904223)%4294967296;return int(x/65536)%n}
  BEGIN{x=seed*7919+5; split("mov ldi ldk add sub mul div mod and or xor shl shr sar eq ne lt le neg not jmp jz jnz call ret halt ld st alloc host addi muli andi shli shri sari divi modi eqi nei lti lei gti gei header prologue exit0 host_putc host_puti host_getc trap_div0 trap_oob trap_stack trap_depth trap_alloc trap_fuel jmp_back jz_back jnz_back", N, " ");
   split("R_A R_B R_C I_A I_B I_C K_B MEMW STKEND NREGS8 NP8 CNT MSG_DIV0 MSG_OOB MSG_STACK MSG_DEPTH MSG_ALLOC RSBASE MBASE HEAP0 DATAIMG ND HDR_ENTRY FILESZ MEMSZ FUEL0 MSG_FUEL REL_DIV0 REL_OOB REL_STACK REL_DEPTH REL_ALLOC REL_PUTI REL_PUTC REL_TA REL_TB REL_FA REL_EXIT REL_ENTRY REL_FUEL IMM8C FUELCHK LDA(a) LDA(b) LDC(c) STA(b) STA(a)", K, " "); nn=0; for(i in N) nn++; nk=0; for(i in K) nk++; hx="0123456789ABCDEF";
   m=1+r(20); for(i=1;i<=nn;i++) used[i]=0; for(q=0;q<m;q++){ i=1+r(nn); if(used[i]) continue; used[i]=1; line=N[i] ":"; t=r(12); for(j=0;j<t;j++){ if(r(3)==0) line=line " " K[1+r(nk)]; else line=line " " substr(hx,1+r(16),1) substr(hx,1+r(16),1) } if(r(9)==0) line=line " # yorum"; if(r(15)==0) line=line " BILINMEZ"; if(r(40)==0) line="bilinmeyen_ad: 00"; print line; if(r(8)==0) print "# aciklama"; if(r(10)==0) print "" } }' > $T/sab_r.txt; cmp_sab "rastgele-$s" $T/sab_r.txt; done
# yinelenen tanim: arac reddeder (referans sessizce ustune yazar) -> bilincli fark
printf 'mov: 48 90\nmov: 90\n' > $T/dup.txt; { cat $R/src/ops.h; echo "@@SABLON"; cat $T/dup.txt; } | arac sablonc > /dev/null 2>&1 && fail "sablonc yinelenen tanimi reddetmedi" || pass
# ---- 4) doldur: rastgele sozluk ve sablonlar
cmp_dol() { awk -f ref/doldur_ref.awk "$2" > $T/rd.txt 2>/dev/null; rr=$?; arac doldur < "$2" > $T/md.txt 2>/dev/null; rm_=$?
  if [ $rr = 0 ]; then [ $rm_ = 0 ] && cmp -s $T/rd.txt $T/md.txt && pass || fail "doldur farkli: $1 (rr=$rr rm=$rm_)"; else [ $rm_ != 0 ] && { pass; nrej=$((nrej+1)); } || fail "doldur: referans reddetti, arac kabul etti: $1"; fi; }
for s in $(seq 1 $((NR * 2))); do awk -v seed=$s 'function r(n){x=(x*1664525+1013904223)%4294967296;return int(x/65536)%n}
  function key(  nm,l,j){nm="";l=1+r(4);for(j=0;j<l;j++)nm=nm substr("ABCab01_",1+r(8),1);return nm}
  BEGIN{x=seed*7919+7; for(i=0;i<8;i++) kk[i]=key(); nd=r(8); for(i=0;i<nd;i++){ if(r(7)==0) print "# yorum " r(9); v=""; l=r(6); for(j=0;j<l;j++) v=v (j?" ":"") r(1000); if (r(5)==0) v=v "\r"; print kk[r(8)] (r(6)==0 ? "\t" : " ") v }
   print "@@SABLON"; nl=1+r(8); for(i=0;i<nl;i++){ s=""; np=r(6); for(j=0;j<np;j++){ q=r(7); if(q==0) s=s "@" kk[r(8)] "@"; else if(q==1) s=s "@" kk[r(8)]; else if(q==2) s=s "@@"; else if(q==3) s=s "metin "; else if(q==4) s=s "a@b "; else if(q==5) s=s "@" kk[r(8)] "@@" kk[r(8)] "@"; else s=s "x " } print s } }' > $T/dol_r.txt; cmp_dol "rastgele-$s" $T/dol_r.txt; done
# ---- 5) araclarin ciktisiyla uretilen kaynaklar eski awk/sed yoluyla uretilenlerle ayni mi: asm/gen2/valid/yap .pma
for t in asm elf2 dogr yap; do :; done
rm -rf $T
echo "araclar (doldur/opstab/fmtc/sablonc): gecen $ok, kalan $bad (ayni ret: $nrej)"; [ $bad = 0 ]
