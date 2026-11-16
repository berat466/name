import * as THREE from 'three';
import { MapControls } from 'three/addons/controls/MapControls.js';

// ---------------------------------------------------------------------------
// Sahne
// ---------------------------------------------------------------------------
const params = new URLSearchParams(location.search);
// dokunmatik cihaz (telefon/tablet): uçuş kamerası, düşük bellek ve hafif çizim
const isTouch = params.get('touch') === '1' || (params.get('touch') !== '0' &&
  (matchMedia('(pointer: coarse)').matches || 'ontouchstart' in window));
const renderer = new THREE.WebGLRenderer({ antialias: !isTouch });
renderer.setPixelRatio(Math.min(devicePixelRatio, isTouch ? 1.5 : 2));
renderer.setSize(innerWidth, innerHeight);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.0;
document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();
const SKY = new THREE.Color(0x9fb7cc);
scene.background = SKY;
scene.fog = new THREE.FogExp2(SKY, 0.000045);

const camera = new THREE.PerspectiveCamera(55, innerWidth / innerHeight, 2, 80000);

scene.add(new THREE.HemisphereLight(0xcfe3ff, 0x4a4030, 0.9));
const sun = new THREE.DirectionalLight(0xfff1dc, 2.6);
sun.position.set(-0.5, 0.8, 0.35).multiplyScalar(10000);
scene.add(sun);

const controls = new MapControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.maxPolarAngle = Math.PI * 0.49;
controls.minDistance = 20;
controls.maxDistance = 45000;
controls.screenSpacePanning = false;
controls.zoomToCursor = true;

// ---------------------------------------------------------------------------
// Dokular (Poly Haven, CC0, 2K) — birden çok PBR seti doku dizisi (texture array) olarak
// ---------------------------------------------------------------------------
const maxAniso = renderer.capabilities.getMaxAnisotropy();
// Telefonda 1K (4 kat az bellek); ?q=2k ile 2K zorlanabilir
const TEX = (isTouch && params.get('q') !== '2k') ? 1024 : 2048;

async function loadImg(url) {
  const img = new Image();
  img.src = url;
  await img.decode();
  return img;
}
// Her set için iki katman üretir:
//   albedo  : diff.jpg (sRGB)
//   nra     : R,G = normal.xy · B = pürüzlülük · A = ortam kapatma (AO)
async function loadPbrArray(names) {
  const n = names.length, layer = TEX * TEX * 4;
  const alb = new Uint8Array(layer * n), nra = new Uint8Array(layer * n);
  const cv = document.createElement('canvas');
  cv.width = cv.height = TEX;
  const ctx = cv.getContext('2d', { willReadFrequently: true });
  const read = (img) => { ctx.drawImage(img, 0, 0, TEX, TEX); return ctx.getImageData(0, 0, TEX, TEX).data; };

  // tüm görselleri aynı anda indir ve çöz (sırayla beklemek açılışı uzatıyordu)
  const loading = names.map((nm0) => Promise.all(['diff', 'nor_gl', 'arm'].map((m) => loadImg(`./textures/${nm0}/${m}.jpg`))));
  for (let i = 0; i < n; i++) {
    const [d, nm, arm] = await loading[i];
    alb.set(read(d), layer * i);
    const N = read(nm), A = read(arm), o = layer * i;
    for (let p = 0; p < layer; p += 4) {
      nra[o + p] = N[p]; nra[o + p + 1] = N[p + 1]; nra[o + p + 2] = A[p + 1]; nra[o + p + 3] = A[p];
    }
  }
  const mk = (data, srgb) => {
    const t = new THREE.DataArrayTexture(data, TEX, TEX, n);
    t.wrapS = t.wrapT = THREE.RepeatWrapping;
    t.minFilter = THREE.LinearMipmapLinearFilter;
    t.magFilter = THREE.LinearFilter;
    t.generateMipmaps = true;
    t.anisotropy = maxAniso;
    if (srgb) t.colorSpace = THREE.SRGBColorSpace;
    t.needsUpdate = true;
    return t;
  };
  return { alb: mk(alb, true), nra: mk(nra, false) };
}

// Arazi katmanları (sıra shader ile aynı olmalı)
const TERRAIN_SETS = [
  'forrest_ground_01', // 0 çim
  'aerial_grass_rock', // 1 çim + taş
  'dirt_floor',        // 2 toprak lekeleri
  'forest_leaves_02',  // 3 orman zemini
  'aerial_rocks_02',   // 4 kaya
  'mossy_rock',        // 5 yosunlu kaya
];
// Yol katmanları
// 0-2 asfalt, 3-4 toprak (yol kenarı ve boşluklar), 5 beton (köprü)
const ROAD_SETS = ['asphalt_02', 'aerial_asphalt_01', 'asphalt_04', 'dirt_floor', 'brown_mud', 'concrete_pavement_02'];

const [terrainTex, roadTex] = await Promise.all([TERRAIN_SETS, ROAD_SETS].map(loadPbrArray));

// Ortak GLSL: gürültü, iki ölçekli örnekleme, nra çözme
const GLSL_COMMON = `
  float hash(vec2 p){ vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
  float vnoise(vec2 p){
    vec2 i = floor(p), f = fract(p); f = f*f*(3.0-2.0*f);
    return mix(mix(hash(i), hash(i+vec2(1,0)), f.x), mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y);
  }
  // Tekrar desenini kırmak için aynı katmanı iki ölçekte örnekle
  vec4 smp(highp sampler2DArray t, vec2 uv, float l){
    return mix(texture(t, vec3(uv, l)), texture(t, vec3(uv * 0.137 + 0.31, l)), 0.35);
  }
  vec3 accA; vec4 accN; float accW;
  void addLayer(highp sampler2DArray A, highp sampler2DArray N, vec2 uv, float l, float w){
    if (w < 0.004) return;
    accA += smp(A, uv, l).rgb * w;
    accN += smp(N, uv, l) * w;
    accW += w;
  }
`;
// Ortak parça: birikmiş katmanlardan normal / pürüzlülük / AO çıkar (dünya uzayı)
const GLSL_RESOLVE = `
  accA /= accW; accN /= accW;
  vec2 nxy = accN.xy * 2.0 - 1.0;
  nxy.y = -nxy.y; // doku dizisi Y çevrilmeden yüklendi
  vec3 tn = vec3(nxy, sqrt(max(0.0, 1.0 - dot(nxy, nxy))));
  blendR = accN.z; blendAO = accN.w;
`;

