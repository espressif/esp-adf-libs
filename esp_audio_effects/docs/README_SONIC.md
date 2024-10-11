# SONIC

`Sonic` module is used to adjust both the pitch and playback speed of audio. By setting the speed parameter, you can achieve variable playback speeds without altering the pitch. Conversely, by adjusting the pitch parameter, you can modify the pitch without changing the playback speed. Additionally, by adjusting both the speed and pitch parameters simultaneously, you can achieve different combinations of speed and pitch.

# Features

- Support sample rates (Hz) that are integer multiples of 4000 and 11025, with a maximum value of 192000
- Support all channel number
- Support bits per sample: s16, s24, s32
- Support speed: [0.5, 2.0]
- Support pitch: [0.5, 2.0]

# Performance

Tested with the following system configuration:  
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |      --      |  --           |     --            |
|   ESP32-S3R8   |      v5.3    | 240MHz        |   80MHz           |

| speed    | Heap Memory(Byte) | CPU loading(%) | Memory(Byte) | CPU loading(%) |Memory(Byte) | CPU loading(%) |
|   --     |     --            |     --         |     --       |       --       |    --       |      --        |
|  bit     |     16            |     16         |     24       |       24       |    32       |      32        |
|  0.50    |     5.3k          |     1.09       |     11.3k    |       2.54     |    12.7k    |      2.57      |
|  0.75    |     5.8k          |     0.38       |     11k      |       1.01     |    10.8k    |      0.99      |
|  1.25    |     5.0k          |     0.22       |     8.1k     |       0.67     |    8.1k     |      0.60      |
|  1.50    |     4.3k          |     0.34       |     7.3k     |       0.84     |    7.3k     |      0.85      |
|  1.75    |     3.7k          |     0.43       |     7.4k     |       1.06     |    7.4k     |      1.03      |
|  2.00    |     3.8k          |     0.48       |     7.5k     |       1.18     |    7.5k     |      1.21      |

| pitch    | Heap Memory(Byte) | CPU loading(%) | Memory(Byte) | CPU loading(%) | Memory(Byte) | CPU loading(%) |
|   --     |     --            |     --         |    --        |     --         |     --       |       --       |
|  bit     |     16            |     16         |    24        |     24         |     32       |       32       |
|  0.50    |     5.4k          |     0.84       |   9.4k       |    1.45        |     10.5k    |       1.48     |
|  0.75    |     5.5k          |     0.64       |   11.3k      |    0.98        |     10.0k    |       1.00     |
|  1.25    |     9.3k          |     0.70       |   16.8k      |    1.08        |     12.4k    |       0.88     |
|  1.50    |     9.2k          |     0.95       |   12.9k      |    1.69        |     19.6k    |       1.45     |
|  1.75    |     10.4k         |     1.23       |   17.6k      |    2.38        |     18.5k    |       2.28     |
|  2.00    |     9.7k          |     1.49       |   21.1k      |    2.92        |     19.9k    |       2.80     |

Note:<br>
1) The test music is 8 kHz mono, and the CPU load for music at different sampling rates and channels is approximately `sample rate / 8000 * channel` times that of this configuration.
2) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [Sonic](../test_app/main/test_sonic.c)

# Copyrights and Licenses

Sonic is a third party copyrighted code and is included under the following licenses:

- [Sonic](https://github.com/waywardgeek/sonic) library is Copyright 2010, Bill Cox and is licensed under the Apache 2.0 license.

# FAQ  

1) What are the functions of sonic?
   - Time-Stretching without Pitch-Shifting:
      - Use the esp_ae_sonic_set_speed interface to adjust audio speed:
         - Speed < 1: Audio playback slows down.
         - Speed = 1: Playback speed remains unchanged.
         - Speed > 1: Audio playback speeds up.
   - Pitch-Shifting without Time-Stretching:
      - Use the esp_ae_sonic_set_pitch interface to adjust audio pitch:
         - Pitch < 1: Audio pitch decreases.
         - Pitch = 1: Pitch remains unchanged.
         - Pitch > 1: Audio pitch increases.
   - Time-Stretching and Pitch-Shifting:
      - Simultaneously call the esp_ae_sonic_set_speed and esp_ae_sonic_set_pitch interfaces to achieve both time-stretching and pitch adjustment of the audio.

2) Can using sonic achieve loli sound?
   >Using Sonic alone may not be sufficient to achieve a "loli voice" effect. While Sonic can alter audio speed and pitch, the specific characteristics associated with a loli voice typically require more nuanced adjustments beyond simple modulation.
   >Achieving this effect may involve modifications to:
   - **Resonance frequencies**
   - **Harmonic characteristics**
   - **Other sonic qualities**
   >Therefore, while Sonic is a useful tool for basic audio manipulation, achieving a genuine loli voice effect may necessitate additional audio processing techniques or dedicated voice transformation tools.

3) Can Sonic transform the sound of a piano into the sound of a violin?
   >No, because Sonic can only change the pitch of the audio, while the timbre is determined by the harmonic structure and resonance characteristics of the instrument. As a result, it typically cannot completely transform the sound of one instrument into another.
