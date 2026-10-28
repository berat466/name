# Araclari (doldur, opstab, fmtc, sablonc, asm...) calistirmak icin ortak yardimci. `. arac.sh` ile kullanilir.
#   ARAC tanimliysa: $ARAC/AD.elf (bagimsiz ELF araclar; pm yok)   degilse: "$PM run oz/arac/AD.pmb" (onyukleme; PM = pm yolu)
# arac AD  : stdin -> stdout calistirir
ARACDIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
arac() { local n=$1; if [ -n "$ARAC" ] && [ -x "$ARAC/$n.elf" ]; then (ulimit -v 900000; timeout 60 "$ARAC/$n.elf"); else (ulimit -v 900000; timeout 60 "${PM:?pm yolu ya da ARAC gerekli}" run "$ARACDIR/$n.pmb"); fi; }
# sinif: asm girdi.pma cikti.pmb
arac_asm() { if [ -n "$ASM2" ]; then $ASM2 "$1" "$2"; else "${PM:?}" asm "$1" -o "$2" --strip; fi; }
