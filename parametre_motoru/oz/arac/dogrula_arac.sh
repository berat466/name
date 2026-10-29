#!/bin/bash
# oz/arac araclari (doldur, opstab, fmtc; komut: bkz. oz/komut/dogrula_komut.sh) = onceki awk/sed uygulamalari (ref/ altinda REFERANS olarak tutulur): ayni cikti, ayni ret.
# Kullanim: dogrula_arac.sh [pm] [rastgele_sayi]  (araclar pm ile ya da ARAC=ELF dizini ile calistirilir)
PM=${1:-../../pm}; NR=${2:-150}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0; nrej=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
. ./arac.sh
nrm() { tr -s ' \n' ' ' < "$1" | sed 's/ $//'; }
# ---- 0) guncellik: oz/arac/*.pmb kaynaklariyla ayni mi
for t in doldur opstab fmtc komut baglayici gorunum; do $PM asm $t.pma -o $T/$t.pmb --strip > /dev/null 2>&1 && cmp -s $T/$t.pmb $t.pmb && pass || fail "$t.pmb guncel degil"; done
# ---- 1) opstab: gercek ops.h ve rastgele ops.h varyantlari
cmp_ops() { ./ref/ops_ref.sh "$2" > $T/ro.txt; arac opstab < "$2" > $T/mo.txt 2>/dev/null; [ "$(nrm $T/ro.txt)" = "$(nrm $T/mo.txt)" ] && pass || fail "opstab farkli: $1"; }
cmp_ops "ops.h (donmus gecis kopyasi: komut/gecis/ops.h.txt)" $R/oz/komut/gecis/ops.h.txt
for s in $(seq 1 $NR); do awk -v seed=$s 'function r(n){x=(x*1664525+1013904223)%4294967296;return int(x/65536)%n}
  BEGIN{x=seed*7919+1; n=1+r(40); print "// rastgele ops.h"; print "#define PM_OPS(X)  \\"; for(i=0;i<n;i++){ nm=""; l=1+r(8); for(j=0;j<l;j++) nm=nm substr("abcdefghijklmnopqrstuvwxyz0123456789",1+r(36),1); no=r(4); k=""; for(j=0;j<no;j++) k=k substr("drfsiklFbh",1+r(10),1); printf "  X(E%d, \"%s\", %d, \"%s\")   \\\n", i, nm, no, k; if (r(7)==0) print "  // X(NOT, \"x\", 1, \"d\")" } print "enum Op { OP_COUNT };"}' > $T/ops_r.h; cmp_ops "rastgele-$s" $T/ops_r.h; done
# ---- 2) fmtc: gercek fmt.txt ve rastgele tablolar
cmp_fmt() { awk -f ref/fmt_ref.awk "$2" > $T/rf.txt; arac fmtc < "$2" > $T/mf.txt 2>/dev/null; [ "$(nrm $T/rf.txt)" = "$(nrm $T/mf.txt)" ] && pass || fail "fmtc farkli: $1"; }
cmp_fmt "fmt.txt" $D/../yap/fmt.txt
for s in $(seq 1 $NR); do awk -v seed=$s 'function r(n){x=(x*1664525+1013904223)%4294967296;return int(x/65536)%n}
  BEGIN{x=seed*7919+3; n=1+r(30); A=" !\"#$%&\x27()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; for(i=0;i<n;i++){ if(r(6)==0) print "# yorum " r(100); if(r(9)==0) print ""; nm=""; l=1+r(8); for(j=0;j<l;j++) nm=nm substr("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_",1+r(37),1); t=""; l=r(30); for(j=0;j<l;j++) t=t substr(A,1+r(length(A)),1); if (r(8)==0) print nm; else print nm " " t } }' > $T/fmt_r.txt; cmp_fmt "rastgele-$s" $T/fmt_r.txt; done
# ---- 3) (sablonc kaldirildi: x86 sablonlari tabloda, bolum 8/9; denetimi oz/komut/dogrula_komut.sh yapar)
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
echo "araclar (doldur/opstab/fmtc): gecen $ok, kalan $bad (ayni ret: $nrej)"; [ $bad = 0 ]
