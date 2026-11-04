# V81_LOCAL_BENCHMARK -- Windows tek komut: MSYS2 kur -> derle -> veri uret+SHA dogrula -> dogrula -> V80/V81 hiz olc
# Kullanim (PowerShell):  Set-ExecutionPolicy -Scope Process Bypass;  .\RUN_ALL.ps1            (varsayilan: 5 cift, 4 is parcacigi, ISA otomatik)
#   .\RUN_ALL.ps1 -Pairs 7 -Threads 8 -Isa avx2     |  .\RUN_ALL.ps1 -SkipSetup (MSYS2 paketleri zaten kuruluysa)
# Klasor yolunda BOSLUK ve Turkce karakter olmasin (ornek: C:\V81_LOCAL_BENCHMARK).
param([string]$Msys = "C:\msys64", [int]$Pairs = 5, [int]$Threads = 4, [ValidateSet('auto','avx2','avx512','both')][string]$Isa = 'auto', [switch]$SkipSetup)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($root -match '\s') { throw "Klasor yolunda bosluk var: $root -- paketi bosluksuz bir yola (ornek C:\V81_LOCAL_BENCHMARK) tasiyin." }
$bash = Join-Path $Msys "usr\bin\bash.exe"
if (-not (Test-Path $bash)) {
    Write-Host "MSYS2 bulunamadi; winget ile kuruluyor..."
    winget install -e --id MSYS2.MSYS2 --accept-package-agreements --accept-source-agreements --silent
    if (-not (Test-Path $bash)) { throw "MSYS2 kurulamadi. https://www.msys2.org adresinden elle kurup tekrar calistirin." }
}
$env:MSYSTEM = 'UCRT64'; $env:CHERE_INVOKING = '1'
function Invoke-Msys([string]$cmd) { & $bash -lc $cmd; if ($LASTEXITCODE -ne 0) { throw "MSYS2 komutu basarisiz (cikis $LASTEXITCODE): $cmd" } }
if (-not $SkipSetup) {
    try { Invoke-Msys "pacman -Syu --noconfirm" } catch { Write-Host "Ilk guncelleme kabugu kapatti olabilir; tekrar deneniyor..." }
    Invoke-Msys "pacman -Syu --noconfirm"
    Invoke-Msys "pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-openblas mingw-w64-ucrt-x86_64-python"
}
$unix = (& (Join-Path $Msys "usr\bin\cygpath.exe") -u $root).Trim()
# Satir sonu guvenligi: git/zip CRLF'e cevirmis olabilir -> bash betikleri icin LF'e normalle
$fix = "cd '" + $unix + "' && sed -i 's/\r$//' scripts/*.sh scripts/*.py reference/*.json reference/*.txt"
Invoke-Msys $fix
Write-Host "=== Cihaz bilgisi ==="
Invoke-Msys "cd '$unix' && (grep -m1 'model name' /proc/cpuinfo || true) && (grep -o -m1 -w 'avx512f\|avx2' /proc/cpuinfo | sort -u | tr '\n' ' ' || true) && gcc --version | head -1 && cmake --version | head -1"
Invoke-Msys "cd '$unix' && chmod +x scripts/*.sh && THREADS=$Threads TH_ISA=$Isa scripts/tum_adimlar.sh $Pairs"
Write-Host "Bitti. Sonuclar: $root\results\  (verify_*.log, bench_*_ozet.md, bench_*_ozet.json)"
