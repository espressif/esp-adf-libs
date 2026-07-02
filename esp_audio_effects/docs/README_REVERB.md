# Reverb

- [中文版](./README_REVERB_CN.md)

`Reverb` is an audio effect that simulates the natural reflections of sound in an enclosed space. This implementation uses the Freeverb (Schroeder-Moorer) algorithm, which consists of 8 parallel comb filters with low-pass damping feedback followed by 4 series allpass filters for diffusion. The algorithm is lightweight and suitable for ESP32 embedded platforms.

# Features

- Support full range of sample rates and channel configurations
- Support bits per sample: s16, s24, s32
- Support data layout: interleaved, non-interleaved
- Adjustable parameters:
  - `room_size`: Room size factor controlling comb-filter feedback gain and reverb tail length, range [0.0, 1.0]
  - `damping`: High-frequency damping factor controlling high-frequency absorption rate in the feedback loop, range [0.0, 1.0]; higher values cause faster high-frequency decay
  - `wet_level`: Wet (reverb) signal level, range [-96.0, 0.0] dB
  - `dry_level`: Dry (original) signal level, range [-96.0, 0.0] dB
  - `pre_delay_ms`: Pre-delay time before reverb onset, adding initial delay before the reverb starts, range [0, 200] ms (configured at creation time only)

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |     --       |  --           |     --            |
|   ESP32-S3R8   |    v5.3      | 240MHz        |   80MHz           |

16 kHz mono:

| Bits per sample | Heap Memory (Byte) | CPU loading (%) |
|       --        |         --           |       --        |
|       16        |       < 20k          |      < 2.3      |
|       24        |       < 20k          |      < 2.4      |
|       32        |       < 20k          |      < 2.3      |

48 kHz mono:

| Bits per sample | Heap Memory (Byte) | CPU loading (%) |
|       --        |         --           |       --        |
|       16        |       < 59k          |      < 6.8      |
|       24        |       < 59k          |      < 7.0      |
|       32        |       < 59k          |      < 6.9      |

Note:
1) Memory usage is mainly determined by the delay lines of the comb and allpass filters and is proportional to the sample rate and channel count. Dual-channel memory is approximately twice that of mono.
2) CPU load for other sampling rates and channel configurations can be estimated using the formula:
   > CPU Load ≈ (sample_rate / 16000) × channel_count × base_load<br>

   where `base_load` is the CPU load listed in the 16 kHz mono table above.
3) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [Reverb](../example/esp_audio_effects_demo/main/esp_audio_effects_demo.c)

# FAQ

1) What if reverb memory usage is too high?
   > Memory is mainly consumed by delay lines and is proportional to the sample rate. Lowering the sample rate (e.g., from 48 kHz to 16 kHz) significantly reduces memory usage (by approximately 3×).

2) How should `room_size` and `damping` be used together?
   > A larger `room_size` produces a longer reverb tail; a larger `damping` causes faster high-frequency decay. Large `room_size` + low `damping` simulates a hall; small `room_size` + high `damping` simulates a small room.

3) What is the purpose of `pre_delay_ms`?
   > Pre-delay simulates the distance from the sound source to reflective surfaces, adding a sense of space. Values of 10–40 ms generally produce natural spatial effects. This parameter can only be set via the configuration structure when calling `esp_ae_reverb_open()` and cannot be changed at runtime.
