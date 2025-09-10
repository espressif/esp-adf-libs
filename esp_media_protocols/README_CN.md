# ESP_MEDIA_PROTOCOLS
[English](./README.md)

## 概述

ESP Media Protocols 库为 Espressif SoC 提供了主流媒体流和通信协议的完整实现。该库帮助开发者快速构建包括 VoIP 系统、流媒体服务器、智能家居设备、音频同步系统等多种多媒体应用。

# 特性

## 支持的协议

| 协议       | 描述                                                    |
|-----------|--------------------------------------------------------|
| SIP       | 会话发起协议，适用于建立、修改和终止呼叫，常用于 VoIP 通信      |
| RTSP      | 实时流协议，支持媒体流的播放、暂停、快进等操作                 |
| RTMP      | 实时消息协议，适用于推流和拉流直播                           |
| MRM       | 多设备共享音乐协议，多设备音频分发与同步                      |
| UPnP      | 通用即插即用协议，实现设备发现与互操作，常用于家庭网络设备       |

### SIP 协议

Session Initiation Protocol，基于 IP 网络实现音视频通话。
- **说明文档:** [ESP_SIP](docs/zh_CN/SIP_README_CN.md)
- **头文件:** [`esp_rtc.h`](include/esp_rtc.h)，[`esp_sip.h`](include/esp_sip.h)(deprecated)
- **功能特性:**
  - 支持多种音视频格式
  - DTMF 支持（RFC2833）
  - SSL/TLS 加密
  - 支持自定义 SIP 头
- **典型应用:**
  - IP 电话与 VoIP 应用
  - 视频会议系统
  - 可视对讲
  - 紧急通信设备

### RTSP 协议

Real-Time Streaming Protocol，实现音视频流传输控制。
- **说明文档:** [ESP_RTSP](docs/zh_CN/RTSP_README_CN.md)
- **头文件:** [`esp_rtsp.h`](include/esp_rtsp.h)
- **功能特性:**
  - 支持多种音视频格式
  - UDP/TCP 传输
  - 支持服务端和客户端模式
  - 支持推流和拉流
  - 媒体流管理（DESCRIBE, SETUP, PLAY, TEARDOWN）
- **典型应用:**
  - 网络摄像头与安防系统
  - 视频流媒体服务器
  - 媒体播放器与录像机
  - 远程监控

### RTMP 协议

Real-Time Messaging Protocol，实现与平台或自建服务器的直播流应用。
- **说明文档:** [ESP_RTMP](docs/zh_CN/RTMP_README_CN.md)
- **头文件:** [`esp_rtmp_push.h`](include/esp_rtmp_push.h)，[`esp_rtmp_server.h`](include/esp_rtmp_server.h)，[`esp_rtmp_src.h`](include/esp_rtmp_src.h)，[`esp_rtmp_types.h`](include/esp_rtmp_types.h)
- **功能特性:**
  - 支持多种音视频格式
  - SSL/TLS 加密（RTMPS）
  - RTMP 客户端（推流）
  - RTMP 服务端（接收与分发流）
  - RTMP 源（拉流）
- **典型应用:**
  - YouTube/Twitch 等平台直播
  - 自建流媒体平台
  - IoT 摄像头推流
  - 网络分发

### MRM 协议

Multi-Room Music，实现多设备音频同步播放。
- **说明文档:** [ESP_MRM_Client](docs/zh_CN/MRM_README_CN.md)
- **头文件:** [`esp_mrm_client.h`](include/esp_mrm_client.h)
- **功能特性:**
  - 组播通信
  - 主从架构
  - 动态角色分配
  - 网络时间同步
  - 自动音频同步调整
- **典型应用:**
  - 全屋音响系统
  - 多设备同步播放
  - 商业音频系统
  - 派对应用

### UPnP 协议

Universal Plug and Play，实现媒体共享和设备发现，支持 DLNA 规范。
- **说明文档:** [ESP_UPnP](docs/zh_CN/UPnP_README_CN.md)
- **头文件:** [`esp_ssdp.h`](include/esp_ssdp.h)，[`esp_upnp.h`](include/esp_upnp.h)，[`esp_upnp_service.h`](include/esp_upnp_service.h)，[`esp_upnp_notify.h`](include/esp_upnp_notify.h)
- **功能特性:**
  - 集成 HTTP 服务器
  - 基于 XML 的服务描述
  - 通过 SSDP 设备发现
  - 基于 SOAP 的控制
  - 事件通知
  - 支持媒体服务器/渲染器/控制器
- **典型应用:**
  - 媒体服务器与 NAS
  - 智能电视与流媒体设备
  - 家庭自动化系统
  - 媒体中心应用

## 协议特性对比

| 协议    | 实时性 | 数据流  | 控制流 | 设备发现 | 加密    | 复杂度 |
|--------|------- |-------|-------|---------|--------|--------|
| SIP    | 高     | 支持   | 支持   | 手动    | 支持    | 中     |
| RTSP   | 高     | 支持   | 支持   | 手动    | 不支持  | 中     |
| RTMP   | 中     | 支持   | 基本   | 手动    | 支持    | 中     |
| MRM    | 高     | 支持   | 支持   | 自动    | 不支持  | 低     |
| UPnP   | 低     | 支持   | 支持   | 自动    | 不支持  | 中     |

# 支持的芯片

下表展示了 ESP_MEDIA_PROTOCOLS 对 Espressif SoC 的支持情况。“&#10004;”表示支持，“&#10006;”表示不支持。

|芯片名称     |      v0.5.0      |
|:----------:|:----------------:|
|ESP32       |     &#10004;     |
|ESP32-S2    |     &#10004;     |
|ESP32-S3    |     &#10004;     |
|ESP32-C2    |     &#10004;     |
|ESP32-C3    |     &#10004;     |
|ESP32-C5    |     &#10004;     |
|ESP32-C6    |     &#10004;     |
|ESP32-P4    |     &#10004;     |
