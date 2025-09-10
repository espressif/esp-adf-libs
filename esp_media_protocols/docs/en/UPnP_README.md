# UPnP Protocol
[中文版](../zh_CN/UPnP_README_CN.md)

Universal Plug and Play with DLNA compliance for seamless media sharing and device discovery.

# Features

- HTTP server integration
- XML-based service descriptions
- Device discovery via SSDP
- SOAP-based control actions
- Event notifications
- Media server/renderer/controller roles

# Use Cases

- Media servers and NAS devices
- Smart TV and streaming devices
- Home automation systems
- Media center applications

# Performance

Tested with the following system configuration:
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## Resource Utilization

| Total Memory(Byte)| Inram Memory(Byte) | Psram Memory(Byte) | CPU loading(%) |
|        ---        |         ---        |         ---        |       ---      |
|       61148       |        15092       |        46056       |     < 0.02     |

Note:
1) The CPU load values in the table represent average measurements.
2) The test result only represents protocol stack. For details on data including media stream processing, please refer to the example docs below.

# Best Practice

Here is an example of using [UPnP device discovery and interoperate](https://github.com/espressif/esp-adf/tree/master/examples/advanced_examples/dlna)

# FAQ

refer to [upnp issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20dlna)
