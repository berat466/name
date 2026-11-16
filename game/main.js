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

const [terrainTex, roadTex] = await Promise.all([loadPbrArray(TERRAIN_SETS), loadPbrArray(ROAD_SETS)]);

// Ortak GLSL: gürültü, iki ölçekli örnekleme, nra çözme
const GLSL_COMMON = `
  float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
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

function patchStandard(mat, uniforms, vtxExtra, fragDecl, fragBody) {
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
// Yol malzemesi: 3 asfalt PBR setinin karışımı (yıpranma, yama, çatlak)
// ---------------------------------------------------------------------------
function roadMaterial() {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: roadTex.alb }, tN: { value: roadTex.nra } },
    { decl: 'attribute vec2 ruv;\nattribute vec2 rdir;\nvarying vec2 vRuv;\nvarying vec2 vRdir;', body: 'vRuv = ruv; vRdir = rdir;' },
    'varying vec2 vRuv;\nvarying vec2 vRdir;', `
    float n1 = vnoise(vWP.xz * 0.02);
    float n2 = vnoise(vWP.xz * 0.07 + 33.0);
    float wB = smoothstep(0.4, 0.8, n1) * 0.6;     // farklı asfalt yüzeyi
    float wC = smoothstep(0.55, 0.85, n2) * 0.5;   // çatlak / yama
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / 5.0, 0.0, (1.0 - wB) * (1.0 - wC));
    addLayer(tA, tN, vRuv / 8.0, 1.0, wB * (1.0 - wC));
    addLayer(tA, tN, vRuv / 4.0, 2.0, wC);
    ${GLSL_RESOLVE}
    diffuseColor.rgb *= accA;

    vec3 N0 = vec3(0.0, 1.0, 0.0);
    vec3 Tg = normalize(vec3(vRdir.y, 0.0, -vRdir.x)); // yolun enine yön
    vec3 Bt = normalize(vec3(vRdir.x, 0.0, vRdir.y));  // yol boyunca
    blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
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

// ---------------------------------------------------------------------------
// Yollar (OpenStreetMap verisi varsa)
// ---------------------------------------------------------------------------
async function buildRoads() {
  let data;
  try {
    const res = await fetch('./data/roads.json');
    if (!res.ok) return 0;
    data = await res.json();
  } catch { return 0; }

  const STEP = 10; // metre: araziye oturması için yolu bu aralıkla böl
  const pos = [], nor = [], uv = [], dir = [], idx = [];
  const ORDER = { tertiary: 0, secondary: 1, primary: 2, trunk: 3, motorway: 4 };

  for (const road of data.roads) {
    const p = road.p, half = road.w / 2;
    // yeniden örnekle
    const pts = [];
    for (let i = 0; i < p.length - 2; i += 2) {
      const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
      const n = Math.max(1, Math.ceil(Math.hypot(bx - ax, bz - az) / STEP));
      for (let s = 0; s < n; s++) pts.push([ax + (bx - ax) * s / n, az + (bz - az) * s / n]);
    }
    pts.push([p[p.length - 2], p[p.length - 1]]);
    if (pts.length < 2) continue;

    // yükseklik: köprüde uç noktalar arasında doğrusal, değilse arazi + pay
    const lift = 0.6 + ORDER[road.c] * 0.15 + Math.max(0, road.l) * 6;
    const ys = pts.map(([x, z]) => heightAt(x, z) + lift);
    if (road.b) {
      const y0 = ys[0], y1 = ys[ys.length - 1];
      for (let i = 0; i < ys.length; i++) ys[i] = Math.max(ys[i], y0 + (y1 - y0) * i / (ys.length - 1) + 4);
    }

    const base = pos.length / 3;
    let dist = 0;
    for (let i = 0; i < pts.length; i++) {
      const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
      let tx = b[0] - a[0], tz = b[1] - a[1];
      const tl = Math.hypot(tx, tz) || 1; tx /= tl; tz /= tl;
      const [x, z] = pts[i], y = ys[i];
      if (i > 0) dist += Math.hypot(x - pts[i - 1][0], z - pts[i - 1][1]);
      pos.push(x - tz * half, y, z + tx * half, x + tz * half, y, z - tx * half);
      nor.push(0, 1, 0, 0, 1, 0);
      // metre cinsinden: enine (-w/2..w/2), boyuna mesafe
      uv.push(-half, dist, half, dist);
      dir.push(tx, tz, tx, tz);
      if (i > 0) {
        const q = base + (i - 1) * 2;
        idx.push(q, q + 2, q + 1, q + 1, q + 2, q + 3);
      }
    }
  }

  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(nor, 3));
  g.setAttribute('ruv', new THREE.Float32BufferAttribute(uv, 2));
  g.setAttribute('rdir', new THREE.Float32BufferAttribute(dir, 2));
  g.setIndex(idx);
  g.computeBoundingSphere();
  const mesh = new THREE.Mesh(g, roadMaterial());
  mesh.renderOrder = 1;
  scene.add(mesh);
  return data.roads.length;
}
const roadCount = await buildRoads();

// ---------------------------------------------------------------------------
// Başlangıç kamerası: Seul merkezi (Jongno), güneybatıdan bakış
// ---------------------------------------------------------------------------
function toWorld(lat, lon) {
  const [latMin, lonMin, latMax] = meta.bbox;
  const mlon = 111320 * Math.cos(((meta.bbox[0] + meta.bbox[2]) / 2) * Math.PI / 180);
  return [(lon - lonMin) * mlon - Wm / 2, (latMax - lat) * 111320 - Dm / 2];
}
const [cx, cz] = toWorld(37.556, 126.985);
const params = new URLSearchParams(location.search);
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
