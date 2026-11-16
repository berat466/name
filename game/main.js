import * as THREE from 'three';
import { MapControls } from 'three/addons/controls/MapControls.js';

// ---------------------------------------------------------------------------
// Sahne
// ---------------------------------------------------------------------------
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
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
const TEX = 2048;

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

  for (let i = 0; i < n; i++) {
    const base = `./textures/${names[i]}/`;
    const [d, nm, arm] = await Promise.all(['diff', 'nor_gl', 'arm'].map((m) => loadImg(base + m + '.jpg')));
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
const ROAD_SETS = ['asphalt_02', 'aerial_asphalt_01', 'asphalt_04'];
// Kaldırım katmanları
const WALK_SETS = ['herringbone_pavement_04', 'concrete_pavement_02'];

const [terrainTex, roadTex, walkTex] = await Promise.all(
  [TERRAIN_SETS, ROAD_SETS, WALK_SETS].map(loadPbrArray));

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
function patchStandard(mat, uniforms, vtxExtra, fragDecl, fragBody) {
  // her malzeme kendi shader'ını derlesin (aksi halde three.js aynı programı paylaştırır)
  const key = 'patched-' + (shaderId++);
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
  const mat = new THREE.MeshStandardMaterial({ roughness: 1, metalness: 0 });
  patchStandard(mat, { tA: { value: terrainTex.alb }, tN: { value: terrainTex.nra } }, {}, '', `
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
    diffuseColor.rgb *= col;

    vec3 Tg = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
    vec3 Bt = normalize(cross(Tg, N0));
    blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
  `);
  return mat;
}

// ---------------------------------------------------------------------------
// Yol malzemesi: 3 asfalt PBR setinin karışımı + şerit çizgileri
//   ruv   = (yol boyunca metre, enine metre)
//   rdir  = yol yönü (x, z)
//   rinfo = (şerit sayısı, tek yön mü, yarı genişlik, yol uzunluğu)
// ---------------------------------------------------------------------------
const ROAD_VTX = {
  decl: 'attribute vec2 ruv;\nattribute vec2 rdir;\nattribute vec4 rinfo;\nvarying vec2 vRuv;\nvarying vec2 vRdir;\nvarying vec4 vInfo;',
  body: 'vRuv = ruv; vRdir = rdir; vInfo = rinfo;',
};
const ROAD_FRAG_DECL = 'varying vec2 vRuv;\nvarying vec2 vRdir;\nvarying vec4 vInfo;';
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
  `, `
    float n1 = vnoise(vWP.xz * 0.02);
    float n2 = vnoise(vWP.xz * 0.07 + 33.0);
    float wB = smoothstep(0.4, 0.8, n1) * 0.6;     // farklı asfalt yüzeyi
    float wC = smoothstep(0.55, 0.85, n2) * 0.5;   // çatlak / yama
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / 5.0, 0.0, (1.0 - wB) * (1.0 - wC));
    addLayer(tA, tN, vRuv / 8.0, 1.0, wB * (1.0 - wC));
    addLayer(tA, tN, vRuv / 4.0, 2.0, wC);
    ${GLSL_RESOLVE}

    // --- şerit çizgileri (Kore standardı: orta sarı çift çizgi, beyaz kesikli şerit, beyaz kenar) ---
    float along = vRuv.x, u = vRuv.y;
    float lanes = vInfo.x, oneway = vInfo.y, halfW = vInfo.z, jd = vInfo.w;
    AA = fwidth(u) + 0.005;
    float inner = halfW - 0.5;                       // kenar çizgisi konumu
    float dash = step(fract(along / 15.0), 0.4);     // 6 m çizgi, 9 m boşluk
    float white = aaLine(abs(u) - inner, 0.10);
    float yellow = 0.0;
    if (oneway < 0.5) {
      yellow = max(aaLine(u - 0.15, 0.08), aaLine(u + 0.15, 0.08));
      float side = max(1.0, floor(lanes / 2.0));
      float lw = inner / side;
      for (int k = 1; k < 5; k++) {
        if (float(k) >= side) break;
        white = max(white, max(aaLine(u - lw * float(k), 0.09), aaLine(u + lw * float(k), 0.09)) * dash);
      }
    } else {
      float lw = 2.0 * inner / lanes;
      for (int k = 1; k < 10; k++) {
        if (float(k) >= lanes) break;
        white = max(white, aaLine(u + inner - lw * float(k), 0.09) * dash);
      }
    }
    // kavşak içinde çizgi yok; çizgiler hafif aşınmış
    float fade = smoothstep(0.0, 4.0, jd);
    float wear = mix(0.7, 1.0, vnoise(vec2(along * 0.25, 7.0)));
    white *= fade * wear; yellow *= fade * wear;
    vec3 col = accA;
    col = mix(col, vec3(0.82), white);
    col = mix(col, vec3(0.85, 0.62, 0.08), yellow);
    blendR = mix(blendR, 0.55, max(white, yellow));
    diffuseColor.rgb *= col;
    ${GLSL_ROAD_TBN}
  `);
  return mat;
}

// Kaldırım / bordür malzemesi: 2 PBR seti (parke taşı üst yüzey, beton bordür ve yan duvar)
function walkMaterial() {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: walkTex.alb }, tN: { value: walkTex.nra } }, ROAD_VTX, ROAD_FRAG_DECL, `
    float layer = vInfo.x;  // 0 = parke, 1 = beton
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / (layer < 0.5 ? 3.0 : 2.0), layer < 0.5 ? 0.0 : 1.0, 1.0);
    ${GLSL_RESOLVE}
    diffuseColor.rgb *= accA * (0.9 + 0.2 * vnoise(vWP.xz * 0.05));
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
// Yollar (OpenStreetMap): profil, arazi oyma, asfalt + kaldırım + bordür
// ---------------------------------------------------------------------------
const roadData = await fetch('./data/roads.json').then((r) => (r.ok ? r.json() : null)).catch(() => null);
const ROAD_ORDER = { residential: 0, unclassified: 0, tertiary: 1, secondary: 2, primary: 3, trunk: 4, motorway: 5 };
const MINOR = new Set(['residential', 'unclassified', 'tertiary']);
const STEP = 12;        // yeniden örnekleme aralığı (m)
const PIECE = 64;       // parça başına en fazla nokta (görüş kesimi için)
const WALK_W = 2.5, CURB_H = 0.15, SKIRT = 3.0;
const smooth = (a, b, x) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };

