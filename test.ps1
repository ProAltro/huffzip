# test.ps1 — huffzip round-trip tests
# Run from the repo root: .\test.ps1

$exe  = ".\huffzip.exe"
$tmp  = "$env:TEMP\huffzip_test"
$pass = 0
$fail = 0

New-Item -ItemType Directory -Force -Path $tmp | Out-Null

# ── helpers ────────────────────────────────────────────────────────────────

function Test-String {
    param([string]$name, [string]$content, [switch]$HuffmanOnly)

    $src  = "$tmp\$name.txt"
    $comp = "$tmp\$name.huff"
    $dec  = "$tmp\$name.dec.txt"

    [System.IO.File]::WriteAllBytes($src, [System.Text.Encoding]::ASCII.GetBytes($content))
    Run-Huffzip $src $comp $dec $HuffmanOnly

    $orig = [System.IO.File]::ReadAllBytes($src)
    $got  = [System.IO.File]::ReadAllBytes($dec)
    Report $name $HuffmanOnly ([System.Linq.Enumerable]::SequenceEqual($orig, $got))
}

function Test-File {
    param([string]$path, [switch]$HuffmanOnly)

    $name = [System.IO.Path]::GetFileName($path)
    $comp = "$tmp\$name.huff"
    $dec  = "$tmp\$name.dec"

    Run-Huffzip $path $comp $dec $HuffmanOnly

    $orig = [System.IO.File]::ReadAllBytes($path)
    $got  = [System.IO.File]::ReadAllBytes($dec)
    Report $name $HuffmanOnly ([System.Linq.Enumerable]::SequenceEqual($orig, $got))
}

function Run-Huffzip([string]$src, [string]$comp, [string]$dec, [bool]$HuffmanOnly) {
    if ($HuffmanOnly) { & $exe --huffman $src $comp 2>$null }
    else              { & $exe           $src $comp 2>$null }
    & $exe -u $comp $dec 2>$null
}

function Report([string]$name, [bool]$HuffmanOnly, [bool]$ok) {
    $mode = if ($HuffmanOnly) { "Huffman-only" } else { "LZ77+Huffman" }
    if ($ok) { Write-Host "  [PASS] $name ($mode)"; $script:pass++ }
    else      { Write-Host "  [FAIL] $name ($mode)"; $script:fail++ }
}

# ── string tests ──────────────────────────────────────────────────────────

Write-Host ""
Write-Host "=============================="
Write-Host "  huffzip round-trip tests"
Write-Host "=============================="
Write-Host ""
Write-Host "  -- synthetic strings --"

$strings = @{
    "short"      = "Hello, World!"
    "repetitive" = "abcabc" * 100
    "sentence"   = "The quick brown fox jumps over the lazy dog."
    "long"       = ("Lorem ipsum dolor sit amet. " * 40).Trim()
    "single"     = "a" * 100
}

foreach ($name in $strings.Keys) {
    Test-String -name $name              -content $strings[$name]
    Test-String -name "${name}_huff"     -content $strings[$name] -HuffmanOnly
}

# ── real file tests ───────────────────────────────────────────────────────

Write-Host ""
Write-Host "  -- real files --"

$files = @(
    ".\src\main.cpp",
    ".\src\huffman.cpp",
    ".\src\shannon.cpp",
    ".\README.md"
)

foreach ($f in $files) {
    if (Test-Path $f) {
        Test-File -path $f
        Test-File -path $f -HuffmanOnly
    } else {
        Write-Host "  [SKIP] $f not found"
    }
}

# ── summary ───────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "=============================="
Write-Host "  Results: $pass passed, $fail failed"
Write-Host "=============================="
Write-Host ""

Remove-Item -Recurse -Force $tmp

exit $fail
