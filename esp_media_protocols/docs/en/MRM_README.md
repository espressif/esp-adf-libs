# MRM Protocol
[中文版](../zh_CN/MRM_README_CN.md)

Multi-Room Music Protocol for whole-home multi-device synchronized audio playback system.

# Features

- Multicast communication
- Master-slave architecture
- Dynamic role assignment
- Network time synchronization
- Automatic audio sync adjustment

# Use Cases

- Whole-home audio systems
- Synchronized speaker groups
- Commercial audio installations
- Party mode applications

# Performance

Tested with the following system configuration:
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## Resource Utilization

| Total Memory(Byte)| Inram Memory(Byte) | Psram Memory(Byte) | CPU loading(%) |
|        ---        |         ---        |         ---        |       ---      |
|       11784       |         1020       |        10764       |     < 0.01     |

Note:
1) The CPU load values in the table represent average measurements.
2) The test result only represents protocol stack. For details on data including media stream processing, please refer to the example docs below.

# Best Practice

Here is an example of using [Multi-Room Music](https://github.com/espressif/esp-adf/tree/master/examples/advanced_examples/multi-room)

# FAQ

refer to [mrm issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20mrm)
