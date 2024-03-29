# CHANNEL CONVERSION

`CHANNEL CONVERSION` module is designed to transform the original audio channel into a target channel. The implementation algorithm works by multiplying the input data with a specified weight matrix, resulting in the output data.

# Features

- Support full range of sample rates and channel
- Support bits per sample: s16, s24, s32
- Support data layout: interleaved, non-interleaved

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |     --       |  --           |     --            |
|   ESP32-S3R8   |    v5.3      | 240MHz        |   80MHz           |

| Bits per sample | Heap Memory(Byte) | CPU loading(%) |
|       --        |  --               |     --         |
|       16        |  40               |    < 0.07      |
|       24        |  40               |    < 0.20      |
|       32        |  40               |    < 0.24      |

Note:
1) This test result is based on an 8 kHz dual-channel audio being converted to mono. The channel conversion array contains two non-zero coefficients.
2) The CPU load for channel conversion can be calculated using the formula: `sample_rate / 8000 * non_zero_cof_num / 2`. The term `non_zero_cof_num` refers to the number of non-zero coefficients in the channel conversion array.
3) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [CHANNEL CONVERSION](../test_app/main/test_ch_cvt.c)

# FAQ

1) How does the CHANNEL CONVERSION function work?
   >Channel conversion utilizes coefficient array operations to transform audio signals from one channel configuration to another. Below is an algorithmic representation of the formula, along with an example:<br>
   The A1, A2, and A3 means three-channel input data, the B1 and B2 means two-channel output data,
   Wxx means weight coefficient.<br>
   The algorithm representation is:<br>
   ><p align="center">
       B1 = W1 * A1 + W2 * A2 + W3 * A3;
       B2 = W4 * A1 + W5 * A2 + W6 * A3;
   ></p>
   >E.g. The weight coefficient is {0.5, 0, 0.5, 0, 0.6, 0.4}
   ><p align="center">
       B1 = 0.5 * A1 + 0 * A2 + 0.5 * A3;
       B2 = 0 * A1 + 0.6 * A2 + 0.4 * A3;
   ></p>
   >If user not set weight coefficient array, all the weight value will be set to 1/src_ch_num.

2) Will the audio be distorted if the gain setting is too high?
   >Yes, if the gain setting is too high, the mixed signal can exceed its maximum allowable amplitude, leading to clipping distortion.

3) How to choose the appropriate CHANNEL CONVERSION coefficient?
   >The coefficients used for channel conversion are influenced by the speaker layout. According to the Dolby standard, the following tables outline the layouts for various channel conversions, with each table representing a distinct output format.<br>
   ><p align="center"> 
   |Channel layout|  Mono  |  Left  |  Right   |  Front Left |  Front Right |  Center | Low Frequency Effects | Surround Left |  Surround Right |  Back Left |  Back Right |
   |:------------:|:------:|:------:|:--------:|:-----------:|:------------:|:-------:|:---------------------:|:-------------:|:---------------:|:----------:|:-----------:|
   |term          |   M    |   L    |   R      |      FL     |      FR      |   C     |           LFE         |      SL       |      SR         |      BL    |      BR     |
   ></p> 
   >Note: This table shows the term of the channel layout.<br>
   ><p align="center"> 
   a. **MONO**

   |Output|  Mono  |     Stereo      |          Quad             |                 5.1                       |                            7.1                              |
   |:----:|:------:|:---------------:|:-------------------------:|:-----------------------------------------:|:-----------------------------------------------------------:|
   |  M   |   M    | L×0.707+R×0.707 |FL×0.5+FR×0.5+SL×0.5+SR×0.5|FL×0.447+FR×0.447+C×0.447+BL×0.447+BR×0.447|FL×0.378+FR×0.378+C×0.378+SL×0.378+SR×0.378+BL×0.378+BR×0.378|

   b. **STEREO**

   |Output|  Mono  | Stereo |  Quad     |     5.1           |         7.1                |
   |:----:|:------:|:------:|:---------:|:-----------------:|:--------------------------:|
   |  L   | M×0.707|    L   |FL+SL×0.707|FL+C×0.707+BL×0.707|FL+C×0.707+SL×0.707+BL×0.596|
   |  R   | M×0.707|    R   |FR+SR×0.707|FR+C×0.707+BR×0.707|FR+C×0.707+SR×0.707+BR×0.596|

   c. **QUAD**

   |Output|  Mono  | Stereo |  Quad     |     5.1           |               7.1                |
   |:----:|:------:|:------:|:---------:|:-----------------:|:--------------------------------:|
   |  FL  | M×0.707|    L   |    FL     |     FL+C×0.707    |FL×0.965+FR×0.258+C×0.707+SL×0.707|
   |  FR  | M×0.707|    R   |    FR     |     FR+C×0.707    |FL×0.258+FR×0.965+C×0.707+SR×0.707|
   |  SL  |        |        |    SL     |         BL        |    SL×0.707+BL×0.965+BR×0.258    |
   |  SR  |        |        |    SR     |         BR        |    SR×0.707+BL×0.258+BR×0.965    |

   d. **5.1**

   |Output|  Mono  | Stereo |             Quad         |    5.1    |       7.1                |
   |:----:|:------:|:------:|:------------------------:|:---------:|:------------------------:|
   |  FL  | M×0.707|    L   |       FL×0.961           |     FL    |    FL+SL×0.367           |
   |  FR  | M×0.707|    R   |       FR×0.961           |     FR    |    FR+SR×0.367           |
   |  C   |        |        |                          |     C     |         C                |
   |  LFE |        |        |                          |     LFE   |        LFE               |
   |  SL  |        |        |FL×0.274+SL×0.960+SR×0.422|     BL    |SL×0.930+BL×0.700+BR×0.460|
   |  SR  |        |        |FR×0.274+SL×0.422+SR×0.960|     BR    |SR×0.930+BL×0.460+BR×0.700|

   e. **7.1**

   |Output|  Mono  | Stereo |      Quad       |    5.1     |  7.1 |
   |:----:|:------:|:------:|:---------------:|:----------:|:----:|
   |  FL  | M×0.707|    L   |    FL×0.939     |     FL     |  FL  |
   |  FR  | M×0.707|    R   |    FR×0.939     |     FR     |  FR  |
   |  C   |        |        |                 |     C      |  C   |
   |  LFE |        |        |                 |     LFE    |  LFE |
   |  SL  |        |        |FL×0.344+SL×0.344|  BL×0.883  |  SL  |
   |  SR  |        |        |FR×0.344+SR×0.344|  BR×0.883  |  SR  |
   |  BL  |        |        |    SL×0.939     |  BL×0.470  |  BL  |
   |  BR  |        |        |    SR×0.939     |  BR×0.470  |  BR  |
   ></p> 
