# Açık dünya (tarayıcı, Three.js)

Çalıştırmak için:

```
cd game
python3 -m http.server 8000
```

Sonra tarayıcıda http://localhost:8000 adresini aç.

Harita: Busan, 60,5 × 60,5 km. Veri `data/` klasöründe, `tools/build_map.py` ile üretilir
(Copernicus GLO-30 yükseklik + OpenStreetMap yol ve su).

- Doğal arazi (`heights.u16`) hiç değiştirilmez. Kesim-dolgu oyunda ayrı katman olarak uygulanır.
- Yol kotu tüm ağ üzerinde çözülür: kavşaklar aynı kotta, köprüler alttaki yolun 5,5 m üstünde.
- Yakın arazi 2 m ızgarayla yeniden üretilir: yol platformu düz, kenarlarda şev (yarma 1:1,5, dolgu 1:2),
  şev sığmazsa istinat duvarı. Yol, kaldırım ve boya aynı yüzeye basılır.
- Köprüler ve tünel ağızları ayrı 3B modeldir.

Yükseklik: Copernicus DEM GLO-30 © DLR e.V. ve © Airbus Defence and Space GmbH (Copernicus, AB ve ESA).
Yol ve su: © OpenStreetMap katkıda bulunanlar (ODbL). Dokular: Poly Haven (CC0).

## Kontroller
- **Telefon:** uçuş modu. Sol yarıya bas ve sürükle → ilerle; sağ yarıyı sürükle → etrafına bak; ▲ ▼ → yüksel / alçal;
  yuvarlak düğme → hız; sağ üstteki düğme → harita modu. Telefonda doku 1K (2K için `?q=2k`).
- **Bilgisayar:** harita modu (fare). ✈ düğmesi uçuş moduna geçirir (WASD + fare, Q/E, Shift).
