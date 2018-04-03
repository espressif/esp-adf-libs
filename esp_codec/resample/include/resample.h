// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

/*
 * It's output pcm size aligning bits. 2 - align with 4 bytes, 4 - align with 16 bytes.
 *
 */
#define PCM_INOUT_NUM_RESTRICT (4)

#if (PCM_INOUT_NUM_RESTRICT < 0) || (PCM_INOUT_NUM_RESTRICT > 64)
#error input/output pcm number restrict setup error
#endif


//#define FIXED_POINT

#ifdef FIXED_POINT
#define data_int16 short
#else
#define data_int16 float
#endif

typedef void *esp_resample_handle_t;

/**
 * @brief  Create an esp_resample_handle_t handle with specific parameters
 *
 * Notes: Decode mode, the input pcm number control the output pcm number
 *        Encode mode, the output pcm number control the input pcm number
 *
 * @param[in]       pcm_inout_num_restrict      Out data size will be align with this number.
 * @param[in]       src_rate                    Source sample rate
 * @param[in]       tar_rate                    Destination sample rate
 * @param[in]       in_chan                     Number of source channel
 * @param[in]       out_chan                    Number of destination channel
 * @param[in]       down_ch_idx                 Select right or left channel will be keep,
 *                                                  only work with source channel more than destination channel.
 * @param[in]       resample_mode               Decode mode and encode mode supported,`0` means decode mode, `1` means encode mode
 * @param[in/out]   in_pcm_size                 In decode mode, it's input parameter, for fixed input number of pcm.
 *                                              In encode mode, it's output parameter, indicate how many pcm size needed with one frame.
 * @param[in/out]   out_pcm_size                In decode mode, it's output parameter, indicate how many pcm size will buffering with one frame.
 *                                              In encode mode, it's input parameter, for fixed output number of pcm.
 * @return
 *     - NULL:  Fail
 *     - Others: Success
 *
 */
esp_resample_handle_t resample_open(int pcm_inout_num_restrict, int src_rate, int tar_rate,
                                    int in_chan, int out_chan, int down_ch_idx, int resample_mode,
                                    int *in_pcm_size, int *out_pcm_size);

/**
 * @brief  Process the data with input.
 *
 * Notes: This function configures and initializes the TWDT.
 *
 * @param[in]           handle              Handle from resample_open.
 * @param[in]           in_pcm              Input buffer.
 * @param[in]           out_pcm             Output buffer.
 * @param[in]           in_pcm_num          It's input pcm numbers.
 * @param[in/out]       output_pcm_num      In decode mode, it's output parameter, indicate how many pcm numbers in `out_pcm`.
 *                                          In encode mode, it's input parameter, indicate how many pcm numbers in `out_pcm`.
 * @return
 *     - < 0 :  Fail
 *     - >= 0:  In encode mode, it's consumed input pcm numbers, if is not equal to `output_pcm_num`, need to move the in buffer.
 *              In decode mode, it will be equal to `output_pcm_num`.
 */
int resample_process(esp_resample_handle_t handle, short *in_pcm, short *out_pcm, int in_pcm_num,
                     int *output_pcm_num);

/**
 * @brief  Destroy the specific `esp_resample_handle_t`.
 *
 * @param[in]       handle      Handle from resample_open.
 *
 * @return  NONE.
 */
void resample_close(esp_resample_handle_t handle);
