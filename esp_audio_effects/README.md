# ESP_AUDIO_EFFECTS

- [中文版](./README_CN.md)

Espressif Audio Effects (ESP_AUDIO_EFFECTS) is the official audio processing module developed by Espressif Systems for SoCs. The ESP Audio Effects module offers a range of professional, high-performance audio processing algorithms that can be used to modify, enhance, or alter the characteristics of audio signals. The supported modules include Automatic Level Control (ALC), Sample Rate Conversion, Bit Depth Conversion, Channel Conversion, Equalization, Data Weaving, Mixing, Fading, Sonic，Dynamic Range Control (DRC) and Multi-band Compressor (MBC) Processing.

# Detailed Introduction of Each Module

The following table listed the sample_rate, channels and bits_per_sample supported by each module. If user wants to know the detailed introduction, performance, examples, and other information of each module, can click on the README link in the `Module` column  

|               Module                       |    Sample Rate                                  | Channel  |  Bits Per Sample    |       Data Layout         | Initial Supported Version |
|:------------------------------------------:|:-----------------------------------------------:|:--------:|:-------------------:|:-------------------------:|:-------------------------:|
| [ALC](docs/README_ALC.md)                  |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [RATE CONVERSION](docs/README_RATE_CVT.md) |4-192 kHz, and integer multiples of 4000 or 11025|Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [BIT CONVERSION](docs/README_BIT_CVT.md)   |       Full range                                |Full range|  u8, s16, s24, s32  |Interleave and Deinterleave|           v1.0.0          |
| [CHANNEL CONVERSION](docs/README_CH_CVT.md)|       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [DATA WEAVER](docs/README_DATA_WEAVER.md)  |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [EQUALIZER](docs/README_EQ.md)             |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [FADE](docs/README_FADE.md)                |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [MIXER](docs/README_MIXER.md)              |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.0.0          |
| [SONIC](docs/README_SONIC.md)              |4-192 kHz, and integer multiples of 4000 or 11025|Full range|  s16, s24, s32      |       Interleave          |           v1.0.0          |
| [DRC](docs/README_DRC.md)                  |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.2.0          |
| [MBC](docs/README_MBC.md)                  |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|           v1.2.0          |

#  Audio Effects Release and SoC Compatibility

The following table shows the support of ESP_AUDIO_CODEC for Espressif SoCs. The "&#10004;" means supported, and the "&#10006;" means not supported. 

|Version      |  ESP32   |   ESP32-S2 |   ESP32-S3 |   ESP32-C2 |  ESP32-C3 |  ESP32-C5 |  ESP32-C6 |   ESP32-P4  |   ESP32-H4 |
|:-----------:|:--------:|:----------:|:----------:|:----------:|:---------:|:---------:|:---------:|:-----------:|:----------:|
|   v1.0.0    | &#10004; |  &#10004;  |  &#10004;  |  &#10004;  | &#10004;  | &#10004;  | &#10004;  |  &#10004;   |  &#10006;  |
|   v1.0.1    | &#10004; |  &#10004;  |  &#10004;  |  &#10004;  | &#10004;  | &#10004;  | &#10004;  |  &#10004;   |  &#10006;  |
|   v1.0.2    | &#10004; |  &#10004;  |  &#10004;  |  &#10004;  | &#10004;  | &#10004;  | &#10004;  |  &#10004;   |  &#10006;  |
|   v1.1.0    | &#10004; |  &#10004;  |  &#10004;  |  &#10004;  | &#10004;  | &#10004;  | &#10004;  |  &#10004;   |  &#10004;  |
|   v1.2.0    | &#10004; |  &#10004;  |  &#10004;  |  &#10004;  | &#10004;  | &#10004;  | &#10004;  |  &#10004;   |  &#10004;  |
