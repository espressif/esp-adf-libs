# ESP 音频特效演示

- [English Version](./README.md)

## 概述

本示例演示了 ESP Audio Effects 库提供的各种音频特效和处理功能。展示了如何使用不同的音频特效模块，包括 ALC、Fade、EQ、Sonic、DRC、MBC、Mixer 和 Basic Audio Info Convert。

该演示处理嵌入式音频文件，并通过音频编解码器播放，同时应用实时音频特效。

## 支持的演示

本示例支持以下音频特效演示：

1. **ALC (自动电平控制)** - 控制音量的增加和减少
2. **Fade (淡入淡出)** - 淡入淡出特效
3. **EQ (均衡器)** - 增强人声效果演示
4. **Sonic (变速变调)** - 速度和音调调整
5. **DRC (动态范围压缩)** - 两个子演示：
   - 声音平衡演示
   - 去除冲击演示
6. **MBC (多频段压缩器)** - 增强人声效果演示
7. **Mixer (混音器)** - 混合多个音频流并控制淡入淡出
8. **Basic Audio Info Convert (基础音频信息转换)** - 转换音频格式（采样率、声道、位深）

## 前置条件

- ESP-IDF release/v5.3 或更高版本
- 支持的开发板（如 ESP32-S3-Korvo2-V3）

## 编译和烧录

编译本例程前需要先确保已配置 ESP-IDF 的环境，如果已配置可跳到下一项配置，如果未配置需要先在 ESP-IDF 根目录运行下面脚本设置编译环境，有关配置和使用 ESP-IDF 完整步骤，请参阅 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/index.html)

```
./install.sh
. ./export.sh
```

下面是简略编译步骤：

1. **进入工程目录**
   ```bash
   cd $YOUR_ESP_AUDIO_EFFECTS_PATH/example/esp_audio_effects_demo
   ```

2. **搭建基础环境**

   根据需要设置的目标芯片和开发板，修改 `install.sh` 文件
   ```bash
   sh ./install.sh
   ```

3. **配置示例**

   ```bash
   idf.py menuconfig
   ```
   导航到 `Audio Effects Example` 菜单：
   - 选择要运行的音频特效演示
   - 对于 DRC 演示，可以进一步选择：
   - 声音平衡演示（默认）
   - 去除冲击演示

4. **编译工程**

   ```bash
   idf.py build
   ```

5. **烧录与监控**（将 `PORT` 替换为实际串口号）
   ```bash
   idf.py -p PORT flash monitor
   ```

6. 使用 `Ctrl-]` 退出串口监控界面。

## 演示详情

### ALC 演示

**功能**：控制音量的增加和减少。

**行为**：
- 使用 ALC 处理播放音频
- 在 4 秒时，将 ALC 增益设置为 +6dB 以演示增益调整

**音频文件**：`voice_with_music.pcm` (16kHz, 1ch, 16bit)

### Fade 演示

**功能**：对音频应用淡入淡出效果。

**行为**：
- 开始时应用淡入效果
- 在 4 秒时，切换到淡出效果

**音频文件**：`voice_with_music.pcm` (16kHz, 1ch, 16bit)

### EQ 演示

**功能**：对音频应用多频段均衡。

**配置**：
- 4 个滤波器：高通（120Hz）、峰值（350Hz、2200Hz、4500Hz）
- 所有滤波器初始禁用
- 在 4 秒时，启用所有滤波器

**音频文件**：`voice_with_music.pcm` (16kHz, 1ch, 16bit)

### Sonic 演示

**功能**：独立调整播放速度和音调。

**行为**：
- 以正常速度和音调播放音频
- 在 4 秒时，将速度设置为 1.2 倍，音调设置为 0.8 倍

**音频文件**：`voice_with_music.pcm` (16kHz, 1ch, 16bit)

### DRC 演示

**功能**：对音频应用动态范围控制。

#### 声音平衡演示

**配置**：
- 压缩曲线：{0.0, -25.0}, {-50.0, -35.0}, {-100.0, -100.0}
- 启动时间：3ms
- 释放时间：50ms

**音频文件**：`voice_flick_up_and_down.pcm` (16kHz, 1ch, 16bit)

#### 去除冲击演示

**配置**：
- 压缩曲线：{0.0, -20.0}, {-20.0, -20.0}, {-100.0, -100.0}
- 启动时间：1ms
- 释放时间：200ms

**音频文件**：`voice_with_hit.pcm` (16kHz, 1ch, 16bit)

**行为**：
- 首先播放原始音频
- 然后播放 DRC 处理后的音频以演示效果

### MBC 演示

**功能**：应用 4 个频段的多频段压缩。

**配置**：
- 频段：400Hz、2100Hz、6000Hz
- 所有频段初始旁路
- 在 4 秒时，启用所有频段

**音频文件**：`voice_with_music.pcm` (16kHz, 1ch, 16bit)

### Mixer 演示

**功能**：混合两个音频流并控制淡入淡出。

**行为**：
- 流 1：背景音乐（`voice_with_music.pcm`）
- 流 2：提示音信号（`tone.pcm`）- 通过队列延迟 4 秒后启动
- 对两个流应用淡入淡出效果
- 演示基于队列的音频流传输

**音频文件**：
- `voice_with_music.pcm` (16kHz, 1ch, 16bit)
- `tone.pcm` (16kHz, 1ch, 16bit)

### Basic Audio Info Convert 演示

**功能**：使用位转换、声道转换和采样率转换来转换音频格式。

**转换流程**：
1. **声道转换**：2 声道 → 1 声道（24bit）
2. **位转换**：24bit → 16bit（1 声道）
3. **采样率转换**：48kHz → 16kHz（1 声道，16bit）

**输入格式**：48kHz，2 声道，24bit
**输出格式**：16kHz，1 声道，16bit

**音频文件**：`manen_48000_2_24_10.pcm` (48kHz, 2ch, 24bit)

**配置**：
- 声道转换：使用默认权重（平均）
- 位转换：24bit 有符号 → 16bit 有符号
- 采样率转换：复杂度 3（最高质量），内存优化模式

## 音频文件

演示使用编译到固件中的嵌入式音频文件：

- `voice_with_music.pcm` - 默认音频文件（16kHz, 1ch, 16bit）
- `voice_flick_up_and_down.pcm` - 用于 DRC 声音平衡演示
- `voice_with_hit.pcm` - 用于 DRC 去除冲击演示
- `tone.pcm` - 用于 Mixer 演示
- `manen_48000_2_24_10.pcm` - 用于 Basic Audio Info Convert 演示（48kHz, 2ch, 24bit）

这些文件会根据所选演示配置自动嵌入。

## 代码结构

```
main/
├── esp_audio_effects_demo.c    # 主演示代码
├── CMakeLists.txt              # 构建配置
├── Kconfig.projbuild           # Kconfig 选项
└── *.pcm                       # 嵌入式音频文件
```

## 配置选项

### 主选择（Choice）

- `ESP_AUDIO_EFFECTS_DEMO_SELECT_ALC` - ALC 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_FADE` - Fade 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_EQ` - EQ 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_SONIC` - Sonic 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_DRC` - DRC 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_MBC` - MBC 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER` - Mixer 演示
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT` - Basic Audio Info Convert 演示

### DRC 子选择（Choice，依赖于 DRC）

- `ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO` - 声音平衡演示（默认）
- `ESP_AUDIO_EFFECTS_DEMO_REMOVE_HIT_DEMO` - 去除冲击演示
