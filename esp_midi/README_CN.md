# ESP_MIDI

ESP_MIDI 是乐鑫推出的 MIDI（乐器数字接口）软件库，提供高效的 MIDI 文件解析与实时音频合成功能。

ESP_MIDI 支持 SoundFont 音色库和自定义音频库，可输出高保真、无失真的音频效果。结合 MIDI 文件体积小和音色库资源丰富的特点，ESP_MIDI 实现了卓越音质与高效性能的平衡，为开发者提供完善的 MIDI 处理方案。

## 功能特性

- 完整的标准 MIDI 文件（SMF）格式解析支持
- 实时 MIDI 事件编码和解码，包括音符开关（Note On/Off）、音色切换（Program Change）、控制变化（Control Change）、弯音（Pitch Bend）、通道压力（Channel Pressure）和元事件（速度、拍号等）
- 支持单事件解析，带状态跟踪，支持增量解析
- MIDI 编码：编码 MIDI 文件头、音轨头、速度元事件和 MIDI 事件到数据包
- 支持 BPM（每分钟节拍数）设置和速度变化，支持动态调整
- 使用 SoundFont 样本从 MIDI 事件进行高质量音频合成
- 完整的 SoundFont 2（SF2）文件解析和播放
- 支持用户自定义声音库
- 基于回调的音频输出接口，可与不同音频后端灵活集成
- 播放控制：暂停、恢复和时间分割（每四分音符的 tick 数）配置

## 架构流程

ESP_MIDI 的处理流程如下：

```text
    MIDI 文件 (SMF)
         │
         ▼
    MIDI 解析器 ──► MIDI 事件 ──► SoundFont ──► 合成器 ──► 音频输出
    (midi_parse)    (事件处理)    (sf_parse)    (synth)    (回调接口)
```

## 快速开始

### 添加组件

将本组件添加到您的 ESP-IDF 项目中：

```bash
cd your_project
idf.py add-dependency "espressif/esp_midi"
```

或者，您也可以将本仓库作为组件直接添加到项目的 `components` 目录中。

在项目的 `CMakeLists.txt` 中添加依赖：

   ```cmake
   idf_component_register(
       ...
       REQUIRES esp_midi
   )
   ```

## 使用示例

更多 API 使用详情，请参考 `example` 和 `test_apps/test_case.c` 文件 。

## 支持的芯片

| 芯片      | 支持状态 |
| ---------| -------- |
| ESP32    | &#10004; |
| ESP32-S2 | &#10004; |
| ESP32-S3 | &#10004; |
| ESP32-P4 | &#10004; |
| ESP32-C2 | &#10004; |
| ESP32-C3 | &#10004; |
| ESP32-C5 | &#10004; |
| ESP32-C6 | &#10004; |