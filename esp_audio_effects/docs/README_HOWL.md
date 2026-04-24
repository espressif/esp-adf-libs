# Howl (Howling Suppression)

- [中文版本](./README_HOWL_CN.md)

Howling Suppression (HS) is used to detect and attenuate acoustic feedback (howling) in the acoustic loop formed by microphone, amplifier, and speaker. The algorithm uses FFT-based spectrum analysis with multiple criteria to distinguish narrowband, sustained howling from normal speech or music. Detection criteria include PAPR (Peak-to-Average Power Ratio), PHPR (Peak-to-Harmonic Power Ratio), PNPR (Peak-to-Neighboring Power Ratio), and optional IMSD (Instantaneous Mean Spectral Deviation). When a frequency bin exceeds configured thresholds within the valid frequency band, it is identified as a howling bin. The algorithm applies dynamic biquad notch filters to these bins, and can also reduce global gain (loop margin control) to lower loop gain and suppress new howling peaks. Processing is frame-based, and frame length depends on sample rate (for example, 512 or 1024 samples per channel).

## Features

- Supported sample rates: **8000, 16000, 32000, 44100, 48000** Hz  
- Supported bit depth: **16, 24, 32** bit  
- Supported arbitrary channel count; each channel is processed independently  
- Configurable detection thresholds:  
  - **papr_th**: PAPR threshold (dB), range **[-10.0, 20.0]**  
  - **phpr_th**: PHPR threshold (dB), range **[0.0, 100.0]**  
  - **pnpr_th**: PNPR threshold (dB), range **[0.0, 100.0]**  
  - **imsd_th**: IMSD threshold when IMSD is enabled, range **[0.0, 20.0]**  
  - **do_imsd_check**: when **true**, enables the IMSD criterion, which helps reduce false positives in music/mixed-audio scenarios (higher memory and CPU); when **false**, disables IMSD to save memory and CPU for speech-only scenarios  
- Data layout: interleaved (`esp_ae_howl_process`) and deinterleaved (`esp_ae_howl_deintlv_process`); in-place processing supported

## Directory Structure

| Path | Description |
|------|-------------|
| `include/esp_ae_howl.h` | HOWL configuration and `esp_ae_howl_*` APIs |
| `include/esp_ae_types.h` | `esp_ae_err_t`, `esp_ae_sample_t`, and related common types |
| `example/ae_howl/` | Example project (see README in that directory) |
| `docs/README_HOWL.md` / `README_HOWL_CN.md` | Module documentation |

## Quick Start

- **Environment**: Use an ESP-IDF version aligned with your project (performance baseline below is approximately **release/v5.5** on **ESP32-S3R8**).  
- **Example**: [`example/ae_howl/README_CN.md`](../example/ae_howl/README_CN.md); reference code: [`esp_howl_demo.c`](../example/ae_howl/main/esp_howl_demo.c).  
- **Call sequence**: `esp_ae_howl_open` -> `esp_ae_howl_get_frame_size` -> loop `esp_ae_howl_process` (or `esp_ae_howl_deintlv_process`) -> `esp_ae_howl_close`.

## Notes

- **Frame size**: do not hardcode byte length. Always use `esp_ae_howl_get_frame_size`.  
- **IMSD**: `imsd_th` is valid only when **`enable_imsd == true`**.  
- **Thresholds**: overly large `papr_th` / `phpr_th` / `pnpr_th` with overly small `imsd_th` can make detection too strict and increase miss risk.  
- **Multi-channel CPU**: estimate using `channel_count` with the formula below.

## Performance

Reference conditions: **ESP32-S3R8**, IDF **release/v5.5**, CPU **240 MHz**, SPI RAM **80 MHz**. Actual values vary with sample rate, channel count, and `enable_imsd`.

### Memory

Condition: sample rate **8 / 16 kHz**, mono  

| Bit depth | enable_imsd | Memory (Byte) |
|-----------|-------------|---------------|
| 16        | false       | <9K           |
| 16        | true        | <31K          |
| 24        | false       | <10K          |
| 24        | true        | <32K          |
| 32        | false       | <10K          |
| 32        | true        | <32K          |

