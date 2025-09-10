# RTSP 协议
[English](../en/RTSP_README.md)

Real-Time Streaming Protocol，实现音视频流传输控制。

# 功能特性

- 支持多种音视频格式
- UDP/TCP 传输
- 支持服务端和客户端模式
- 支持推流和拉流
- 媒体流管理（DESCRIBE, SETUP, PLAY, TEARDOWN）

# 典型应用

- 网络摄像头与安防系统
- 视频流媒体服务器
- 媒体播放器与录像机
- 远程监控

# 支持的格式

| 格式       | ESP_RTSP  |
| --------- | --------- |
| PCM       | 否         |
| G711 alaw | 是         |
| G711 ulaw | 是         |
| AAC       | 是         |
| MP3       | 否         |
| H264      | 是         |
| MJPEG     | 是         |

# 支持的方法

| 方法      | ESP_RTSP | 备注                    |
| -------- | --------- | ---------------------- |
| OPTIONS  | 是         | -                      |
| DESCRIBE | 是         | -                      |
| SETUP    | 是         | -                      |
| PLAY     | 是         | -                      |
| TEARDOWN | 是         | -                      |
| ANNOUNCE | 是         | ESP_RTSP 作为服务器不支持 |
| RECORD   | 是         | ESP_RTSP 作为服务器不支持 |

# 性能

测试使用的系统配置:
|      芯片       | IDF 版本     | CPU 主频       | SPI Ram 主频       |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## 资源使用

| 总内存(Byte)       | Inram 内存(Byte)    | Psram 内存(字节)    | CPU 占用率(%)   |
|        ---        |         ---        |         ---        |       ---      |
|       10060       |         652        |        9408        |     < 0.01     |

备注:
1) 表中的 CPU 占用率值为平均测量值。
2) 测试结果仅代表协议栈的性能。有关包括媒体流处理在内的数据详细信息，请参阅下面的示例文档。

# 应用示例

以下是一个应用示例 [RTSP server/client](https://github.com/espressif/esp-adf/tree/master/examples/protocols/esp-rtsp)

# 常见问题解答

参考 [rtsp issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20rtsp)
