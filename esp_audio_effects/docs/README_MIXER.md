# MIXER

`MIXER` module is used to combine multiple audio signals into a single audio signal, with each signal assigned a specific weight. These weight values are multiplied by the audio data from their corresponding channels, and the resulting values from each channel are summed to create the final mixed audio signal.

# Features

- Support full range of sample rates and channel
- Support bits per sample: s16, s24, s32
- Support set two stable weight
- Support set mode to change stable weight
- Support data layout: interleaved, non-interleaved

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |      --      |  --           |     --            |
|   ESP32-S3R8   |     v5.3     | 240MHz        |   80MHz           |

| Bits per sample|  Heap Memory(Byte)| CPU loading(%) |
|       --       |       --          |     --         |
|       16       |       4.5k        |    <0.3        |
|       24       |       4.5k        |    <0.42       |
|       32       |       4.5k        |    <0.46       |

Note:
1) This test result is derived from two 10-second, 8 kHz mono audio streams, with a transition time of 10 seconds set for the mixing process.
2) The CPU load of the mixing process for different sampling rates and channels is approximately `sample_rate / 8000 * channel` times greater than that of this configuration.
3) The memory usage of the mixing process is related to the transition time, and the memory requirement is approximately `4 * transition_time (ms)`.

# Usage

Here is an example of using [MIXER](../test_app/main/test_mixer.c)

# FAQ  

1) How Mixer works?
   >The mixer combines multiple audio streams into a single output stream. Users must first specify the number of audio streams to be mixed, ensuring that each stream has the same sample rate, number of channels, and bits per sample. Next, users set `weight1`, `weight2`, and `transmit_time` for each audio stream, where the weights are used to adjust the input audio volume. Initially, the output audio data for each channel is equal to the input audio data multiplied by `weight1`. If the mode of a specific channel is changed to `ESP_AE_MIXER_MODE_FADE_UPWARD`, the weight will transition from `weight1` to `weight2` over the specified response time.

2) What is the relationship between weight and decibels?
   >The weight is used to adjust the proportional factor of signal amplitude. The formula for the conversion relationship between weight and decibels is as follows:
   ><p align="center"> 
                    dB=20log10(weight)
   ></p> 

3) How can users modify the weight of each route?
   >Each path has two weights: weight1 and weight2. The initial weight is set to weight1.
   - To change the weight to weight2, set the mode to `ESP_AE_MIXER_MODE_FADE_UPWARD`.
   - To revert back to weight1, set the mode to `ESP_AE_MIXER_MODE_FADE_DOWNWARD`.

4) Will it cause any distortion when using a mixer?
   >Using a mixer can potentially lead to distortion. Distortion occurs when the mixer's output signal exceeds its maximum handling amplitude, resulting in signal clipping or other forms of degradation. To avoid distortion and maintain audio quality, it's essential to configure the mixer weight properly.

5) What is the amplitude response curve of a mixer?
   >Currently, only linear response curves are supported.
