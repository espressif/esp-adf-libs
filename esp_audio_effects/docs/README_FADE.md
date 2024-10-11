# FADE

`FADE` module is used to gradually introduce or remove sound, creating a smooth transition effect by increasing or decreasing the volume.

# Features

- Support full range of sample rates and channel
- Support bits per sample: s16, s24, s32
- Support mode: fade in, fade out
- Support curve: line, quadratic, sqrt
- Support data layout: interleaved, non-interleaved

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |      --      |  --           |     --            |
|   ESP32-S3R8   |     v5.3     | 240MHz        |   80MHz           |

| Bits per sample| Heap Memory(Byte) | CPU loading(%)|
|  --            |  --               |     --        |
|  16            |  4.5k             |   < 0.2       |
|  24            |  4.5k             |   < 0.2       |
|  32            |  4.5k             |   < 0.2       |

Note:
1) This test result is based on a 10-second, 8 kHz mono audio stream with a fade duration of 10 seconds.
2) The CPU load of the fade process for other sampling rates and channels is approximately `sample_rate / 8000 * channel` times that of this configuration.
3) The memory usage of the fade process is related to the transition time, and the memory value is approximately `2 * transition_time (ms)`.

# Usage

Here is an example of using [FADE](../test_app/main/test_fade.c)

# FAQ  

1) What are the application scenarios for audio fading in and out?
   >Fading in and out creates smoother, more natural audio transitions, preventing abrupt starts or stops. It helps reduce unwanted noise and enhances the overall listening experience.

2) How to choose the curve for fading in and out?
   - Linear curve: Produces a smooth, even change in volume throughout, creating a consistent transition.
   - Square curve: Starts with a slower, more gradual change, gradually speeding up over time. This results in smoother, more natural volume adjustments early on.
   - Square root curve: Begins with a faster volume change, which gradually slows down. Ideal for emphasizing both the start and the end of an audio segment, giving it a more dynamic feel at key moments.
