# ESP_AUDIO_EFFECTS

Espressif Audio Effects (ESP_AUDIO_EFFECTS) is the official audio processing module developed by Espressif Systems for SoCs. The ESP Audio Effects module offers a range of professional, high-performance audio processing algorithms that can be used to modify, enhance, or alter the characteristics of audio signals. The supported modules include Automatic Level Control (ALC), Sample Rate Conversion, Bit Depth Conversion, Channel Conversion, Equalization, Data Weaving, Mixing, Fading, and Sonic Processing.

# Detailed Introduction of Each Module

The following table listed the sample_rate, channels and bits_per_sample supported by each module. If user wants to know the detailed introduction, performance, examples, and other information of each module, can click on the README link in the `Module` column  

|               Module                       |    Sample Rate                                  | Channel  |  Bits Per Sample    |       Data Layout         |
|:------------------------------------------:|:-----------------------------------------------:|:--------:|:-------------------:|:-------------------------:|
| [ALC](docs/README_ALC.md)                  |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [RATE CONVERSION](docs/README_RATE_CVT.md) |4-192 kHz, and integer multiples of 4000 or 11025|Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [BIT CONVERSION](docs/README_BIT_CVT.md)   |       Full range                                |Full range|  u8, s16, s24, s32  |Interleave and Deinterleave|
| [CHANNEL CONVERSION](docs/README_CH_CVT.md)|       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [DATA WEAVER](docs/README_DATA_WEAVER.md)  |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [EQUALIZER](docs/README_EQ.md)             |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [FADE](docs/README_FADE.md)                |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [MIXER](docs/README_MIXER.md)              |       Full range                                |Full range|  s16, s24, s32      |Interleave and Deinterleave|
| [SONIC](docs/README_SONIC.md)              |4-192 kHz, and integer multiples of 4000 or 11025|Full range|  s16, s24, s32      |       Interleave          |

#  Audio Effects Release and SoC Compatibility

The following table shows the support of ESP_AUDIO_CODEC for Espressif SoCs. The "&#10004;" means supported, and the "&#10006;" means not supported. 
 
|Chip         |      v1.0.1      |
|:-----------:|:----------------:|
|ESP32        |     &#10004;     |
|ESP32-S2     |     &#10004;     |
|ESP32-S3     |     &#10004;     |
|ESP32-C2     |     &#10004;     |
|ESP32-C3     |     &#10004;     |
|ESP32-C5     |     &#10004;     |
|ESP32-C6     |     &#10004;     |
|ESP32-P4     |     &#10004;     |
