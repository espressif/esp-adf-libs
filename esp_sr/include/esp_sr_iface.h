/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _ESP_SR_IFACE_H_
#define _ESP_SR_IFACE_H_

#include "stdint.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque model data container
typedef struct model_iface_data_t model_iface_data_t;

/**
 * @brief Set wake words recognition operating mode.
 *
 * The probability of being wake words is increasing with a higher mode.
 * As a consequence also the false alarm rate goes up.
 *
 */
typedef enum {
    DET_MODE_90 = 0,  // Normal, response accuracy rate will be about 90%
    DET_MODE_95       // Aggressive, response accuracy rate will be about 95%
} det_mode_t;

/**
 * @brief wake word information
 */
typedef struct {
    int wake_word_num;     // The number of all wake words
    char **wake_word_list; // The name list of wake words
} wake_word_info_t;

/**
 * @brief Easy function type to initialze a model instance with a detection mode
 *
 * @param det_mode      The wake words detection mode to trigger wake words, the range of det_threshold is 0.5~0.9999
 *
 * @return
 *      - NULL   : Failed
 *      - Others : Object of model_iface_data_t
 */
typedef model_iface_data_t *(*esp_sr_iface_op_create_t)(det_mode_t det_mode);

/**
 * @brief Callback function type to fetch the number of samples that need to be passed to the detect function
 *
 * @Note   Every speech recognition model processes a certain number of samples at the same time.
 *         This function is used to query that number and should be called before the `detect`.
 *         The returned number is in 16-bit samples, not in bytes.
 *
 * @param model The model object to query
 *
 * @return
 *     - The number of samples to feed to the detect function
 */
typedef int (*esp_sr_iface_op_get_samp_chunksize_t)(model_iface_data_t *model);

/**
 * @brief Get the sample rate of the samples to feed to the detect function
 *
 * @param model     The model object to query
 *
 * @return
 *     - The sample rate, unit Hz
 */
typedef int (*esp_sr_iface_op_get_samp_rate_t)(model_iface_data_t *model);

/**
 * @brief Get the number of wake words
 *
 * @param model The model object to query
 *
 * @return
 *     - The number of wake words
 */
typedef int (*esp_sr_iface_op_get_word_num_t)(model_iface_data_t *model);

/**
 * @brief Get the name of wake word by index
 *
 * @Warning The index of wake word start with `1`

 * @param model         The model object to query
 * @param word_index    The index of wake word
 *
 * @return
 *     - Name of the wake word
 */
typedef char *(*esp_sr_iface_op_get_word_name_t)(model_iface_data_t *model, int word_index);

/**
 * @brief Get the structure which contains the information about wake words
 *
 * @param model         The model object to query
 *
 * @param word_list     The structure which contains the number and names of wake words
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL The word_list is NULL
 */
typedef esp_err_t (*esp_sr_iface_op_get_word_list_t)(model_iface_data_t *model, wake_word_info_t *word_list);

/**
 * @brief Set the detection threshold to manually abjust the probability
 *
 * @param model             The model object to query
 * @param det_threshold     The threshold to trigger wake words, the range of det_threshold is 0.5~0.9999
 * @param word_index        The index of wake word
 *
 * @return
 *     - 0: setting failed
 *     - 1: setting success
 */
typedef int (*esp_sr_iface_op_set_det_threshold_t)(model_iface_data_t *model, float det_threshold, int word_index);

/**
 * @brief Get the wake word detection threshold of different modes
 *
 * @param model             The model object to query
 * @param det_mode          The wake words recognition operating mode
 * @param word_index        The index of wake word
 *
 * @return
 *     - The detection threshold
 */
typedef float (*esp_sr_iface_op_get_det_threshold_t)(model_iface_data_t *model, det_mode_t det_mode, int word_index);

/**
 * @brief Feed samples of an audio stream to the speech recognition model and detect if there is a keyword found.
 *
 * @Warning The index of wake word starts with `1`, `0` means no wake words is detected.
 *
 * @param model The model object to query
 * @param samples An array of 16-bit signed audio samples. The array size used can be queried by the
 *        `get_samp_chunksize` function.
 *
 * @return
 *     - 0      :  no wake word is detected
 *     - Others : The index of wake words
 */
typedef int (*esp_sr_iface_op_detect_t)(model_iface_data_t *model, int16_t *samples);

/**
 * @brief Destroy a speech recognition model
 *
 * @param model Model object to destroy
 *
 * @return NONE
 */
typedef void (*esp_sr_iface_op_destroy_t)(model_iface_data_t *model);

/**
 * This structure contains the functions used to do operations on a speech recognition model.
 */
typedef struct {
    esp_sr_iface_op_create_t create;
    esp_sr_iface_op_get_samp_chunksize_t get_samp_chunksize;
    esp_sr_iface_op_get_samp_rate_t get_samp_rate;
    esp_sr_iface_op_get_word_num_t get_word_num;
    esp_sr_iface_op_get_word_name_t get_word_name;
    esp_sr_iface_op_get_word_list_t get_word_list;
    esp_sr_iface_op_set_det_threshold_t set_det_threshold;
    esp_sr_iface_op_get_det_threshold_t get_det_threshold_by_mode;
    esp_sr_iface_op_detect_t detect;
    esp_sr_iface_op_destroy_t destroy;
} esp_sr_iface_t;

/*
* Programming Guide:
*
* @code{c}
* static const sr_model_iface_t *model = &sr_model_wakenet3_quantized;
*
* // Initialize wakeNet model data
* static model_iface_data_t *model_data = model->create(DET_MODE_90);
*
* // Set parameters of buffer
* int audio_chunksize = model->get_samp_chunksize(model_data);
* int frequency = model->get_samp_rate(model_data);
* int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
*
* // Get voice data feed to buffer
* ...
*
* // Detect
* int r = model->detect(model_data, buffer);
* if (r > 0) {
*     printf("Detection triggered output %d.\n",  r);
* }
*
* // Destroy model
* model->destroy(model_data)
*
* @endcode
*
*/

#ifdef __cplusplus
extern "C" {
#endif

#endif /* _ESP_SR_IFACE_H_ */
