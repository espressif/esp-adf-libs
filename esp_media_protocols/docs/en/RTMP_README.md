# RTMP Protocol
[中文版](../zh_CN/RTMP_README_CN.md)

Real-Time Messaging Protocol for live streaming to platforms and custom servers.

# Features

- Multiple audio/video codecs
- SSL/TLS encryption(RTMPS)
- RTMP client (push streams)
- RTMP server (receive and distribute streams)
- RTMP source (pull streams)

# Use Cases

- Live streaming to YouTube/Twitch
- Custom streaming platforms
- IoT camera streaming
- Content distribution networks

# Support Formats

|   Format  |  ESP_RTMP |  Standard  |              Notes             |
|    ---    |    ---    |    ---     |               ---              |
|    PCM    |     Y     |     Y      |                -               |
| G711 alaw |     Y     |     Y      |                -               |
| G711 ulaw |     Y     |     Y      |                -               |
|    AAC    |     Y     |     Y      |                -               |
|    MP3    |     Y     |     Y      |                -               |
|    H264   |     Y     |     Y      |                -               |
|   MJPEG   |     Y     |     N      | Transfer MJPEG using CodecID 1 |

# Performance

Tested with the following system configuration:
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## Concurrent Connections

The server mode supports concurrent connections of 6 clients

## Resource Utilization

| Total Memory(Byte)| Inram Memory(Byte) | Psram Memory(Byte) | CPU loading(%) |
|        ---        |         ---        |         ---        |       ---      |
|       16872       |         1286       |        15586       |     < 0.04     |

Note:
1) The CPU load values in the table represent average measurements.
2) The test result only represents protocol stack. For details on data including media stream processing, please refer to the example docs below.

# Best Practice

Here is an example of using [RTMP streaming application](https://github.com/espressif/esp-adf/tree/master/examples/protocols/rtmp)

# FAQ

refer to [rtmp issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20rtmp)
