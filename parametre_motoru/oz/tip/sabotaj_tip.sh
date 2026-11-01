#!/bin/bash
# dogrula_tip.sh'in yakaladigini kanitlar: tipdogr.pma'yi tek tek bozar (mutant), kendi kopyasinda yeniden derler ve dogrula_tip.sh'in BASARISIZ olmasini bekler.
# Kullanim: sabotaj_tip.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-4}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); . $D/../slot.sh; [ -n "$HIZLI_SLOT" ] \&\& JOBS=1000
taban() { rm -rf $1; mkdir -p $1/oz; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/elf2 $R/oz/tip $1/oz/; }
mut() {   # etiket eski yeni  (tipdogr.pma'da ilk eski metni yenisiyle degistirir)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    python3 - "$w/oz/tip/tipdogr.pma" "$2" "$3" <<'PY' || { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
import sys
p, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
s = open(p).read()
if old not in s: sys.exit(5)
open(p, 'w').write(s.replace(old, new, 1))
PY
    (cd $w/oz/tip && $PM asm tipdogr.pma -o tipdogr.pmb --strip > /dev/null 2>&1) || { echo "mutant $1: derlenemedi"; exit 4; }
    if $D/../ilkhata.sh $T/log_$1 bash -c "cd $w/oz/tip && timeout 600 ./dogrula_tip.sh $PM"; then echo "KACTI: $1"; else echo "yakalandi: $1 ($(grep -c HATA $T/log_$1) hata)"; fi
    rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; ( slot_al; "$@"; slot_birak ) & }
bg mut match_kesin 'gti z, sp, 0
    jz z, m1
    eq z, t, sp' 'gti z, sp, 0
    jz z, m1
    ne z, t, sp'
bg mut match_ayni_b 'm2: eqi z, sp, -5
    jz z, m3
    eq z, t, tb' 'm2: eqi z, sp, -5
    jz z, m3
    eq z, t, ta'
bg mut match_int_sinif 'gei z, cl, 1
    jz z, mno
    lei z, cl, 4
    ret z
m5:' 'gei z, cl, 1
    jz z, mno
    lei z, cl, 5
    ret z
m5:'
bg mut match_kayan 'm5: eqi z, sp, -2
    jz z, m6
    eqi z, cl, 5' 'm5: eqi z, sp, -2
    jz z, m6
    eqi z, cl, 1'
bg mut match_adres 'm9: ; -7 ADDR ve -10 INT64: 64 bit tamsayi
    eqi z, cl, 1
    jz z, mno
    eqi z, bt, 64' 'm9: ; -7 ADDR ve -10 INT64: 64 bit tamsayi
    eqi z, cl, 1
    jz z, mno
    eqi z, bt, 32'
bg mut match_intdst 'm8: eqi z, sp, -9
    jz z, m9
    gei z, cl, 1' 'm8: eqi z, sp, -9
    jz z, m9
    gei z, cl, 5'
bg mut donusum_komut 'eq z, o, opid
    jz z, c2
    ret 1' 'jmp c3
c3:
    jz z, c2
    ret 1'
bg mut donusum_kaynak 'ne z, f, ft
    jnz z, c2' 'ne z, f, f
    jnz z, c2'
bg mut donusum_hedef 'ne z, to, tt
    jnz z, c2' 'ne z, to, to
    jnz z, c2'
bg mut cast_ozdes 'eqi z, sa, -8
    jz z, d_r2
    mov want, -1' 'eqi z, sa, -8
    jz z, d_r2
    mov want, op'
bg mut kural_secimi 'd_rule: ; donusum kurali
    eqi z, rk, 1' 'd_rule: ; donusum kurali
    eqi z, rk, 7'
bg mut hedef_kesin 'jz rt, d_cls
    eq z, rt, dt
    jnz z, d_rule' 'jz rt, d_cls
    ne z, rt, dt
    jnz z, d_rule'
bg mut kaynak_denetimi_b 's4: call kd, isdst, c1
    jnz kd, s5' 's4: call kd, isdst, c1
    jmp s5'
