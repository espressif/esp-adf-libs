# EQ

`EQ` (Equalizer) module is used to enable the adjustment of sound quality. It compensates for defects in speakers and acoustic environments, modifies various sound sources, and offers additional specialized functions. The core component of an equalizer is a filter, and an equalizer consists of multiple filters. Each filter serves a different function, which is explained in detail below.

# Features

- Support full range of sample rates and channel
- Support bits per sample: s16, s24, s32
- Support a combination of one or more filters
- Support filter type: high pass, low pass, high shelf, low shelf, peak
- Support to adjust the design parameters of the filter in real time
- Support data layout: interleaved, non-interleaved

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |     --       |  --           |     --            |  
|   ESP32-S3R8   |    v5.3      | 240MHz        |   80MHz           |

| Bits per sample| Heap Memory(Byte) | CPU loading(%) |
|       --       |  --               |     --         |  
|       16       |  128              |    0.09        |
|       24       |  2176             |    0.23        |
|       32       |  128              |    0.20        |

Note:
1) The test music is in 8 kHz mono format. The CPU load for music with different sampling rates and channel configurations can be estimated using the formula:
   >CPU Load = (sample_rate / 8000) * channel_count * filter_num * base_load<br>

   where `base_load` is the CPU load listed in table.
2) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [EQ](../test_app/main/test_eq.c)

# FAQ

1) What audio processing tasks are parameter equalizers suitable for?
   >Parametric equalizers are suitable for music recording, mixing, baseband processing, audio restoration, as well as in music players and sound systems. They offer refined audio tuning capabilities, allowing for more precise timbre shaping and balance in audio.

2) How to choose the filter mode, center frequency, gain, and Q value parameters of parameter equalizers?
   >Each EQ frequency band can be individually controlled and disabled. Users can select filter modes and set filter parameters for each frequency band. The following table provides a brief overview of the parameters that can be configured based on the selected filter mode for a given frequency band:
   ><p align="center"> 
   |Filter Mode  |         fc                               |         Q value                          |         Gain                             |
   |:-----------:|:----------------------------------------:|:----------------------------------------:|:----------------------------------------:|
   |High Pass    | ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/no-icon.png) |
   |Low Pass     | ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/no-icon.png) |
   |High Shelf   | ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)|
   |Low Shelf    | ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)|
   |Peak         | ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)| ![alt text](../docs/_static/yes-icon.png)|
   ></p> 
   >Next, provide a detailed introduction to the parameter selection of each filter and its impact on audio:<br>
   - Low-Pass Filter (LPF): Employ a low-pass filter to attenuate frequencies above a specific `fc` while passing lower frequencies. This filter is useful for eliminating high-frequency noise or unwanted treble from a signal. In this filter mode, the `fc` and `Q value` can be set in parameter form:<br>
      - The `fc` defines after what frequency there will be a decrease in volume, and the `fc` of `LPF` is the maximum value of the `fc` of all eq frequency bands.<br>
      - The `Q value` controls filter resonance in the following way (the higher the `Q value`, the more severe the signal attenuation near the `fc`):<br>
      At `Q=0.7`, the filter will not exhibit resonant peaks. The decrease in volume at the `fc` is -3 dB.<br>
      At `Q=1.0`, the filter exhibits resonance peaks below the `fc`. The volume at the `fc` is 0 dB.<br>
      At `Q=2.0`, the filter exhibits a resonant peak (+6 dB) at the `fc`.<br>
   - High-Pass Filter (HPF): Use a high-pass filter to attenuate frequencies below a certain cutoff point while allowing higher frequencies to pass through. Choose this filter when you want to remove low-frequency rumble or unwanted bass from a signal. In this filter mode, the `fc` and `Q value` can be set in parameter form:<br>
      - The `fc` defines before what frequency there will be a decrease in volume, and the `fc` of `HPF` is the minimum value of the `fc` of all eq frequency bands.<br>
      - The `Q value` characteristics refer to low-pass `LPF`.<br>
   - Low-Shelf Filter(LSF): A low-shelf filter boosts or attenuates frequencies below a designated `fc` while leaving higher frequencies unaffected. Employ this filter to enhance or reduce the bass response in audio signals. In this filter mode, the `fc`, `Q value` and `gain` can be set in parameter form:<br>
      - The `fc` defines before what frequency there will be an increase or decrease in volume.<br>
      - The `gain` controls the amplitude of volume increase or decrease before the `fc`.<br>
      - The `Q value` characteristics refer to low-pass `LPF`.<br>
   - High-Shelf Filter(HSF): A high-shelf filter boosts or attenuates frequencies above a specified `fc` while leaving lower frequencies unchanged. Use this filter to add brightness or warmth to audio signals.<br>
      - The `fc` defines after what frequency there will be an increase or decrease in volume.<br>
      - The `gain` controls the amplitude of volume increase or decrease before the `fc`.<br>
      - The `Q value` characteristics refer to low-pass `LPF`.<br>
   - Peak Filter: A peaking filter boosts or cuts frequencies around a central frequency with a specified bandwidth. This filter is versatile and can be used for precise frequency adjustments or for creating resonant effects. In this filter mode, the `fc`, `Q value` and `gain` can be set in parameter form:<br>
      - The `fc` controls at what frequency the output volume will peak relative to the input volume.<br>
      - The `gain` controls the volume gain at the `fc`.<br>
      - The `Q value` controls the width of the frequency band affected by the filter (centered around the `fc`). The lower the `Q value`, the wider the width of the affected frequency band (the flatter the peak curve). The higher the `Q value`, the narrower the width of the affected frequency band (the steeper the peak curve).
