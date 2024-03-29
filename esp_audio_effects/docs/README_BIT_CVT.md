# BIT CONVERSION

`BIT CONVERSION` module is used to convert the original audio bits per sample into the target bits per sample.

# Features

- Support full range of sample rates and channel
- Support bits per sample: u8, s16, s24, s32
- Support data layout: interleaved, non-interleaved

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |      --      |  --           |     --            |     
|   ESP32-S3R8   |     v5.3     | 240MHz        |   80MHz           |

| Heap Memory(Byte) | CPU loading(%) |
|  --               |     --         |    
|  20               |    < 0.06      |

Note:
1) The test music is in 8 kHz mono format. The CPU load for music with different sampling rates and channel configurations can be estimated using the formula:
   >CPU Load = (sample_rate / 8000) * channel_count * base_load<br>
   
   where `base_load` is the CPU load listed in table.
2) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [BIT CONVERSION](../test_app/main/test_bit_cvt.c)

# FAQ      
1) Why support unsigned 8-bit instead of signed 8-bit?
   >Because the 8-bit PCM audio data is unsigned.
