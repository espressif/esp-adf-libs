# SIP Protocol
[中文版](../zh_CN/SIP_README_CN.md)

Session Initiation Protocol implementation for audio and video communication over IP networks.

# Features

- Multiple audio/video codecs
- DTMF support (RFC2833)
- SSL/TLS encryption
- Custom SIP headers

# Use Cases

- IP phones and VoIP applications
- Video conferencing systems
- Intercom systems
- Emergency communication devices

# Support Formats

|   Format  |  ESP_SIP  |
|    ---    |    ---    |
|    PCM    |     N     |
| G711 alaw |     Y     |
| G711 ulaw |     Y     |
|    OPUS   |     Y     |
|    AAC    |     N     |
|    MP3    |     N     |
|    H264   |     Y     |
|   MJPEG   |     Y     |

# Support Methods

|  Methods  |  ESP_SIP  |              Notes             |
|    ---    |    ---    |               ---              |
|  REGISTER |     Y     | ESP_SIP as a server not support|
|  OPTIONS  |     Y     |                -               |
|   INVITE  |     Y     |                -               |
|   CANCEL  |     Y     |                -               |
|    BYE    |     Y     |                -               |
|    ACK    |     Y     |                -               |

# Performance

Tested with the following system configuration:
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## Resource Utilization

| Total Memory(Byte)| Inram Memory(Byte) | Psram Memory(Byte) | CPU loading(%) |
|        ---        |         ---        |         ---        |       ---      |
|       52440       |         2696       |        49744       |     < 0.99     |

Note:
1) The CPU load values in the table represent average measurements.
2) The test result only represents protocol stack. For details on data including media stream processing, please refer to the example docs below.

# Best Practice

- Here is an example of using [SIP audio](https://github.com/espressif/esp-adf/tree/master/examples/protocols/voip)
- Here is an example of using [SIP audio/video](https://github.com/espressif/esp-adf/tree/master/examples/protocols/esp-rtc)

# FAQ

refer to [sip issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20sip)
