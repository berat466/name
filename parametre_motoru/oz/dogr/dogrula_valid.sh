#!/bin/bash
# Parametre tabanli PMB1 dogrulayici + analiz (oz/dogr/valid.pmb) = C++ referansi (pm listedogrula) ve yukleyici (decode_program).
# Kullanim: dogrula_valid.sh [pm] [liste_mutanti_program_basina] [bayt_mutanti_program_basina]
# Her program zaman ve bellek siniri altinda calistirilir. Hepsi otomatik, ajan yok.
PM=${1:-../../pm}; MUTL=${2:-150}; MUTB=${3:-100}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
ok=0; bad=0; acc=0; rejn=0; z0=0
$PM asm valid.pma -o $T/v.pmb --strip > /dev/null 2>&1 || { echo "valid.pma derlenmedi"; exit 1; }
[ -n "$VALID" ] || cmp -s $T/v.pmb valid.pmb && ok=$((ok+1)) || { bad=$((bad+1)); echo "HATA: valid.pmb guncel degil (valid.pma ile uyusmuyor)"; }
g++ -O2 -o $T/mut ../mutasyon.cpp || exit 1
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
par() { (ulimit -v 600000; timeout 20 $PM run ${VALID:-valid.pmb} < $1 2>/dev/null); }    # cikti: "1 Z" ya da "0 K"
ref() { timeout 20 $PM listedogrula $1; }
cmp_liste() {   # etiket dosya
  local a b; a=$(ref $2 | head -1); b=$(par $2 | head -1)
  # reddedilirse yalniz "0" karsilastirilir (ret nedeni kodu referansta yok)
  [ "${a%% *}" = 0 ] && b=${b%% *}
  if [ "$a" = "$b" ] || { [ "${a%% *}" = 0 ] && [ "$b" = 0 ]; }; then pass; case "$a" in "1 1") acc=$((acc+1));; "1 0") acc=$((acc+1)); z0=$((z0+1));; *) rejn=$((rejn+1));; esac
  else fail "$1: referans='$a' parametre='$b'"; cp $2 /tmp/valid_fark_$ok$bad.txt; fi
}
# ---- 1) derlem
corp=$T/corp; mkdir -p $corp; n=0
for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' 2>/dev/null | sort); do for s in "" "--strip"; do $PM asm $f -o $corp/k$n.pmb $s > /dev/null 2>&1 && n=$((n+1)); done; done
for f in $(find $R/uygulamalar $R/tests -name '*.pmt' 2>/dev/null | sort); do (cd $(dirname $f) && $PM sablon $f -o $corp/k$n.pmb > /dev/null 2>&1) && n=$((n+1)); done
for f in $(find $R/oz $R/ai_testi3 $R/ai_testi4 -name '*.pmy' 2>/dev/null | sort); do $PM yapisal $f -o $corp/k$n.pmb > /dev/null 2>&1 && n=$((n+1)); done
for p in $corp/*.pmb; do $PM liste $p > ${p%.pmb}.l 2>/dev/null; cmp_liste "derlem $(basename $p)" ${p%.pmb}.l; done
echo "derlem: $n PMB1 dosyasi"
# ---- 2) elle yazilmis listeler: her ret kurali ve sinir. lst ad BEKLENEN satirlar...  (BEKLENEN: "1 1", "1 0" ya da "0"; hem C++ referansi hem parametre tablosu buna esit olmali)
lst() { local name=$1 exp=$2; shift 2; printf '%s\n' "$@" > $T/h.txt; local a b; a=$(ref $T/h.txt | head -1); b=$(par $T/h.txt | head -1)
  [ "$exp" = 0 ] && { [ "${a%% *}" = 0 ] && a=0; [ "${b%% *}" = 0 ] && b=0; }
  if [ "$a" = "$exp" ] && [ "$b" = "$exp" ]; then pass; else fail "elle '$name': beklenen='$exp' referans='$a' parametre='$b'"; fi; }
H="0 1 0"; E0="0"; BOS="0 0 0 0 1"   # (kullanilmiyor; okunabilirlik)
lst ret_tanimsiz        "1 0" "0 1 0" "0" "0" "0" "1" "0 1 1" "35 0"
lst bos_fonk            "0"   "0 1 0" "0" "0" "0" "1" "0 1 0"
lst halt_tek            "1 1" "0 1 0" "0" "0" "0" "1" "0 0 1" "36"
lst jmp_kendine         "1 1" "0 1 0" "0" "0" "0" "1" "0 0 1" "31 0"
lst giris_yok           "0"   "0 1 1" "0" "0" "0" "1" "0 0 1" "36"
lst giris_param         "0"   "0 1 0" "0" "0" "0" "1" "1 1 1" "36"
lst np_gt_nr            "0"   "0 1 0" "0" "0" "0" "1" "2 1 1" "36"
lst np_gt_nr_ikinci     "0"   "0 1 0" "0" "0" "0" "2" "0 1 1" "36" "2 1 1" "36"
lst np_eq_nr_ikinci     "1 1" "0 1 0" "0" "0" "0" "2" "0 1 1" "36" "2 2 1" "36"
lst son_komut_yanlis    "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "1 0 5"
lst reg_sinir           "0"   "0 1 0" "0" "0" "0" "1" "0 1 2" "1 0 5" "35 1"
lst reg_sinir_ok        "1 1" "0 1 0" "0" "0" "0" "1" "0 1 2" "1 0 5" "35 0"
lst atlama_sinir        "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "31 1"
lst atlama_negatif      "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "31 -1"
lst sabit_indeks        "0"   "0 1 0" "0" "0" "0" "1" "0 1 2" "2 0 0" "35 0"
lst sabit_indeks_ok     "1 1" "0 1 0" "1 7" "0" "0" "1" "0 1 2" "2 0 0" "35 0"
lst fonk_indeks         "0"   "0 1 0" "0" "0" "0" "1" "0 2 3" "1 0 0" "34 1 1" "36"
lst host_bilinmeyen     "0"   "0 1 0" "0" "0" "0" "1" "0 2 3" "1 0 0" "40 7 0" "36"
lst host_pencere        "0"   "0 1 0" "0" "0" "0" "1" "0 1 3" "1 0 65" "40 1 1" "36"
lst host_ok             "1 1" "0 1 0" "0" "0" "0" "1" "0 1 3" "1 0 65" "40 1 0" "36"
lst host_getc_pencere   "0"   "0 1 0" "0" "0" "0" "1" "0 1 2" "40 3 1" "36"
lst host_getc_ok        "1 1" "0 1 0" "0" "0" "0" "1" "0 2 2" "40 3 1" "36"
lst host_present_ok     "1 1" "0 1 0" "0" "0" "0" "1" "0 3 5" "1 0 1" "1 1 2" "1 2 3" "40 4 0" "36"
lst host_present_kisa   "0"   "0 1 0" "0" "0" "0" "1" "0 2 4" "1 0 1" "1 1 2" "40 4 0" "36"
lst host_present_tanim  "1 0" "0 1 0" "0" "0" "0" "1" "0 3 4" "1 0 1" "1 1 2" "40 4 0" "36"
lst host_putc_tanimsiz  "1 0" "0 1 0" "0" "0" "0" "1" "0 1 2" "40 0 0" "36"
lst divi_sifir          "0"   "0 1 0" "0" "0" "0" "1" "0 2 3" "1 0 5" "47 1 0 0" "36"
lst modi_eksi1          "0"   "0 1 0" "0" "0" "0" "1" "0 2 3" "1 0 5" "48 1 0 -1" "36"
lst divi_ok             "1 1" "0 1 0" "0" "0" "0" "1" "0 2 3" "1 0 5" "47 1 0 2" "36"
lst veri_sigmiyor       "0"   "0 4 0" "0" "2 1 2" "2" "1" "0 1 1" "36"
lst veri_sigiyor        "1 1" "0 5 0" "0" "2 1 2" "2" "1" "0 1 1" "36"
lst bss_buyuk           "0"   "0 3 0" "0" "0" "4" "1" "0 1 1" "36"
lst mem0                "0"   "0 0 0" "0" "0" "0" "1" "0 1 1" "36"
lst mem_sinir           "1 1" "0 134217728 0" "0" "0" "0" "1" "0 1 1" "36"
lst mem_asiri           "0"   "0 134217729 0" "0" "0" "0" "1" "0 1 1" "36"
lst giris_65534         "0"   "0 1 65534" "0" "0" "0" "1" "0 1 1" "36"
lst bayrak2             "0"   "2 1 0" "0" "0" "0" "1" "0 1 1" "36"
lst isim_ok             "1 1" "1 1 0" "0" "0" "0" "1" "0 1 1" "36" "4 109 97 105 110"
lst isim_256_ok         "1 1" "1 1 0" "0" "0" "0" "1" "0 1 1" "36" "256 $(seq -s' ' 1 256 | sed 's/[0-9]*/65/g')"
lst isim_257            "0"   "1 1 0" "0" "0" "0" "1" "0 1 1" "36" "257"
lst isim_257_tam        "0"   "1 1 0" "0" "0" "0" "1" "0 1 1" "36" "257 $(seq -s" " 1 257 | sed "s/[0-9]*/65/g")"
lst isim_karakter       "0"   "1 1 0" "0" "0" "0" "1" "0 1 1" "36" "1 256"
lst isim_eksik          "0"   "1 1 0" "0" "0" "0" "1" "0 1 1" "36"
lst fazla_simge         "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "36" "7"
lst sabit_u64           "1 1" "0 1 0" "1 18446744073709551615" "0" "0" "1" "0 1 1" "36"
lst sabit_u64_tasma     "0"   "0 1 0" "1 18446744073709551616" "0" "0" "1" "0 1 1" "36"
lst sabit_eksi          "0"   "0 1 0" "1 -1" "0" "0" "1" "0 1 1" "36"
lst veri_i64            "1 1" "0 4 0" "0" "2 9223372036854775807 -9223372036854775808" "0" "1" "0 1 1" "36"
lst veri_tasma          "0"   "0 4 0" "0" "1 9223372036854775808" "0" "1" "0 1 1" "36"
lst veri_eksi0          "1 1" "0 4 0" "0" "1 -0" "0" "1" "0 1 1" "36"
lst veri_eksi_tasma     "0"   "0 4 0" "0" "1 -9223372036854775809" "0" "1" "0 1 1" "36"
lst operand_i32_ust     "0"   "0 1 0" "0" "0" "0" "1" "0 1 2" "1 0 2147483648" "36"
lst operand_i32_alt     "0"   "0 1 0" "0" "0" "0" "1" "0 1 2" "1 0 -2147483649" "36"
lst imm_i32_alt_ok      "1 1" "0 1 0" "0" "0" "0" "1" "0 1 2" "1 0 -2147483648" "36"
lst imm_i32_ust_ok      "1 1" "0 1 0" "0" "0" "0" "1" "0 1 2" "1 0 2147483647" "36"
lst isaret_alan         "0"   "0 1 -1" "0" "0" "0" "1" "0 1 1" "36"
lst bilinmeyen_op       "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "55"
lst artik_arti          "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "+36"
lst harf                "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "3x"
lst yalniz_eksi         "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "-"
lst bos_girdi           "0"   ""
lst kesik               "0"   "0 1 0" "0" "0" "0" "1" "0 1 1"
lst cift_eksi           "0"   "0 1 0" "0" "0" "0" "1" "0 1 1" "--36"
lst bosluk_cesitleri    "1 1" "0	1 0" "0" "0" "0" "1" "0 1 1" "36"
# cagri/pencere kurallari
lst cagri_ok            "1 0" "0 1 0" "0" "0" "0" "2" "0 2 3" "1 0 1" "34 1 0" "36" "2 2 1" "35 0"
lst cagri_ok2           "1 1" "0 1 0" "0" "0" "0" "2" "0 2 4" "1 0 1" "1 1 2" "34 1 0" "36" "2 2 1" "35 0"
lst cagri_pencere_dar   "0"   "0 1 0" "0" "0" "0" "2" "0 1 3" "1 0 1" "34 1 0" "36" "2 2 1" "35 0"
lst cagri_np0_pencere   "0"   "0 1 0" "0" "0" "0" "2" "0 1 2" "34 1 1" "36" "0 1 1" "35 0"
lst cagri_np0_ok        "1 0" "0 1 0" "0" "0" "0" "2" "0 2 2" "34 1 1" "36" "0 1 1" "35 0"
# analiz: kesin atama
lst analiz_param_ok     "1 1" "0 1 0" "0" "0" "0" "2" "0 3 4" "1 0 1" "1 1 2" "34 1 0" "36" "2 2 1" "35 0"
lst analiz_cagri_sonrasi_sonuc "1 1" "0 1 0" "0" "0" "0" "2" "0 3 5" "1 0 1" "1 1 2" "34 1 0" "3 2 0 0" "36" "2 2 1" "35 0"
lst analiz_cagri_sonrasi_pencere "1 0" "0 1 0" "0" "0" "0" "2" "0 3 5" "1 0 1" "1 1 2" "34 1 0" "3 2 0 1" "36" "2 2 1" "35 0"
lst analiz_cagri_arg_tanimsiz "1 0" "0 1 0" "0" "0" "0" "2" "0 3 3" "1 0 1" "34 1 0" "36" "2 2 1" "35 0"
lst analiz_ikinci_fonk_param "1 1" "0 1 0" "0" "0" "0" "2" "0 1 3" "1 0 1" "34 1 0" "36" "1 2 2" "3 1 0 0" "35 1"
lst analiz_kullanilmayan_fonk_ihlal "1 0" "0 1 0" "0" "0" "0" "2" "0 1 1" "36" "0 1 1" "35 0"
lst analiz_ulasilamaz_okuma "1 1" "0 1 0" "0" "0" "0" "1" "0 1 2" "36" "35 0"
lst analiz_dal_biri     "1 1" "0 1 0" "0" "0" "0" "1" "0 3 6" "1 0 1" "32 0 4" "1 1 7" "31 5" "1 2 8" "36"
lst analiz_dal_ikisi    "1 1" "0 1 0" "0" "0" "0" "1" "0 2 6" "1 0 1" "32 0 4" "1 1 7" "31 5" "1 1 8" "35 1"
lst analiz_dal_okuma    "1 0" "0 1 0" "0" "0" "0" "1" "0 3 7" "1 0 1" "32 0 4" "1 1 7" "31 5" "1 2 8" "3 0 1 2" "36"
lst analiz_dongu        "1 1" "0 1 0" "0" "0" "0" "1" "0 1 4" "1 0 3" "41 0 0 -1" "33 0 1" "35 0"
lst analiz_dongu_ihlal  "1 0" "0 2 0" "0" "0" "0" "1" "0 2 5" "1 0 3" "33 0 4" "1 1 1" "31 1" "35 1"
lst analiz_st_okuma     "1 0" "0 1 0" "0" "0" "0" "1" "0 2 3" "1 0 0" "38 1 0 0" "36"
lst analiz_st_ok        "1 1" "0 1 0" "0" "0" "0" "1" "0 2 4" "1 0 0" "1 1 0" "38 1 0 0" "36"
lst analiz_sifir_yazmac "1 1" "0 1 0" "0" "0" "0" "1" "0 0 1" "36"
# ---- 3) liste duzeyi mutantlar: kabul/ret karari ve zero_free ayni
m=0; for p in $corp/*.pmb; do
  m=$((m+1)); [ $((m % 3)) = 1 ] || continue
  for s in $(seq 1 $MUTL); do awk -v seed=$((m*1000+s)) -v k=$((1+s%3)) -f mut.awk ${p%.pmb}.l > $T/m.txt; cmp_liste "mutant $(basename $p) tohum $s" $T/m.txt; done
done
echo "liste mutantlari tamam"
# ---- 4) zincir: bayt mutantlari -> cozucu (coz.pmy) -> dogrulayici = yukleyici karari (decode_program)
m=0; for p in $(ls $corp/*.pmb | awk 'NR % 5 == 2'); do
  mkdir -p $T/mu; rm -f $T/mu/*; $T/mut $p $T/mu $MUTB $((m+7)); m=$((m+1))
  for q in $T/mu/*.bin; do
    $PM dump $q > /dev/null 2>&1; ra=$?    # yukleyici (parse + validate + analyze)
    (ulimit -v 600000; timeout 20 $PM run ../coz.pmy < $q > $T/z.txt 2>/dev/null); rc=$?
    if [ $rc != 0 ]; then rb=1; else b=$(par $T/z.txt | head -1); case "$b" in 1*) rb=0;; *) rb=1;; esac; fi
    if { [ $ra = 0 ] && [ $rb = 0 ]; } || { [ $ra != 0 ] && [ $rb != 0 ]; }; then pass; [ $ra = 0 ] && acc=$((acc+1)) || rejn=$((rejn+1)); else fail "zincir karari farkli $(basename $q): yukleyici rc=$ra, zincir rc=$rb"; cp $q /tmp/valid_zincir_$m.bin; fi
  done
done
rm -rf $T
echo "PMB1 dogrulayici+analiz (parametre): gecen $ok, kalan $bad (kabul $acc, bunlardan zero_free=0: $z0, ret $rejn)"; [ $bad = 0 ]
