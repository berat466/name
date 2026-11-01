#!/bin/bash
# PLAN BOLUM 01 -- PROGRAM DUZEYI TUR SISTEMI: sayisal program (PMB1 liste) + PMT1 tur notu + TYPES bolumu (komut @@imza) -> tipdogr (sayisal PMA araci).
# Tur kurallari TABLODAN gelir: komut imzalari (OPSIG), donusum matrisi (DONUSUM), host imzalari (HOSTSIG). Olumlu programlar kabul, olumsuzlar KESIN hata koduyla reddedilir.
# Kullanim: dogrula_tip.sh [pm]
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
TAB=${TAB:-$R/oz/komut/komutlar.pmk}; KOMUT=$R/oz/arac/komut.pmb; TIP=${TIP:-$D/tipdogr.pmb}
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $KOMUT); }
KM tur < $TAB > $T/tur.txt; KM imza < $TAB > $T/imza.txt
[ -s $T/imza.txt ] && pass || fail "imza sozlugu uretilemedi"
$PM asm tipdogr.pma -o $T/tip_yeni.pmb --strip > /dev/null 2>&1 && cmp -s $T/tip_yeni.pmb $D/tipdogr.pmb && pass || fail "tipdogr.pmb guncel degil (pm asm tipdogr.pma --strip)"
# tipdogr'un native ELF'i (gen2): sayisal arac kendi zincirinde uretilip calisir
$PM liste $D/tipdogr.pmb > $T/tip.l && (ulimit -v 2000000; $PM run $R/oz/elf2/gen2.pmb < $T/tip.l > $T/tipdogr.elf 2>/dev/null) && chmod +x $T/tipdogr.elf && pass || fail "tipdogr ELF uretilemedi"
# tc ETIKET BEKLENEN "tur bildirimi" < program   (bildirim satirlari ';' ile ayrilir)
tc() {
  local et=$1 exp=$2 spec=$3; cat > $T/p.pma
  $PM asm $T/p.pma -o $T/p.pmb --strip > $T/asm.err 2>&1 || { fail "$et: derlenemedi ($(head -c 100 $T/asm.err))"; return; }
  $PM liste $T/p.pmb > $T/p.l
  local pad="--pad $T/p.l $T/imza.txt"; [ -n "$EXACT" ] && pad=""
  [ -n "$LSED" ] && sed -i "$LSED" $T/p.l
  echo "$spec" | tr ';' '\n' | python3 $D/tipler.py $T/tur.txt $pad > $T/p.t 2>$T/t.err || { fail "$et: bildirim hatasi ($(head -c 100 $T/t.err))"; return; }
  [ -n "$PTSED" ] && sed -i "$PTSED" $T/p.t
  out=$( { echo "@@IMZA"; cat $T/imza.txt; echo "@@PROGRAM"; cat $T/p.l; echo "@@TIPLER"; cat $T/p.t; } | (ulimit -v 1500000; timeout 60 $PM run $TIP) )
  [ "$out" = "$exp" ] && pass || fail "$et: beklenen '$exp', bulunan '$out'"
  # ayni girdi ELF (gen2 ile uretilmis tipdogr.elf) ile: pm yorumlayicisiyla birebir ayni cikti
  oute=$( { echo "@@IMZA"; cat $T/imza.txt; echo "@@PROGRAM"; cat $T/p.l; echo "@@TIPLER"; cat $T/p.t; } | (ulimit -v 4000000; timeout 120 $T/tipdogr.elf) )
  [ "$oute" = "$out" ] && pass || fail "$et: ELF cikti '$oute' pm cikti '$out'"
}
# ---------------- OLUMLU ----------------
tc "int zinciri + donusumler" "1" "0 - : i32 i32 i32 i8 bool i32" <<'EOT'
func main() {
 ldi a, 5
 ldi b, 7
 add_i32 c, a, b
 sext8 d, c
 tobool e, d
 jz e, L
 ldi a, 9
L: halt
}
entry main
EOT
tc "cagri + host + DIN pencere" "1" "0 i32 : i32 i32 i32 i32;1 - : i32 i32 i32 dyn" <<'EOT'
func inc(a) {
 ldi b, 1
 add_i32 c, a, b
 ret c
}
func main() {
 ldi x, 3
 call y, inc, x
 add_i32 z, y, x
 host puti, z
 halt
}
entry main
EOT
tc "f32 aritmetigi + dar donusum" "1" "0 - : f64 f64 f64 f32 f32 bool i8 f32" <<'EOT'
func main() {
 mov a, 1.5
 mov b, 2.5
 fadd c, a, b
 rnd_f32 d, c
 add_f32 e, d, d
 flt g, e, d
 f2i_i8 h, e
 halt
}
entry main
EOT
tc "cast: ozdes donusum (i8 -> i64)" "1" "0 - : i8 i64 i8" <<'EOT'
func main() {
 ldi a, 3
 cast b, a
 halt
}
entry main
EOT
tc "DIN yazmac farkli turlerle yeniden kullanilir" "1" "0 - : i32 f32 dyn i32 f32 dyn" <<'EOT'
func main() {
 ldi a, 1
 mov b, 2.5
 mov d, a
 add_i32 e, a, d
 mov d, b
 add_f32 f, b, d
 halt
}
entry main
EOT
tc "bellek: ld/st adres u64, deger herhangi" "1" "0 - : u64 i32 u64 f32" <<'EOT'
func main() {
 ldi a, 8
 ldi b, 4
 st b, a, 0
 ld c, a, 0
 halt
}
entry main
EOT
# ---------------- OLUMSUZ (kesin kod, islev, komut) ----------------
tc "kaynak turu uyumsuz: add_i32 (i64 kaynak)" "0 21 0 2" "0 - : i32 i64 i32" <<'EOT'
func main() {
 ldi a, 5
 ldi b, 7
 add_i32 c, a, b
 halt
}
entry main
EOT
tc "hedef turu uyumsuz: add_i32 sonucu i64" "0 22 0 2" "0 - : i32 i32 i64" <<'EOT'
func main() {
 ldi a, 5
 ldi b, 7
 add_i32 c, a, b
 halt
}
entry main
EOT
tc "f32 komutuna f64 kaynak" "0 21 0 2" "0 - : f32 f64 f32" <<'EOT'
func main() {
 mov a, 1.5
 mov b, 2.5
 add_f32 c, a, b
 halt
}
entry main
EOT
tc "fadd'e f32 kaynak (f64 gerekir)" "0 21 0 2" "0 - : f64 f32 f64" <<'EOT'
func main() {
 mov a, 1.5
 mov b, 2.5
 fadd c, a, b
 halt
}
entry main
EOT
tc "kayan nokta ile tamsayi komutu" "0 21 0 2" "0 - : f64 f64 i64" <<'EOT'
func main() {
 mov a, 1.5
 mov b, 2.5
 add c, a, b
 halt
}
entry main
EOT
tc "MOV farkli tur" "0 22 0 1" "0 - : i32 i64" <<'EOT'
func main() {
 ldi a, 5
 mov b, a
 halt
}
entry main
EOT
tc "kosul yazmaci kayan nokta" "0 21 0 1" "0 - : f32 i32" <<'EOT'
func main() {
 mov a, 1.5
 jz a, L
 ldi b, 1
L: halt
}
entry main
EOT
tc "LDI kayan nokta hedefe" "0 22 0 0" "0 - : f32" <<'EOT'
func main() {
 ldi a, 5
 halt
}
entry main
EOT
tc "cast: ozdes olmayan donusum (i64 -> i8)" "0 23 0 1" "0 - : i64 i8" <<'EOT'
func main() {
 ldi a, 3
 cast b, a
 halt
}
entry main
EOT
tc "sext8: kayan kaynak" "0 21 0 1" "0 - : f32 i8" <<'EOT'
func main() {
 mov a, 1.5
 sext8 b, a
 halt
}
entry main
EOT
tc "donusum komutu yanlis hedef turu (sext8 -> i16)" "0 22 0 1" "0 - : i32 i16" <<'EOT'
func main() {
 ldi a, 3
 sext8 b, a
 halt
}
entry main
EOT
tc "f2i_i8: tamsayi kaynak" "0 21 0 1" "0 - : i32 i8" <<'EOT'
func main() {
 ldi a, 3
 f2i_i8 b, a
 halt
}
entry main
EOT
tc "SHR isaretli kaynak (u64 gerekir)" "0 21 0 2" "0 - : i64 i64 i64" <<'EOT'
func main() {
 ldi a, 3
 ldi b, 1
 shr c, a, b
 halt
}
entry main
EOT
tc "shl_i8 sayac kayan nokta" "0 21 0 2" "0 - : i8 f32 i8" <<'EOT'
func main() {
 ldi a, 3
 mov b, 1.5
 shl_i8 c, a, b
 halt
}
entry main
EOT
tc "bool beklenen yerde tamsayi (flt sonucu i32)" "0 22 0 2" "0 - : f32 f32 i32" <<'EOT'
func main() {
 mov a, 1.5
 mov b, 2.5
 flt c, a, b
 halt
}
entry main
EOT
tc "ld adresi f32" "0 21 0 1" "0 - : f32 i32" <<'EOT'
func main() {
 mov a, 1.5
 ld b, a, 0
 halt
}
entry main
EOT
tc "DIN yazmac bir yolda atanmadan okunur" "0 21 0 3" "0 - : i32 dyn i32" <<'EOT'
func main() {
 ldi q, 1
 jz q, L
 mov a, q
L: add_i32 b, a, a
 halt
}
entry main
EOT
tc "DIN yollar birlesince farkli tur" "0 21 0 6" "0 - : i32 f32 dyn i32" <<'EOT'
func main() {
 ldi a, 1
 mov b, 2.5
 jz a, L
 mov d, a
 jmp M
L: mov d, b
M: add_i32 e, d, d
 halt
}
entry main
EOT
tc "cagri argumani uyumsuz" "0 24 1 2" "0 i32 : i32 i32 i32 i32;1 - : i64 i32 i32 dyn" <<'EOT'
func inc(a) {
 ldi b, 1
 add_i32 c, a, b
 ret c
}
func main() {
 ldi x, 3
 call y, inc, x
 add_i32 z, y, x
 halt
}
entry main
EOT
tc "cagri sonucu uyumsuz (bildirilen i32, donus i64)" "0 25 1 1" "0 i64 : i32 i64;1 - : i32 i32 i32" <<'EOT'
func f(a) {
 ldi b, 1
 ret b
}
func main() {
 ldi x, 3
 call y, f, x
 halt
}
entry main
EOT
tc "RET turu bildirilenle ayni degil" "0 26 0 2" "0 i64 : i32 i32 i32 i32;1 - : i32" <<'EOT'
func inc(a) {
 ldi b, 1
 add_i32 c, a, b
 ret c
}
func main() {
 halt
}
entry main
EOT
tc "host arguman turu (putf'ye tamsayi)" "0 27 0 2" "0 - : i32 i32 dyn" <<'EOT'
func main() {
 ldi a, 3
 mov d, a
 host putf, d
 halt
}
entry main
EOT
# ---------------- PMT1 yapisal olumsuzlar ----------------
EXACT=1 tc "kayit sayisi islev sayisina esit degil" "0 3 0 0" "" <<'EOT'
func main() {
 halt
}
entry main
EOT
EXACT=1 tc "yazmac sayisi uyusmuyor" "0 5 0 0" "0 - : i32 i32 i32" <<'EOT'
func main() {
 ldi a, 3
 halt
}
entry main
EOT
tc "gecersiz tur kimligi" "0 6 0 0" "0 - : 200" <<'EOT'
func main() {
 ldi a, 3
 halt
}
entry main
EOT
# ---------------- cok sozcuklu bildirimler, PMT1 basligi, sira, host ----------------
tc "cok sozcuklu: gecerli parametre bildirimi (i128 + DEVAM)" "1" "0 - : i128 + i32;1 - : i32" <<'EOT'
func f(a, b, c) {
 ret c
}
func main() {
 halt
}
entry main
EOT
tc "cok sozcuklu: DEVAM eksik (son yazmac)" "0 7 0 1" "0 - : i32 i128" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 halt
}
entry main
EOT
tc "cok sozcuklu: son yazmacta bas, DEVAM yok" "0 7 0 2" "0 - : i32 i32 i128" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 halt
}
entry main
EOT
tc "cok sozcuklu: DEVAM yerine baska tur" "0 7 0 0" "0 - : i128 i32 i32" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 ldi c, 3
 halt
}
entry main
EOT
tc "DEVAM bas olmadan (ilk yazmac)" "0 7 0 0" "0 - : + i32" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 halt
}
entry main
EOT
tc "DEVAM tek sozcuklu degerin ardindan" "0 7 0 1" "0 - : i32 + i32" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 ldi c, 3
 halt
}
entry main
EOT
tc "DIN yazmac ilk durumda tanimsiz (dongu)" "0 21 0 2" "0 - : i32 i64 i64 dyn" <<'EOT'
func main() {
 ldi n, 1
 ldi one, 1
L: add t, d, one
 ldi d, 7
 jnz n, L
 halt
}
entry main
EOT
tc "host donus turu uyumsuz (getc i64, bildirilen i32)" "0 27 0 0" "0 - : i32" <<'EOT'
func main() {
 host c, getc
 halt
}
entry main
EOT
LSED='s/^40 1 /40 99 /' tc "host imzasi yok (host kimligi 99)" "0 27 0 1" "0 - : i32 i32" <<'EOT'
func main() {
 ldi a, 1
 host puti, a
 halt
}
entry main
EOT
tc "kayit sirasi: islev 1 once" "0 4 1 0" "1 - : i32;0 - : i32" <<'EOT'
func f(a) {
 ret a
}
func main() {
 halt
}
entry main
EOT
PTSED='s/^1347441713/1347441712/' tc "PMT1 sihirli sayi yanlis" "0 1 0 0" "0 - : i32" <<'EOT'
func main() {
 halt
}
entry main
EOT
PTSED='s/^1347441713 1 /1347441713 2 /' tc "PMT1 surum yanlis" "0 2 0 0" "0 - : i32" <<'EOT'
func main() {
 halt
}
entry main
EOT
# ---------------- 128 bit (genis operand) ----------------
tc "i128: pack/add/trunc/hi64 (gecerli)" "1" "0 - : i64 i64 i128 + i128 + i128 + i64 i64" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 pack_i128 y, a, b
 add_i128 z, x, y
 trunc_i128_i64 t, z
 hi64_i128 h, z
 halt
}
entry main
EOT
tc "i128 komutuna u128 kaynak" "0 21 0 4" "0 - : i64 i64 i128 + u128 + i128 + i64" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 pack_u128 y, a, b
 add_i128 z, x, y
 halt
}
entry main
EOT
tc "eq_128: i128 ile u128 karisik" "0 21 0 4" "0 - : i64 i64 i128 + u128 + bool" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 pack_u128 y, a, b
 eq_128 c, x, y
 halt
}
entry main
EOT
tc "eq_128: i128 ile i128 (gecerli, ANY128)" "1" "0 - : i64 i64 i128 + i128 + bool" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 pack_i128 y, a, b
 eq_128 c, x, y
 halt
}
entry main
EOT
tc "trunc: hedef tur uyumsuz (i64 beklenir, i32 bildirildi)" "0 22 0 3" "0 - : i64 i64 i128 + i32" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 trunc_i128_i64 t, x
 halt
}
entry main
EOT
tc "kosul: lt_i128 sonucu i32" "0 22 0 4" "0 - : i64 i64 i128 + i128 + i32" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 pack_i128 y, a, b
 lt_i128 c, x, y
 halt
}
entry main
EOT
tc "cok sozcuklu kaynak dar yazmacta (x i64)" "0 21 0 3" "0 - : i64 i64 i128 + i64 i64" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 pack_i128 x, a, b
 add_i128 z, a, x
 halt
}
entry main
EOT
tc "genis donusum: sext64_i128 kayan kaynak" "0 21 0 1" "0 - : f64 i128 + i32" <<'EOT'
func main() {
 mov a, 1.5
 sext64_i128 x, a
 halt
}
entry main
EOT
tc "genis donusum: gecerli (i32 -> u128 sext64_u128)" "1" "0 - : i32 u128 + i32" <<'EOT'
func main() {
 ldi a, 7
 sext64_u128 x, a
 halt
}
entry main
EOT
tc "genis donusum: yanlis komut (i32 -> i128 icin zext64_i128 degil sext64_i128)" "0 23 0 1" "0 - : i32 i128 + i32" <<'EOT'
func main() {
 ldi a, 7
 zext64_i128 x, a
 halt
}
entry main
EOT
# ---------------- genis kayan nokta (f80 / f128) ----------------
tc "f128/f80: aritmetik, karsilastirma, donusumler (gecerli)" "1" "0 - : i64 f64 f128 + f128 + f128 + f80 + f80 + bool bool f64 i64 f32" <<'EOT'
func main() {
 ldi a, 1
 mov d0, 1.5
 cv_i64_f128 x, a
 cv_f64_f128 y, d0
 add_f128 z, x, y
 mul_f128 z, z, y
 sqrt_f128 z, z
 cv_f128_f80 w, z
 cv_f64_f80 v, d0
 div_f80 w, w, v
 lt_f80 c, w, v
 tobool_f128 e, z
 cv_f80_f64 f, w
 cv_f80_i64 g, w
 cv_f128_f32 h, z
 halt
}
entry main
EOT
tc "add_f80 komutuna f128 kaynak" "0 21 0 4" "0 - : i64 f64 f128 + f80 + f80 +" <<'EOT'
func main() {
 ldi a, 1
 mov d0, 1.5
 cv_i64_f128 x, a
 cv_f64_f80 y, d0
 add_f80 z, x, y
 halt
}
entry main
EOT
tc "f128 hedefe f80 sonucu yazan komut (add_f128 hedefi f80)" "0 22 0 4" "0 - : i64 f64 f128 + f128 + f80 +" <<'EOT'
func main() {
 ldi a, 1
 mov d0, 1.5
 cv_i64_f128 x, a
 cv_f64_f128 y, d0
 add_f128 z, x, y
 halt
}
entry main
EOT
tc "genis kayan: dar yazmaca f128 sonucu (cv_i64_f128 hedefi i64)" "0 22 0 1" "0 - : i64 i64" <<'EOT'
func main() {
 ldi a, 1
 cv_i64_f128 b, a
 halt
}
entry main
EOT
tc "kosul: lt_f128 sonucu i32" "0 22 0 3" "0 - : i64 f128 + f128 + i32" <<'EOT'
func main() {
 ldi a, 1
 cv_i64_f128 x, a
 cv_i64_f128 y, a
 lt_f128 c, x, y
 halt
}
entry main
EOT
tc "f128 -> f80 donusumu: kaynak turu uymayan komut (cv_f80_f128 kaynagi f80, verilen f128)" "0 21 0 2" "0 - : i64 f128 + f128 +" <<'EOT'
func main() {
 ldi a, 1
 cv_i64_f128 x, a
 cv_f80_f128 y, x
 halt
}
entry main
EOT
echo "tip denetleyicisi: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
