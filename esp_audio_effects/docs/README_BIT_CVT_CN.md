# 位深转换（BIT CONVERSION）

- [English](./README_BIT_CVT.md)

`BIT CONVERSION` 模块用于将原始音频的每采样位数转换为目标每采样位数。

## 特性

- 支持全范围采样率与声道数
- 支持位深：u8、s16、s24、s32
- 支持数据布局：交织（interleaved）、非交织（non-interleaved）

## 性能

测试配置：

| 芯片 | IDF 版本 | CPU 频率 | SPI RAM 频率 |
| -- | -- | -- | -- |
| ESP32-S3R8 | v5.3 | 240MHz | 80MHz |

| 堆内存（Byte） | CPU 负载（%） |
| -- | -- |
| 20 | < 0.06 |

说明：
1) 测试音频为 8 kHz 单声道。不同采样率和声道配置下的 CPU 负载可按如下公式估算：
   > CPU Load = (sample_rate / 8000) × channel_count × base_load
   其中 `base_load` 为表格所列基准负载。
2) 表中 CPU 负载为平均值。

## 使用

使用示例参见：[BIT CONVERSION](../example/esp_audio_effects_demo/main/esp_audio_effects_demo.c)

## 常见问题（FAQ）
1) 为什么支持无符号 8 位而不是有符号 8 位？
   > 因为 8 位 PCM 音频数据是无符号的。