let shaderId = 0;
function patchStandard(mat, uniforms, vtxExtra, fragDecl, fragBody, fixedKey) {
  // her malzeme türü kendi shader'ını derlesin (aksi halde three.js aynı programı paylaştırır);
  // fixedKey verilirse aynı türden malzemeler (ör. her yol karosu) tek programı paylaşır
  const key = fixedKey || 'patched-' + (shaderId++);
  mat.customProgramCacheKey = () => key;
  mat.onBeforeCompile = (sh) => {
    Object.assign(sh.uniforms, uniforms);
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', '#include <common>\nvarying vec3 vWP;\nvarying vec3 vWN;\n' + (vtxExtra.decl || ''))
      .replace('#include <project_vertex>', `#include <project_vertex>
        vWP = (modelMatrix * vec4(transformed, 1.0)).xyz;
        vWN = normalize(mat3(modelMatrix) * objectNormal);
        ${vtxExtra.body || ''}`);
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', `#include <common>
        varying vec3 vWP;
        varying vec3 vWN;
        uniform highp sampler2DArray tA, tN;
        vec3 blendN; float blendR, blendAO;
        ${GLSL_COMMON}
        ${fragDecl}`)
      .replace('#include <map_fragment>', fragBody)
      .replace('#include <roughnessmap_fragment>', 'float roughnessFactor = roughness * blendR;')
      .replace('#include <normal_fragment_maps>', 'normal = normalize((viewMatrix * vec4(blendN, 0.0)).xyz);')
      .replace('#include <aomap_fragment>', 'reflectedLight.indirectDiffuse *= mix(1.0, blendAO, 0.8);');
  };
}

// ---------------------------------------------------------------------------
// Arazi malzemesi: eğim, yükseklik ve gürültüye göre 6 PBR setinin karışımı
// ---------------------------------------------------------------------------
function terrainMaterial() {
  const mat = new THREE.MeshStandardMaterial({ roughness: 1, metalness: 0,
    polygonOffset: true, polygonOffsetFactor: 2, polygonOffsetUnits: 4 });  // arazi yolun gerisinde: taşma olmaz
  patchStandard(mat, { tA: { value: terrainTex.alb }, tN: { value: terrainTex.nra },
    tRoad: { value: overviewTex }, tWater: { value: waterTex }, uWorld: { value: new THREE.Vector2(Wm, Dm) },
    uNear: { value: new THREE.Vector2(PATCH_RADIUS * 0.82, PATCH_RADIUS * 1.0) } }, {}, 'uniform sampler2D tRoad, tWater; uniform vec2 uWorld, uNear;', `
    vec3 N0 = normalize(vWN);
    float slope = 1.0 - N0.y;
    float n1 = vnoise(vWP.xz * 0.004) * 0.6 + vnoise(vWP.xz * 0.02) * 0.4;
    float n2 = vnoise(vWP.xz * 0.011 + 17.0);
    float n3 = vnoise(vWP.xz * 0.012 + 91.0) * 0.7 + vnoise(vWP.xz * 0.06 + 5.0) * 0.3;

    // bölgeler
    float rock = smoothstep(0.30, 0.50, slope + (n1 - 0.5) * 0.15);
    float forest = smoothstep(55.0, 110.0, vWP.y + (n1 - 0.5) * 70.0) * (1.0 - rock);
    forest = max(forest, smoothstep(0.10, 0.22, slope) * (1.0 - rock) * 0.9);
    float grass = max(0.0, 1.0 - rock - forest);

    // bölge içi set karışımı
    float gB = smoothstep(0.35, 0.65, n2);            // çim A <-> çim+taş
    float dirt = smoothstep(0.58, 0.85, n3) * 0.55;   // toprak lekeleri (yumuşak geçiş)
    float rB = smoothstep(0.4, 0.6, n2);              // kaya <-> yosunlu kaya
    float fB = smoothstep(0.45, 0.75, n3) * 0.5;      // orman zemininde çimli açıklıklar

    float w0 = grass * (1.0 - gB) * (1.0 - dirt);
    float w1 = grass * gB * (1.0 - dirt) + forest * fB;
    float w2 = grass * dirt;
    float w3 = forest * (1.0 - fB);
    float w4 = rock * (1.0 - rB);
    float w5 = rock * rB;

    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vWP.xz / 5.0, 0.0, w0);
    addLayer(tA, tN, vWP.xz / 16.0, 1.0, w1);
    addLayer(tA, tN, vWP.xz / 4.0, 2.0, w2);
    addLayer(tA, tN, vWP.xz / 3.0, 3.0, w3);
    addLayer(tA, tN, vWP.xz / 14.0, 4.0, w4);
    addLayer(tA, tN, vWP.xz / 6.0, 5.0, w5);
    ${GLSL_RESOLVE}

    vec3 col = accA;
    col *= mix(vec3(1.0), vec3(0.85, 1.0, 0.7), grass);                // çimi biraz canlandır
    col = mix(col, vec3(0.10, 0.16, 0.06), forest * 0.45);              // yukarıdan ağaç örtüsü
    col *= 0.85 + 0.3 * vnoise(vWP.xz * 0.0015);                        // makro varyasyon
    // uzaktaki yollar: tüm şehrin düşük çözünürlüklü yol maskesi (yakında basılı karolar çizer)
    float rmask = texture2D(tRoad, (vWP.xz + uWorld * 0.5) / uWorld).r;
    float farF = smoothstep(uNear.x, uNear.y, length(vWP.xz - cameraPosition.xz));
    col = mix(col, vec3(0.2, 0.2, 0.22), rmask * farF * 0.9);
    // su: kıyıda sığ yeşilimsi, ortada derin mavi
    float wsoft = texture2D(tWater, (vWP.xz + uWorld * 0.5) / uWorld).r;
    float wm = smoothstep(0.4, 0.6, wsoft);
    float shore = smoothstep(0.4, 0.95, wsoft);
    vec3 waterCol = mix(vec3(0.16, 0.26, 0.22), vec3(0.04, 0.13, 0.2), shore);
    col = mix(col, waterCol, wm);
    blendR = mix(blendR, 0.06, wm);
    diffuseColor.rgb *= col;

    vec3 Tg = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
    vec3 Bt = normalize(cross(Tg, N0));
    blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
    blendN = normalize(mix(blendN, N0, wm));   // su düz
  `);
  return mat;
}

// ---------------------------------------------------------------------------
// Yol malzemesi: 3 asfalt PBR seti + lastik izi, yağ lekesi, kenar kiri + şerit çizgileri + yaya geçidi
//   ruv    = (yol boyunca metre, enine metre; sol kenar = -yarı genişlik)
//   rdir   = yol yönü (x, z)
//   rinfo  = (şerit sayısı, tek yön mü, şerit genişliği, kavşağa uzaklık)
//   rinfo2 = (sol omuz, sağ omuz, yarı genişlik, yaya geçidi var mı)
// ---------------------------------------------------------------------------
const ROAD_VTX = {
  decl: `attribute vec2 ruv; attribute vec2 rdir; attribute vec4 rinfo; attribute vec4 rinfo2;
    varying vec2 vRuv; varying vec2 vRdir; varying vec4 vInfo; varying vec4 vInfo2;`,
  body: 'vRuv = ruv; vRdir = rdir; vInfo = rinfo; vInfo2 = rinfo2;',
};
const ROAD_FRAG_DECL = 'varying vec2 vRuv; varying vec2 vRdir; varying vec4 vInfo; varying vec4 vInfo2;';
// yol boyunca teğet uzay (Tg = boyuna, Bt = enine)
const GLSL_ROAD_TBN = `
  vec3 N0 = normalize(vWN);
  vec3 Tg = normalize(vec3(vRdir.x, 0.0, vRdir.y));
  vec3 Bt = normalize(cross(N0, Tg));
  blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
`;

function roadMaterial() {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: roadTex.alb }, tN: { value: roadTex.nra } }, ROAD_VTX, ROAD_FRAG_DECL + `
    float AA;  // türev döngü dışında hesaplanır (döngü içinde tanımsız olur)
    float aaLine(float d, float w){
      float cov = clamp(w / AA, 0.0, 1.0);  // piksel altı çizgiyi soldur, titremesin
      return (1.0 - smoothstep(max(0.0, w - AA), w + AA, abs(d))) * cov;
    }
    float band(float d, float w, float soft){ return 1.0 - smoothstep(w, w + soft, abs(d)); }
  `, `
    float along = vRuv.x, u = vRuv.y;
    float lanes = vInfo.x, oneway = vInfo.y, lw = vInfo.z, jd = vInfo.w;
    float shL = vInfo2.x, shR = vInfo2.y, halfW = vInfo2.z, zebra = vInfo2.w;
    AA = fwidth(u) + 0.005;

    // --- 3 asfalt PBR seti ---
    float n1 = vnoise(vWP.xz * 0.02);
    float n2 = vnoise(vWP.xz * 0.07 + 33.0);
    float wB = smoothstep(0.4, 0.8, n1) * 0.6;     // farklı asfalt yüzeyi
    float wC = smoothstep(0.55, 0.85, n2) * 0.5;   // çatlak / yama
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / 5.0, 0.0, (1.0 - wB) * (1.0 - wC));
    addLayer(tA, tN, vRuv / 8.0, 1.0, wB * (1.0 - wC));
    addLayer(tA, tN, vRuv / 4.0, 2.0, wC);
    ${GLSL_RESOLVE}
    vec3 col = accA * vec3(0.62, 0.63, 0.66);      // koyu, hafif mavimsi şehir asfaltı

    // carriageway (omuzlar hariç) ve şerit merkezleri
    float cL = -halfW + shL, cR = halfW - shR;
    float laneF = (u - cL) / lw;                    // soldan kaçıncı şerit (kesirli)
    float inLane = fract(laneF) - 0.5;              // şerit içi konum (-0.5..0.5)
    float onCarriage = step(cL, u) * step(u, cR);

    // lastik izleri: her şeritte iki tekerlek yolu, daha koyu ve daha az pürüzlü
    float tire = (band(inLane * lw - 0.85, 0.25, 0.35) + band(inLane * lw + 0.85, 0.25, 0.35)) * onCarriage;
    tire *= 0.6 + 0.4 * vnoise(vec2(along * 0.05, laneF * 3.0));
    // yağ lekesi: şerit ortasında, kesik kesik
    float oil = band(inLane * lw, 0.35, 0.4) * smoothstep(0.55, 0.8, vnoise(vec2(along * 0.12, floor(laneF) * 7.0))) * onCarriage;
    // kenar kiri: bordüre yakın birikmiş kir
    float edge = 1.0 - smoothstep(0.0, 0.9, min(u + halfW, halfW - u));
    col *= 1.0 - 0.18 * tire - 0.25 * oil;
    col = mix(col, col * vec3(0.85, 0.8, 0.72), edge * 0.6);
    blendR = clamp(blendR - 0.12 * tire - 0.25 * oil, 0.2, 1.0);

    // --- şerit çizgileri (orta sarı çift çizgi, beyaz kesikli şerit, beyaz kenar çizgisi) ---
    float dash = step(fract(along / 15.0), 0.4);     // 6 m çizgi, 9 m boşluk
    float white = max(aaLine(u - cL, 0.10), aaLine(u - cR, 0.10)) * step(0.2, min(shL, shR) + 0.2);
    float yellow = 0.0;
    if (oneway < 0.5) {
      float mid = (cL + cR) * 0.5;
      yellow = max(aaLine(u - mid - 0.15, 0.08), aaLine(u - mid + 0.15, 0.08));
      float side = max(1.0, floor(lanes / 2.0));
      float slw = (cR - cL) * 0.5 / side;
      for (int k = 1; k < 5; k++) {
        if (float(k) >= side) break;
        white = max(white, max(aaLine(u - mid - slw * float(k), 0.08), aaLine(u - mid + slw * float(k), 0.08)) * dash);
      }
    } else {
      for (int k = 1; k < 10; k++) {
        if (float(k) >= lanes) break;
        white = max(white, aaLine(u - cL - lw * float(k), 0.08) * dash);
      }
    }
    float wear = mix(0.65, 1.0, vnoise(vec2(along * 0.25, 7.0)));
    float fade = smoothstep(0.0, 4.0, jd);           // kavşak içinde çizgi yok
    white *= fade * wear; yellow *= fade * wear;

    // yaya geçidi (zebra): kavşağa 1-5 m uzaklıkta, 0.5 m beyaz / 0.5 m boşluk
    float zb = zebra * step(1.0, jd) * step(jd, 5.0) * step(0.5, fract(u / 1.0)) * onCarriage;
    white = max(white, zb * wear);

    col = mix(col, vec3(0.8), white);
    col = mix(col, vec3(0.85, 0.6, 0.07), yellow);
    blendR = mix(blendR, 0.5, max(white, yellow));
    diffuseColor.rgb *= col;
    ${GLSL_ROAD_TBN}
  `);
  return mat;
}

// Kaldırım / bordür / bariyer malzemesi: 2 PBR seti (parke taşı, beton)
function walkMaterial() {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: roadTex.alb }, tN: { value: roadTex.nra } }, ROAD_VTX, ROAD_FRAG_DECL, `
    float layer = vInfo.x;  // 0 = parke, 1 = beton
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / 2.5, 5.0, 1.0);
    ${GLSL_RESOLVE}
    diffuseColor.rgb *= accA * (0.85 + 0.25 * vnoise(vWP.xz * 0.05));
    ${GLSL_ROAD_TBN}
  `);
  return mat;
}

// ---------------------------------------------------------------------------
// Yükseklik verisi
// ---------------------------------------------------------------------------
const meta = await (await fetch('./data/terrain.json')).json();
const H = new Float32Array(await (await fetch('./data/heights.f32')).arrayBuffer());
const { cols, rows, width: Wm, depth: Dm } = meta;
const dx = Wm / (cols - 1), dz = Dm / (rows - 1);

const hAt = (c, r) => H[Math.min(rows - 1, Math.max(0, r)) * cols + Math.min(cols - 1, Math.max(0, c))];
// Dünya koordinatında (x, z) çift doğrusal yükseklik
export function heightAt(x, z) {
  const fc = (x + Wm / 2) / dx, fr = (z + Dm / 2) / dz;
  const c = Math.floor(fc), r = Math.floor(fr), u = fc - c, v = fr - r;
  return (hAt(c, r) * (1 - u) + hAt(c + 1, r) * u) * (1 - v) + (hAt(c, r + 1) * (1 - u) + hAt(c + 1, r + 1) * u) * v;
}

// ---------------------------------------------------------------------------
// Yollar (OpenStreetMap, gerçek Seul yol ağı)
//   1) her yolu örnekle, araziye göre eğim sınırlı profil çıkar
//   2) kavşakları bul, yolları kavşakta aynı yükseklikte buluştur
//   3) araziyi yola oturt (yarma / dolgu + şev)
//   4) görüntüyü kameranın çevresinde akış halinde üret (GTA tarzı streaming)
// ---------------------------------------------------------------------------
const roadData = await fetch('./data/roads.json').then((r) => (r.ok ? r.json() : null)).catch(() => null);
const waterData = await fetch('./data/water.json').then((r) => (r.ok ? r.json() : null)).catch(() => null);
const ROAD_ORDER = { residential: 0, unclassified: 0, tertiary: 1, secondary: 2, primary: 3, trunk: 4, motorway: 5 };
const MINOR = new Set(['residential', 'unclassified', 'tertiary']);
// sınıf -> [örnekleme aralığı m, profil yumuşatma (örnek), en fazla eğim]
const PROFILE = {
  motorway: [6], trunk: [6], primary: [6], secondary: [8],
  tertiary: [10], unclassified: [10], residential: [10],
};
const PIECE = 48;          // parça başına en fazla nokta
const TILE = 1500;         // akış karosu (m)
const CURB_H = 0.15, SKIRT = 2.0, LIFT = 0.12;   // yol, arazinin 12 cm üstünde (z-çakışması olmasın)
const smooth = (a, b, x) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };

const preparedRoads = [];
function tileAt(x, z) {
  const tx = Math.floor(x / TILE), tz = Math.floor(z / TILE), key = `${tx},${tz}`;
  let t = roadTileMap.get(key);
  if (!t) roadTileMap.set(key, (t = { pieces: [], built: {}, cx: (tx + 0.5) * TILE, cz: (tz + 0.5) * TILE }));
  return t;
}
const roadTileMap = new Map();   // "tx,tz" -> köprü parçaları (vektör geometri)

// Zemindeki yollar "basılı doku" olarak çizilir (aşağıda). Her 256 m'lik karo için hangi yolların ve
// kavşakların o karoya değdiğini tutarız.
const BT = 256;                                  // basılı yol karosu (m)
const bakeTiles = new Map();                     // sayısal anahtar -> { tx, tz, roads: [], junc: [] }
const bkey = (tx, tz) => (tx + 512) * 1024 + (tz + 512);
function bakeTile(tx, tz) {
  const k = bkey(tx, tz);
  let t = bakeTiles.get(k);
  if (!t) bakeTiles.set(k, (t = { tx, tz, roads: [], junc: [], patch: null }));
  return t;
}

function prepareRoads() {
  if (!roadData) return;
  // Kavşaklar: birden fazla yolun paylaştığı OSM noktaları
  const jkey = (x, z) => `${Math.round(x * 2)},${Math.round(z * 2)}`;
  const jmap = new Map();
  for (const road of roadData.roads) {
    for (let i = 0; i < road.p.length; i += 2) {
      const k = jkey(road.p[i], road.p[i + 1]);
      let e = jmap.get(k);
      if (!e) jmap.set(k, (e = { n: 0, mid: 0, hw: 0, x: road.p[i], z: road.p[i + 1], arms: null }));
      e.n++;
      e.hw = Math.max(e.hw, road.w / 2);
      if (i > 0 && i < road.p.length - 2) e.mid++;   // yolun ortasından geçiyor
    }
  }
  // gerçek kavşak: 3+ yol uçu ya da bir yol başka yolun ortasından geçiyor.
  // İki yolun yalnızca uç uca eklendiği nokta (OSM'in yolu bölmesi) kavşak değildir.
  const isJ = (e) => e && (e.n > 2 || (e.n === 2 && e.mid > 0));

  const roads = roadData.roads;
  for (let ri = 0; ri < roads.length; ri++) {
    const road = roads[ri], p = road.p;

    if (!road.b) {   // zemin yolları arazinin üstüne basılır; köprüler ayrı 3B model (aşağıda)
      // --- zemin yolu: dokuya basılacak. Değdiği karoları kaydet.
      const pad = road.w / 2 + road.s + 3;
      const seen = new Set();
      const add = (x, z) => {
        const x0 = Math.floor((x - pad) / BT), x1 = Math.floor((x + pad) / BT);
        const z0 = Math.floor((z - pad) / BT), z1 = Math.floor((z + pad) / BT);
        for (let a = x0; a <= x1; a++) for (let b = z0; b <= z1; b++) seen.add(bkey(a, b));
      };
      for (let i = 0; i < p.length - 2; i += 2) {
        const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
        const n = Math.max(1, Math.ceil(Math.hypot(bx - ax, bz - az) / 16));
        for (let s = 0; s < n; s++) add(ax + (bx - ax) * s / n, az + (bz - az) * s / n);
      }
      add(p[p.length - 2], p[p.length - 1]);
      for (const k of seen) bakeTile((k >> 10) - 512, (k & 1023) - 512).roads.push(ri);

      // kavşak kolları: her kavşak noktasında bu yolun düğümden uzağa giden yönleri
      for (let i = 0; i < p.length; i += 2) {
        const e = jmap.get(jkey(p[i], p[i + 1]));
        if (!isJ(e)) continue;
        e.arms ||= [];
        for (const j of [i - 2, i + 2]) {
          if (j < 0 || j >= p.length) continue;
          let dx = p[j] - p[i], dz = p[j + 1] - p[i + 1];
          const l = Math.hypot(dx, dz);
          if (l < 0.5) continue;
          e.arms.push({ dx: dx / l, dz: dz / l, w: road.w, sl: road.sl, sr: road.sr, s: road.s, c: road.c });
        }
      }
      continue;
    }

    // --- köprü: vektör geometri (tabliye, korkuluk, ayaklar)
    const [STEP] = PROFILE[road.c];
    const xs = [], zs = [];
    for (let i = 0; i < p.length; i += 2) {
      if (i >= p.length - 2) { xs.push(p[i]); zs.push(p[i + 1]); break; }
      const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
      const n = Math.max(1, Math.ceil(Math.hypot(bx - ax, bz - az) / STEP));
      for (let s = 0; s < n; s++) { xs.push(ax + (bx - ax) * s / n); zs.push(az + (bz - az) * s / n); }
    }
    const N = xs.length;
    if (N < 2) continue;
    const dist = new Float32Array(N);
    for (let i = 1; i < N; i++) dist[i] = dist[i - 1] + Math.hypot(xs[i] - xs[i - 1], zs[i] - zs[i - 1]);
    const raw = xs.map((x, i) => heightMesh(x, zs[i]));
    let ys = raw.map((y) => y + LIFT);
    // köprü: uçlar arasında düz tabliye, altındaki araziden en az 1 m yukarıda
    const y0 = ys[0], y1 = ys[N - 1];
    const total = dist[N - 1];
    ys = ys.map((y, i) => Math.max(raw[i] + LIFT + Math.min(1.5, 0.075 * Math.min(dist[i], total - dist[i])),
      y0 + (y1 - y0) * i / (N - 1)));
    const dirx = new Float32Array(N), dirz = new Float32Array(N);
    for (let i = 0; i < N; i++) {
      const a = Math.max(0, i - 1), c = Math.min(N - 1, i + 1);
      const tx = xs[c] - xs[a], tz = zs[c] - zs[a], l = Math.hypot(tx, tz) || 1;
      dirx[i] = tx / l; dirz[i] = tz / l;
    }
    const R = { road, N, xs: Float32Array.from(xs), zs: Float32Array.from(zs), ys: Float32Array.from(ys), dist,
      jd: new Float32Array(N).fill(1e6), dirx, dirz, zebra: 0 };
    preparedRoads.push(R);
    for (let a = 0; a < N - 1; a += PIECE - 1) {
      const e = Math.min(N, a + PIECE), m = (a + e) >> 1;
      tileAt(R.xs[m], R.zs[m]).pieces.push({ R, a, e });
    }
  }

  // kavşakları, yarıçaplarının değdiği karolara kaydet
  for (const e of jmap.values()) {
    if (!isJ(e) || !e.arms) continue;
    const m = e.hw + 8;
    for (let a = Math.floor((e.x - m) / BT); a <= Math.floor((e.x + m) / BT); a++)
      for (let b = Math.floor((e.z - m) / BT); b <= Math.floor((e.z + m) / BT); b++) bakeTile(a, b).junc.push(e);
  }
}

// ---------------------------------------------------------------------------
// Basılı yollar (oyunlardaki yöntem): yol, kaldırım, şerit çizgisi, yaya geçidi ve kavşaklar
// 256 m'lik karo başına iki maske dokusuna çizilir; doku, arazinin yüzeyine oturan ince bir
// ağın üstüne basılır. Yol bu yüzden arazinin parçasıdır, kavşaklar tek parçadır.
//   doku 1:  R = asfalt, G = kaldırım, B = beyaz boya (asfalt üstünde R=B=1)
//   doku 2:  R = sarı boya, G = lastik izi
// ---------------------------------------------------------------------------
const BRES = isTouch ? 512 : 1024;     // karo doku çözünürlüğü (piksel); 0.25 m/px (telefonda 0.5 m/px)
const BPAD = 2;                         // karo kenar payı (m): komşu karolar arasında dikiş olmasın
const BEXT = BT + 2 * BPAD, BK = BRES / BEXT;   // px / m
const PATCH_RADIUS = isTouch ? 900 : 1300;       // bu yarıçapta basılı yollar çizilir
const GROUND_LIFT = 0.1;                         // yol arazinin 10 cm üstünde (z-çakışması olmasın)

function bakeMasks(t) {
  const x0 = t.tx * BT - BPAD, z0 = t.tz * BT - BPAD;
  const mk = () => {
    const c = document.createElement('canvas');
    c.width = c.height = BRES;
    const g = c.getContext('2d', { alpha: false });
    g.fillStyle = '#000'; g.fillRect(0, 0, BRES, BRES);
    g.lineJoin = 'round'; g.lineCap = 'butt';
    return [c, g];
  };
  const [c1, g1] = mk(), [c2, g2] = mk();
  const X = (x) => (x - x0) * BK, Z = (z) => (z - z0) * BK;
  const roads = t.roads.map((i) => roadData.roads[i]).sort((a, b) => ROAD_ORDER[a.c] - ROAD_ORDER[b.c]);

  // polilinyi çiz; off (m): yolun sağına doğru kaydırma
  const trace = (g, p, off) => {
    const n = p.length / 2;
    g.beginPath();
    for (let i = 0; i < n; i++) {
      let x = X(p[2 * i]), z = Z(p[2 * i + 1]);
      if (off) {
        const a = Math.max(0, i - 1), b = Math.min(n - 1, i + 1);
        let tx = p[2 * b] - p[2 * a], tz = p[2 * b + 1] - p[2 * a + 1];
        const l = Math.hypot(tx, tz) || 1; tx /= l; tz /= l;
        x += tz * off * BK; z -= tx * off * BK;
      }
      if (i) g.lineTo(x, z); else g.moveTo(x, z);
    }
  };
  const line = (g, p, off, color, widthM, dash) => {
    trace(g, p, off);
    g.strokeStyle = color; g.lineWidth = Math.max(widthM * BK, 0.9);
    g.setLineDash(dash ? [6 * BK, 9 * BK] : []);
    g.stroke();
  };

  // 1) kaldırımlar (asfaltın altında kalır) 2) asfalt
  for (const r of roads) if (r.s > 0) line(g1, r.p, 0, '#0f0', r.w + 2 * r.s + 0.04);   // yol kenarı: toprak şerit
  for (const r of roads) line(g1, r.p, 0, '#f00', r.w + 0.04);   // her kenar +2 cm: komşu parçalar birbirine biner

  // 3) boya: sarı orta çizgi, beyaz şerit ayırıcı, kenar çizgisi, lastik izi (yerel sokaklarda yok)
  for (const r of roads) {
    if (r.c === 'residential' || r.c === 'unclassified') continue;
    const half = r.w / 2, cL = -half + r.sl, cR = half - r.sr, W = '#f0f';
    const centers = [];
    if (!r.o) {
      const mid = (cL + cR) / 2, side = Math.max(1, Math.floor(r.n / 2)), slw = (cR - cL) / 2 / side;
      line(g2, r.p, mid - 0.15, '#f00', 0.13); line(g2, r.p, mid + 0.15, '#f00', 0.13);   // sarı çift çizgi
      for (let k = 1; k < side; k++) { line(g1, r.p, mid - slw * k, W, 0.13, true); line(g1, r.p, mid + slw * k, W, 0.13, true); }
      for (let k = 1; k <= side; k++) { centers.push(mid - slw * (k - 0.5)); centers.push(mid + slw * (k - 0.5)); }
    } else {
      for (let k = 1; k < r.n; k++) line(g1, r.p, cL + r.lw * k, W, 0.13, true);
      for (let k = 1; k <= r.n; k++) centers.push(cL + r.lw * (k - 0.5));
    }
    line(g1, r.p, cL, W, 0.14); line(g1, r.p, cR, W, 0.14);                              // kenar çizgileri
    if (ROAD_ORDER[r.c] >= 2) for (const c of centers) { line(g2, r.p, c - 0.85, 'rgb(0,110,0)', 0.45); line(g2, r.p, c + 0.85, 'rgb(0,110,0)', 0.45); }
  }

  // 4) kavşaklar: ortadaki boyayı sil (düz asfalt), kollara yaya geçidi çiz
  for (const e of t.junc) {
    const rad = (e.hw + 1.5) * BK;
    // doku 1: yalnızca beyaz boyayı sil (multiply ile B=0; asfalt ve kaldırım korunur, şişkinlik olmaz)
    g1.globalCompositeOperation = 'multiply';
    g1.fillStyle = 'rgb(255,255,0)';
    g1.beginPath(); g1.arc(X(e.x), Z(e.z), rad, 0, Math.PI * 2); g1.fill();
    g1.globalCompositeOperation = 'source-over';
    g2.fillStyle = '#000';                       // doku 2: sarı çizgi ve lastik izi sil
    g2.beginPath(); g2.arc(X(e.x), Z(e.z), rad, 0, Math.PI * 2); g2.fill();
    for (const a of e.arms) {
      continue;  // sadeleştirme: yaya geçidi yok
      const usable = a.w - a.sl - a.sr, d0 = e.hw + 1.2, nx = a.dz, nz = -a.dx;   // nx,nz: yolun sağı
      g1.strokeStyle = '#f0f'; g1.lineWidth = 0.5 * BK; g1.setLineDash([]);
      for (let j = 0; j < Math.floor(usable); j++) {
        const o = -usable / 2 + 0.5 + j;
        g1.beginPath();
        g1.moveTo(X(e.x + a.dx * d0 + nx * o), Z(e.z + a.dz * d0 + nz * o));
        g1.lineTo(X(e.x + a.dx * (d0 + 3) + nx * o), Z(e.z + a.dz * (d0 + 3) + nz * o));
        g1.stroke();
      }
    }
  }
  return [c1, c2];
}

// maske dokularını arazi yüzeyine oturan ince bir ağa basar
function buildPatch(t) {
  const [c1, c2] = bakeMasks(t);
  const tex = (c) => {
    const tx = new THREE.CanvasTexture(c);
    tx.flipY = false;
    tx.wrapS = tx.wrapT = THREE.ClampToEdgeWrapping;
    tx.minFilter = THREE.LinearMipmapLinearFilter; tx.magFilter = THREE.LinearFilter;
    tx.anisotropy = maxAniso;
    return tx;
  };
  const t1 = tex(c1), t2 = tex(c2);
  const x0 = t.tx * BT - BPAD, z0 = t.tz * BT - BPAD;
  const NS = Math.ceil(BEXT / 2), n = NS + 1, step = BEXT / NS;   // ~2 m ızgara
  const pos = new Float32Array(n * n * 3), nor = new Float32Array(n * n * 3), uv = new Float32Array(n * n * 2);
  let v = 0;
  for (let r = 0; r < n; r++) for (let c = 0; c < n; c++, v++) {
    const x = x0 + c * step, z = z0 + r * step;
    pos[3 * v] = x; pos[3 * v + 1] = heightMesh(x, z) + GROUND_LIFT; pos[3 * v + 2] = z;
    const hx = heightMesh(x + 1.5, z) - heightMesh(x - 1.5, z), hz = heightMesh(x, z + 1.5) - heightMesh(x, z - 1.5);
    const l = Math.hypot(hx, 3, hz);
    nor[3 * v] = -hx / l; nor[3 * v + 1] = 3 / l; nor[3 * v + 2] = -hz / l;
    uv[2 * v] = c / NS; uv[2 * v + 1] = r / NS;
  }
  const idx = new Uint16Array(NS * NS * 6);
  let k = 0;
  for (let r = 0; r < NS; r++) for (let c = 0; c < NS; c++) {
    const a = r * n + c, b = a + 1, d = a + n, e = d + 1;
    idx[k++] = a; idx[k++] = d; idx[k++] = b; idx[k++] = b; idx[k++] = d; idx[k++] = e;
  }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.BufferAttribute(nor, 3));
  g.setAttribute('uv', new THREE.BufferAttribute(uv, 2));
  g.setIndex(new THREE.BufferAttribute(idx, 1));
  g.computeBoundingSphere();
  const mesh = new THREE.Mesh(g, roadPatchMaterial(t1, t2));
  mesh.renderOrder = 1;
  mesh.userData = { t1, t2 };
  scene.add(mesh);
  t.patch = mesh;
}
function disposePatch(t) {
  const m = t.patch;
  if (!m) return;
  scene.remove(m);
  m.geometry.dispose(); m.material.dispose(); m.userData.t1.dispose(); m.userData.t2.dispose();
  t.patch = null;
}
// kameranın çevresindeki karoları bas, uzaktakileri sil
function streamPatches(px, pz, maxBuild) {
  const tx0 = Math.floor(px / BT), tz0 = Math.floor(pz / BT), rng = Math.ceil(PATCH_RADIUS / BT) + 1;
  const want = [];
  for (let a = tx0 - rng; a <= tx0 + rng; a++) for (let b = tz0 - rng; b <= tz0 + rng; b++) {
    const t = bakeTiles.get(bkey(a, b));
    if (!t || !t.roads.length) continue;
    const d = Math.hypot((a + 0.5) * BT - px, (b + 0.5) * BT - pz);
    if (d < PATCH_RADIUS && !t.patch) want.push([d, t]);
  }
  want.sort((p, q) => p[0] - q[0]);
  for (let i = 0; i < Math.min(want.length, maxBuild); i++) buildPatch(want[i][1]);
  for (const t of bakeTiles.values()) {
    if (t.patch && Math.hypot((t.tx + 0.5) * BT - px, (t.tz + 0.5) * BT - pz) > PATCH_RADIUS + 400) disposePatch(t);
  }
}

// Basılı yol malzemesi: asfalt 3 + kaldırım 2 PBR seti, maskeye göre karışır
function roadPatchMaterial(t1, t2) {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: roadTex.alb }, tN: { value: roadTex.nra }, tM1: { value: t1 }, tM2: { value: t2 } },
    { decl: 'varying vec2 vMask;', body: 'vMask = uv;' },
    'varying vec2 vMask; uniform sampler2D tM1, tM2;', `
    vec4 m1 = texture2D(tM1, vMask), m2 = texture2D(tM2, vMask);
    float cov = m1.r + m1.g;
    if (cov < 0.5) discard;                         // yol/kaldırım değil: arazi görünsün
    float wa = m1.r / cov, ws = 1.0 - wa;           // asfalt / kaldırım ağırlığı
    float n1 = vnoise(vWP.xz * 0.02), n2 = vnoise(vWP.xz * 0.07 + 33.0), n3 = vnoise(vWP.xz * 0.05 + 7.0);
    float wB = smoothstep(0.4, 0.8, n1) * 0.6, wC = smoothstep(0.55, 0.85, n2) * 0.5, cw = smoothstep(0.45, 0.75, n3);
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vWP.xz / 5.0, 0.0, wa * (1.0 - wB) * (1.0 - wC));
    addLayer(tA, tN, vWP.xz / 8.0, 1.0, wa * wB * (1.0 - wC));
    addLayer(tA, tN, vWP.xz / 4.0, 2.0, wa * wC);
    addLayer(tA, tN, vWP.xz / 4.0, 3.0, ws * (1.0 - cw));
    addLayer(tA, tN, vWP.xz / 4.0, 4.0, ws * cw);
    ${GLSL_RESOLVE}
    vec3 col = accA * mix(vec3(0.52, 0.47, 0.42), vec3(0.62, 0.63, 0.66), wa);   // toprak koyu kahve, asfalt koyu gri
    col *= 1.0 - 0.2 * (4.0 * m1.r * m1.g);         // asfalt / toprak sınırında hafif koyulaşma
    col *= 1.0 - 0.4 * m2.g * wa;                   // lastik izleri
    float wear = mix(0.65, 1.0, vnoise(vWP.xz * 0.35));
    float white = smoothstep(0.3, 0.7, m1.b) * wear * wa;
    float yellow = smoothstep(0.3, 0.7, m2.r) * wear * wa;
    col = mix(col, vec3(0.82), white);
    col = mix(col, vec3(0.85, 0.6, 0.07), yellow);
    blendR = mix(blendR, 0.5, max(white, yellow));
    diffuseColor.rgb *= col;
    vec3 N0 = normalize(vWN);
    vec3 Tg = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
    vec3 Bt = normalize(cross(Tg, N0));
    blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
  `, 'road-patch');
  return mat;
}

// Su maskesi (Han Nehri, göller): tüm şehir tek doku, arazi shader'ı suyu boyar
function buildWaterTex() {
  const OW = 4096, OH = Math.round(OW * Dm / Wm), sx = OW / Wm, sz = OH / Dm;
  const c = document.createElement('canvas');
  c.width = OW; c.height = OH;
  const g = c.getContext('2d', { alpha: false });
  g.fillStyle = '#000'; g.fillRect(0, 0, OW, OH);
  g.fillStyle = '#fff';
  if (waterData) {
    for (const rings of waterData.polys) {
      g.beginPath();
      for (const r of rings) {
        for (let i = 0; i < r.length; i += 2) {
          const x = (r[i] + Wm / 2) * sx, z = (r[i + 1] + Dm / 2) * sz;
          if (i) g.lineTo(x, z); else g.moveTo(x, z);
        }
        g.closePath();
      }
      g.fill('evenodd');   // dış halka dolu, iç halkalar (adalar) boş
    }
  }
  const tx = new THREE.CanvasTexture(c);
  tx.flipY = false;
  tx.wrapS = tx.wrapT = THREE.ClampToEdgeWrapping;
  tx.anisotropy = maxAniso;
  return tx;
}

// Uzaktaki yollar: tüm şehrin tek bir düşük çözünürlüklü maskesi (arazi shader'ı okur)
function buildOverview() {
  const OW = 4096, OH = Math.round(OW * Dm / Wm), sx = OW / Wm, sz = OH / Dm;
  const c = document.createElement('canvas');
  c.width = OW; c.height = OH;
  const g = c.getContext('2d', { alpha: false });
  g.fillStyle = '#000'; g.fillRect(0, 0, OW, OH);
  g.strokeStyle = '#fff'; g.lineJoin = 'round'; g.lineCap = 'round';
  if (roadData) {
    for (const cls of ['residential', 'unclassified', 'tertiary', 'secondary', 'primary', 'trunk', 'motorway']) {
      g.beginPath();
      let w = 0, any = false;
      for (const r of roadData.roads) {
        if (r.c !== cls) continue;
        any = true; w = Math.max(w, r.w + 2 * r.s);
        for (let i = 0; i < r.p.length; i += 2) {
          const x = (r.p[i] + Wm / 2) * sx, z = (r.p[i + 1] + Dm / 2) * sz;
          if (i) g.lineTo(x, z); else g.moveTo(x, z);
        }
      }
      if (!any) continue;
      g.lineWidth = Math.max(1.3, w * sx * 0.7);   // uzaktan en az ~1,3 px görünsün
      g.stroke();
    }
  }
  const tx = new THREE.CanvasTexture(c);
  tx.flipY = false;
  tx.wrapS = tx.wrapT = THREE.ClampToEdgeWrapping;
  tx.anisotropy = maxAniso;
  return tx;
}

class StripBuilder {
  constructor() { this.pos = []; this.nor = []; this.ruv = []; this.rdir = []; this.rinfo = []; this.rinfo2 = []; this.idx = []; }
  // n satır; row(j) -> [vA, vB], vertex = [x,y,z, nx,ny,nz, u,v]
  strip(n, row, dir, info, info2) {
    const base = this.pos.length / 3;
    for (let j = 0; j < n; j++) {
      const pair = row(j), d = dir(j), i1 = info(j);
      for (const v of pair) {
        this.pos.push(v[0], v[1], v[2]); this.nor.push(v[3], v[4], v[5]); this.ruv.push(v[6], v[7]);
        this.rdir.push(d[0], d[1]); this.rinfo.push(...i1); this.rinfo2.push(...info2);
      }
      if (j > 0) {
        const q = base + (j - 1) * 2;
        this.idx.push(q, q + 2, q + 1, q + 1, q + 2, q + 3);
      }
    }
  }
  mesh(mat) {
    if (!this.idx.length) return null;
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('normal', new THREE.Float32BufferAttribute(this.nor, 3));
    g.setAttribute('ruv', new THREE.Float32BufferAttribute(this.ruv, 2));
    g.setAttribute('rdir', new THREE.Float32BufferAttribute(this.rdir, 2));
    g.setAttribute('rinfo', new THREE.Float32BufferAttribute(this.rinfo, 4));
    g.setAttribute('rinfo2', new THREE.Float32BufferAttribute(this.rinfo2, 4));
    g.setIndex(this.pos.length / 3 > 65535 ? new THREE.Uint32BufferAttribute(this.idx, 1) : new THREE.Uint16BufferAttribute(this.idx, 1));
    g.computeBoundingSphere();
    const m = new THREE.Mesh(g, mat);
    m.renderOrder = 1;
    return m;
  }
}

// Bir yol parçasının (a..e) geometrisi. GTA / BeamNG yaklaşımı:
//  - zemindeki yol araziye oturur; her yan duvar araziye kadar iner (altı boş kalmaz)
//  - yükselen yerde beton istinat duvarı, köprüde kalın tabliye + korkuluk + ayaklar
function emitPiece(rb, wb, { R, a, e }) {
  const { road, xs, zs, ys, dist, jd, dirx, dirz } = R;
  const n = e - a, half = road.w / 2, up = [0, 1, 0], down = [0, -1, 0];
  const I = (j) => a + j;
  const px = (i, o) => xs[i] + dirz[i] * o, pz = (i, o) => zs[i] - dirx[i] * o;
  const at = (i, o, y, nrm, u, v) => [px(i, o), y, pz(i, o), nrm[0], nrm[1], nrm[2], u, v];
  const ground = (i, o) => heightMesh(px(i, o), pz(i, o));
  // duvar alt ucu: araziye kadar (biraz içine), en az 0.5 m
  const foot = (i, o, y) => Math.min(y - 0.5, ground(i, o) - 0.5);
  const dir = (j) => [dirx[I(j)], dirz[I(j)]];
  const info2 = [road.sl, road.sr, half, R.zebra];
  const C = () => [1, 0, 0, 0], P = () => [0, 0, 0, 0];
  const bridge = !!road.b, DECK = 1.4;
  // her enine noktada arazi yüksekliği (köprüde tabliye düz)
  // yol sınıfına göre 1,5 cm yükseklik farkı: kavşakta üst üste binen yollarda ana yol üstte kalır,
  // aynı düzlemde titreme (z-çakışması) olmaz
  const bias = ROAD_ORDER[road.c] * 0.03;
  const gy = (i, o) => (bridge ? ys[i] : ground(i, o) + LIFT + bias);
  // şev: kenardan (o, yTop) araziye doğru iner; arazi yukarıdaysa (yarma) yukarı çıkar
  const graded = (Wf, o, side, yTop) => {
    const outv = (i) => [dirz[i] * side, 0, -dirx[i] * side];
    const L = (i) => Math.min(14, Math.max(0.4, Math.abs(yTop(i) - ground(i, o + side * 3)) * 2));
    const STEPS = 3;
    for (let k = 0; k < STEPS; k++) {
      const t0 = k / STEPS, t1 = (k + 1) / STEPS;
      const yAt = (i, t) => {
        const l = L(i), g = ground(i, o + side * l * t);
        return yTop(i) + (g - 0.05 - yTop(i)) * (t * t * (3 - 2 * t));
      };
      Wf((i) => [
        at(i, o + side * L(i) * t0, yAt(i, t0), outv(i), dist[i], L(i) * t0),
        at(i, o + side * L(i) * t1, yAt(i, t1), outv(i), dist[i], L(i) * t1),
      ]);
    }
  };

  // asfalt
  const offs = [-half, -half / 3, half / 3, half];
  for (let k = 0; k < 3; k++) {
    const o0 = offs[k], o1 = offs[k + 1];
    rb.strip(n, (j) => { const i = I(j); return [at(i, o0, gy(i, o0), up, dist[i], o0), at(i, o1, gy(i, o1), up, dist[i], o1)]; },
      dir, (j) => [road.n, road.o, road.lw, jd[I(j)]], info2);
  }

  // kavşak dışındaki ardışık gruplar (kaldırım ve bariyer kavşakta kesilir)
  const runs = [];
  let cur = [];
  for (let j = 0; j < n; j++) {
    if (jd[I(j)] > 0 || bridge) cur.push(j);
    else { if (cur.length > 1) runs.push(cur); cur = []; }
  }
  if (cur.length > 1) runs.push(cur);
  const highway = road.c === 'motorway' || road.c === 'trunk';

  // kavşak içinde (kaldırım yok) asfalt kenarı da araziye kadar kapatılır
  if (!bridge) {
    const jruns = [];
    let jc = [];
    for (let j = 0; j < n; j++) {
      if (jd[I(j)] <= 0) jc.push(j);
      else { if (jc.length > 1) jruns.push(jc); jc = []; }
    }
    if (jc.length > 1) jruns.push(jc);
    for (const run of jruns) {
      const rn = run.length, RI = (k) => I(run[k]), rdir = (k) => [dirx[RI(k)], dirz[RI(k)]];
      for (const side of [-1, 1]) {
        const eo = half * side, nrm = (i) => [dirz[i] * side, 0, -dirx[i] * side];
        wb.strip(rn, (k) => { const i = RI(k);
          return [at(i, eo, gy(i, eo) - 0.02, nrm(i), dist[i], 0), at(i, eo, ground(i, eo) - 0.4, nrm(i), dist[i], -0.4)]; },
          rdir, C, info2);
      }
    }
  }

  for (const run of runs) {
    const rn = run.length, RI = (k) => I(run[k]), rdir = (k) => [dirx[RI(k)], dirz[RI(k)]];
    const W = (f, info = C) => wb.strip(rn, (k) => f(RI(k)), rdir, info, info2);
    let outerL = -half, outerR = half;   // tabliye / dolgu dış kenarı
    let topAt = (i) => ys[i];            // dış kenardaki üst yükseklik

    for (const side of [-1, 1]) {
      const out = (i) => [dirz[i] * side, 0, -dirx[i] * side];
      const inw = (i) => [-dirz[i] * side, 0, dirx[i] * side];
      const eo = half * side;
      let o = eo, yTop = (i) => gy(i, eo);
      if (road.s > 0) {
        // bordür yüzü + kaldırım (parke): kaldırım da arazi yüzeyini izler
        const oo = (half + road.s) * side; o = oo;
        yTop = (i) => gy(i, oo) + CURB_H;
        W((i) => [at(i, eo, ground(i, eo) - 0.4, inw(i), dist[i], -0.4), at(i, eo, gy(i, eo) + CURB_H, inw(i), dist[i], CURB_H)]);
        W((i) => [at(i, eo, gy(i, eo) + CURB_H, up, dist[i], eo), at(i, oo, yTop(i), up, dist[i], oo)], P);
      } else if (highway && side < 0 && road.o && !bridge) {
        // otoban orta bariyeri (New Jersey tipi beton, 0.9 m)
        const b1 = eo - 0.6, bh = 0.9, yb = (i) => gy(i, eo);
        W((i) => [at(i, eo, yb(i) - 0.05, inw(i), dist[i], 0), at(i, eo - 0.15, yb(i) + bh, inw(i), dist[i], bh)]);
        W((i) => [at(i, eo - 0.15, yb(i) + bh, up, dist[i], 0), at(i, b1 + 0.15, yb(i) + bh, up, dist[i], 0.3)]);
        W((i) => [at(i, b1 + 0.15, yb(i) + bh, out(i), dist[i], bh), at(i, b1, foot(i, b1, yb(i)), out(i), dist[i], -2)]);
        o = b1;
      } else {
        // beton omuz
        const oo = (half + 0.5) * side; o = oo;
        W((i) => [at(i, eo, gy(i, eo) - 0.02, up, dist[i], eo), at(i, oo, gy(i, oo) - 0.02, up, dist[i], oo)]);
        yTop = (i) => gy(i, oo) - 0.02;
      }
      if (side < 0) outerL = o; else outerR = o;
      topAt = yTop;

      if (bridge) {
        // köprü korkuluğu (beton parapet 1.0 m) + tabliye dış yüzü (kalınlık DECK)
        const ph = 1.0, o2 = o + 0.35 * side;
        W((i) => [at(i, o, yTop(i), inw(i), dist[i], 0), at(i, o, yTop(i) + ph, inw(i), dist[i], ph)]);
        W((i) => [at(i, o, yTop(i) + ph, up, dist[i], 0), at(i, o2, yTop(i) + ph, up, dist[i], 0.35)]);
        W((i) => [at(i, o2, yTop(i) + ph, out(i), dist[i], ph), at(i, o2, ys[i] - DECK, out(i), dist[i], -DECK)]);
        if (side < 0) outerL = o2; else outerR = o2;
      } else if (!(highway && side < 0 && road.o && road.s === 0)) {
        // dış yan duvar: araziye kadar iner (dolguda istinat duvarı olur, altı asla boş kalmaz)
        // dış kenardan araziye eğimli şev (1:2): yol araziye karışır, dik duvar yok
        graded(W, o, side, yTop, i0 => i0);
      }
    }

    if (bridge) {
      // tabliye alt yüzü
      W((i) => [at(i, outerL, ys[i] - DECK, down, dist[i], outerL), at(i, outerR, ys[i] - DECK, down, dist[i], outerR)]);
      // ayaklar: her ~30 m'de, yerden 2 m'den yüksekse; geniş tabliyede iki ayak
      const width = outerR - outerL;
      const cols2 = width > 14 ? [outerL + width * 0.25, outerR - width * 0.25] : [(outerL + outerR) / 2];
      let last = -1e9;
      for (let k = 0; k < rn; k++) {
        const i = RI(k);
        if (dist[i] - last < 30 || jd[i] < 5) continue;
        const yb = ys[i] - DECK;
        for (const oc of cols2) {
          const g = ground(i, oc);
          if (yb - g < 2) continue;
          pillar(wb, px(i, oc), pz(i, oc), dirx[i], dirz[i], yb, g - 1, info2);
        }
        // ayak başlığı (enine kiriş)
        if (yb - ground(i, (outerL + outerR) / 2) >= 2) {
          beam(wb, i, outerL + 0.5, outerR - 0.5, yb, yb - 1.2, px, pz, dirx, dirz, info2);
        }
        last = dist[i];
      }
    }
  }
}

// kare kesitli beton ayak (1.6 m): 4 yan yüz
function pillar(wb, x, z, tx, tz, yTop, yBot, info2) {
  const s = 0.8, rx = tz, rz = -tx;    // sağ vektör
  const c = [[-1, -1], [1, -1], [1, 1], [-1, 1]].map(([u, v]) => [x + (rx * u + tx * v) * s, z + (rz * u + tz * v) * s]);
  for (let f = 0; f < 4; f++) {
    const p0 = c[f], p1 = c[(f + 1) % 4];
    let nx = (p0[0] + p1[0]) / 2 - x, nz = (p0[1] + p1[1]) / 2 - z;
    const l = Math.hypot(nx, nz) || 1; nx /= l; nz /= l;
    const d = [p1[0] - p0[0], p1[1] - p0[1]], dl = Math.hypot(d[0], d[1]);
    wb.strip(2, (j) => {
      const p = j ? p1 : p0;
      return [[p[0], yTop, p[1], nx, 0, nz, j * dl, 0], [p[0], yBot, p[1], nx, 0, nz, j * dl, yBot - yTop]];
    }, () => [d[0] / dl, d[1] / dl], () => [1, 0, 0, 0], info2);
  }
}

// ayak başlığı: tabliye altında enine beton kiriş (ön/arka yüz + alt yüz)
function beam(wb, i, o0, o1, yTop, yBot, px, pz, dirx, dirz, info2) {
  const t = 0.8;
  for (const f of [-1, 1]) {
    const ox = dirx[i] * t * f, oz = dirz[i] * t * f;
    wb.strip(2, (j) => {
      const o = j ? o1 : o0;
      return [[px(i, o) + ox, yTop, pz(i, o) + oz, dirx[i] * f, 0, dirz[i] * f, o, 0],
              [px(i, o) + ox, yBot, pz(i, o) + oz, dirx[i] * f, 0, dirz[i] * f, o, yBot - yTop]];
    }, () => [dirz[i], -dirx[i]], () => [1, 0, 0, 0], info2);
  }
  wb.strip(2, (j) => {
    const o = j ? o1 : o0;
    return [[px(i, o) - dirx[i] * t, yBot, pz(i, o) - dirz[i] * t, 0, -1, 0, o, 0],
            [px(i, o) + dirx[i] * t, yBot, pz(i, o) + dirz[i] * t, 0, -1, 0, o, 2 * t]];
  }, () => [dirz[i], -dirx[i]], () => [1, 0, 0, 0], info2);
}

let roadMat, walkMat;
const builtTiles = new Set();
// part: 'major' (ana yollar) veya 'minor' (ara sokaklar)
function buildTilePart(t, part) {
  roadMat ||= roadMaterial(); walkMat ||= walkMaterial();
  const rb = new StripBuilder(), wb = new StripBuilder();
  for (const pc of t.pieces) if (MINOR.has(pc.R.road.c) === (part === 'minor')) emitPiece(rb, wb, pc);
  const g = new THREE.Group();
  const a = rb.mesh(roadMat), b = wb.mesh(walkMat);
  if (a) g.add(a);
  if (b) g.add(b);
  scene.add(g);
  t.built[part] = g;
  builtTiles.add(t);
}
function disposeTilePart(t, part) {
  const g = t.built[part];
  if (!g) return;
  scene.remove(g);
  g.traverse((o) => o.geometry && o.geometry.dispose());
  delete t.built[part];
  if (!t.built.major && !t.built.minor) builtTiles.delete(t);
}
// Kameranın çevresindeki karoları üret, uzaktakileri sil. budgetMs: kare başına süre sınırı
const STREAM = { major: 4500, minor: 2200 };
function streamRoads(px, pz, budgetMs = 6) {
  const t0 = performance.now();
  const want = [];
  for (const t of roadTileMap.values()) {
    const d = Math.hypot(t.cx - px, t.cz - pz) - TILE * 0.7;
    for (const part of ['major', 'minor']) {
      if (d < STREAM[part] && !t.built[part]) want.push([d, t, part]);
      else if (d > STREAM[part] + 800 && t.built[part]) disposeTilePart(t, part);
    }
  }
  want.sort((a, b) => a[0] - b[0]);
  for (const [, t, part] of want) {
    buildTilePart(t, part);
    if (performance.now() - t0 > budgetMs) break;
  }
}

prepareRoads();

// Arazi mesh'inin gerçek yüzeyi: her hücre (a,d,b) ve (b,d,e) üçgenlerine bölünür (köşegen b-d).
// Yolların arazinin altında kalmaması için çift doğrusal değil, bu üçgen yüzey okunur.
export function heightMesh(x, z) {
  const fc = (x + Wm / 2) / dx, fr = (z + Dm / 2) / dz;
  const c = Math.floor(fc), r = Math.floor(fr), u = fc - c, v = fr - r;
  const ha = hAt(c, r), hb = hAt(c + 1, r), hd = hAt(c, r + 1), he = hAt(c + 1, r + 1);
  return u + v <= 1 ? ha + u * (hb - ha) + v * (hd - ha) : he + (1 - u) * (hd - he) + (1 - v) * (hb - he);
}

// Araziyi 128x128 hücrelik parçalara böl (görüş dışındakiler çizilmez)
const overviewTex = buildOverview();
const waterTex = buildWaterTex();
const terrainMat = terrainMaterial();
const CH = 128;
const terrain = new THREE.Group();
for (let r0 = 0; r0 < rows - 1; r0 += CH) {
  for (let c0 = 0; c0 < cols - 1; c0 += CH) {
    const nc = Math.min(CH, cols - 1 - c0) + 1, nr = Math.min(CH, rows - 1 - r0) + 1;
    const pos = new Float32Array(nc * nr * 3), nor = new Float32Array(nc * nr * 3);
    let i = 0;
    for (let r = 0; r < nr; r++) for (let c = 0; c < nc; c++, i += 3) {
      const C = c0 + c, R = r0 + r;
      pos[i] = C * dx - Wm / 2; pos[i + 1] = hAt(C, R); pos[i + 2] = R * dz - Dm / 2;
      const nx = (hAt(C - 1, R) - hAt(C + 1, R)) / (2 * dx), nz = (hAt(C, R - 1) - hAt(C, R + 1)) / (2 * dz);
      const l = Math.hypot(nx, 1, nz);
      nor[i] = nx / l; nor[i + 1] = 1 / l; nor[i + 2] = nz / l;
    }
    const idx = new Uint32Array((nc - 1) * (nr - 1) * 6);
    let k = 0;
    for (let r = 0; r < nr - 1; r++) for (let c = 0; c < nc - 1; c++) {
      const a = r * nc + c, b = a + 1, d = a + nc, e = d + 1;
      idx[k++] = a; idx[k++] = d; idx[k++] = b; idx[k++] = b; idx[k++] = d; idx[k++] = e;
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('normal', new THREE.BufferAttribute(nor, 3));
    g.setIndex(new THREE.BufferAttribute(idx, 1));
    g.computeBoundingSphere();
    terrain.add(new THREE.Mesh(g, terrainMat));
  }
}
scene.add(terrain);

const roadCount = roadData ? roadData.roads.length : 0;

// ---------------------------------------------------------------------------
// Başlangıç kamerası ve kamera modları
//   uçuş (fly): telefonda varsayılan; sol baş parmak hareket, sağ baş parmak bakış
//   harita (orbit): masaüstünde varsayılan; fare ile kaydır / döndür / yakınlaş
// ---------------------------------------------------------------------------
function toWorld(lat, lon) {
  const [, lonMin, latMax] = meta.bbox;
  const mlon = 111320 * Math.cos(((meta.bbox[0] + meta.bbox[2]) / 2) * Math.PI / 180);
  return [(lon - lonMin) * mlon - Wm / 2, (latMax - lat) * 111320 - Dm / 2];
}
function toLatLon(x, z) {
  const [, lonMin, latMax] = meta.bbox;
  const mlon = 111320 * Math.cos(((meta.bbox[0] + meta.bbox[2]) / 2) * Math.PI / 180);
  return [latMax - (z + Dm / 2) / 111320, lonMin + (x + Wm / 2) / mlon];
}
let [cx, cz] = toWorld(Number(params.get('lat') || 37.5715), Number(params.get('lon') || 126.9768)); // varsayılan: Gwanghwamun
cx += Number(params.get('ox') || 0); cz += Number(params.get('oz') || 0); // test için kaydırma
const camDist = Number(params.get('d') || (isTouch ? 700 : 2500));
controls.target.set(cx, heightAt(cx, cz), cz);
camera.position.set(cx - camDist * 0.5, heightAt(cx, cz) + camDist * Number(params.get('hy') || 0.55), cz + camDist * 0.8);
controls.update();

camera.rotation.order = 'YXZ';
let mode = params.get('mode') || (isTouch ? 'fly' : 'orbit');
let yaw = 0, pitch = 0;
const tmpV = new THREE.Vector3();
function anglesFromCamera() {
  camera.getWorldDirection(tmpV);
  yaw = Math.atan2(-tmpV.x, -tmpV.z);
  pitch = Math.asin(THREE.MathUtils.clamp(tmpV.y, -1, 1));
}
// ---- arayüz ----
const ui = document.createElement('div');
ui.id = 'ui';
ui.innerHTML = `
  <div id="joy"><div id="knob"></div></div>
  <div class="btn" id="bMode" title="Kamera modu">🗺</div>
  <div class="btn" id="bUp">▲</div>
  <div class="btn" id="bDown">▼</div>
  <div class="btn" id="bSpeed"></div>
  <div id="hint">Sol baş parmak: ekrana bas ve sürükle → ilerle<br>Sağ baş parmak: sürükle → etrafına bak<br>▲ ▼: yüksel / alçal</div>`;
document.body.appendChild(ui);
const $ = (id) => document.getElementById(id);
const SPEEDS = [15, 60, 200, 600];            // m/s
let speedIdx = params.has('speed') ? Number(params.get('speed')) : 1;
const joy = { x: 0, y: 0 }, hold = { up: false, down: false };
const refreshUi = () => {
  const fly = mode === 'fly';
  for (const id of ['bUp', 'bDown', 'bSpeed']) $(id).style.display = fly ? 'grid' : 'none';
  $('bSpeed').textContent = `${SPEEDS[speedIdx]}\nm/s`;
  $('bMode').textContent = fly ? '🗺' : '✈';
  $('hint').style.display = fly && isTouch ? 'block' : 'none';
};
function setMode(m) {
  mode = m;
  controls.enabled = m === 'orbit';
  if (m === 'fly') anglesFromCamera();
  else {
    // bakılan yeri hedef yap (en çok 3 km ileri)
    camera.getWorldDirection(tmpV);
    const agl = Math.max(30, camera.position.y - heightAt(camera.position.x, camera.position.z));
    const dist = Math.min(3000, agl / Math.max(0.15, -tmpV.y));
    controls.target.copy(camera.position).addScaledVector(tmpV, dist);
    controls.target.y = heightAt(controls.target.x, controls.target.z);
  }
  refreshUi();
}
controls.enabled = mode === 'orbit';
if (mode === 'fly') anglesFromCamera();
refreshUi();
setTimeout(() => { $('hint').style.opacity = 0; }, 9000);

$('bMode').addEventListener('click', () => setMode(mode === 'fly' ? 'orbit' : 'fly'));
$('bSpeed').addEventListener('click', () => { speedIdx = (speedIdx + 1) % SPEEDS.length; refreshUi(); });
for (const [id, key] of [['bUp', 'up'], ['bDown', 'down']]) {
  const el = $(id);
  el.addEventListener('pointerdown', (e) => { hold[key] = true; try { el.setPointerCapture(e.pointerId); } catch {} e.preventDefault(); });
  const off = () => { hold[key] = false; };
  el.addEventListener('pointerup', off); el.addEventListener('pointercancel', off); el.addEventListener('lostpointercapture', off);
}

// ---- dokunma / fare: sol bölge = joystick, diğer her yer = bakış ----
const cv = renderer.domElement;
cv.style.touchAction = 'none';
const ptrs = new Map();
const JR = 56; // joystick yarıçapı (px)
cv.addEventListener('pointerdown', (e) => {
  if (mode !== 'fly') return;
  try { cv.setPointerCapture(e.pointerId); } catch {}
  const joyFree = ![...ptrs.values()].some((p) => p.role === 'joy');
  const role = e.pointerType === 'touch' && joyFree && e.clientX < innerWidth * 0.5 && e.clientY > innerHeight * 0.3 ? 'joy' : 'look';
  ptrs.set(e.pointerId, { role, sx: e.clientX, sy: e.clientY, lx: e.clientX, ly: e.clientY });
  if (role === 'joy') {
    $('joy').style.display = 'block'; $('joy').style.left = e.clientX + 'px'; $('joy').style.top = e.clientY + 'px';
    $('knob').style.transform = 'translate(0,0)';
  }
});
cv.addEventListener('pointermove', (e) => {
  const p = ptrs.get(e.pointerId);
  if (!p || mode !== 'fly') return;
  if (p.role === 'joy') {
    let dx = e.clientX - p.sx, dy = e.clientY - p.sy;
    const l = Math.hypot(dx, dy);
    if (l > JR) { dx *= JR / l; dy *= JR / l; }
    joy.x = dx / JR; joy.y = -dy / JR;
    $('knob').style.transform = `translate(${dx}px,${dy}px)`;
  } else {
    yaw -= (e.clientX - p.lx) * 0.0045;
    pitch = THREE.MathUtils.clamp(pitch - (e.clientY - p.ly) * 0.0045, -1.5, 1.5);
    p.lx = e.clientX; p.ly = e.clientY;
  }
});
const endPtr = (e) => {
  const p = ptrs.get(e.pointerId);
  if (!p) return;
  if (p.role === 'joy') { joy.x = joy.y = 0; $('joy').style.display = 'none'; }
  ptrs.delete(e.pointerId);
};
cv.addEventListener('pointerup', endPtr); cv.addEventListener('pointercancel', endPtr);

document.getElementById('info').textContent =
  `${(Wm / 1000).toFixed(1)} × ${(Dm / 1000).toFixed(1)} km · ${roadCount ? roadCount + ' yol' : 'yol yok'}`;
const hudHelp = document.getElementById('help');
if (hudHelp) hudHelp.style.display = isTouch ? 'none' : 'block';

// ---------------------------------------------------------------------------
// Hareket döngüsü
// ---------------------------------------------------------------------------
const keys = new Set();
addEventListener('keydown', (e) => keys.add(e.code));
addEventListener('keyup', (e) => keys.delete(e.code));
const clock = new THREE.Clock();
const fwd = new THREE.Vector3(), right = new THREE.Vector3(), move = new THREE.Vector3();
let frame = 0;

function tick() {
  const dt = Math.min(clock.getDelta(), 0.1);
  let sx, sz;
  if (mode === 'fly') {
    const boost = keys.has('ShiftLeft') || keys.has('ShiftRight') ? 3 : 1;
    const f = THREE.MathUtils.clamp(joy.y + (keys.has('KeyW') || keys.has('ArrowUp') ? 1 : 0) - (keys.has('KeyS') || keys.has('ArrowDown') ? 1 : 0), -1, 1);
    const r = THREE.MathUtils.clamp(joy.x + (keys.has('KeyD') || keys.has('ArrowRight') ? 1 : 0) - (keys.has('KeyA') || keys.has('ArrowLeft') ? 1 : 0), -1, 1);
    const v = (hold.up || keys.has('Space') || keys.has('KeyE') ? 1 : 0) - (hold.down || keys.has('KeyQ') ? 1 : 0);
    camera.rotation.set(pitch, yaw, 0);
    camera.getWorldDirection(fwd);
    right.crossVectors(fwd, camera.up).normalize();
    const sp = SPEEDS[speedIdx] * boost;
    move.set(0, 0, 0).addScaledVector(fwd, f).addScaledVector(right, r);
    if (move.lengthSq() > 1) move.normalize();
    camera.position.addScaledVector(move, sp * dt);
    camera.position.y += v * sp * 0.6 * dt;
    sx = camera.position.x; sz = camera.position.z;
  } else {
    const dist = camera.position.distanceTo(controls.target);
    const speed = dist * 0.8 * dt;
    camera.getWorldDirection(fwd); fwd.y = 0; fwd.normalize();
    right.crossVectors(fwd, camera.up).normalize();
    move.set(0, 0, 0);
    if (keys.has('KeyW') || keys.has('ArrowUp')) move.add(fwd);
    if (keys.has('KeyS') || keys.has('ArrowDown')) move.sub(fwd);
    if (keys.has('KeyD') || keys.has('ArrowRight')) move.add(right);
    if (keys.has('KeyA') || keys.has('ArrowLeft')) move.sub(right);
    if (move.lengthSq()) {
      move.normalize().multiplyScalar(speed);
      camera.position.add(move); controls.target.add(move);
    }
    sx = controls.target.x; sz = controls.target.z;
  }
  // kamera arazinin altına girmesin; harita sınırları içinde kalsın
  camera.position.x = THREE.MathUtils.clamp(camera.position.x, -Wm / 2, Wm / 2);
  camera.position.z = THREE.MathUtils.clamp(camera.position.z, -Dm / 2, Dm / 2);
  const gy = heightAt(camera.position.x, camera.position.z) + 3;
  if (camera.position.y < gy) camera.position.y = gy;

  // yolları kameranın çevresinde akış halinde üret
  streamRoads(sx, sz, params.has('still') ? 1e9 : 6);
  streamPatches(sx, sz, params.has('still') ? 1e6 : 1);

  if (mode === 'orbit') controls.update();
  renderer.render(scene, camera);
  if (++frame % 20 === 1) {
    const [la, lo] = toLatLon(camera.position.x, camera.position.z);
    $('pos').textContent = `${la.toFixed(4)}, ${lo.toFixed(4)} · yük. ${Math.round(camera.position.y - heightAt(camera.position.x, camera.position.z))} m`;
  }
  if (!params.has('still')) requestAnimationFrame(tick); // ?still: tek kare (test için)
}

addEventListener('resize', () => {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

const load = document.getElementById('load');
load.style.opacity = 0;
setTimeout(() => load.remove(), 500);
window.__cam = camera; // test için
window.__ready = true;
tick();
