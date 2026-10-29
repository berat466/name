#!/bin/bash
# dogrula_cagri.sh'in yakaladigini kanitlar: ret/halt/call/host mikro-islem sablonlarini (komut tablosu) ve gen2'nin ilgili kodunu tek tek bozar (mutant),
# kendi kopya agacinda gen2'yi yeniden uretir ve dogrula_cagri.sh'in BASARISIZ olmasini bekler. Kullanim: sabotaj_cagri.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-3}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
mut_tablo() {   # etiket sed_ifadesi (komut tablosu okunabilir gorunumu)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    { echo "@@gor"; cat $R/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb | sed "$2" | { echo "@@yaz"; cat; } | $PM run $R/oz/arac/komut.pmb > $w/oz/komut/komutlar.pmk
    cmp -s $w/oz/komut/komutlar.pmk $R/oz/komut/komutlar.pmk && { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    [ "$(echo "@@dogrula"; cat $w/oz/komut/komutlar.pmk)" ] ; (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    if (cd $w/oz/elf2 && TAB=$w/oz/komut/komutlar.pmk timeout 400 ./dogrula_cagri.sh $PM > $T/log_$1 2>&1); then echo "KACTI: $1"; else echo "yakalandi: $1 ($(grep -c HATA $T/log_$1) hata)"; fi
    rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
mut_gen2() {   # etiket perl_ifadesi (oz/elf2/gen2.pma.in)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    (cd $w/oz/elf2 && perl -0pi -e "$2" gen2.pma.in && ! cmp -s gen2.pma.in $R/oz/elf2/gen2.pma.in) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    (cd $w/oz/nesne && TOOLS=gen2 GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    if (cd $w/oz/elf2 && timeout 400 ./dogrula_cagri.sh $PM > $T/log_$1 2>&1); then echo "KACTI: $1"; else echo "yakalandi: $1 ($(grep -c HATA $T/log_$1) hata)"; fi
    rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; "$@" & }
# ---- tablo: sablonlar
bg mut_tablo ret_yok 's/^tpl prim ret : .*/tpl prim ret : -/'
bg mut_tablo ret_ofset 's/tpl prim ret : 48 8B 44 24 TMP_F0 48 89 03 C3/tpl prim ret : 48 8B 44 24 TMP_F0 48 89 43 08 C3/'
bg mut_tablo ret_alan 's/tpl prim ret : 48 8B 44 24 TMP_F0/tpl prim ret : 48 8B 44 24 TMP_F1/'
bg mut_tablo halt_hedef 's/tpl prim halt : E9 REL_EXIT/tpl prim halt : E9 REL_EXITC/'
bg mut_tablo call_yok 's/^tpl prim call : .*/tpl prim call : -/'
bg mut_tablo call_taban 's/ OPR_F1 48 C7 C1 STKEND/ OPR_F0 48 C7 C1 STKEND/'
bg mut_tablo call_yazmac_sayisi 's/ FNR8_F0 0F 82/ FNP8_F0 0F 82/'
bg mut_tablo call_hedef 's/E8 FREL_F0 49/E8 FREL_F1 49/'
bg mut_tablo call_yakit 's/^\(tpl prim call : \)4D 85 F6 0F 84 REL_FUEL 49 FF CE /\1/'
bg mut_tablo call_derinlik 's/ 49 FF C5 48 8D B8/ 49 FF C4 48 8D B8/'
bg mut_tablo call_sifirlama 's/ FCNT_F0 48 89 C3/ FNR8_F0 48 89 C3/'
bg mut_tablo host_yok 's/^tpl prim host : .*/tpl prim host : -/'
bg mut_tablo host_alan 's/tpl prim host : HOSTSP_F0/tpl prim host : HOSTSP_F1/'
# ---- gen2 kodu
bg mut_gen2 scanrt_genel 's/call hsl, hostslot, op\n    jnz hsl, sr1/call hsl, hostslot, op\n    jmp sr1/'
bg mut_gen2 hostmap_putf 's/(func hostmap\(\) \{.*?)addi t, na, \@RTF_PUTF\@/$1addi t, na, 0/s'
bg mut_gen2 hostmap_present 's/sub t, h, \@H_PRESENT\@\n    add t, t, \@RTF_PRESENT\@/sub t, h, \@H_PRESENT\@\n    add t, t, 0/'
bg mut_gen2 fn_rel 's/(f1: add t, fv, 20.*?sub r, r, p\n    sub r, r, )4/${1}3/s'
bg mut_gen2 fn_nregs 's/(eqi z, ty, 7\n    jz z, f3\n    add t, fn, FNR\n    ld r, t, 0\n    mul r, r, )8/${1}4/'
bg mut_gen2 fn_np 's/(eqi z, ty, 8\n    jz z, f4\n    add t, fn, FNP\n    ld r, t, 0\n    mul r, r, )8/${1}4/'
bg mut_gen2 fn_cnt 's/ld q, t, 0\n    sub r, r, q/ld q, t, 0\n    add r, r, q/'
bg mut_gen2 host_yuva 's/ld sl, G, 61\n    eqi z, sl, 1\n    jz z, f6/ld sl, G, 61\n    eqi z, sl, 2\n    jz z, f6/'
bg mut_gen2 host_geri 's/call x, emitop, op2\n    st sa, G, 20/call x, emitop, op2/'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|uretilemedi/{h++; print} END{printf "sabotaj(cagri): %d yakalandi, %d kacti, %d uygulanamadi/uretilemedi\n", y, k, h}'
rm -rf $T
