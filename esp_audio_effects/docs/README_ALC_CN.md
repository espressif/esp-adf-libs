# ALC（自动电平控制）

- [English](./README_ALC.md)

`ALC`（Automatic Level Control，自动电平控制）模块用于自动调节音频信号的电平，在不同输入电平下确保输出音量稳定。其主要目的在于防止音量的突增或突降，从而提供更一致的听感体验。

## 特性

- 支持全采样率与声道配置
- 支持位深：s16、s24、s32
- 支持增益范围（-∞, 63] dB；当增益设置小于 -64 dB 时，音频将被静音
- 支持数据布局：交织（interleaved）与非交织（non-interleaved）

## 性能

测试环境：<br>
| 芯片 | IDF 版本 | CPU 频率 | SPI RAM 频率 |
| -- | -- | -- | -- |
| ESP32-S3R8 | v5.3 | 240MHz | 80MHz |

| 位深 | 堆内存（Byte） | CPU 占用（%） |
| -- | -- | -- |
| 16 | < 5k | < 0.3 |
| 24 | < 5k | < 0.4 |
| 32 | < 5k | < 0.4 |

说明：
1) 测试音频为 8 kHz 单声道。不同采样率与声道配置的 CPU 占用可按如下公式估算：
   > CPU Load = (sample_rate / 8000) × channel_count × base_load<br>

   其中 `base_load` 为表中给出的基准占用。
2) 表格中的 CPU 占用为平均测量值。

## 使用

以下为 ALC 的使用示例：[ALC](../example/esp_audio_effects_demo/main/esp_audio_effects_demo.c)

## 常见问题（FAQ）

1) 增益设置过高会导致失真吗？
   > 不会。当增益设置过高时，ALC 会在其动态范围内对信号进行限制，避免被放大至超过允许的最大幅度，从而防止削顶失真（clipping）。
