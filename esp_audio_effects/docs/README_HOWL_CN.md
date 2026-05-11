# Howl（啸叫抑制）

- [English Version](./README_HOWL.md)

啸叫抑制（Howling Suppression-HS）用于检测并衰减由麦克风、功放与扬声器形成的声学闭环中的声反馈（啸叫）。算法基于 FFT 频谱分析及多种判据，将窄带、持续的啸叫与正常语音或音乐区分开。检测判据包括：PAPR（峰均功率比）、PHPR（峰谐波功率比）、PNPR（峰邻功率比），以及可选的 IMSD（瞬时平均谱偏差）。当某频点超过配置的阈值且处于有效频带内时，即被判定为啸叫频点。算法在这些频点施加动态双二阶陷波滤波器，并可降低整体增益（环路裕度控制）以减小环路增益，抑制新啸叫峰的产生。处理按帧进行，帧长由采样率决定（如每声道 512 或 1024 采样点）。

## 功能特性

- 支持采样率：8000、16000、32000、44100、48000 Hz  
- 支持位深：16、24、32 bit  
- 支持任意声道数，按声道独立处理  
- 可配置检测阈值：  
  - **papr_th**：PAPR 阈值 (dB)，范围 [-10.0, 20.0]  
  - **phpr_th**：PHPR 阈值 (dB)，范围 [0.0, 100.0]  
  - **pnpr_th**：PNPR 阈值 (dB)，范围 [0.0, 100.0]  
  - **imsd_th**：启用 IMSD 时的 IMSD 阈值，范围 [0.0, 20.0]  
  - **do_imsd_check**：为 true 时启用 IMSD 判据，有利于音乐/混合音频场景减少误报（内存与 CPU 更高）；为 false 时关闭 IMSD 以节省内存和 CPU，适用于纯语音场景。  
- 数据布局：支持交织（`esp_ae_howl_process`）与非交织（`esp_ae_howl_deintlv_process`）；支持原地处理。

## 目录结构

| 路径 | 说明 |
|------|------|
| `include/esp_ae_howl.h` | HOWL 配置与 `esp_ae_howl_*` API |
| `include/esp_ae_types.h` | `esp_ae_err_t`、`esp_ae_sample_t` 等 |
| `example/ae_howl/` | 示例工程（见该目录 README） |
| `docs/README_HOWL.md` / `README_HOWL_CN.md` | 模块文档 |

## 快速开始

- **环境**：ESP-IDF 与工程一致即可（性能表基准约 **release/v5.5**、**ESP32-S3R8**）。  
- **示例**：[`example/ae_howl/README_CN.md`](../example/ae_howl/README_CN.md)；参考代码 [`esp_howl_demo.c`](../example/ae_howl/main/esp_howl_demo.c)。  
- **调用顺序**：`esp_ae_howl_open` → `esp_ae_howl_get_frame_size` → 循环 `esp_ae_howl_process`（或 `esp_ae_howl_deintlv_process`）→ `esp_ae_howl_close`。

## 注意事项

- **帧长**：禁止手写字节数，必须以 `esp_ae_howl_get_frame_size` 为准。  
- **IMSD**：`imsd_th` 仅在 **`enable_imsd == true`** 时有效。  
- **阈值**：`papr_th` / `phpr_th` / `pnpr_th` 过大且 `imsd_th` 过小会导致判定过严、漏检增加。  
- **多声道 CPU**：可按下文公式用 `channel_count` 估算。

## 性能

参考条件：**ESP32-S3R8**，IDF **release/v5.5**，CPU **240 MHz**，SPI RAM **80 MHz**。实际随采样率、声道、`enable_imsd` 变化。

### 内存

条件：采样率 **8 / 16 kHz**，单声道  

| 位深 | enable_imsd | 内存 (Byte) |
|------|-------------|-------------|
| 16   | false       | &lt;9K      |
| 16   | true        | &lt;31K     |
| 24   | false       | &lt;10K     |
| 24   | true        | &lt;32K     |
| 32   | false       | &lt;10K     |
| 32   | true        | &lt;32K     |

