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
# tc ETIKET BEKLENEN "tur bildirimi" < program   (bildirim satirlari ';' ile ayrilir)
tc() {
  local et=$1 exp=$2 spec=$3; cat > $T/p.pma
  $PM asm $T/p.pma -o $T/p.pmb --strip > $T/asm.err 2>&1 || { fail "$et: derlenemedi ($(head -c 100 $T/asm.err))"; return; }
  $PM liste $T/p.pmb > $T/p.l
  local pad="--pad $T/p.l $T/imza.txt"; [ -n "$EXACT" ] && pad=""
  echo "$spec" | tr ';' '\n' | python3 $D/tipler.py $T/tur.txt $pad > $T/p.t 2>$T/t.err || { fail "$et: bildirim hatasi ($(head -c 100 $T/t.err))"; return; }
  out=$( { echo "@@IMZA"; cat $T/imza.txt; echo "@@PROGRAM"; cat $T/p.l; echo "@@TIPLER"; cat $T/p.t; } | (ulimit -v 1500000; timeout 60 $PM run $TIP) )
  [ "$out" = "$exp" ] && pass || fail "$et: beklenen '$exp', bulunan '$out'"
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
echo "tip denetleyicisi: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