// Her yolu sık örnekle, yüksekliğini yumuşat, parçalara ayır
const roadPieces = [];
if (roadData) {
  // Kavşaklar: birden fazla yolun paylaştığı OSM noktaları
  const jkey = (x, z) => `${Math.round(x * 2)},${Math.round(z * 2)}`;
  const jmap = new Map();
  for (const road of roadData.roads) {
    const reach = road.w / 2 + (road.s ? WALK_W : 0.6);
    for (let i = 0; i < road.p.length; i += 2) {
      const k = jkey(road.p[i], road.p[i + 1]);
      const e = jmap.get(k) || { n: 0, r: 0, o: 0, h: heightAt(road.p[i], road.p[i + 1]) };
      e.n++; e.r = Math.max(e.r, reach); e.o = Math.max(e.o, ROAD_ORDER[road.c]);
      jmap.set(k, e);
    }
  }

  for (const road of roadData.roads) {
    const p = road.p;
    const pts = [];
    const junctions = []; // [örnek indeksi, yarıçap]
    for (let i = 0; i < p.length; i += 2) {
      const e = jmap.get(jkey(p[i], p[i + 1]));
      if (e && e.n > 1) junctions.push([pts.length, e.r + 1, e]); // bu noktanın örnek indeksi
      if (i >= p.length - 2) break;
      const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
      const n = Math.max(1, Math.ceil(Math.hypot(bx - ax, bz - az) / STEP));
      for (let s = 0; s < n; s++) pts.push([ax + (bx - ax) * s / n, az + (bz - az) * s / n]);
    }
    pts.push([p[p.length - 2], p[p.length - 1]]);
    if (pts.length < 2) continue;

    const lift = 0.25 + ROAD_ORDER[road.c] * 0.04 + Math.max(0, road.l) * 7;
    const raw = pts.map(([x, z]) => heightAt(x, z));
    // ±3 örneklik hareketli ortalama: tümsekleri düzelt
    let ys = raw.map((_, i) => {
      let s = 0, k = 0;
      for (let j = Math.max(0, i - 3); j <= Math.min(raw.length - 1, i + 3); j++) { s += raw[j]; k++; }
      return s / k + lift;
    });
    const dist = [0];
    for (let i = 1; i < pts.length; i++) dist.push(dist[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
    // kavşakta tüm yollar aynı yükseklikte buluşsun (30 m içinde yumuşak geçiş)
    if (!road.b && road.l <= 0) {
      for (const [si, r, e] of junctions) {
        const target = e.h + 0.25 + (ROAD_ORDER[road.c] === e.o ? 0.02 : 0);
        const d0 = dist[Math.min(si, dist.length - 1)];
        for (let i = 0; i < ys.length; i++) {
          const w = 1 - smooth(r, r + 30, Math.abs(dist[i] - d0));
          if (w > 0) ys[i] += (target - ys[i]) * w;
        }
      }
    }
    if (road.b) {
      const y0 = ys[0], y1 = ys[ys.length - 1];
      ys = ys.map((y, i) => Math.max(y, y0 + (y1 - y0) * i / (ys.length - 1)));
    }
    const len = dist[dist.length - 1];
    // her örneğin en yakın kavşağa uzaklığı (kavşak yarıçapı çıkarılmış); <0 = kavşak içinde
    const jd = dist.map((d) => {
      let m = 1e6;
      for (const [si, r] of junctions) m = Math.min(m, Math.abs(d - dist[Math.min(si, dist.length - 1)]) - r);
      return m;
    });

    for (let a = 0; a < pts.length - 1; a += PIECE - 1) {
      const b = Math.min(pts.length, a + PIECE);
      roadPieces.push({ road, len, pts: pts.slice(a, b), ys: ys.slice(a, b), dist: dist.slice(a, b), jd: jd.slice(a, b), all: pts, off: a });
    }
  }

  // Araziyi yola oturt: yol şeridinde zemin = yol yüksekliğinin hemen altı (hem oyar hem doldurur),
  // kenarda doğal araziye yumuşak geçiş. Önce ara yollar, sonra ana yollar (ana yol baskın).
  const locked = new Uint8Array(H.length);
  const bestD = new Float32Array(H.length).fill(Infinity);   // en yakın yol noktasına uzaklık
  const bestO = new Int8Array(H.length).fill(-1);            // o noktayı yazan yolun sınıfı
  const sorted = roadPieces.filter((pc) => !pc.road.b && pc.road.l <= 0)
    .sort((a, b) => ROAD_ORDER[a.road.c] - ROAD_ORDER[b.road.c]);
  const orig = H.slice();
  for (const pc of sorted) {
    const { road } = pc;
    // ızgara hücresi 25-30 m: yolun iki yanındaki en yakın noktaların kesin kilitlenmesi için yarım köşegen payı
    const R = road.w / 2 + (road.s ? WALK_W : 0.6) + 0.55 * Math.hypot(dx, dz) + STEP / 2;
    for (let i = 0; i < pc.pts.length; i++) {
      const [x, z] = pc.pts[i], y = pc.ys[i] - 0.6, ord = ROAD_ORDER[road.c];
      const c0 = Math.floor((x - R - 2 * dx + Wm / 2) / dx), c1 = Math.ceil((x + R + 2 * dx + Wm / 2) / dx);
      const r0 = Math.floor((z - R - 2 * dz + Dm / 2) / dz), r1 = Math.ceil((z + R + 2 * dz + Dm / 2) / dz);
      for (let r = Math.max(0, r0); r <= Math.min(rows - 1, r1); r++) {
        for (let c = Math.max(0, c0); c <= Math.min(cols - 1, c1); c++) {
          const d = Math.hypot(c * dx - Wm / 2 - x, r * dz - Dm / 2 - z);
          const k = r * cols + c;
          // ana yol baskın; aynı sınıfta en yakın yol noktası kazanır
          if (d < R && (ord > bestO[k] || (ord === bestO[k] && d < bestD[k]))) {
            H[k] = y; locked[k] = 1; bestD[k] = d; bestO[k] = ord;
          }
          else if (d < R + 1.5 * dx && !locked[k]) H[k] = y + (orig[k] - y) * smooth(0, 1, (d - R) / (1.5 * dx));
        }
      }
    }
  }
}

class StripBuilder {
  constructor() { this.pos = []; this.nor = []; this.ruv = []; this.rdir = []; this.rinfo = []; this.idx = []; }
  // rows[i] = [vertexA, vertexB]; her vertex: {p:[x,y,z], n:[x,y,z], uv:[a,b]}
  strip(rowsList, dir, info) {
    const base = this.pos.length / 3;
    rowsList.forEach((pair, i) => {
      for (const v of pair) {
        this.pos.push(...v.p); this.nor.push(...v.n); this.ruv.push(...v.uv);
        this.rdir.push(...dir[i]); this.rinfo.push(...(typeof info === 'function' ? info(i) : info)); // i: şerit içi sıra
      }
      if (i > 0) {
        const q = base + (i - 1) * 2;
        this.idx.push(q, q + 2, q + 1, q + 1, q + 2, q + 3);
      }
    });
  }
  mesh(mat) {
    if (!this.idx.length) return null;
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('normal', new THREE.Float32BufferAttribute(this.nor, 3));
    g.setAttribute('ruv', new THREE.Float32BufferAttribute(this.ruv, 2));
    g.setAttribute('rdir', new THREE.Float32BufferAttribute(this.rdir, 2));
    g.setAttribute('rinfo', new THREE.Float32BufferAttribute(this.rinfo, 4));
    g.setIndex(this.idx.length > 65535 ? new THREE.Uint32BufferAttribute(this.idx, 1) : new THREE.Uint16BufferAttribute(this.idx, 1));
    g.computeBoundingSphere();
    const m = new THREE.Mesh(g, mat);
    m.renderOrder = 1;
    return m;
  }
}

const roadTiles = [];   // { group, center, minor }
function buildRoadMeshes() {
  if (!roadPieces.length) return;
  const TILE = 2000;
  const tiles = new Map();
  const up = [0, 1, 0];
  for (const pc of roadPieces) {
    const m = pc.pts[pc.pts.length >> 1];
    const minor = MINOR.has(pc.road.c);
    const key = `${Math.floor(m[0] / TILE)},${Math.floor(m[1] / TILE)},${minor ? 1 : 0}`;
    if (!tiles.has(key)) tiles.set(key, { road: new StripBuilder(), walk: new StripBuilder(), minor });
    const t = tiles.get(key);
    const { road, pts, ys, dist, jd, all, off } = pc;
    const half = road.w / 2;

    // yön ve sağ vektörü (komşu noktalardan, parça sınırında da süreklilik için tüm yoldan)
    const dirs = [], rights = [];
    for (let i = 0; i < pts.length; i++) {
      const gi = off + i;
      const a = all[Math.max(0, gi - 1)], b = all[Math.min(all.length - 1, gi + 1)];
      let tx = b[0] - a[0], tz = b[1] - a[1];
      const l = Math.hypot(tx, tz) || 1; tx /= l; tz /= l;
      dirs.push([tx, tz]);
      rights.push([tz, -tx]);
    }
    const at = (i, o, y, n, uv) => ({ p: [pts[i][0] + rights[i][0] * o, y, pts[i][1] + rights[i][1] * o], n, uv });
    // indeks listesi üzerinden şerit ekle
    const strip = (b, idx, f, info) => b.strip(idx.map(f), idx.map((i) => dirs[i]), info);
    const allIdx = pts.map((_, i) => i);

    // asfalt (rinfo.w = kavşağa uzaklık → kavşakta şerit çizgisi yok)
    strip(t.road, allIdx, (i) => [at(i, -half, ys[i], up, [dist[i], -half]), at(i, half, ys[i], up, [dist[i], half])],
      (j) => [road.n, road.o, half, jd[allIdx[j]]]);

    // kaldırım / omuz: kavşak içinde kesilir
    const runs = [];
    let cur = [];
    for (let i = 0; i < pts.length; i++) {
      if (jd[i] > 0) cur.push(i);
      else { if (cur.length > 1) runs.push(cur); cur = []; }
    }
    if (cur.length > 1) runs.push(cur);

    for (const run of runs) {
      for (const side of [-1, 1]) {
        const out = (i) => [rights[i][0] * side, 0, rights[i][1] * side];
        const inw = (i) => [-rights[i][0] * side, 0, -rights[i][1] * side];
        const e = half * side;
        if (road.s) {
          const o = (half + WALK_W) * side, yt = (i) => ys[i] + CURB_H;
          // bordür yüzü (yola bakan), kaldırım üstü (parke), dış yan duvar
          strip(t.walk, run, (i) => [at(i, e, ys[i] - 0.05, inw(i), [dist[i], 0]), at(i, e, yt(i), inw(i), [dist[i], CURB_H])], [1, 0, 0, 0]);
          strip(t.walk, run, (i) => [at(i, e, yt(i), up, [dist[i], e]), at(i, o, yt(i), up, [dist[i], o])], [0, 0, 0, 0]);
          strip(t.walk, run, (i) => [at(i, o, yt(i), out(i), [dist[i], 0]), at(i, o, ys[i] - SKIRT, out(i), [dist[i], -SKIRT])], [1, 0, 0, 0]);
        } else {
          const o = (half + 0.6) * side;
          strip(t.walk, run, (i) => [at(i, e, ys[i] - 0.02, up, [dist[i], e]), at(i, o, ys[i] - 0.02, up, [dist[i], o])], [1, 0, 0, 0]);
          strip(t.walk, run, (i) => [at(i, o, ys[i] - 0.02, out(i), [dist[i], 0]), at(i, o, ys[i] - SKIRT, out(i), [dist[i], -SKIRT])], [1, 0, 0, 0]);
        }
      }
    }
  }

  const rMat = roadMaterial(), wMat = walkMaterial();
  for (const t of tiles.values()) {
    const group = new THREE.Group();
    const a = t.road.mesh(rMat), b = t.walk.mesh(wMat);
    if (a) group.add(a);
    if (b) group.add(b);
    const box = new THREE.Box3().setFromObject(group);
    roadTiles.push({ group, center: box.getCenter(new THREE.Vector3()), minor: t.minor });
    scene.add(group);
  }
}

// Araziyi 128x128 hücrelik parçalara böl (görüş dışındakiler çizilmez)
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

buildRoadMeshes();
const roadCount = roadData ? roadData.roads.length : 0;

// ---------------------------------------------------------------------------
// Başlangıç kamerası: Seul merkezi (Jongno), güneybatıdan bakış
// ---------------------------------------------------------------------------
function toWorld(lat, lon) {
  const [latMin, lonMin, latMax] = meta.bbox;
  const mlon = 111320 * Math.cos(((meta.bbox[0] + meta.bbox[2]) / 2) * Math.PI / 180);
  return [(lon - lonMin) * mlon - Wm / 2, (latMax - lat) * 111320 - Dm / 2];
}
const params = new URLSearchParams(location.search);
let [cx, cz] = toWorld(37.556, 126.985);
cx += Number(params.get('ox') || 0); cz += Number(params.get('oz') || 0); // test için kaydırma
const camDist = Number(params.get('d') || 2500);
controls.target.set(cx, heightAt(cx, cz), cz);
camera.position.set(cx - camDist * 0.5, heightAt(cx, cz) + camDist * 0.55, cz + camDist * 0.8);
controls.update();

document.getElementById('info').textContent =
  `Alan: ${(Wm / 1000).toFixed(1)} × ${(Dm / 1000).toFixed(1)} km · ${roadCount ? roadCount + ' yol' : 'yol verisi yok'}`;

// ---------------------------------------------------------------------------
// Klavye ile hareket
// ---------------------------------------------------------------------------
const keys = new Set();
addEventListener('keydown', (e) => keys.add(e.code));
addEventListener('keyup', (e) => keys.delete(e.code));
const clock = new THREE.Clock();
const fwd = new THREE.Vector3(), right = new THREE.Vector3();

function tick() {
  const dt = Math.min(clock.getDelta(), 0.1);
  const dist = camera.position.distanceTo(controls.target);
  const speed = dist * 0.8 * dt;
  camera.getWorldDirection(fwd); fwd.y = 0; fwd.normalize();
  right.crossVectors(fwd, camera.up).normalize();
  const move = new THREE.Vector3();
  if (keys.has('KeyW') || keys.has('ArrowUp')) move.add(fwd);
  if (keys.has('KeyS') || keys.has('ArrowDown')) move.sub(fwd);
  if (keys.has('KeyD') || keys.has('ArrowRight')) move.add(right);
  if (keys.has('KeyA') || keys.has('ArrowLeft')) move.sub(right);
  if (move.lengthSq()) {
    move.normalize().multiplyScalar(speed);
    camera.position.add(move); controls.target.add(move);
  }
  // kamera arazinin altına girmesin
  const gy = heightAt(camera.position.x, camera.position.z) + 3;
  if (camera.position.y < gy) camera.position.y = gy;

  // uzak ara sokakları gizle
  for (const t of roadTiles) t.group.visible = camera.position.distanceTo(t.center) < (t.minor ? 5000 : 16000);

  controls.update();
  renderer.render(scene, camera);
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
window.__ready = true;
tick();
