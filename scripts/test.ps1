$ErrorActionPreference = 'Stop'
$zig = 'C:\ProgramData\chocolatey\lib\zig\tools\zig-x86_64-windows-0.17.0\zig.exe'
if (-not (Test-Path $zig)) { throw "未找到 Zig 编译器: $zig" }

$silk = @(Get-ChildItem 'third_party/silk/src' -Filter '*.c' | ForEach-Object FullName)
$includes = @('-Isrc', '-Ithird_party/miniaudio', '-Ithird_party/stb', '-Ithird_party/silk/interface', '-Ithird_party/silk/src')
$tests = @(
    @{ Name = 'cli'; Sources = @('tests/test_cli.c', 'src/cli.c') },
    @{ Name = 'silk_encoder'; Sources = @('tests/test_silk_encoder.c', 'src/silk_encoder.c') + $silk },
    @{ Name = 'convert'; Sources = @('tests/test_convert.c', 'src/convert.c', 'src/silk_encoder.c', 'src/miniaudio_impl.c') + $silk },
    @{ Name = 'decode'; Sources = @('tests/test_decode.c', 'src/silk_decoder.c', 'src/silk_encoder.c') + $silk }
)
New-Item -ItemType Directory -Force -Path 'build-tests' | Out-Null
foreach ($test in $tests) {
    $testExe = Join-Path 'build-tests' "test-$($test.Name).exe"
    # Zig 的优化构建默认定义 NDEBUG，测试必须显式启用 assert。
    & $zig cc -target x86_64-windows-gnu -O2 -UNDEBUG -D_CRT_SECURE_NO_WARNINGS -DNTDDI_VERSION=0x0A000000 $includes @($test.Sources) -o $testExe
    if ($LASTEXITCODE -is [int] -and $LASTEXITCODE -ne 0) { throw "编译测试 $($test.Name) 失败: $LASTEXITCODE" }
    & ".\$testExe"
    if ($LASTEXITCODE -is [int] -and $LASTEXITCODE -ne 0) { throw "测试 $($test.Name) 失败: $LASTEXITCODE" }
    Write-Host "PASS $($test.Name)"
}
