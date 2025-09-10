# RTMP 协议
[English](../en/RTMP_README.md)

Real-Time Messaging Protocol，实现与平台或自建服务器的直播流应用。

# 功能特性

- 支持多种音视频格式
- SSL/TLS 加密（RTMPS）
- RTMP 客户端（推流）
- RTMP 服务端（接收与分发流）
- RTMP 源（拉流）

# 典型应用

- YouTube/Twitch 等平台直播
- 自建流媒体平台
- IoT 摄像头推流
- 网络分发

# 支持的格式

| 格式       | ESP_RTMP  | 标准 | 备注                    |
| --------- | --------- | --- | ----------------------- |
| PCM       | 是         | 是  | -                       |
| G711 alaw | 是         | 是  | -                       |
| G711 ulaw | 是         | 是  | -                       |
| AAC       | 是         | 是  | -                       |
| MP3       | 是         | 是  | -                       |
| H264      | 是         | 是  | -                       |
| MJPEG     | 是         | 否  | 使用 CodecID 1 传输 MJPEG |

# 性能

测试使用的系统配置:
|      芯片       | IDF 版本     | CPU 主频       | SPI Ram 主频       |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## 并发连接

服务器模式支持6个客户端的并发连接

## 资源使用

| 总内存(Byte)       | Inram 内存(Byte)    | Psram 内存(字节)    | CPU 占用率(%)   |
|        ---        |         ---        |         ---        |       ---      |
|       16872       |         1286       |        15586       |     < 0.04     |

备注:
1) 表中的 CPU 占用率值为平均测量值。
2) 测试结果仅代表协议栈的性能。有关包括媒体流处理在内的数据详细信息，请参阅下面的示例文档。

# 应用示例

以下是一个应用示例 [RTMP streaming application](https://github.com/espressif/esp-adf/tree/master/examples/protocols/rtmp)

# 常见问题解答

参考 [rtmp issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20rtmp)
