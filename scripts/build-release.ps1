$ErrorActionPreference = 'Stop'
$zig = 'C:\ProgramData\chocolatey\lib\zig\tools\zig-x86_64-windows-0.17.0\zig.exe'
if (-not (Test-Path $zig)) { throw "未找到 Zig 编译器: $zig" }

$sourceFiles = @(
    'src/main.c', 'src/cli.c', 'src/convert.c', 'src/silk_decoder.c', 'src/silk_encoder.c', 'src/miniaudio_impl.c'
) + @(Get-ChildItem 'third_party/silk/src' -Filter '*.c' | ForEach-Object FullName)

New-Item -ItemType Directory -Force -Path 'dist' | Out-Null
& $zig cc -target x86_64-windows-gnu -municode -O3 -DNDEBUG -D_CRT_SECURE_NO_WARNINGS -DNTDDI_VERSION=0x0A000000 '-Isrc' '-Ithird_party/miniaudio' '-Ithird_party/stb' '-Ithird_party/silk/interface' '-Ithird_party/silk/src' $sourceFiles -o 'dist/audio2silk.exe'
if ($LASTEXITCODE -is [int] -and $LASTEXITCODE -ne 0) { throw "编译失败，退出码 $LASTEXITCODE" }
Write-Host '已生成 dist/audio2silk.exe'
