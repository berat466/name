
### full (n=5 tur, medyan; model hazir min-maks: once 16.77-17.92, sonra 5.11-6.32, sonra-U1 4.95-5.60)

| Asama | ONCE | SONRA | SONRA-U1 |
|---|---|---|---|
| Veri okuma | 0.049 | 0.049 | 0.047 |
| Program basi -> zincir (okuma+gomme genisletme+n-gram ogretici) | 0.176 | 0.181 | 0.174 |
| OOF/Q (V51 K-tablolari + kapi) | 2.264 | 2.275 | 2.338 |
| Ozellik uretimi (F1, V39 havuz, V49) | 1.136 | 1.169 | 1.141 |
| Gram/RHS (F^TF, F^TY, C-blok sayimi, slot/tampon) | 0.838 | 0.406 | 0.379 |
| Schur/Cholesky (grup Cholesky + Y0 + X + T + U) | 8.539 | 1.190 | 0.832 |
|   alt kalem: Y0 cozumu (duvar payi) | 4.984 | 0.088 | 0.074 |
|   alt kalem: Y0 ile ilgili toplam (cozum + float donusum/U + ekleme) | 6.825 | 0.761 | 0.457 |
| Saf agirlik uretimi (W_F cozumu + W_C) | 3.383 | 0.258 | 0.317 |
| Q_havuz tablosu | 0.179 | 0.051 | 0.050 |
| Diger/olculmeyen | 0.529 | 0.046 | 0.047 |
| TAM MODEL HAZIR | 17.185 | 5.598 | 5.286 |
| Validation degerlendirmesi (ayri) | 0.403 | 0.238 | 0.219 |
| Tepe RAM (MB) | 4229 | 2124 | 1923 |
| Validation CE | 4.609033 | 4.609033 | 4.609033 |

### hafif (n=5 tur, medyan; model hazir min-maks: once 6.12-6.59, sonra 4.25-4.44, sonra-U1 3.97-4.67)

| Asama | ONCE | SONRA | SONRA-U1 |
|---|---|---|---|
| Veri okuma | 0.044 | 0.045 | 0.043 |
| Program basi -> zincir (okuma+gomme genisletme+n-gram ogretici) | 0.162 | 0.169 | 0.170 |
| OOF/Q (V51 K-tablolari + kapi) | 2.248 | 2.174 | 2.222 |
| Ozellik uretimi (F1, V39 havuz, V49) | 1.132 | 1.104 | 1.157 |
| Gram/RHS (F^TF, F^TY, C-blok sayimi, slot/tampon) | 0.568 | 0.341 | 0.311 |
| Schur/Cholesky (grup Cholesky + Y0 + X + T + U) | 1.940 | 0.362 | 0.322 |
|   alt kalem: Y0 cozumu (duvar payi) | 1.280 | 0.026 | 0.028 |
|   alt kalem: Y0 ile ilgili toplam (cozum + float donusum/U + ekleme) | 1.505 | 0.260 | 0.213 |
| Saf agirlik uretimi (W_F cozumu + W_C) | 0.226 | 0.136 | 0.135 |
| Q_havuz tablosu | 0.050 | 0.050 | 0.052 |
| Diger/olculmeyen | 0.131 | 0.041 | 0.042 |
| TAM MODEL HAZIR | 6.437 | 4.390 | 4.468 |
| Validation degerlendirmesi (ayri) | 0.303 | 0.220 | 0.224 |
| Tepe RAM (MB) | 2393 | 1346 | 1346 |
| Validation CE | 4.612900 | 4.612900 | 4.612900 |