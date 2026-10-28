# Sayi listesini bayta cevirir. Yalnizca bicim kodlar; komutlari, etiketleri, adlari bilmez.
#   N    isaretsiz tamsayi -> LEB128
#   zN   isaretli tamsayi  -> zigzag + LEB128   (ornek: z-5, z0, z12)
# '#' satir sonuna kadar aciklamadir. Her bayt tek bir sayi degildir: sayi 128 ve uzeriyse birden cok bayt uretir.
function leb(v,   b) {
  while (v >= 128) { b = v % 128 + 128; printf "%c", b; v = int(v / 128) }
  printf "%c", v
}
{
  sub(/#.*/, "")
  for (i = 1; i <= NF; i++) {
    t = $i
    if (t ~ /^z-?[0-9]+$/) {
      n = substr(t, 2) + 0
      leb(n >= 0 ? 2 * n : -2 * n - 1)
    } else if (t ~ /^[0-9]+$/) leb(t + 0)
    else { printf "PAKET_HATA: satir %d: gecersiz sayi '%s'\n", NR, t > "/dev/stderr"; bad = 1 }
  }
}
END { exit bad }
