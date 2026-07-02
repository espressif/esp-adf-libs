# Delay

- [中文版](./README_DELAY_CN.md)

`Delay` is an audio effect that produces one or more delayed copies of the input signal, creating an echo effect. This implementation uses a simple feedback delay line based on a circular buffer. The delayed signal is fed back into the buffer with a configurable feedback coefficient, producing repeated echoes that gradually decay.

# Features

- Support full range of sample rates and channel configurations
- Support bits per sample: s16, s24, s32
- Support data layout: interleaved, non-interleaved
- Configurable maximum delay time (affects memory allocation), range [0, 1000] ms; 0 selects the default of 1000 ms
- Adjustable parameters:
  - `delay_time_ms`: Delay time controlling the echo interval, range [0, max_delay_ms] ms; 0 = bypass (output equals input, no delay applied)
  - `feedback`: Feedback coefficient controlling echo repetitions, range [0.0, 0.95]; 0.0 = single delay with no echo repetition
  - `mix`: Wet/dry mix ratio via equal-power crossfade, range [0.0, 1.0]; 0.0 = dry only, 1.0 = wet only, 0.5 ≈ balanced (both ≈ -3 dB)
- All parameter fields must be set explicitly before calling `esp_ae_delay_open()`; the module does not apply built-in defaults. See the `ESP_AE_DELAY_DEFAULT_*` macros for recommended starting values

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |     --       |  --           |     --            |
|   ESP32-S3R8   |    v5.3      | 240MHz        |   80MHz           |

16 kHz mono (max_delay = 500 ms):

| Bits per sample | Heap Memory (Byte) | CPU loading (%) |
|       --        |         --           |       --        |
|       16        |       < 32k          |      < 0.3      |
|       24        |       < 32k          |      < 0.4      |
|       32        |       < 32k          |      < 0.4      |

48 kHz mono (max_delay = 500 ms):

| Bits per sample | Heap Memory (Byte) | CPU loading (%) |
|       --        |         --           |       --        |
|       16        |       < 96k          |      < 1.1      |
|       24        |       < 96k          |      < 1.4      |
|       32        |       < 96k          |      < 1.2      |

Note:
1) Memory usage is mainly determined by the delay-line buffer and is proportional to `max_delay_ms × sample_rate` and to the channel count. Dual-channel memory is approximately twice that of mono.
2) CPU load for other sampling rates and channel configurations can be estimated using the formula:
   > CPU Load ≈ (sample_rate / 16000) × channel_count × base_load<br>

   where `base_load` is the CPU load listed in the 16 kHz mono table above.
3) The CPU load values in the table represent average measurements.
4) Reducing `max_delay_ms` significantly lowers memory usage (linear relationship).

# Usage

Here is an example of using [Delay](../example/esp_audio_effects_demo/main/esp_audio_effects_demo.c)

# FAQ

1) What if delay memory usage is too high?
   > Memory usage ≈ max_delay_ms × sample_rate / 1000 × 4 (bytes) × channel. Reducing `max_delay_ms` or lowering the sample rate directly reduces memory usage.

2) How does the `feedback` parameter affect the effect?
   > When `feedback = 0`, only a single echo is produced; higher `feedback` values produce more repeated echoes that decay more slowly. Recommended values of 0.3–0.5 yield natural echo effects; values above 0.7 may cause noticeable oscillation.

3) What is the difference between `delay_time_ms` and `max_delay_ms`?
   > `max_delay_ms` is set at creation time and determines the buffer size (fixed allocation). `delay_time_ms` is the actual delay time and can be adjusted at runtime, but must not exceed `max_delay_ms`. Setting `delay_time_ms` to 0 bypasses delay processing.

4) How does the `mix` parameter control the wet/dry balance?
   > `mix` blends the original and delayed signals via equal-power crossfade. `mix = 0.0` outputs dry signal only, `mix = 1.0` outputs wet signal only, and `mix = 0.5` gives both at approximately -3 dB for a balanced blend.
