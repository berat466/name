#!/bin/bash
# Sinir degerli PMY1 modelleri uretir (sayi listesi -> baytlar): sinir-1, sinir, sinir+1. Kullanim: sinir_modelleri.sh CIKTI_DIZINI
# Test araci (awk ile paketleme); yap.pmb / pm yapisal ayni dosyalari islemeli.
O=${1:?dizin}; mkdir -p $O; D=$(cd "$(dirname "$0")" && pwd); PK="awk -f $D/../../ai_testi3/paketle.awk"
H="80 77 89 49"
# M W H FR glob arr fns bhs tms ets gis ags hb ilk son   (her bolum kendi sayaciyla)
M() { echo "$H ${1:-0} ${2:-0} 0 ${3:-0} ${4:-0} ${5:-0} ${6:-0} ${7:-0} ${8:-0} ${9:-0} ${10:-0} ${11:-0} ${12:-0} ${13:-0} ${14:-0}"; }
mk() { echo "$2" | $PK > $O/$1.pmy 2>/dev/null; }
rep() { local s=""; for ((i=0;i<$2;i++)); do s="$s$1 "; done; echo -n "$s"; }
# 1) ifade derinligi: tek YAZI komutu, N ic ice tekli; derinlik > 200 reddedilir
for N in 199 200 201 202 203; do mk "ifade_derinlik_$N" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 "1 8 $(rep '16 1' $N)1 z0" 0)"; done
# 2) blok derinligi: N ic ice SURDE; derinlik > 100 reddedilir
for N in 99 100 101 102; do e="0"; for ((i=0;i<N;i++)); do e="1 3 1 z1 $e"; done; mk "blok_ic_$N" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 "$e" 0)"; done
# 3) sahne boyutu
for wh in "4096 4096" "4097 4096" "4096 4097" "0 1" "1 0" "4096 1" "1 1" "0 0"; do set -- $wh; mk "sahne_$1_$2" "$(M $1 $2)"; done
# 4) islev parametre/yerel sinirlari
for pl in "255 255" "255 256" "256 256" "0 257" "0 256" "2 1" "0 0"; do set -- $pl; mk "islev_$1_$2" "$(M 0 0 0 0 0 "1 $1 $2 0")"; done
# 5) dizi boyutu ve baslangic sayisi
for sz in 0 1 16777215 16777216 16777217; do mk "dizi_boyut_$sz" "$(M 0 0 0 0 "1 $sz 0")"; done
mk "dizi_init_fazla" "$(M 0 0 0 0 '1 2 3 z1 z2 z3')"
mk "dizi_init_tam" "$(M 0 0 0 0 '1 2 2 z1 z2')"
# 6) sablon ornek sayisi, alan baslaticilari (kare dongusu gerekmesin: kural/cizim yok)
for n in 0 1 10000000 10000001; do mk "sablon_n_$n" "$(M 0 0 0 0 0 0 0 "1 $n 1 1 z5 0 0")"; done
mk "sablon_mod0" "$(M 0 0 0 0 0 0 0 '1 4 1 3 0 z1 z1 0 0')"
mk "sablon_mod1" "$(M 0 0 0 0 0 0 0 '1 4 1 3 1 z1 z1 0 0')"
mk "sablon_bol0" "$(M 0 0 0 0 0 0 0 '1 4 1 4 0 z1 z1 0 0')"
mk "sablon_bol1" "$(M 0 0 0 0 0 0 0 '1 4 1 4 1 z1 z1 0 0')"
mk "sablon_rast_ters" "$(M 0 0 0 0 0 0 0 '1 4 1 5 z5 z4 z0 0 0')"
mk "sablon_rast_esit" "$(M 0 0 0 0 0 0 0 '1 4 1 5 z5 z5 z0 0 0')"
mk "sablon_alan_kodu_0" "$(M 0 0 0 0 0 0 0 '1 4 1 0 z1 0 0')"
mk "sablon_alan_kodu_6" "$(M 0 0 0 0 0 0 0 '1 4 1 6 z1 0 0')"
mk "sablon_dogrusal" "$(M 0 0 0 0 0 0 0 '1 4 1 2 z3 z-2 0 0')"
# 7) ikili 20/21/0, tekli 3/4/0, ifade 18/19/20, komut 12/13/14
for op in 0 1 20 21; do mk "ikili_op_$op" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 "1 8 15 $op 1 z1 1 z2" 0)"; done
for op in 0 1 3 4; do mk "tekli_op_$op" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 "1 8 16 $op 1 z1" 0)"; done
for k in 0 17 18 19 20; do mk "ifade_kodu_$k" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 "1 8 $k 1 z1 1 z1 1 z1" 0)"; done
for k in 0 10 11 12 13 14; do mk "komut_kodu_$k" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 "1 $k 1 1 z1" 0)"; done
# 8) davranis zinciri bh0 -> bh1 -> ... (son bos); ilk blok bh0'i cagirir. Acilim derinligi > 16 reddedilir
for L in 15 16 17 18; do b="$L"; for ((i=0;i<L;i++)); do if ((i+1<L)); then b="$b 0 1 6 $((i+1)) 0"; else b="$b 0 0"; fi; done; mk "davran_zincir_$L" "$(M 0 0 0 0 0 0 "$b" 0 0 0 0 0 '1 6 0 0' 0)"; done
mk "davran_arg_sayisi" "$(M 0 0 0 0 0 0 '1 2 0' 0 0 0 0 0 '1 6 0 1 1 z1' 0)"
mk "davran_arg_tam" "$(M 0 0 0 0 0 0 '1 1 1 8 14 0' 0 0 0 0 0 '1 6 0 1 1 z5' 0)"
mk "davran_yok" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 '1 6 3 0' 0)"
mk "param_disi" "$(M 0 0 0 0 0 0 0 0 0 0 0 0 '1 8 14 0' 0)"
# 9) sablon/islev sayisi sinirlari (1000 / 5000)
for n in 1000 1001; do s="$n"; for ((i=0;i<n;i++)); do s="$s 1 0 0 0"; done; mk "sablon_sayisi_$n" "$(M 0 0 0 0 0 0 0 "$s")"; done
for n in 5000 5001; do s="$n"; for ((i=0;i<n;i++)); do s="$s 0 0 0"; done; mk "islev_sayisi_$n" "$(M 0 0 0 0 0 "$s")"; done
# 10) kare dongusu kurallari: sahne yok + kare yok + kural var
mk "dongu_yok_kural_var" "$(M 0 0 0 0 0 0 0 '1 3 1 1 z1 1 8 1 z1 0' 0)"
mk "dongu_kare1" "$(M 0 0 1 0 0 0 0 '1 3 1 1 z1 1 8 1 z1 0' 0)"
mk "bitis_dongu" "$(M 0 0 0 0 0 0 0 0 0 0 0 '1 1 z1' 0 0)"
# 11) bicim: fazla bayt, kesik, bos, yanlis baslik
mk "fazla_bayt" "$(M) 7"
mk "kesik" "$H 0 0 0 0"
mk "yanlis_baslik" "80 77 89 50 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"
: > $O/bos.pmy
