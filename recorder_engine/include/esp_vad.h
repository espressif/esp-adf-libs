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

#ifndef _ESP_VAD_H_
#define _ESP_VAD_H_

#include <stdint.h>

#define SAMPLE_RATE_HZ 16000      //Supports 32000, 16000, 8000
#define VAD_FRAME_LENGTH_MS 10    //Supports 10ms, 20ms, 30ms

/**
 * @brief Sets the VAD operating mode. A more aggressive (higher mode) VAD is more
 * restrictive in reporting speech.
 */
typedef enum {
    VAD_MODE_0 = 0,
    VAD_MODE_1,
    VAD_MODE_2,
    VAD_MODE_3
} vad_mode_t;

typedef enum {
    VAD_SILENCE = 0,
    VAD_SPEECH
} vad_state_t;

typedef void* vad_handle_t;

typedef struct vad_runner_t vad_runner_t;

extern const vad_runner_t VAD_RUNNER;

/**
 * @brief Creates an instance to the VAD structure.
 *
 * @param vad_mode          Sets the VAD operating mode.
 *
 * @param sample_rate_hz    The Sampling frequency (Hz) can be 32000, 16000, 8000, default: 16000.
 *
 * @param one_frame_ms      The length of the audio processing can be 10ms, 20ms, 30ms, default: 30.
 *
 * @return
 *         - NULL: Create failed
 *         - Others: The instance of VAD
 */
typedef vad_handle_t (*vad_runner_create_t)(vad_mode_t vad_mode, int sample_rate_hz, int one_frame_ms);

/**
 * @brief Feed samples of an audio stream to the VAD and check if there is someone speaking.
 *
 * @param inst      The instance of VAD.
 *
 * @param data      An array of 16-bit signed audio samples.
 *
 * @return
 *         - VAD_SILENCE if no voice
 *         - VAD_SPEECH  if voice is detected
 *
 */
typedef vad_state_t (*vad_runner_process_t)(vad_handle_t inst, int16_t *data);

/**
 * @brief Free the VAD instance
 *
 * @param inst The instance of VAD.
 *
 * @return None
 *
 */
typedef void (*vad_runner_destroy_t)(vad_handle_t inst);

/**
 * @brief Vad runner callback funcitons.
 */
struct vad_runner_t {
    vad_runner_create_t vad_create;
    vad_runner_process_t vad_process;
    vad_runner_destroy_t vad_destroy;
};

/*
Programming Guide:

static vad_runner_t *vad_runner = &VAD_RUNNER;    //defeine the VAD_RUNNER

int16_t *buffer = malloc((VAD_FRAME_LENGTH_MS * SAMPLE_RATE_HZ /1000) * sizeof(int16_t));  //Apply for a memory to store the audio data.

vad_handle_t vad_inst = vad_runner->vad_create(VAD_MODE_3, SAMPLE_RATE_HZ, VAD_FRAME_LENGTH_MS);     //Creates an instance to the VAD structure.

while (1) {
   //Use buffer to receive the audio data from MIC.
   vad_state_t vad_state = vad_runner->vad_process(vad_inst, buffer);      //Feed samples to the VAD process and get the result.
}

vad_runner->vad_destroy(vad_inst);   // Free the VAD instance at the end of whole VAD process

*/

#endif //_ESP_VAD_H_
