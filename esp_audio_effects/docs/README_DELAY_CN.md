# Delay（延迟/回声）

- [English](./README_DELAY.md)

`Delay`（延迟）模块用于产生输入信号的一个或多个延迟副本，创建回声效果。本实现采用基于循环缓冲区的简单反馈延迟线，延迟信号以可配置的反馈系数回馈到缓冲区，产生逐渐衰减的重复回声。

## 特性

- 支持全采样率与声道配置
- 支持位深：s16、s24、s32
- 支持数据布局：交织（interleaved）与非交织（non-interleaved）
- 最大延迟时间可配置（影响内存分配），范围 [0, 1000] ms；设为 0 时使用默认值 1000 ms
- 可调参数：
  - `delay_time_ms`：延迟时间，控制回声间隔，范围 [0, max_delay_ms] ms；0 表示旁路（输出等于输入，不施加延迟）
  - `feedback`：反馈系数，控制回声重复次数，范围 [0.0, 0.95]；0.0 表示单次延迟、无重复回声
  - `mix`：干湿比混合系数（等功率交叉淡入淡出），范围 [0.0, 1.0]；0.0 = 全干声，1.0 = 全湿声，0.5 ≈ 平衡（各约 -3 dB）
- 调用 `esp_ae_delay_open()` 前须显式设置所有参数字段，模块不提供内置默认值；可参考 `ESP_AE_DELAY_DEFAULT_*` 宏获取推荐起始值

## 性能

测试环境：<br>
| 芯片 | IDF 版本 | CPU 频率 | SPI RAM 频率 |
| -- | -- | -- | -- |
| ESP32-S3R8 | v5.3 | 240MHz | 80MHz |

16 kHz 单声道（max_delay = 500 ms）：

| 位深 | 堆内存（Byte） | CPU 占用（%） |
| -- | -- | -- |
| 16 | < 32k | < 0.3 |
| 24 | < 32k | < 0.4 |
| 32 | < 32k | < 0.4 |

48 kHz 单声道（max_delay = 500 ms）：

| 位深 | 堆内存（Byte） | CPU 占用（%） |
| -- | -- | -- |
| 16 | < 96k | < 1.1 |
| 24 | < 96k | < 1.4 |
| 32 | < 96k | < 1.2 |

说明：
1) 内存占用主要由延迟线缓冲区决定，与 `max_delay_ms × sample_rate` 成正比，与声道数成正比。双声道内存约为单声道的 2 倍。
2) 不同采样率与声道配置的 CPU 占用可按如下公式估算：
   > CPU Load ≈ (sample_rate / 16000) × channel_count × base_load<br>

   其中 `base_load` 为 16 kHz 单声道表中给出的基准占用。
3) 表格中的 CPU 占用为平均测量值。
4) 减小 `max_delay_ms` 可显著降低内存占用（线性关系）。

## 使用

以下为 Delay 的使用示例：[Delay](../example/esp_audio_effects_demo/main/esp_audio_effects_demo.c)

## 常见问题（FAQ）

1) 延迟内存占用过大怎么办？
   > 内存占用 ≈ max_delay_ms × sample_rate / 1000 × 4（字节）× channel。减小 `max_delay_ms` 或降低采样率可直接减少内存占用。

2) `feedback` 参数如何影响效果？
   > `feedback = 0` 时仅产生单次回声；`feedback` 越大回声重复次数越多、衰减越慢。建议值 0.3-0.5 可获得自然的回声效果，超过 0.7 可能产生明显的振荡感。

3) `delay_time_ms` 和 `max_delay_ms` 的区别？
   > `max_delay_ms` 在创建时确定缓冲区大小（固定分配），`delay_time_ms` 为实际延迟时间，可在运行时动态调整，但不能超过 `max_delay_ms`。将 `delay_time_ms` 设为 0 可旁路延迟处理。

4) `mix` 参数如何控制干湿比？
   > `mix` 通过等功率交叉淡入淡出混合原始信号与延迟信号。`mix = 0.0` 仅输出干声，`mix = 1.0` 仅输出湿声，`mix = 0.5` 时干湿声各约 -3 dB，听感较为平衡。
