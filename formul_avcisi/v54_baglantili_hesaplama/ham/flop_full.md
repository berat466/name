### full (mc=39946; 5 kosu medyan)

| Faz | Is | TEORIK FLOP (GF) | Sure (s) | CPU-s | Cekirdek kullanimi | Ulasilan GF/s | Tepe ile orani |
|---|---|---|---|---|---|---|---|
| onhazir+okuma+F1 | F1 gemm (n+va+te) x192x512 + tanh ~59M | 24.18 | 0.510 | 1.75 | 3.42 | 47.4 | %9 (f32 tepe 546) |
| OOF/Q: K-tablolari+kapi | 6 x sgemm 2001^3 + proj gemm + Sf gemm + kapi/top-k/qtab (FLOP yalniz gemm) | 107.28 | 1.018 | 4.01 | 3.94 | 105.4 | %19 (f32 tepe 546) |
| V39 havuz | ortalama+karisim (bellek bagimli) ~4 flop/eleman | 0.25 | 0.074 | 0.22 | 2.99 | 3.4 | (bellek/ozel fonksiyon) |
| V49 donusumu | istatistik+turetilmis sutunlar ~12 flop/eleman | 0.76 | 0.216 | 0.59 | 2.72 | 3.5 | (bellek/ozel fonksiyon) |
| Gram: F^T F | n x 513^2 sgemm | 59.50 | 0.165 | 0.64 | 3.90 | 360.6 | %66 (f32 tepe 546) |
| Gram: F^T Y | n x 513 skaler ekleme | 0.06 | 0.100 | 0.38 | 3.76 | 0.6 | (bellek/ozel fonksiyon) |
| Gram: slot sayimi (ACC,ACF,RC) | n x 2 slot x 513 ekleme | 0.12 | 0.116 | 0.38 | 3.23 | 1.0 | (bellek/ozel fonksiyon) |
| grup Cholesky + Y0 + X | Cholesky sum sg^3/3 + Y0 (yoluk) + X (513 sutun) | 14.03 | 0.439 | 1.69 | 3.84 | 32.0 | %14 (f64 tepe 232) |
| T=ACF'X + U=ACF'Y0 | T dgemm 513x513xmc + U YOGUN dgemm 513 x 2001 x mc (varsayilan, bit-ozdes); matematiksel asgari U = nz x 513 | 103.04 | 0.738 | 2.90 | 3.93 | 139.6 | %60 (f64 tepe 232) |
| Schur: W_F | Cholesky 513 + 513^2 x 2001 cozum | 1.14 | 0.023 | 0.09 | 4.04 | 49.7 | %21 (f64 tepe 232) |
| W_C = Y0 - X W_F | sgemm mc x 513 x 2001 | 82.01 | 0.300 | 1.18 | 3.94 | 273.4 | %50 (f32 tepe 546) |
| Q_havuz tablosu | ~10 komsu x V x V | 0.08 | 0.027 | 0.11 | 4.11 | 3.0 | (bellek/ozel fonksiyon) |
| **Toplam** | | **392.4** | **3.73** | | | **105.3** | sgemm tepe 546 GF/s |

V53-oncesi yogun Y0 cozumu ek is yuku: +23.1 GF (Y0 yogun/seyrek: 5.9x). U icin yurutulen yogun dgemm 82.0 GF; matematiksel asgari (yalniz sifir-olmayan) 5.3 GF (15.6x). Yurutulen toplam 392.4 GF, asgari toplam 315.7 GF.
