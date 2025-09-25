# MRM 协议
[English](../en/MRM_README.md)

Multi-Room Music，实现多设备音频同步播放。

# 功能特性

- 组播通信
- 主从架构
- 动态角色分配
- 网络时间同步
- 自动音频同步调整

# 典型应用

- 全屋音响系统
- 多设备同步播放
- 商业音频系统
- 派对应用

# 性能

测试使用的系统配置:
|      芯片       | IDF 版本     | CPU 主频       | SPI Ram 主频       |
|       ---      |      ---     |      ---      |        ---        |
|  ESP32-S3N16R8 |      v5.3    |     240MHz    |       80MHz       |

## 资源使用

| 总内存(Byte)       | Inram 内存(Byte)    | Psram 内存(字节)    | CPU 占用率(%)   |
|        ---        |         ---        |         ---        |       ---      |
|       11784       |         1020       |        10764       |     < 0.01     |

备注:
1) 表中的 CPU 占用率值为平均测量值。
2) 测试结果仅代表协议栈的性能。有关包括媒体流处理在内的数据详细信息，请参阅下面的示例文档。

# 应用示例

以下是一个应用示例 [Multi-Room Music](https://github.com/espressif/esp-adf/tree/master/examples/advanced_examples/multi-room)

# 常见问题解答

参考 [mrm issue](https://github.com/espressif/esp-adf/issues?q=is%3Aissue%20mrm)