条件：采样率 **32 / 44.1 / 48 kHz**，单声道  

| 位深 | enable_imsd | 内存 (Byte) |
|------|-------------|-------------|
| 16   | false       | &lt;15K     |
| 16   | true        | &lt;58K     |
| 24   | false       | &lt;17K     |
| 24   | true        | &lt;60K     |
| 32   | false       | &lt;17K     |
| 32   | true        | &lt;69K     |

### CPU

| 位深 | enable_imsd | CPU loading (%) |
|------|-------------|-----------------|
| 16   | false       | &lt;2.1         |
| 16   | true        | &lt;4.2         |
| 24   | false       | &lt;2.1         |
| 24   | true        | &lt;4.3         |
| 32   | false       | &lt;2.1         |
| 32   | true        | &lt;4.2         |

说明：

1. 8 kHz 与 16 kHz 内存档位相同；32 / 44.1 / 48 kHz 档位相同。  
2. CPU 在 **8 kHz、单声道** 测试音频下测得。其它条件估算：  
   `CPU Load ≈ (sample_rate / 8000) × channel_count × base_load`  
   `base_load` 取上表对应项；强啸叫时峰值可能更高。  
3. **`enable_imsd == true`** 时因历史缓冲与 IMSD，内存与 CPU 高于 `false`。

## SoC 兼容性

表格在 **ESP32-S3R8** 上标定；其它芯片可用上式做量级估算，以实测为准。

## 常见问题

1) **啸叫抑制是如何工作的？**

每帧 FFT 后按频点算 PAPR、PHPR、PNPR（及可选 IMSD）；超阈值且落在有效频带的频点判为啸叫，用动态双二阶陷波抑制；活跃陷波数有上限。检测到啸叫时可降整体增益（环路裕度控制）；长期无啸叫则增益与陷波逐步恢复。

2) **帧长如何确定？**

仅由采样率决定块长（如 16 kHz：512 点/声道；48 kHz：1024 点/声道）。须用 **`esp_ae_howl_get_frame_size`** 得到字节数，每次处理**恰好**该长度。

3) **输入与输出能否共用同一 buffer？**

可以。`esp_ae_howl_process` 与 `esp_ae_howl_deintlv_process` 均支持原地处理。

4) **如何设置 `papr_th`、`phpr_th`、`pnpr_th`、`imsd_th`？**

先参考 [`esp_howl_demo.c`](../example/ae_howl/main/esp_howl_demo.c)，再按麦克风/功放/腔体微调。经验：`papr_th`、`phpr_th`、`pnpr_th` **越大**且 `imsd_th` **越小**，判定越严、漏检风险越高。

- **papr_th**：峰相对平均能量的突出程度；越大越偏向更“尖”的峰。  
- **phpr_th**：峰相对谐波（2/3 倍频）能量差；越大越强调非谐波尖峰。  
- **pnpr_th**：峰相对邻近频点能量差；越大越强调窄带突起。  
- **imsd_th**：谱稳定性；**仅 `enable_imsd == true` 时有效**；越小越只保留极稳定疑似啸叫峰。

5) **开启 IMSD 对内存与 CPU 的影响？**

- **内存**：`enable_imsd == false` 为较低档；`true` 明显增加。8/16 kHz 单声道约 **3～3.5×**（如 16 bit：&lt;9K → &lt;31K）；32/44.1/48 kHz 约 **3.5～4×**（如 16 bit：&lt;15K → &lt;58K）。  
- **CPU**：关 IMSD 时 16/24/32 bit 基准约 **&lt;2.1%**；开 IMSD 约 **2×**（16/32 bit **&lt;4.2%**，24 bit **&lt;4.3%**）。  
- **调参**：仅 **`enable_imsd == true`** 时 `imsd_th` 生效；**音乐**或**人声+音乐**建议 **`true`**。**纯语音**可 **`false`** 以降低内存与 CPU；持续单音场景误触发可能略增。
