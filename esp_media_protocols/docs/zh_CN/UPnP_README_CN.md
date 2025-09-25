# UPnP 协议
[English](../en/UPnP_README.md)

Universal Plug and Play，实现媒体共享和设备发现，支持 DLNA 规范。

# 功能特性

- 集成 HTTP 服务器
- 基于 XML 的服务描述
- 通过 SSDP 设备发现
- 基于 SOAP 的控制
- 事件通知
- 支持媒体服务器/渲染器/控制器

# 典型应用
- 媒体服务器与 NAS
- 智能电视与流媒体设备
- 家庭自动化系统
- 媒体中心应用

# 性能

测试使用的系统配置:
|      芯片       | IDF 版本     | CPU 主频       | SPI Ram 主频       |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## 资源使用

| 总内存(Byte)       | Inram 内存(Byte)    | Psram 内存(字节)    | CPU 占用率(%)   |
|        ---        |         ---        |         ---        |       ---      |
|       61148       |        15092       |        46056       |     < 0.02     |

备注:
1) 表中的 CPU 占用率值为平均测量值。
2) 测试结果仅代表协议栈的性能。有关包括媒体流处理在内的数据详细信息，请参阅下面的示例文档。

# 应用示例

以下是一个应用示例 [UPnP device discovery and interoperate](https://github.com/espressif/esp-adf/tree/master/examples/advanced_examples/dlna)

# 常见问题解答

参考 [upnp issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20dlna)
