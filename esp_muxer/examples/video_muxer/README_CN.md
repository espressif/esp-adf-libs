# 视频 Muxer 示例

本示例演示如何使用 ESP Muxer 组件将采集源（摄像头与麦克风）的**音频与视频进行混流**并**保存到存储**（SD 卡）中。

## 概述

为简化整体流程，本应用在内部使用带 `esp_muxer` 支持的 `esp_capture`：

1. **采集**：从摄像头（V4L2）采集视频，从板载音频 ADC（编解码器）采集音频。
2. **编码**：视频采用 MJPEG 或 H264，音频采用 AAC。
3. **混流**：将编码后的音视频流写入容器格式（TS、FLV、MP4、AVI）。
4. **写入**：将混流后的文件写入 SD 卡（例如 `/sdcard/muxed_h264.ts`、`/sdcard/muxed_mjpeg.flv`）。

同步以**音频**源为主（`ESP_CAPTURE_SYNC_MODE_AUDIO`）。

## 典型应用场景

本示例可直接作为**将摄像头与麦克风录制到存储文件**的产品的起点：

| 应用场景 | 说明 |
|----------|------|
| **行车记录仪 (Dashcam)** | 车载录制：连续或事件触发的视频 + 音频保存到 SD 卡。 |
| **PVR（个人录像机）** | 摄像头/麦克风本地录像，用于安防、会议或直播采集。 |
| **执法记录仪 / 便携录像** | 便携音视频录制，可选循环覆盖、按分片切文件。 |
| **安防 / 类 NVR** | 多段或按时间分片录制（如通过 `slice_cb` 与时长控制）。 |

> **简单用法：** 直接使用本示例即可快速验证：连接摄像头、音频 ADC 与 SD 卡，编译运行即可。无需额外业务逻辑即可在 SD 卡上得到**已混流的音视频文件**（TS、FLV 等）。根据产品需求调整 `slice_cb`、`MAX_RECORD_DURATION` 和 `muxer_lists[]`（例如循环录制、按事件单文件或固定一种容器格式）。

## 硬件与前提条件

- 开发板需具备：
  - **摄像头**：CSI、DVP（由 v4l2 支持）
  - **音频 ADC**（编解码器）用于麦克风输入
  - 已挂载的 **SD 卡**（如作为默认存储）
- ESP-IDF V5.5.2 或更高版本，并针对目标芯片（如 ESP32、ESP32-S3、ESP32-P4）完成本组件的编译

## Muxer 与流参数

| 项目 | 默认值 |
|------|--------|
| **视频编码** | MJPEG；在支持时也会使用 H264 |
| **视频分辨率** | 640×480 @ 5 fps（ESP32-P4 上为 1280×720 @ 30 fps） |
| **音频编码** | AAC |
| **音频** | 16 kHz，2 声道，16 bit |
| **容器格式** | TS、FLV（代码中也支持 MP4、AVI） |
| **输出路径** | `/sdcard/muxed_<codec>.<ext>`（如 `muxed_h264.ts`、`muxed_mjpeg.flv`） |

输出路径与分片行为由**分片回调**（`slice_cb`）控制，该回调为每个分片返回文件路径（本示例中每次录制对应一个文件）。

## 存储（文件输出）

- 通过**分片回调**配置 muxer，使每个“分片”写入到您提供的文件路径。
- **仅启用 muxer，关闭流输出**，使数据只写入文件：
  - `esp_capture_sink_enable_muxer(sink, true)`
  - `esp_capture_sink_disable_stream(sink, ESP_CAPTURE_STREAM_TYPE_VIDEO)`
  - `esp_capture_sink_disable_stream(sink, ESP_CAPTURE_STREAM_TYPE_AUDIO)`
- 每种格式/编码的录制时长为 **`MAX_RECORD_DURATION`**（默认 10 秒）。

## 编译与运行

- 先选择目标开发板

```
idf.py gen-bmgr-config -l
idf.py gen-bmgr-config -b esp32_s3_korvo2_v3
```

- 编译与烧录

```bash
idf.py build
idf.py -p <port> flash monitor
```

> [!NOTE]
> 编译目标会被在选择目标开发板时自动选择，用户不需要手动调用 `idf.py set-target`
> 其他支持的开发板（通过 `esp_board_manager`）请按上述相同步骤操作。
> 自定义开发板请参阅 [自定义开发板指南](https://github.com/espressif/esp-gmf/blob/main/packages/esp_board_manager/docs/how_to_customize_board.md) 了解详情。

## 代码流程简述

1. **板级初始化**：摄像头、音频 ADC、SD 卡（文件输出所必需）。
2. **注册**默认音频编码器、视频编码器与 muxer。
3. **创建**视频源（摄像头）与可选音频源（编解码器）。
4. **打开**以音频同步的 capture，随后针对每种 muxer 类型：
   - **配置 sink**：设置视频与音频格式（编码、分辨率、采样率、声道数）。
   - **添加 muxer**：通过分片回调将输出写到 `/sdcard/muxed_<codec>.<ext>`。
   - **启用 muxer**，关闭视频/音频流输出。
   - **启动**采集，等待 `MAX_RECORD_DURATION` 后**停止**。
5. **反初始化** capture 与源，并取消注册默认组件。

## 说明

- 若 SD 卡挂载失败，示例将不运行并打印 "Skip muxer for mount sdcard failed"。
- 示例中编码器与调度（栈大小、核心、优先级）针对 H264/OPUS 做了配置，可在 `capture_scheduler` 与格式宏中按需修改。
- 若只需录制一种容器或编码格式，可修改 `muxer_lists[]` 或传入 `record_file()` 的视频编码类型。
