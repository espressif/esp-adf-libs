# 混音器（MIXER）

- [English](./README_MIXER.md)

`MIXER` 模块用于将多路音频信号按各自权重混合为一路音频。每路信号的权重与其音频样本相乘，再对各通道结果求和，得到最终混音输出。

## 特性

- 支持全范围采样率与声道
- 支持位深：s16、s24、s32
- 支持设置两组稳定权重（weight1、weight2）
- 支持通过模式切换在两组权重之间过渡
- 支持数据布局：交织、非交织

## 性能

测试配置：

| 芯片 | IDF 版本 | CPU 频率 | SPI RAM 频率 |
| -- | -- | -- | -- |
| ESP32-S3R8 | v5.3 | 240MHz | 80MHz |

| 位深 | 堆内存（Byte） | CPU 负载（%） |
| -- | -- | -- |
| 16 | 4.5k | < 0.3 |
| 24 | 4.5k | < 0.42 |
| 32 | 4.5k | < 0.46 |

说明：
1) 测试基于两路 10 秒 8 kHz 单声道音频，混音过渡时间设为 10 秒。
2) 其他采样率与声道的 CPU 负载约为 `sample_rate / 8000 × channel` 倍。
3) 内存使用与过渡时间有关，约为 `4 × transition_time(ms)`。

## 使用

示例参见：[MIXER](../example/esp_audio_effects_demo/main/esp_audio_effects_demo.c)

## 常见问题（FAQ）
1) Mixer 的工作原理？
   > Mixer 将多路音频合成为一路输出。需先指定混音路数，并保证每路的采样率、声道和位深一致。随后为每路设置 `weight1`、`weight2` 与 `transmit_time`。初始输出为输入×`weight1`。当将某路模式设为 `ESP_AE_MIXER_MODE_FADE_UPWARD` 时，权重将在响应时间内由 `weight1` 过渡到 `weight2`。

2) 权重与分贝的关系？
   > 权重用于缩放幅度，其与 dB 的换算：
   ><p align="center"> 
                    dB=20log10(weight)
   ></p> 

3) 如何修改每路的权重？
   > 每路有两组权重：weight1、weight2，初始为 weight1。
   > - 切换到 weight2：设置模式 `ESP_AE_MIXER_MODE_FADE_UPWARD`
   > - 切回 weight1：设置模式 `ESP_AE_MIXER_MODE_FADE_DOWNWARD`

4) 使用混音器会失真吗？
   > 可能。当混音结果超过最大幅度时将发生削顶失真。请合理设置权重以避免失真。

5) 混音器的幅频响应曲线？
   > 当前仅支持线性响应。
