# ALC

`ALC` (Automatic Level Control) module is used to automatically adjust the volume level of audio signals to ensure stable output volume at different input signal levels. ALC is mainly used to prevent sudden increase or decrease in audio volume, thus providing a more consistent audio experience.

# Features

- Support full range of sample rates and channel
- Support bits per sample: s16, s24, s32
- Support the gain range is (-∞, 63], the gain setting is less than -64, audio will set to mute. Unit: dB.
- Support data layout: interleaved, non-interleaved

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |      --      |  --           |     --            |  
|   ESP32-S3R8   |     v5.3     | 240MHz        |   80MHz           |

| Bits per sample| Heap Memory(Byte) | CPU loading(%) |
|       --       |  --               |     --         |   
|       16       |  < 5k             |    < 0.3       |
|       24       |  < 5k             |    < 0.4       |
|       32       |  < 5k             |    < 0.4       |

Note:
1) The test music is in 8 kHz mono format. The CPU load for music with different sampling rates and channel configurations can be estimated using the formula:
   >CPU Load = (sample_rate / 8000) * channel_count * base_load<br>

   where `base_load` is the CPU load listed in table.
2) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [ALC](../test_app/main/test_alc.c)

# FAQ

1) Will the audio become distorted if the gain setting is set too high?
   >No，If the gain setting is too high, ALC will limit it within the dynamic range, preventing the signal from being amplified beyond its maximum allowable amplitude, thus avoiding clipping distortion.
