#!/bin/bash
# dogrula_gorunum.sh'in yakaladigini kanitlar: oz/arac/gorunum.pma'yi tek tek bozar (mutant), kendi kucuk agacinda yeniden derler, dogrula_gorunum.sh'in BASARISIZ olmasini bekler.
# Once KISA=1 (bolum 1-4: gercek nesneler, rastgele/bozulmus gorunumler, olumsuz ornekler); kacan mutant icin TAM kosu (uctan uca duzenleme + ELF) denenir.
# Kullanim: sabotaj_gorunum.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-4}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/..; T=$(mktemp -d)
taban() {   # dizin: oz agacinin tam kopyasi (kisa kosu yalniz bir kismini kullanir; kacan mutantlar icin tam kosu hepsini ister)
  rm -rf $1; mkdir -p $1; cp -r $R $1/oz
}
mut() {   # etiket perl_ifadesi
  local et=$1 ifd=$2 w=$T/w_$1
  [ -z "$SADECE" ] || [[ "$et" =~ $SADECE ]] || return
  (
    taban $w
    (cd $w/oz/arac && perl -0pi -e "$ifd" gorunum.pma && ! cmp -s gorunum.pma $R/arac/gorunum.pma) || { echo "SABOTAJ HATASI: $et uygulanamadi"; exit 3; }
    (cd $w/oz/arac && $PM asm gorunum.pma -o gorunum.pmb --strip > /dev/null 2>&1) || { echo "mutant $et: derlenemedi"; exit 4; }
    if (cd $w/oz/nesne && KISA=1 ./dogrula_gorunum.sh $PM 12 12 > $T/log_$et 2>&1); then
      if (cd $w/oz/nesne && ./dogrula_gorunum.sh $PM 6 6 > $T/logt_$et 2>&1); then echo "KACTI: $et"; exit 1; else echo "yakalandi (tam kosu): $et ($(grep -c HATA $T/logt_$et) hata)"; exit 0; fi
    else echo "yakalandi: $et ($(grep -c HATA $T/log_$et) hata)"; exit 0; fi
  ) > $T/sonuc_$et 2>&1
  cat $T/sonuc_$et; rm -rf $w
}
calistir() {   # mut cagrisini arka plana al, JOBS ile sinirla
  while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done
  mut "$@" &
}
# Bilerek YOK (esdeger mutant): veri sayisi konumundaki bagil kayit kumesi denetimi (`eq z, m, nh`): kaldirilsa bile ayni nesneler ayni kodla (8) reddedilir -- sonda tuketilen kayit sayisi
#   (`cn == nb`) ikinci kapidir; iki kapi ayni hata kodunu verir (derinlemesine savunma).
#   gor_bagil_operand_degil (yazmac vb. isleneninde bagil kayit): kapi kaldirilsa kullanilmayan kayit sonda `cn == nb` ile ayni kodla (8) reddedilir.
#   yaz_bos_ham (`5{*1}` bos anahtar adi): kapi kaldirilsa uzunluk-0 ad hicbir anahtarla eslesmez, ayni kodla (56) reddedilir.
# ---- sayi bicimleme / ayristirma
calistir parsenum_basamak 's/gti z, nd, 20\n/gti z, nd, 19\n/'
calistir parsenum_basil_sifir 's/eqi z, c, 48\n    jz z, q3/eqi z, c, 49\n    jz z, q3/'
calistir pu_isaretsiz 's/(func pu\(v\) \{.*?)shri t, v, 1/$1sari t, v, 1/s'
calistir putu_isaretsiz 's/(func putu\(v\) \{.*?)shri t, v, 1/$1sari t, v, 1/s'
# ---- yer tutucu tanima
calistir sentinel_taban 's/(func sentinel\(v\) \{\n    ld nk, G, 8\n    mov s1, )1845493760/${1}1845493761/'
calistir sentinel_dizi_maske 's/andi t, d, 65535\n    jnz t, nn/andi t, d, 65534\n    jnz t, nn/'
calistir sentinel_dizi_kaydir 's/shri k, d, 16\n    ge z, k, nk\n    jnz z, nn/shri k, d, 15\n    ge z, k, nk\n    jnz z, nn/'
calistir sentinel_eksi 's/a2: neg w, v\n    sub d, w, s1/a2: mov w, v\n    sub d, w, s1/'
# ---- gor: adres ifadeleri
calistir gor_bss_ofset 's/addi off, off, -1\n    ld nsp, G, 26/addi off, off, 0\n    ld nsp, G, 26/'
calistir gor_baslik_ofset 's/sub d, v, hi\n    addi d, d, -1\n/sub d, v, hi\n    addi d, d, 0\n/'
calistir gor_sonra_ofset 's/addi d, d, -2\n    call x, poff, d/addi d, d, -1\n    call x, poff, d/'
calistir gor_ilk_m 's/ne z, s, stp\n    jnz z, no/eq z, s, stp\n    jnz z, no/'
calistir gor_ham_sira 's/lt z, x1, x2\n    jz z, o2n/gt z, x1, x2\n    jz z, o2n/'
calistir gor_ham_ayirac 's/call x, pc, 124/call x, pc, 44/'
calistir gor_bolge_secimi 's/gt z, so, off\n    jnz z, sbe/ge z, so, off\n    jnz z, sbe/'
calistir gor_katsayi_bir 's/eqi z, cf, 1\n    jnz z, r2a\n    mov allone, 0/eqi z, cf, 2\n    jnz z, r2a\n    mov allone, 0/'
# ---- gor: yazim
calistir gor_satir16 's/addi e, i, 16/addi e, i, 15/'
calistir gor_yorum_kosulu 's/f5: jz allok, f8\n    jz anynz, f8/f5: jz allok, f8/'
calistir gor_etiket_isaret 's/gt z, v, nc\n    jnz z, m7/ge z, v, nc\n    jnz z, m7/'
calistir gor_etiket_yaz 's/p0: gt z, i, nc\n    jnz z, p9/p0: ge z, i, nc\n    jnz z, p9/'
calistir gor_host_adi 's/ld oc, G, 4\n    add t, oc, v\n    call x, pname, t/ld oc, G, 5\n    add t, oc, v\n    call x, pname, t/'
calistir gor_sabit_yer_tutucu 's/(func consttok\(v\) \{\n    lti z, v, 0\n    jnz z, plain\n    call sk, sentinel, v)/func consttok(v) {\n    lti z, v, 0\n    jnz z, plain\n    mov sk, 0/'
calistir gor_islev_sinir 's/ld nf, G, 20\n    ge z, v, nf\n    jnz z, q4a/ld nf, G, 20\n    gt z, v, nf\n    jnz z, q4a/'
# ---- gor: dogrulama kapilari
calistir gor_baslik_tekrar 's/jz hk, d2\n    call x, fail, 11/jz hk, d2/'
calistir gor_ad_tekrar 's/call x, fail, 14\nx2:/mov zz9, 0\nx2:/'
calistir gor_bagil_tekrar 's/ne z, k2, ky\n    jnz z, q2n\n    call x, fail, 9/ne z, k2, ky\n    jnz z, q2n/'
calistir gor_bagil_skaler 's/eqi z, kd, 1\n    jnz z, q1\n    call x, fail, 9/eqi z, kd, 1\n    jnz z, q1/'
calistir gor_buyuk_sayi 's/jnz ng, n2\n    call x, fail, 12/jnz ng, n2/'
calistir gor_negatif_sabit 's/jz ng, c2\n    call x, fail, 12/jz ng, c2/'
calistir gor_ad_uzunluk 's/gti z, ln, 256\n    jnz z, nbad/gti z, ln, 255\n    jnz z, nbad/'
calistir gor_bolge_artan 's/le z, so, prev\n    jnz z, sbad/lt z, so, prev\n    jnz z, sbad/'
calistir gor_operand_aralik 's/mov s1, 2147483647\n    gt z, v, s1\n    jz z, i7/mov s1, 2147483648\n    gt z, v, s1\n    jz z, i7/'
calistir opclass_imm 's/andi t, f, 4\n    jz t, c2\n    ret 2/andi t, f, 8\n    jz t, c2\n    ret 2/'
# ---- yaz: ifadeler
calistir yaz_eksi_yer_tutucu 's/jz ng, rout\n    neg v, v/jz ng, rout\n    mov v, v/'
calistir yaz_tilde_taban 's/addi v, jj, 2\n/addi v, jj, 1\n/'
calistir yaz_amp_taban 's/addi v, jj, 1\n    add v, v, off\n    ret v\nj8:/addi v, jj, 0\n    add v, v, off\n    ret v\nj8:/'
calistir yaz_bss_taban 's/addi v, nd, 1\n    add v, v, off\n    ret v\nj3:/mov v, nd\n    add v, v, off\n    ret v\nj3:/'
calistir yaz_bolge_ofset 's/add v, v, so\n    add v, v, off\n    ret v\nj5:/add v, v, off\n    ret v\nj5:/'
calistir yaz_onceki_basliklar 's/add a, h, HDI\n    ld hi, a, 0\n    ge z, hi, j/add a, h, HDI\n    ld hi, a, 0\n    gt z, hi, j/'
calistir yaz_ham_katsayi 's/sub kl, j, p0\n/sub kl, j, p0\n    addi kl, kl, 0\n/; s/ge z, c0, e\n    jnz z, kbad/lt z, c0, e\n    jnz z, kbad/'
calistir yaz_hex_degil 's/eqi z, c, 36\n    jz z, i2/eqi z, c, 37\n    jz z, i2/'
# ---- yaz: uretim
calistir yaz_baslik_degeri 's/mov s2, 1879048192\n    add v, v, s2\n    call x, put, v/mov s2, 1879048193\n    add v, v, s2\n    call x, put, v/'
calistir yaz_dizi_kaydir 's/muli v, k, 65536/muli v, k, 65535/'
calistir yaz_veri_sayisi_katsayi 's/mov one, 1\n    call x, addrel, ndpos, k, one/mov one, 2\n    call x, addrel, ndpos, k, one/'
calistir yaz_sirala_artir 's/addi pp, pp, 1\n    st pp, a, 0/addi pp, pp, 0\n    st pp, a, 0/'
calistir yaz_baslik_sihir 's/host puti, 1347243825/host puti, 1347243826/'
calistir yaz_anahtar_taban 's/add b, s1, k\n    host puti, b/mov b, s1\n    host puti, b/'
calistir yaz_etiket_tekrar 's/call x, fail, 58\nb2c:/mov zz9, 0\nb2c:/'
calistir yaz_etiket_ad 's/call ok, isident, IB, s, l\n    jnz ok, b2\n    call x, fail, 58/call ok, isident, IB, s, l\n    jnz ok, b2/'
calistir yaz_host_adi 's/add t, oc, h\n    add a, t, NS/add t, h, h\n    add a, t, NS/'
calistir yaz_operand_sayisi 's/eq z, t, nops\n    jnz z, g4\n    call x, fail, 60/eq z, t, nops\n    jnz z, g4/'
calistir yaz_int32_alt 's/mov lo, -2147483648/mov lo, -2147483647/'
calistir yaz_int32_ust 's/mov hi, 2147483647\n    gt z, v, hi/mov hi, 2147483646\n    gt z, v, hi/'
calistir yaz_islev_tekrar 's/call x, fail, 53/mov zz9, 0/'
calistir yaz_giris_diyez 's/eqi z, c, 35\n    jz z, e1/eqi z, c, 36\n    jz z, e1/'
calistir yaz_yorum_karakteri 's/eqi z, c, 59\n    jnz z, gcm/eqi z, c, 35\n    jnz z, gcm/'
calistir yaz_virgul 's/eqi z, c, 44\n    jnz z, g1n/eqi z, c, 45\n    jnz z, g1n/'
calistir yaz_ad_uzunluk 's/gti z, l, 256\n    jz z, n2\n    call x, fail, 67/gti z, l, 257\n    jz z, n2\n    call x, fail, 67/'
calistir yaz_etiket_son 's/eqi z, c, 58\n    jnz z, g0\ng1:/eqi z, c, 57\n    jnz z, g0\ng1:/'
calistir adlar_dort 's/jnz pass, a2\n    eqi z, nt, 4/jnz pass, a2\n    eqi z, nt, 3/'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | sort | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|derlenemedi/{h++; print} END{printf "sabotaj(gorunum): %d yakalandi, %d kacti, %d uygulanamadi/derlenemedi\n", y, k, h}'
rm -rf $T
