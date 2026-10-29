#!/bin/bash
# MOTORU KAYNAKTAN YENIDEN URET (pm, g++, awk, sed, od OLMADAN): eski ELF takimi (ESKI) ile oz/ kaynaklarindan YENI takimi uretir.
# Girdiler (HEPSI SAYISAL; metin kaynagi yok): oz/komut/komutlar.pmk (KANONIK tablo: komutlar, anlamlar, kurallar, x86 sablonlari), oz/nesne/*.pmo (asm/valid/gen2 sayisal nesneleri), oz/arac/*.pmb ve oz/yap/yap.pmb (sayisal PMB1 araclari), oz/elf2/{rt.pmb,statik.sozluk}, oz/{paketle.pmy,coz.sayi.in,yaz.sayi.in} (sayisal sablonlar).
# Adimlar: 1) asm/valid/gen2: SAYISAL NESNE + komut tablosu -> ESKI baglayici.elf -> ESKI yaz.elf (METINSIZ); oz/arac ve yap: depodaki sayisal PMB1  2) coz/yaz: sayisal sablon (.sayi.in) + tablo -> paketle -> PMY1 -> yap.elf -> asm.elf (olusturulan ara metin yalnizca uretilir, insan metni degil)
#          3) PMB1 -> liste (ESKI coz.elf), dogrulama (ESKI valid.elf: "1 ..." sart)  4) liste -> ELF: ESKI gen2.elf ile ara nesil, ara nesil gen2.elf ile son nesil
#          5) sabit nokta: son gen2.elf kendi ELF'ini bayt-bayt uretir.
# Kullanim: yenile.sh ESKI_DIZIN YENI_DIZIN [KAYNAK_KOK]   (KAYNAK_KOK: oz/ altini iceren agac; varsayilan depo koku)
ESKI=$(realpath "${1:?eski arac dizini}"); YENI=${2:?yeni dizin}; mkdir -p "$YENI"; YENI=$(realpath "$YENI"); D=$(cd "$(dirname "$0")" && pwd); R=$(realpath "${3:-$D/../..}")
W=$YENI/work; rm -rf $W; mkdir -p $W/oz/komut $W/oz/arac $W/oz/asm $W/oz/dogr $W/oz/elf2 $W/oz/yap $W/oz/nesne
# YALNIZ SAYISAL GIRDILER kopyalanir (hicbir .pma / .pma.in / fmt.txt metni yok): tablo, sayisal nesneler, sayisal PMB1 araclari, sayisal sozlukler, PMY1 sablonlari
cp $R/oz/komut/komutlar.pmk $W/oz/komut/; cp $R/oz/arac/*.pmb $R/oz/arac/arac.sh $W/oz/arac/; cp $R/oz/nesne/*.pmo $W/oz/nesne/
cp $R/oz/asm/uret.sh $W/oz/asm/; cp $R/oz/dogr/uret.sh $W/oz/dogr/; cp $R/oz/elf2/uret.sh $R/oz/elf2/rt.pmb $R/oz/elf2/statik.sozluk $W/oz/elf2/; cp $R/oz/yap/yap.pmb $W/oz/yap/
cp $R/oz/paketle.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $W/oz/
cat > $W/asmwrap.sh <<EOW
#!/bin/bash
(ulimit -v 900000; timeout 60 $ESKI/asm.elf < "\$1" > "\$2") || { echo "asm.elf basarisiz: \$1" >&2; exit 1; }
EOW
chmod +x $W/asmwrap.sh; export ASM2=$W/asmwrap.sh ARAC=$ESKI
for t in asm elf2 dogr; do (cd $W/oz/$t && ./uret.sh) > $W/uret_$t.log 2>&1 || { echo "yenile: $t derlenemedi: $(tail -2 $W/uret_$t.log)"; exit 1; }; done
(cd $W/oz && ./uret_ikili.sh "" $W) > $W/uret_ikili.log 2>&1 || { echo "yenile: coz/yaz tablolari komut tablosundan uretilemedi: $(tail -2 $W/uret_ikili.log)"; exit 1; }
cp $W/oz/paketle.pmy $W/paketle.pmy
for t in paketle coz yaz; do (ulimit -v 900000; timeout 60 $ESKI/yap.elf < $W/$t.pmy > $W/$t.pma && timeout 60 $ESKI/asm.elf < $W/$t.pma > $W/$t.pmb) || { echo "yenile: $t.pmy cevrilemedi"; exit 1; }; done
declare -A KAYNAK=([gen2]=$W/oz/elf2/gen2.pmb [asm]=$W/oz/asm/asm.pmb [valid]=$W/oz/dogr/valid.pmb [yap]=$W/oz/yap/yap.pmb [doldur]=$W/oz/arac/doldur.pmb [opstab]=$W/oz/arac/opstab.pmb [fmtc]=$W/oz/arac/fmtc.pmb [komut]=$W/oz/arac/komut.pmb [baglayici]=$W/oz/arac/baglayici.pmb [gorunum]=$W/oz/arac/gorunum.pmb [paketle]=$W/paketle.pmb [coz]=$W/coz.pmb [yaz]=$W/yaz.pmb)
ARALAR="gen2 asm valid yap doldur opstab fmtc komut baglayici gorunum paketle coz yaz"
for b in $ARALAR; do (ulimit -v 900000; timeout 60 $ESKI/coz.elf < ${KAYNAK[$b]} > $W/$b.l) || { echo "yenile: coz.elf reddetti: $b"; exit 1; }
  read -r V < <( (ulimit -v 900000; timeout 60 $ESKI/valid.elf < $W/$b.l) ); case "$V" in 1*) ;; *) echo "yenile: dogrulayici reddetti $b ('$V')"; exit 1;; esac; done
mkdir -p $YENI/ara $YENI/son
for b in $ARALAR; do (ulimit -v 900000; timeout 60 $ESKI/gen2.elf < $W/$b.l > $YENI/ara/$b.elf) || { echo "yenile: ara $b uretilemedi"; exit 1; }; done
chmod +x $YENI/ara/*.elf
for b in $ARALAR; do (ulimit -v 900000; timeout 60 $YENI/ara/gen2.elf < $W/$b.l > $YENI/son/$b.elf) || { echo "yenile: son $b uretilemedi"; exit 1; }; done
chmod +x $YENI/son/*.elf
$YENI/son/gen2.elf < $W/gen2.l | cmp -s - $YENI/son/gen2.elf || { echo "yenile: sabit nokta bozuk"; exit 1; }
cmp -s $YENI/ara/gen2.elf $YENI/son/gen2.elf && echo "yenile: ara nesil == son nesil (uretici degismedi)" || echo "yenile: ara nesil != son nesil (uretici DEGISTI; son nesil kendini yeniden uretti)"
cmp -s $ESKI/gen2.elf $YENI/son/gen2.elf && echo "yenile: yeni gen2.elf == eski gen2.elf" || echo "yenile: yeni gen2.elf != eski gen2.elf ($(stat -c %s $ESKI/gen2.elf) -> $(stat -c %s $YENI/son/gen2.elf) bayt)"
for b in $ARALAR; do cp $YENI/son/$b.elf $YENI/$b.elf; done
echo "yenile: tamam -> $YENI/{${ARALAR// /,}}.elf"
