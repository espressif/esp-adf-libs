# SIP 协议
[English](../en/SIP_README.md)

Session Initiation Protocol，基于 IP 网络实现音视频通话。

# 功能特性

- 支持多种音视频格式
- DTMF 支持（RFC2833）
- SSL/TLS 加密
- 支持自定义 SIP 头
- 支持可配置的域名、TCP/TLS 保活和传输超时
- SIP MESSAGE 和 re-INVITE
- 支持自定义 SDP RTP payload type 和 RTP 帧大小
- RFC 2435 JPEG over RTP
- 支持读取原始 SIP 头
- 支持 SDES-SRTP 协商

# 典型应用

- IP 电话与 VoIP 应用
- 视频会议系统
- 可视对讲
- 紧急通信设备

# 支持的格式

| 格式       | ESP_SIP  |
| --------- | --------- |
| PCM       | 否         |
| G711 alaw | 是         |
| G711 ulaw | 是         |
| OPUS      | 是         |
| AAC       | 否         |
| MP3       | 否         |
| H264      | 是         |
| MJPEG     | 是         |

# 支持的方法

| 方法      | ESP_SIP | 备注                |
| -------- | -------- | --------------------- |
| REGISTER | 是        | ESP_SIP 作为服务器不支持 |
| OPTIONS  | 是        | -                     |
| INVITE   | 是        | -                     |
| CANCEL   | 是        | -                     |
| BYE      | 是        | -                     |
| ACK      | 是        | -                     |
| MESSAGE  | 是        | -                     |
| NOTIFY   | 是        | 仅响应                 |

# SDES-SRTP

| 模式 | 行为 |
|------|------|
| `ESP_RTC_SRTP_OFF` | 关闭 SRTP |
| `ESP_RTC_SRTP_PREFER` | 优先使用 SRTP，允许回退到 RTP |
| `ESP_RTC_SRTP_REQUIRED` | 强制使用 SRTP，拒绝明文 RTP |

使用 `esp_rtc_is_srtp_active()` 查询媒体加密协商结果，使用 `esp_rtc_get_reject_reason()` 获取 SRTP 相关拒绝原因。

# 性能

测试使用的系统配置:
|      芯片       | IDF 版本     | CPU 主频       | SPI Ram 主频       |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## 资源使用

| 总内存(Byte)       | Inram 内存(Byte)    | Psram 内存(字节)    | CPU 占用率(%)   |
|        ---        |         ---        |         ---        |       ---      |
|       52440       |         2696       |        49744       |     < 0.99     |

备注:
1) 表中的 CPU 占用率值为平均测量值。
2) 测试结果仅代表协议栈的性能。有关包括媒体流处理在内的数据详细信息，请参阅下面的示例文档。

# 应用示例

- 以下是一个应用示例 [SIP audio](https://github.com/espressif/esp-adf/tree/release/v2.x/examples/protocols/voip)
- 以下是一个应用示例 [SIP audio/video](https://github.com/espressif/esp-adf/tree/release/v2.x/examples/protocols/esp-rtc)

# 常见问题解答

参考 [sip issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20sip)
