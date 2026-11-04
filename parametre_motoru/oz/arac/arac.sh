# Araclari (doldur, opstab, fmtc, komut, asm...) calistirmak icin ortak yardimci. `. arac.sh` ile kullanilir.
#   ARAC tanimliysa: $ARAC/AD.elf (bagimsiz ELF araclar; pm yok)   degilse: "$PM run oz/arac/AD.pmb" (onyukleme; PM = pm yolu)
# arac AD  : stdin -> stdout calistirir
ARACDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
arac() { local n=$1; if [ -n "$ARAC" ] && [ -x "$ARAC/$n.elf" ]; then (ulimit -v 900000; timeout 60 "$ARAC/$n.elf"); else (ulimit -v 900000; timeout 60 "${PM:?pm yolu ya da ARAC gerekli}" run "$ARACDIR/$n.pmb"); fi; }
# sinif: asm girdi.pma cikti.pmb
arac_asm() { if [ -n "$ASM2" ]; then $ASM2 "$1" "$2"; else "${PM:?}" asm "$1" -o "$2" --strip; fi; }
# arac_pmb CIKTI.pmb : stdin = pm liste bicimi -> PMB1 (ARAC varsa yaz.elf, yoksa `pm yazliste`)
arac_pmb() { local out="$1"; if [ -n "$ARAC" ] && [ -x "$ARAC/yaz.elf" ]; then (ulimit -v 900000; timeout 60 "$ARAC/yaz.elf") > "$out"; else local tmp; tmp=$(mktemp); cat > "$tmp"; "${PM:?}" yazliste "$tmp" -o "$out" > /dev/null; local rc=$?; rm -f "$tmp"; return $rc; fi; }