bg mut kaynak_denetimi_c 's5: call kk, isreg_src, c2
    jz kk, s6
    call ok, matchslot, sc, tc, ta, tb, tc
    jnz ok, s6' 's5: call kk, isreg_src, c2
    jz kk, s6
    call ok, matchslot, sc, tc, ta, tb, tc
    jmp s6'
bg mut cagri_arguman 'call at, curtype, f, nr, pc, r
    call ct, rtype, o0, i
    eq z, at, ct' 'call at, curtype, f, nr, pc, r
    call ct, rtype, o0, i
    ge z, at, 0'
bg mut cagri_sonuc 'sc3: jz md2, sdone
    eq z, dt, crt
    jnz z, sdone' 'sc3: jz md2, sdone
    ge z, dt, 0
    jnz z, sdone'
bg mut cagri_sonuc_durum 'add a, t, ST
    st crt, a, 0
    ; cok sozcuklu' 'add a, t, ST
    mov crt, 253
    st crt, a, 0
    ; cok sozcuklu'
bg mut ret_denetimi 'eq z, ty, rt
    jnz z, sret_done' 'ge z, ty, 0
    jnz z, sret_done'
bg mut host_arguman 'call ok, matchslot, sp3, at, 0, 0, 0
    jnz ok, sh3' 'jmp sh3
sh9:
    jnz ok, sh3'
bg mut host_imzasiz 'jz md2, sh0
    call x, die, 27, f, pc
sh0: ret 0' 'jmp sh0
    call x, die, 27, f, pc
sh0: ret 0'
bg mut host_donus 'sh5: jz md2, sh0
    gti z, ret_s, 0
    jz z, sh6
    eq z, dt, ret_s' 'sh5: jz md2, sh0
    gti z, ret_s, 0
    jz z, sh6
    ge z, dt, 0'
bg mut birlestirme 'mov nv, 253
    st nv, b, 0' 'mov nv, sv
    st nv, b, 0'
bg mut atlama_yukle_b 'jz tg, dj1
    add b, gi, XB' 'jz tg, dj1
    add b, gi, XC'
bg mut atlama_hedefi 'dj1: ld tl, b, 0' 'dj1: ld tl, b, 1'
bg mut sihirli_sayi 'eqi z, m, 1347441713' 'eqi z, m, 1347441712'
bg mut surum 'ld v, PT, 1
    eqi z, v, 1' 'ld v, PT, 1
    eqi z, v, 2'
bg mut dusme 'fall: addi np, pc, 1' 'fall: addi np, pc, 2'
bg mut kayit_nregs 'eq z, pr, nr
    jnz z, t5' 'jmp t5
    jnz z, t5'
bg mut kayit_sayisi 'eq z, nent, nf
    jnz z, t2' 'jmp t2
    jnz z, t2'
bg mut kayit_sira 'ne z, fi, k
    jz z, t4' 'jmp t4
    jz z, t4'
bg mut devam_oncesi 'd5: ; DEVAM: oncesi cok sozcuklu bir bas tarafindan kapsanmali
    jnz r, d5a' 'd5: ; DEVAM: oncesi cok sozcuklu bir bas tarafindan kapsanmali
    jmp d3
    jnz r, d5a'
# cok_sozcuk_devam (q >= nr siniri) bilincle mutantsiz: sinir korumasi atlansa bile okunan sozcuk DEVAM olamayacagindan ayni hata kodu (7) cikar (denk mutant)
bg mut giris_dyn_baslangic 'eqi z, tt, 254
    jz z, a4b
    mov tt, 253' 'eqi z, tt, 254
    jz z, a4b
    call tt, i64id, 0'
# kullanilmayan alan kodlari (-4 a ile ayni, -6 c ile ayni) hicbir komut imzasinda yok: bilincle mutantsiz
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|derlenemedi/{h++; print} END{printf "sabotaj(tip): %d yakalandi, %d kacti, %d uygulanamadi/derlenemedi\n", y, k, h}'
rm -rf $T
