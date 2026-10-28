#!/bin/bash
# Parametre tabanli assembler (oz/asm/asm.pmb) = C++ `pm asm --strip`: bayt-bayt cikti ve kabul/ret karari.
# Kullanim: dogrula_asm.sh [pm] [rastgele_sayisi] [mutant_sayisi]   (tum calistirmalar zaman+bellek siniri altinda)
PM=${1:-../../pm}; NR=${2:-200}; NM=${3:-1500}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
ok=0; bad=0; acc=0; rej=0; flt=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
AS="$D/asm.pmb"
[ -n "$ASMPMB" ] && AS=$ASMPMB
runasm() { (ulimit -v 900000; timeout 30 $PM run $AS < $1 > $T/mine.pmb 2> $T/mine.err); }
cmpsrc() {   # etiket kaynak  (ondalik sabit iceren kaynaklar bilinen farktir, sayilir)
  $PM asm $2 -o $T/ref.pmb --strip > /dev/null 2>&1; rr=$?
  runasm $2; rm_=$?
  if [ $rr = 0 ]; then
    if [ $rm_ = 0 ] && cmp -s $T/ref.pmb $T/mine.pmb; then pass; acc=$((acc+1))
    elif grep -q "HATA [0-9]*: 90" $T/mine.pmb; then flt=$((flt+1))
    else fail "$1: referans kabul etti, parametre rc=$rm_ $(head -c 80 $T/mine.pmb | tr -c '[:print:]\n' '?')"; cp $2 /tmp/asm_fark_$((ok+bad)).pma; fi
  else
    if [ $rm_ != 0 ]; then pass; rej=$((rej+1)); else fail "$1: referans reddetti, parametre kabul etti"; cp $2 /tmp/asm_fark_$((ok+bad)).pma; fi
  fi
}
# ---- 0) guncellik
if [ -z "$ASMPMB" ]; then cp asm.pmb $T/asm.old; ./uret.sh $PM > /dev/null 2>&1; cmp -s asm.pmb $T/asm.old && pass || fail "asm.pmb guncel degil (uret.sh)"; fi
# ---- 1) derlem: depodaki .pma + .pmt/.pmy'den uretilenler
n=0
for f in $(find $R/tests $R/uygulamalar $R/bench $R/oz $R/ai_testi* -name '*.pma' 2>/dev/null | sort); do cmpsrc "$f" $f; n=$((n+1)); done
for f in $(find $R/uygulamalar $R/tests $R/ai_testi* -name '*.pmt' 2>/dev/null | sort); do (cd $(dirname $f) && $PM sablon $f --pma $T/g.pma > /dev/null 2>&1) && { cmpsrc "pmt:$f" $T/g.pma; n=$((n+1)); }; done
for f in $(find $R/oz $R/ai_testi3 $R/ai_testi4 $R/uygulamalar -name '*.pmy' 2>/dev/null | sort); do $PM yapisal $f --pma $T/g.pma > /dev/null 2>&1 && { cmpsrc "pmy:$f" $T/g.pma; n=$((n+1)); }; done
echo "derlem: $n kaynak"
# ---- 2) rastgele programlar (iki uretec)
for s in $(seq 1 $NR); do awk -v seed=$s -f ../elf2/rastgele2.awk > $T/r.pma; cmpsrc "rastgele2-$s" $T/r.pma; awk -v seed=$s -f ../elf/rastgele_pma.awk > $T/r.pma; cmpsrc "rastgele1-$s" $T/r.pma; done
# ---- 3) elle yazilmis uc durumlar
hand() { printf '%b' "$2" > $T/h.pma; cmpsrc "elle:$1" $T/h.pma; }
hand sub_min 'func main() {\n sub x, y0, -2147483648\n}\n'
hand sub_imm 'func main() {\n ldi a, 5\n sub b, a, 7\n sub c, a, -7\n host puti, c\n}\n'
hand add_imm_ilk 'func main() {\n ldi a, 5\n add b, 3, a\n mul c, 4, a\n and d, 7, a\n eq e, 1, a\n ne f, 2, a\n host puti, f\n}\n'
hand karsilastirma 'func main() {\n ldi a, 5\n lt b, a, 3\n lt c, 3, a\n le d, a, 3\n le e, 3, a\n gt f, a, 3\n gt g, 3, a\n ge h, a, 3\n ge i, 3, a\n gt j, a, a\n ge k, a, a\n host puti, k\n}\n'
hand kaydirma 'func main() {\n ldi a, 5\n shl b, a, 3\n shr c, a, 3\n sar d, a, 3\n shl e, a, a\n host puti, e\n}\n'
hand bolme 'func main() {\n ldi a, 50\n div b, a, 5\n div c, a, 0\n div d, a, -1\n mod e, a, 7\n mod f, a, -1\n host puti, f\n}\n'
hand etiketler 'func main() {\n ldi a, 3\nL: sub a, a, 1\n jnz a, L\nM: halt\n}\n'
hand etiket_sonda 'func main() {\n ldi a, 3\n jz a, E\n host puti, a\nE:\n}\n'
hand ayni_satir_etiket 'func main() {\n ldi a, 3\nL: sub a, a, 1\n jnz a, L\n}\n'
hand veri_dizeleri 'data s = "ab\\n\\t\\x41\\x4\\\\ \\" z" 7 0x1F -3 '"'"'a'"'"' '"'"'\\n'"'"'\nfunc main() {\n ld a, s, 0\n host puti, a\n}\n'
hand veri_bos 'data s =\nfunc main() {\n halt\n}\n'
hand veri_tekrar 'data s = 1\ndata s = 2\nfunc main() {\n halt\n}\n'
hand space_veri 'data a = 1 2 3\nspace b 10\ndata c = 9\nspace d 5\nfunc main() {\n ld x, b, 0\n ld y, d, 1\n host puti, x\n}\n'
hand space_sifir 'space b 0\nfunc main() {\n halt\n}\n'
hand space_buyuk 'space b 67108865\nfunc main() {\n halt\n}\n'
hand memory_yok_kucuk 'memory 4\ndata a = 1 2 3 4 5\nfunc main() {\n halt\n}\n'
hand memory_tam 'memory 6\ndata a = 1 2 3 4 5\nfunc main() {\n halt\n}\n'
hand memory_eksi 'memory -5\nfunc main() {\n halt\n}\n'
hand giris_baska 'entry baslat\nfunc baslat() {\n host puti, 5\n}\nfunc main() {\n halt\n}\n'
hand giris_param 'func main(a) {\n halt\n}\n'
hand giris_yok 'func foo() {\n halt\n}\n'
hand cagri 'func f(a, b) {\n add c, a, b\n ret c\n}\nfunc main() {\n call r, f, 3, 4\n host puti, r\n call f, 1, 2\n}\n'
hand cagri_arg_sayisi 'func f(a, b) {\n ret a\n}\nfunc main() {\n call r, f, 3\n}\n'
hand host_biçimleri 'func main() {\n ldi c, 65\n host putc, c\n host puti, 12\n host x, getc\n host puti, x\n}\n'
hand host_arg 'func main() {\n host putc\n}\n'
hand buyuk_sabit 'func main() {\n ldi a, 4294967296\n ldi b, -4294967297\n ldi c, 0x7fffffffffffffff\n ldi d, -9223372036854775808\n mov e, 9223372036854775807\n add f, a, 9999999999\n host puti, f\n}\n'
hand sabit_tasma 'func main() {\n ldi a, 9223372036854775808\n}\n'
hand hex_tasma 'func main() {\n ldi a, 0x10000000000000000\n}\n'
hand hex_u64_tasma 'func main() {\n ldi a, 0xffffffffffffffff\n}\n'
hand dec_u64_tasma 'func main() {\n ldi a, 18446744073709551616\n}\n'
hand eksi_hex 'func main() {\n ldi a, -0x10\n ldi b, +0x10\n ldi c, 0X1f\n}\n'
hand hex_bos 'func main() {\n ldi a, 0x\n}\n'
hand yorumlar 'func main() { ; yorum\n}\n'
hand yorum_icinde_noktali 'data s = "a;b" 5 ; yorum\nfunc main() {\n ld a, s, 0 ; x\n host puti, a\n}\n'
hand karakter 'func main() {\n ldi a, '"'"';'"'"'\n host puti, a\n}\n'
hand ret_sabit 'func f() {\n ret 7\n}\nfunc main() {\n call r, f\n host puti, r\n ret 3\n}\n'
hand ret_yok 'func f() {\n}\nfunc main() {\n call r, f\n}\n'
hand mov_sabit 'func main() {\n mov a, 5\n mov b, a\n mov c, a\n mov a, c\n host puti, b\n}\n'
hand gozetleme_a 'func f(a) {\n add t, a, 1\n mov w, t\n ret w\n}\nfunc main() {\n call r, f, 5\n host puti, r\n}\n'
hand gozetleme_cagri 'func g(a, b) {\n sub c, a, b\n ret c\n}\nfunc main() {\n ldi x, 9\n ldi y, 4\n add t, x, y\n call r, g, t, y\n host puti, r\n}\n'
hand ad_cakismasi 'func main() {\n ldi add, 5\n}\n'
hand ad_cakismasi_host 'func main() {\n ldi putc, 5\n}\n'
hand ad_cakismasi_fonk 'func f() {\n halt\n}\nfunc main() {\n ldi f, 5\n}\n'
hand ad_cakismasi_veri 'data d = 1\nfunc main() {\n ldi d, 5\n}\n'
hand yazmac_gt_adi 'func main() {\n ldi gt, 5\n host puti, gt\n}\n'
hand tanimsiz 'func main() {\n add a, b, 1\n}\n'
hand ldk_metinde 'func main() {\n ldk a, 0\n}\n'
hand bilinmeyen_komut 'func main() {\n zzz a, b\n}\n'
hand operand_sayisi 'func main() {\n add a, b\n}\n'
hand etiket_yok 'func main() {\n jmp Q\n}\n'
hand etiket_tekrar 'func main() {\nL:\nL:\n halt\n}\n'
hand etiket_ek 'func main() {\nL: x\n halt\n}\n'
hand fonk_tekrar 'func f() {\n halt\n}\nfunc f() {\n halt\n}\nfunc main() {\n halt\n}\n'
hand fonk_kapanmadi 'func main() {\n halt\n'
hand fonk_basligi 'func main {\n halt\n}\n'
hand fonk_sonrasi_ayni_satir 'func main() { halt\n}\n'
hand fonk_param_tekrar 'func main() {\n halt\n}\nfunc f(a, a) {\n halt\n}\n'
hand ust_duzey_bilinmeyen 'foo bar\nfunc main() {\n halt\n}\n'
hand bos_dosya ''
hand yalniz_yorum '; sadece yorum\n'
hand crlf 'func main() {\r\n ldi a, 5\r\n host puti, a\r\n}\r\n'
hand sekme_virgul 'func\tmain()\t{\n\tldi\ta,\t5\n\tadd b,a,a\n\thost puti,b\n}\n'
hand ondalik 'func main() {\n fadd a, 1.5, 2.5\n}\n'
hand tamsayi_imm_sinir 'func main() {\n ldi a, 2147483647\n ldi b, -2147483648\n ld c, a, 2147483647\n ld d, a, -2147483648\n}\n'
hand ld_imm_tasma 'func main() {\n ldi a, 1\n ld c, a, 2147483648\n}\n'
hand st_ld 'memory 16\nfunc main() {\n ldi a, 3\n st a, a, 1\n ld b, a, 1\n host puti, b\n}\n'
hand alloc_ 'func main() {\n alloc p, 5\n st p, p, 0\n host puti, p\n}\n'
hand veri_adresi_imm 'data t = 5 6 7\nfunc main() {\n ld a, t, 2\n add b, a, t\n addi c, a, t\n host puti, b\n}\n'
hand kayan 'func main() {\n i2f a, 3\n f2i b, a\n host puti, b\n}\n'
hand cok_arg 'func f(a, b, c, d) {\n ret d\n}\nfunc main() {\n call r, f, 1, 2, 3, 4\n call s, f, r, 2, 3, 4\n host puti, s\n}\n'
# ---- 4) kaynak mutantlari (kabul/ret karari ve kabul edilince bayt-bayt cikti)
src=($(find $R/tests $R/uygulamalar $R/bench -name '*.pma' | sort | grep -v float))
for s in $(seq 1 $NM); do f=${src[$((s % ${#src[@]}))]}; awk -v seed=$s -v k=$((1 + s % 3)) -f mutpma.awk $f > $T/m.pma; cmpsrc "mutant-$s ($(basename $f))" $T/m.pma; done
echo "mutantlar tamam"
# ---- 5) KENDI KAYNAGINI DERLER (sabit nokta) ve diger araclarin kaynaklarini
for f in asm.pma ../elf2/gen2.pma ../dogr/valid.pma; do $PM asm $f -o $T/ref.pmb --strip > /dev/null 2>&1; runasm $f; cmp -s $T/ref.pmb $T/mine.pmb && pass || fail "$f: kendi kaynagi/arac kaynagi farkli"; done
runasm asm.pma; cmp -s $T/mine.pmb asm.pmb && pass || fail "asm.pmb kendi kaynagindan uretilen asm.pmb ile ayni degil (sabit nokta)"
# ---- 6) ELF olarak: asm.elf (gen2 ile uretilmis) ayni cikti
$PM liste $AS > $T/a.l; (ulimit -v 900000; $PM run $D/../elf2/gen2.pmb < $T/a.l > $T/asm.elf) && chmod +x $T/asm.elf && pass || fail "asm.pmb ELF'e cevrilemedi"
na=0; for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' | sort | grep -v float | head -40); do (ulimit -v 900000; timeout 30 $T/asm.elf < $f > $T/e.pmb 2>/dev/null); re=$?; runasm $f; rp=$?
  if [ $re = $rp ] && cmp -s $T/e.pmb $T/mine.pmb; then pass; else fail "asm.elf farkli: $f (rc $re/$rp)"; fi; na=$((na+1)); done
$T/asm.elf < asm.pma | cmp -s - asm.pmb && pass || fail "asm.elf kendi kaynagindan asm.pmb uretmedi"
rm -rf $T
echo "assembler (parametre): gecen $ok, kalan $bad (C++ ile ayni uretim: $acc, ayni ret: $rej, bilinen fark - ondalik sabit: $flt)"; [ $bad = 0 ]
