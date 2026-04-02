# HLS 直播流示例

- [English Version](./README.md)
- 复杂示例：⭐⭐⭐

## 示例简介

本示例展示如何使用 **`esp_hls_stream`** 对 **HLS（HTTP Live Streaming，HTTP 实时流）** 进行解析与播放。

本示例包含两种使用路径：

- **默认**：将 HLS 音频流直接播放到开发板的音频输出。
- **可选**：使用 HLS 提取器（`parse_hls`）解析 HLS 并读取音视频帧。

### 典型场景

- 网络电台与直播广播
- 播客与点播音频流
- DRM 保护的音频（高端音乐服务）

### 技术细节

### HLS 音频播放器（`hls_live_simple_player.c`）

HLS I/O 的行为与普通读端 I/O 相同，将数据送入 audio simple player。Simple player 负责查找帧边界、解码，并通过 **`esp_codec_dev`** 输出 PCM：

`HLS I/O` → `esp_audio_simple_player` → `esp_codec_dev`

为检测实际的容器/编码格式，请在 HLS I/O 上注册 **`file_seg_cb`**，并调用 **`esp_gmf_audio_dec_reconfig_by_sound_info()`**，以使解码器与检测到的格式一致。

### HLS 提取器（`hls_extractor_test.c`）

使用辅助 API **`esp_hls_extractor_io_cfg_init()`** 构建 HLS 提取器配置。然后使用通用提取器 API 驱动 HLS：获取音视频流信息并读取每一帧。

HLS 提取器在内部对 MPEG-TS 分段使用 TS 提取器。使用前**必须同时注册**以下两者：

- **`esp_hls_extractor_register()`**
- **`esp_ts_extractor_register()`**

### 运行流程

1. 安装 media OS 适配器，并通过板级管理器初始化音频设备。
2. 连接 Wi-Fi（`example_connect()`）。
3. 创建 audio simple player。
4. 初始化 HLS I/O（`esp_gmf_io_hls_init()`）。
5. 将 I/O 注册到 player。
6. 开始播放：`esp_audio_simple_player_run_to_end(player, HLS_STREAM_URL, NULL)`。
7. 退出时进行清理。

## 环境配置

### 硬件要求

- 由 **`esp_board_manager`** 支持的乐鑫开发板，且具备：
  - **音频 DAC / 播放通路**
- 在 `sdkconfig.defaults` 中已启用 **PSRAM**（`CONFIG_SPIRAM=y`）。

### 默认 IDF 分支

本示例支持 **IDF `release/v5.4`（≥ v5.4.3）** 与 **`release/v5.5`（≥ v5.5.2）**。

### 软件要求

- 托管组件在首次编译时根据示例及组件中的 **`idf_component.yml`** 解析得到（可能需要网络连接）。
- ESP-IDF 按惯例使用 **`install.sh`** / **`export.sh`**。

## 编译与烧录

### 编译准备

开始编译前，请确认已配置 ESP-IDF 环境。若尚未配置，请在 ESP-IDF 根目录执行以下命令（完整步骤见 [ESP-IDF 编程指南](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)）：

```
./install.sh
. ./export.sh
```

进入**本**示例目录（若您的仓库布局不同，请相应调整路径）：

```
cd <path-to-repo>/esp_hls_stream/esp_hls_stream/examples/hls_live_stream
```

使用板级管理器选择目标芯片与开发板配置（先列出板型，再选择与硬件匹配的板）：

```bash
idf.py gen-bmgr-config -l
idf.py gen-bmgr-config -b <your_board_id>
```

将 `<your_board_id>` 替换为您的实际板级 ID（不同芯片对应不同板型）。

### 项目配置

- **Wi-Fi**：配置 **Example Connection Configuration**（或您工程中的 Wi-Fi 菜单），以便 **`example_connect()`** 能接入您的接入点。
- **HLS URL**：若默认流不可用或需要更换信源，请编辑 `main/hls_live_simple_player.c` 中的 **`HLS_STREAM_URL`**。
- **Audio simple player**：重采样、声道与位深的默认值位于 `sdkconfig.defaults`（例如 **48 kHz**、**2 ch**、**16-bit**）。

### 编译与烧录命令

编译、烧录并监视串口：

```
idf.py -p PORT flash monitor
```

退出监视器：**Ctrl+]**（或您所用 ESP-IDF 版本的默认快捷键）。

## 如何使用本示例

### 功能与用法

- 设备会连接 Wi-Fi 并自动开始播放。
- 音频通过开发板 DAC 输出。
- 日志 TAG 为 **`HLS_LIVE_STREAM`**。

### 可选：提取器模式

```c
parse_hls(HLS_STREAM_URL, 30000);
```

- 在 `app_main` 中启用，或从您自己的代码路径调用。
- 将持续时间参数设为您希望测试运行的时长（毫秒）。

## 技术支持

- 论坛：[esp32.com](https://esp32.com/viewforum.php?f=20)
- 问题（GMF 及相关）：[GitHub 上的 esp-gmf](https://github.com/espressif/esp-gmf/issues)

我们会尽快回复。
