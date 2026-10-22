# Audio2Silk

[![Language](https://img.shields.io/badge/language-C11-00599C.svg)](https://en.cppreference.com/w/c/11)
[![Platform](https://img.shields.io/badge/platform-Windows%207%2B-0078D4.svg)](#系统要求)
[![FFmpeg](https://img.shields.io/badge/FFmpeg-not%20required-success.svg)](#项目特点)

一个面向 Windows 的轻量级命令行工具，用于将常见音频文件转换为微信、QQ 语音使用的腾讯 SILK V3 格式，也可以将 SILK 文件解码为 WAV。

项目使用纯 C 编写，音频解码器和 SILK 编解码器均编译进单个 EXE。运行时不调用 FFmpeg，不需要安装额外 DLL。

## 项目特点

- 单文件 Windows x64 可执行程序，适合脚本、机器人和服务端工具调用。
- 支持 WAV、MP3、FLAC、Ogg/Vorbis 输入。
- 输出微信、QQ 常用的 `#!SILK_V3` 语音流。
- 支持标准 SILK 头和腾讯 SILK 头的解码。
- 自动转换为单声道、24 kHz，并以约 24 kbps 编码。
- 支持包含空格和中文的 Windows 路径。
- 不依赖 FFmpeg 进程或 FFmpeg DLL。

## 快速开始

可以直接使用仓库中的 [`dist/audio2silk.exe`](dist/audio2silk.exe)，也可以按照[源码构建](#源码构建)一节自行编译。

### 编码为 SILK

```cmd
audio2silk.exe "输入音频.mp3" "输出语音.silk"
```

程序严格接受输入、输出两个路径参数。建议始终用双引号包裹路径，以兼容空格和中文目录。

### 解码为 WAV

```cmd
audio2silk.exe -d "输入语音.silk" "输出音频.wav"
```

解码输出固定为单声道、24 kHz、16-bit PCM WAV，可直接播放或交给其他音频工具继续处理。

## 参数说明

```text
audio2silk.exe <输入音频路径> <输出.silk路径>
audio2silk.exe -d <输入.silk路径> <输出.wav路径>
```

| 参数 | 说明 |
| --- | --- |
| `-d` | 启用 SILK 解码模式；不提供时默认为编码模式 |
| `输入音频路径` | WAV、MP3、FLAC 或 Ogg/Vorbis 文件路径 |
| `输出.silk路径` | 编码后生成的 SILK V3 文件路径 |
| `输入.silk路径` | 标准或腾讯头格式的 SILK V3 文件路径 |
| `输出.wav路径` | 解码后生成的 PCM WAV 文件路径 |

## 编码参数

| 项目 | 值 |
| --- | --- |
| 声道 | 单声道 |
| 采样率 | 24,000 Hz |
| 帧长 | 20 ms |
| 目标码率 | 约 24 kbps |
| SILK 文件头 | `#!SILK_V3` |
| 解码 WAV | 16-bit PCM、单声道、24 kHz |

## 支持情况

| 功能或格式 | 状态 | 备注 |
| --- | --- | --- |
| WAV | 支持 | 输入格式由内置 miniaudio 解码 |
| MP3 | 支持 | 无需外部解码器 |
| FLAC | 支持 | 无需外部解码器 |
| Ogg/Vorbis | 支持 | 无需外部解码器 |
| AAC / M4A | 暂不保证 | 建议先转换为 WAV、MP3、FLAC 或 Ogg/Vorbis |
| SILK 编码 | 支持 | 输出微信、QQ 常用的 SILK V3 格式 |
| SILK 解码 | 支持 | 输出 24 kHz 单声道 PCM WAV |
| Windows 7 | 已有实测可运行记录 | 不同系统环境仍可能存在差异 |
| Linux / macOS | 未提供发行版 | 当前发布脚本和宽字符入口面向 Windows |

## 系统要求

- Windows 7 SP1 或更高版本，x64；Windows 7 已有实测可运行记录，但未覆盖所有系统配置。
- 直接运行发行版只需要系统自带的 UCRT，不需要安装 FFmpeg 或其他 DLL。
- 从源码构建需要 Zig 0.17（推荐）或 CMake 3.20+ 与支持 C11 的 Windows 编译器。

## 源码构建

### 使用 Zig

仓库的发布脚本默认使用 Zig 0.17，并生成 Windows x64 单文件程序：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-release.ps1
```

构建结果位于 `dist/audio2silk.exe`。如果 Zig 安装位置不同，请修改 [`scripts/build-release.ps1`](scripts/build-release.ps1) 中的 `$zig` 路径。

### 使用 CMake

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## 运行测试

使用与发布版本相同的 Zig 工具链运行全部测试：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/test.ps1
```

测试覆盖命令行参数、SILK 编码器、常见音频转换和 SILK 解码。

## 退出码

| 退出码 | 含义 |
| ---: | --- |
| `0` | 操作成功 |
| `2` | 参数错误 |
| `11` | 无法打开输入文件 |
| `12` | 输入音频或 SILK 解码失败 |
| `13` | SILK 编码失败 |
| `14` | 无法创建或写入输出文件 |

## 常见问题

### 为什么 SILK 解码后的 WAV 比原始 MP3 大？

这是正常现象。MP3 和 SILK 都是有损压缩格式，而解码生成的 WAV 是未压缩 PCM。以单声道、24 kHz、16-bit PCM 为例，每秒约占用 48 KB，因此 WAV 文件通常会明显大于 MP3 或 SILK 文件。

文件变大不代表丢失的音质被恢复。SILK 编码阶段已经丢弃的信息无法通过解码还原，解码只是把剩余的音频内容展开为播放器可直接读取的 PCM 数据。

### SILK 文件能还原为原始音频吗？

可以解码成可播放的 WAV，但无法无损恢复原始 MP3、WAV 或其他源文件。格式、元数据以及有损编码丢弃的音频细节都不会恢复。

### 为什么不直接使用 FFmpeg？

这个项目的目标是提供一个体积较小、部署简单的独立命令行程序。音频解码、重采样和 SILK 编解码都在进程内完成，部署时无需携带 FFmpeg 可执行文件或动态库。

## 项目结构

```text
src/                 命令行、音频转换及 SILK 编解码实现
tests/               C 语言单元测试和集成测试
third_party/         miniaudio、stb_vorbis 和 SILK SDK
scripts/             Windows 构建与测试脚本
dist/                可直接运行的 Windows x64 程序
CMakeLists.txt       CMake 构建及测试配置
```

## 安全与使用说明

- 本项目与腾讯、微信、QQ、Skype 均无官方关联。
- 请仅处理你有权使用和转换的音频文件。
- SILK 兼容性可能受到接收平台、机器人框架或消息协议封装方式影响；本工具只负责生成或解码 SILK 音频数据。

## 第三方组件与许可证

- SILK SDK 许可证见 [`LICENSES-SILK.txt`](LICENSES-SILK.txt)。
- miniaudio 与 stb_vorbis 的许可证说明见 [`THIRD-PARTY-LICENSES.txt`](THIRD-PARTY-LICENSES.txt)。
- 各第三方源码文件中保留的版权和许可证声明同样适用。

在分发源码或二进制文件前，请一并保留上述许可证和版权声明。

注意：仓库当前未为 Audio2Silk 自身代码声明独立许可证；第三方许可证不代表项目代码也采用相同许可。
