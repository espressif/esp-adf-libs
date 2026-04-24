# ESP 啸叫抑制（卡拉 OK）示例

- [English Version](./README.md)
- 例程难度：⭐⭐

## 例程简介

本示例展示了一个带啸叫抑制的卡拉 OK 处理流程：采集麦克风输入，经可选的 `esp_ae_howl` 啸叫抑制后，与 SD 卡中的背景音乐进行混音，最后通过编解码器播放。

- 示例采用单路实时处理：麦克风采集 -> 可选啸叫抑制 -> 混音 -> DAC 输出。
- 演示了在“麦克风 + 扬声器”声学闭环下，如何抑制持续窄带啸叫。
- 支持通过按键控制啸叫抑制：**单击 PLAY** 开启，**单击 SET** 关闭，便于实时对比效果。

### 信号流程

```
麦克风 (ADC) -> [可选 啸叫抑制] -> 混音器 <- 音乐 (SD: /sdcard/test_dukou.pcm)
                    ↓                              ↓
               audio_buffer                  music_buffer
                    ↓                              ↓
                    └────────── 混音器 ────────────┘
                                    ↓
                            out_buffer -> 编解码器 (DAC) -> 扬声器
```

## 环境配置

### 硬件要求

- **开发板**：支持麦克风（ADC）、扬声器（DAC）与 SD 卡的开发板（如 ESP32-S3-Korvo2/Korvo2 V3）。
- **资源要求**：SD 卡、麦克风输入、扬声器输出。

### 默认 IDF 分支

本例程支持 ESP-IDF `release/v5.4` 及以上版本。

### 软件要求

- SD 卡根目录放置 PCM 文件：`/sdcard/test_dukou.pcm`
- 文件格式：16 kHz，1 声道，16 bit

## 编译和下载

### 编译准备

编译本例程前需先确保已配置 ESP-IDF 环境；若未配置，请先在 ESP-IDF 根目录执行环境安装与导出。完整步骤请参考 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/index.html)。

```bash
./install.sh
. ./export.sh
```

下面是简略步骤：

- 进入本例程工程目录：

```bash
cd $YOUR_ESP_AUDIO_EFFECTS_PATH/example/ae_howl
```

- 在 Linux 中安装 `esp-bmgr-assist`：

```bash
pip install esp-bmgr-assist
pip install --upgrade esp-bmgr-assist
```

- 查看支持的板子：

```bash
idf.py bmgr -l
```

输出示例：

```text
ℹ️  Main Boards:
  [1] dual_eyes_board_v1_0
  [2] esp32_c3_lyra
  [3] esp32_c5_spot
  [4] esp32_p4_function_ev
  [5] esp32_s3_korvo2_v3
  [6] esp32_s3_korvo2l
  [7] esp_box_3
  [8] esp_box_lite
  [9] esp_hi
```

- 选择开发板（将 `number` 替换为上面列表中的编号）：

```bash
idf.py bmgr -b number
```

### 项目配置

本例程主要音频参数与啸叫阈值在代码中配置，如需调整请参考 `main/esp_howl_demo.c` 中 `demo_howl_process()` 的配置项。

### 编译与烧录

- 编译示例程序：

```bash
idf.py build
```

- 烧录并查看串口日志（将 `PORT` 替换为实际串口）：

```bash
idf.py -p PORT flash monitor
```

- 退出监视器可使用 `Ctrl-]`。

## 如何使用例程

### 功能和用法

- 上电后默认关闭啸叫抑制，系统会读取麦克风与 SD 卡音乐并进行混音播放。
- **单击 PLAY** 打开 `esp_ae_howl` 啸叫抑制，**单击 SET** 关闭啸叫抑制，实时比较处理效果。
- 建议先确认 SD 卡中存在 `/sdcard/test_dukou.pcm`，再上电运行。

### 啸叫抑制参数

默认配置（见 `esp_howl_demo.c`）如下：

| 参数            | 取值 | 说明 |
|-----------------|------|------|
| sample_rate     | 16000 | 采样率 (Hz) |
| channel         | 1 | 单声道 |
| bits_per_sample | 16 | 位深 |
| papr_th         | 8.0 | PAPR 阈值 (dB)，范围 [-10, 20] |
| phpr_th         | 45.0 | PHPR 阈值 (dB)，范围 [0, 100] |
| pnpr_th         | 45.0 | PNPR 阈值 (dB)，范围 [0, 100] |
| imsd_th         | 5.0  | IMSD 阈值（do_imsd_check 为 true 时有效），范围 [0, 20] |
| do_imsd_check   | true | 开启 IMSD 可减少音乐场景误报；纯语音可设为 false 以节省内存和 CPU |

## 代码结构

```text
main/
├── esp_howl_demo.c    # 主逻辑：编解码器初始化、啸叫+混音流水线、按键处理
├── CMakeLists.txt     # main 组件构建配置
└── idf_component.yml  # ESP-IDF 组件依赖（可选）
```

- **按键处理**：`simple_button_event_handler` 中，单击 PLAY 开启 `do_howl_process`，单击 SET 关闭 `do_howl_process`。
- **主循环处理**：读麦克风 -> 可选 `esp_ae_howl_process` -> 读音乐 -> `esp_ae_mixer_process` -> 写入 DAC。

## 故障排除

### 无声音或播放异常

- 检查编解码器与板级音频外设是否初始化成功。
- 确认 SD 卡已正确挂载，且 `test_dukou.pcm` 存在于根目录并满足 16 kHz/单声道/16 bit。

### 啸叫仍明显

- 适当增大麦克风与扬声器距离，或降低扬声器音量。
- 确认已通过按键开启啸叫抑制。
- 根据实际声学环境微调 `papr_th`、`phpr_th`、`pnpr_th`、`imsd_th`。

### 按键无反应

- 确认 ADC 按键组已初始化并注册回调。
- 参考 `init_adc_button()` 与板级管理相关文档检查硬件映射。

## 技术支持

请通过以下渠道获取支持：

- 技术讨论： [esp32.com](https://esp32.com/viewforum.php?f=20)
- 问题反馈： [GitHub issue](https://github.com/espressif/esp-adf/issues)
