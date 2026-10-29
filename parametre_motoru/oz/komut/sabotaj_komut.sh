#!/bin/bash
# dogrula_komut.sh'in yakaladigini kanitlar: komut.pma'yi tek tek bozar (mutant), kendi kucuk deposunda (src + oz/{komut,arac,asm,dogr,elf2}) yeniden derler, dogrula_komut.sh'in BASARISIZ olmasini bekler.
# Kullanim: sabotaj_komut.sh [pm]   (SADECE: etiket deseni)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); yakalandi=0; kacti=0
mut() {   # etiket ifade
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/src $R/Makefile $T/w/; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $R/oz/sayisal $T/w/oz/; cp $R/oz/paketle.pmy $R/oz/coz.pmy $R/oz/yaz.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $T/w/oz/
  (cd $T/w/oz/arac && perl -0pi -e "$2" komut.pma && ! cmp -s komut.pma $R/oz/arac/komut.pma) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  (cd $T/w/oz/arac && $PM asm komut.pma -o komut.pmb --strip > /dev/null 2>&1) || { echo "mutant $1: derlenemedi"; kacti=$((kacti+1)); return; }
  if (cd $T/w/oz/komut && timeout 900 ./dogrula_komut.sh $PM ${NR:-12} > $T/log 2>&1); then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
# TABAN: degistirilmemis kopyada denetim gecmeli (aksi halde tum mutantlar yanlis nedenle "yakalanmis" sayilir)
if [ -z "$SADECE" ] || [ -n "$TABAN" ]; then
  rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/src $R/Makefile $T/w/; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $R/oz/sayisal $T/w/oz/; cp $R/oz/paketle.pmy $R/oz/coz.pmy $R/oz/yaz.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $T/w/oz/
  (cd $T/w/oz/komut && timeout 900 ./dogrula_komut.sh $PM ${NR:-12} > $T/taban.log 2>&1) && echo "taban: gecti" || { echo "TABAN BASARISIZ: $(grep -c HATA $T/taban.log) hata"; head -5 $T/taban.log; exit 1; }
fi
mut magic        's/eqi z, m, 1347242801\n    jnz z, v1\n    ret 2/eqi z, m, 1347242801\n    jmp v1\n    ret 2/'
mut buyuk_surum  's/eqi z, mj, 1\n    jnz z, v2\n    ret 3/eqi z, mj, 1\n    jmp v2\n    ret 3/'
mut zorunlu      's/andi t, fl, 1\n    jz t, v7\n    ret 6/andi t, fl, 1\n    jmp v7\n    ret 6/'
mut bolum_yinele 's/jz o, v4\n    ret 5/jmp v4\n    ret 5/'
mut opcode_yinele 's/jz o, o4\n    ret 31/jmp o4\n    ret 31/'
mut sembol_yinele 's/jz same, o8\n    ret 32/jmp o8\n    ret 32/'
mut kind_yinele  's/jz o, k2\n    ret 21/jmp k2\n    ret 21/'
mut tur_basvuru  's/ld o, b, 0\n    jz o, obad2/ld o, b, 0\n    jz kid, obad2/'
mut gor_ext      's/    ld ne2, a, 0\n    jz ne2, x1\n    call x, pdat, W_EXT_S/    ld ne2, a, 0\n    jmp x1\n    call x, pdat, W_EXT_S/'
mut yaz_kuyruk   's/addi nt2, nt2, 1\n    jmp l1\nlbad/addi nt2, nt2, 0\n    jmp l1\nlbad/'
mut tab_ad_sonu  's/host putc, 32\n    host puti, 0\n    addi i, i, 1\n    jmp t1/host putc, 32\n    host puti, 1\n    addi i, i, 1\n    jmp t1/'
mut opsh_ters    's/addi t, n, -1\n    ge z, i, t\n    jnz z, o4/addi t, n, -2\n    ge z, i, t\n    jnz z, o4/'
mut profil_yogun 's/jnz o, p3a\n    ret 53/jmp p3a\n    ret 53/'
mut profil_nk    's/gti z, nk, 3\n    jz z, p3b/gti z, nk, 9\n    jz z, p3b/'
mut kind_karakter 's/lti z, ch, 48\n    jnz z, kbad\n    gti z, ch, 122/lti z, ch, 33\n    jnz z, kbad\n    gti z, ch, 122/'
mut sym_kucuk    's/gei z, c, 65\n    jz z, n1a\n    lei z, c, 90\n    jnz z, n9/gei z, c, 65\n    jz z, n1a\n    lei z, c, 122\n    jnz z, n9/'
mut walk_uzunluk 's/lti z, rl, 4\n    jz z, w2a/lti z, rl, 1\n    jz z, w2a/'
mut bolum_sonu   's/v8: eq z, p, nt\n    jnz z, v9\n    ret 7/v8: eq z, p, nt\n    jmp v9\n    ret 7/'
mut host_yinele  's/jz o, h2\n    ret 41/jmp h2\n    ret 41/'
mut opaque_gor   's/    eqi z, vs, 1\n    jz z, gop\n    ; yapisal bolum/    jmp gop\n    ; yapisal bolum/'
mut pseudo_id_aralik 's/gei z, pid, 256\n    jnz z, pbad/gei z, pid, 99999\n    jnz z, pbad/'
mut pseudo_id_yinele 's/jz o, p2\n    ret 90/jmp p2\n    ret 90/'
mut pseudo_ad_komut 's/jz same, p3\n    ret 91/jmp p3\n    ret 91/'
mut pseudo_ad_onceki 's/jz same, p4\n    ret 92/jmp p4\n    ret 92/'
mut kural_nk 's/lti z, nk, 3\n    jz z, r2\n    ret 82/lti z, nk, 3\n    jmp r2\n    ret 82/'
mut kural_yinele 's/jz o2, r2c\n    ret 83/jmp r2c\n    ret 83/'
mut kural_hedef_nk 's/ne z, nk3, nk\n    jz z, a1b\n    ret 85/ne z, nk3, nk\n    jmp a1b\n    ret 85/'
mut kural_kaynak 's/jnz o, r1b\n    ret 80\nr1a:/jmp r1b\n    ret 80\nr1a:/'
mut kural_eylem 's/eqi z, act, 3\n    jnz z, a4\n    ret 86/eqi z, act, 3\n    jmp a4\n    ret 86/'
mut akis_halt 's/data STR_P_HALT = "halt\\0"/data STR_P_HALT = "hal\\0"/'
mut akis_brz 's/data STR_P_BRZ = "brz\\0"/data STR_P_BRZ = "brq\\0"/'
mut akis_divn 's/data STR_P_DIVN = "divn\\0"/data STR_P_DIVN = "divx\\0"/'
mut kflags 's/add b, o, T\n    ld fl, b, 2/add b, o, T\n    ld fl, b, 1/'
mut rwoff_sira 's/o3: ge z, i, np\n    jnz z, o4/o3: ge z, i, 0\n    jnz z, o4/'
mut token_kod_yinele 's/jz o, t2\n    ret 100/jmp t2\n    ret 100/'
mut token_ad_yinele 's/jz same, t4\n    ret 101/jmp t4\n    ret 101/'
mut token_hex 's/    ret 103\ntnh:/tnh:/'
mut tpl_yinele 's/jz o2, u6\n    ret 113/jmp u6\n    ret 113/'
mut tpl_ozel_ad 's/jz same, u8b\n    ret 115/jmp u8b\n    ret 115/'
mut tpl_ozel_yogun 's/jnz o, u9b\n    ret 116/jmp u9b\n    ret 116/'
mut tpl_simge 's/jnz o, u7a\n    ret 114/jmp u7a\n    ret 114/'
mut tpls_ofset 's/add pos, pos, nt\n    addi pos, pos, 1/add pos, pos, nt/'
mut ts_yuva 's/add t, n, k\n    host puti, t\n    host putc, 10\n    addi k, k, 1/host puti, k\n    host putc, 10\n    addi k, k, 1/'
mut gor_ozel 's/call x, pdat, W_SPK\n    host putc, 32\n    host puti, sr/call x, pdat, W_SPK\n    host putc, 32\n    host puti, 0/'
mut yaz_simge_say 's/call x, ot, v\n    addi nt, nt, 1\n    jmp yk7/call x, ot, v\n    jmp yk7/'
mut yaz_hex 's/muli v, h0, 16\n    add v, v, h1\n    jmp yk11/muli v, h0, 15\n    add v, v, h1\n    jmp yk11/'
mut yaz_tanimsiz 's/mov nt, -1\n    jmp yk7/mov nt, 0\n    jmp yk7/'
mut gor_hex 's/addi c, d, 55\n    host putc, c/addi c, d, 56\n    host putc, c/'
echo "sabotaj(komut): $yakalandi yakalandi, $kacti kacti"
rm -rf $T; [ $kacti = 0 ]
