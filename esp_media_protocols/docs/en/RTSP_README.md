# RTSP Protocol
[中文版](../zh_CN/RTSP_README_CN.md)

Real-Time Streaming Protocol for audio and video streaming applications.

# Features

- Multiple audio/video codecs
- UDP/TCP transport
- Server and client modes
- Push and pull operations
- Stream management (DESCRIBE, SETUP, PLAY, TEARDOWN)

# Use Cases

- IP cameras and surveillance systems
- Video streaming servers
- Media players and recorders
- Remote monitoring applications

# Support Formats

|   Format  |  ESP_RTSP |
|    ---    |    ---    |
|    PCM    |     N     |
| G711 alaw |     Y     |
| G711 ulaw |     Y     |
|    AAC    |     Y     |
|    MP3    |     N     |
|    H264   |     Y     |
|   MJPEG   |     Y     |

# Support Methods

|  Methods  |  ESP_RTSP |              Notes             |
|    ---    |    ---    |               ---              |
|  OPTIONS  |     Y     |                -               |
|  DESCRIBE |     Y     |                -               |
|   SETUP   |     Y     |                -               |
|   PLAY    |     Y     |                -               |
|  TEARDOWN |     Y     |                -               |
|  ANNOUNCE |     Y     |ESP_RTSP as a server not support|
|   RECORD  |     Y     |ESP_RTSP as a server not support|

# Performance

Tested with the following system configuration:
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## Resource Utilization

| Total Memory(Byte)| Inram Memory(Byte) | Psram Memory(Byte) | CPU loading(%) |
|        ---        |         ---        |         ---        |       ---      |
|       10060       |         652        |         9408       |     < 0.01     |

Note:
1) The CPU load values in the table represent average measurements.
2) The test result only represents protocol stack. For details on data including media stream processing, please refer to the example docs below.

# Best Practice

Here is an example of using [RTSP server/client](https://github.com/espressif/esp-adf/tree/master/examples/protocols/esp-rtsp)

# FAQ

refer to [rtsp issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20rtsp)
