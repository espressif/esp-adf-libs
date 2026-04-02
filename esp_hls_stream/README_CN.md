## esp_hls_stream

- [English](./README.md)

`esp_hls_stream` 在乐鑫平台上提供 **HTTP 实时流（HLS，HTTP Live Streaming）** 支持：包括播放列表与媒体分段处理、可选的加密，以及与 **ESP Extractor**、**ESP-GMF I/O** 的对接。

## 背景

HLS 是一种基于 HTTP 的自适应码率流媒体协议。客户端使用**播放列表**（通常扩展名为 **`.m3u8`**）并拉取**媒体分段**，而不是单一大文件，因而适合部署在标准 HTTP 基础设施之后的 Web、移动端与嵌入式播放器。

流可通过 **`#EXT-X-KEY`** 等方式采用 **AES-128** 或 **SAMPLE-AES** 等机制实现**加密与内容保护**（具体是否构成 **DRM** 场景取决于业务与密钥下发方式）。

## 主要特性

- 通过 Extractor 路径实现 HLS **音视频**解复用
- **AES-128** 加密媒体
- 在实现支持的范围内，支持 **SAMPLE-AES** 及相关 HLS 加密模式

## 架构

本组件提供**两种集成层级**。

### 1. 与 Extractor 兼容的 API

该路径支持本库所使用的 **完整** HLS 能力：

- 音视频提取
- AES-128
- SAMPLE-AES

其输出为**压缩**的音视频**访问单元**（或等价的帧数据），可按 Extractor 配置送入解码器。

**注册与典型流程**

- 使用 `esp_hls_extractor_register()` 注册 HLS Extractor，之后与其它 Extractor 实现用法相同。

典型调用序列为：

`esp_extractor_open` → `esp_extractor_parse_stream` → `esp_extractor_read_frame` → `esp_extractor_close`

> **说明：** HLS Extractor 依赖其它已注册的 Extractor（例如 **TS**，以及在需要时的 **audio ES**）来解析容器与基本码流。使用前请确保这些 Extractor **已完成注册**。

### 2. 与 GMF I/O 兼容的 API

HLS 模块可作为标准的 **ESP-GMF 读端 I/O**：应用通过 GMF I/O 接口读取**连续字节流**。该路径使用便捷，并针对**音频**流媒体做了优化。

**限制**

- **仅音频**（按 I/O 实现文档，该路径上无完整音视频管线）
- 该路径**不支持** **SAMPLE-AES**
- **不保证帧边界**；需借助下游元素（例如 **`esp_audio_simple_dec`**）定位并解码每一帧音频

**集成方式**

调用 `esp_gmf_io_hls_init()`，将得到的 I/O 注册到 **GMF 池**中；当 URL 或信源被识别为 HLS 时，可根据播放器或管线配置**自动**选用该 I/O。

### HLS 辅助 API

为简化 Extractor 配置，辅助接口会填写**播放列表/分段 I/O**及相关参数：

- **`esp_hls_extractor_file_cfg_init`** — 通过 **libc 风格**文件 I/O 访问本地文件（与 `esp_hls_extractor_file_cfg_deinit` 配对使用）。
- **`esp_hls_extractor_io_cfg_init`** — 通过**外部 GMF 池**句柄支持**本地、HTTP 或混合**访问（与 `esp_hls_extractor_io_cfg_deinit` 配对使用）。

使用 **`esp_hls_extractor_open_with_cfg()`** 以准备好的配置打开 Extractor。结构体定义见 **`esp_hls_helper.h`** 与 **`esp_hls_types.h`**。

## 依赖

声明于 **`idf_component.yml`**：

| 组件 | 说明 |
|------|------|
| `espressif/esp_extractor` | Extractor 框架与流掩码 |
| `espressif/media_lib_sal` | 媒体与加解密辅助接口 |
| `espressif/gmf_io` | GMF I/O 类型与注册 |

## 配置

在 **menuconfig** 中：**Component config** → **Espressif HLS Stream Configuration**

| 选项 | 说明 |
|------|------|
| `HLS_STREAM_EXTRACTOR_SUPPORT` | 编译 **HLS Extractor** API 支持（默认：**开启**） |
| `HLS_STREAM_IO_SUPPORT` | 编译 **HLS GMF I/O** API 支持（默认：**开启**） |

GMF I/O 路径上的**线程、缓冲区及可选回调**等，见 **`esp_gmf_io_cfg_t`** 及 **`esp_hls_io.h`** 中的相关默认值。

## 使用与示例

端到端集成示例见 [hls_live_stream](examples/hls_live_stream/README.md)：

| 路径 | 示例内容 |
|------|----------|
| **GMF I/O** | 使用 `esp_gmf_io_hls_init()` 配合 **Audio Simple Player**、Wi-Fi 与板级音频输出（`hls_live_simple_player.c`）。 |
| **Extractor** | **`parse_hls()`** — 基于池的 HTTP/文件 I/O 与按帧读取（`hls_extractor_test.c`）。 |


## 技术支持

如需技术支持，请使用以下链接：

- 技术支持：[esp32.com](https://esp32.com/viewforum.php?f=20) 论坛
- 问题反馈与功能需求：[GitHub issue](https://github.com/espressif/esp-gmf/issues)

我们会尽快回复。