Condition: sample rate **32 / 44.1 / 48 kHz**, mono  

| Bit depth | enable_imsd | Memory (Byte) |
|-----------|-------------|---------------|
| 16        | false       | <15K          |
| 16        | true        | <58K          |
| 24        | false       | <17K          |
| 24        | true        | <60K          |
| 32        | false       | <17K          |
| 32        | true        | <69K          |

### CPU

| Bit depth | enable_imsd | CPU loading (%) |
|-----------|-------------|-----------------|
| 16        | false       | <2.1            |
| 16        | true        | <4.2            |
| 24        | false       | <2.1            |
| 24        | true        | <4.3            |
| 32        | false       | <2.1            |
| 32        | true        | <4.2            |

Notes:

1. Memory tiers are the same for 8 kHz and 16 kHz; also the same for 32 / 44.1 / 48 kHz.  
2. CPU values were measured with **8 kHz mono** test audio. For other conditions, estimate with:  
   `CPU Load ≈ (sample_rate / 8000) × channel_count × base_load`  
   where `base_load` is taken from the table above; peaks can be higher under strong howling.  
3. When **`enable_imsd == true`**, history buffers and IMSD computation increase both memory and CPU compared to `false`.

## SoC Compatibility

The table and metrics are calibrated on **ESP32-S3R8**. For other SoCs, use the formula above for order-of-magnitude estimation and validate with real measurements.

## FAQ

1) **How does howling suppression work?**

After per-frame FFT, the algorithm computes PAPR, PHPR, PNPR (and optional IMSD) per frequency bin. Bins that exceed thresholds in the valid band are identified as howling bins and suppressed by dynamic biquad notch filters. The number of active notches is limited. When howling is detected, global gain can be reduced (loop margin control); after a sustained no-howl period, gain and notches are gradually restored.

2) **How is frame size determined?**

Block length is determined only by sample rate (for example, 512 samples/channel at 16 kHz; 1024 samples/channel at 48 kHz). Use **`esp_ae_howl_get_frame_size`** to get the exact byte length, and feed **exactly** that size per process call.

3) **Can input and output share the same buffer?**

Yes. Both `esp_ae_howl_process` and `esp_ae_howl_deintlv_process` support in-place processing.

4) **How should I set `papr_th`, `phpr_th`, `pnpr_th`, and `imsd_th`?**

Start from [`esp_howl_demo.c`](../example/ae_howl/main/esp_howl_demo.c), then tune for microphone/amplifier/enclosure characteristics. Rule of thumb: larger `papr_th` / `phpr_th` / `pnpr_th` and smaller `imsd_th` make detection stricter and increase miss risk.

- **papr_th**: prominence of peak vs average energy; larger values favor sharper peaks.  
- **phpr_th**: energy difference between peak and harmonics (2x/3x); larger values emphasize non-harmonic sharp peaks.  
- **pnpr_th**: energy difference between peak and neighboring bins; larger values emphasize narrowband protrusions.  
- **imsd_th**: spectral stability threshold; **valid only when `enable_imsd == true`**; smaller values retain only very stable suspected howling peaks.

5) **What is the impact of enabling IMSD on memory and CPU?**

- **Memory**: `enable_imsd == false` stays in lower tiers; `true` increases usage significantly. At 8/16 kHz mono, roughly **3-3.5x** (for example, 16-bit: <9K -> <31K). At 32/44.1/48 kHz, roughly **3.5-4x** (for example, 16-bit: <15K -> <58K).  
- **CPU**: with IMSD off, 16/24/32-bit baselines are about **<2.1%**; with IMSD on, about **2x** (16/32-bit **<4.2%**, 24-bit **<4.3%**).  
- **Tuning**: `imsd_th` is effective only when **`enable_imsd == true`**; **music** or **voice+music** usually benefits from `true`. For **speech-only**, `false` can reduce memory and CPU, though false triggers on sustained tones may increase.
