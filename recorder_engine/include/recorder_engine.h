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

#ifndef __RECORDER_ENGINE_H__
#define __RECORDER_ENGINE_H__

#define REC_ONE_BLOCK_SIZE 3200     // 100ms[16k,16bit,1channel]

#define DEFAULT_REC_ENGINE_CONFIG() {\
    .one_frame_time_ms          = 10,\
    .sensitivity                = 0,\
    .vad_off_delay_ms           = 600,\
    .wakeup_time_ms             = 2000,\
    .open                       = NULL,\
    .fetch                      = NULL,\
    .close                      = NULL,\
    .evt_cb                     = NULL,\
}
/*
// Ther are some decription of recorder events with 4 use case.

////////////////////////////////////// Use case 1 /////////////////////////////////////////
                      +                wakeup time                      +
                      +-------------------------------------------------+
                      |                                                 |
                      |                                                 |
Wakeup Time  +--------+                                                 +-----------------+
                      |                                                 |
                      |         +---------------+                       |
                      |         |               |                       |
                      |         |               |                       |
Voice level  +------------------+               +-----------------------+-----------------+
                      |                         |
                      |         +------------------------+              |
                      |         |               |        |              |
                      |         |               |        |              |
EVENT        +------------------+               |        +--------------------------------+
                      |\        |\              |  vad   |\             |\
                      | \       | \             |  off   | \            | \
                      |  WAKEUP |  VAD START    |  time  |  VAD STOP    |   WAKEUP END
                      +  START  +               +        +              +

////////////////////////////////////// Use case 2 /////////////////////////////////////////
                      +                wakeup time                      +
                      +-------------------------------------------------+
                      |                                                 |
                      |                                                 |
Wakeup Time  +--------+                                                 +-----------------+
                      |                                                 |
                      |         +--------------------------------+      |
                      |         |                                |      |
                      |         |                                |      |
Voice level  +------------------+                                +-----------------------+
                      |                                          |
                      |         +---------------------------------------+
                      |         |                                |      | /VAD STOP
                      |         |                                |      |/
EVENT        +------------------+                                |      +-----------------+
                      |\        |\                               |      |\
                      | \       | \                              |  vad | \
                      |  WAKEUP |  VAD START                     |  off |   WAKEUP END
                      +  START  +                                + time +
////////////////////////////////////// Use case 3 /////////////////////////////////////////
                      +                wakeup time                      +
                      +-------------------------------------------------+
                      |                                                 |
                      |                                                 |
Wakeup Time   +-------+                                                 +------------------------+
                      |                                                 |
                      |         +------------------------------------------+
                      |         |                                       |  |
                      |         |                                       |  |
Voice level  +------------------+                                       |  +------+--------------+
                      |                                                 |  |      |
                      |         +---------------------------------------+---------+
                      |         |                                          |      | /VAD STOP
                      |         |                                          |      |/
EVENT        +------------------+                                          |      +--------------+
                      |\        |\                                         |      |\
                      | \       | \                                        | vad  | \
                      |  WAKEUP |  VAD                                     | off  |  WAKEUP END
                      +  START  +  START                                   + time +
////////////////////////////////////// Use case 4 /////////////////////////////////////////
                      +                     wakeup time                      +
                      +------------------------------------------------------+
                      |                                                      |
                      |                                                      |
Wakeup Time  +--------+                                                      +-----------------+
                      |         +----------+      +----+   +---+             |
                      |         |          |      |    |   |   |             |
                      |         |          |      |    |   |   |             |
Voice level  +------------------+          +------+    +---+   +-------------------------------+
                      |                    |               |   |             |
                      |                    +-------------+ |   |             |
                      |       SUSPEND ON \ |SUSPEND OFF\ | |   |             |
                      |                   \|            \| |   |             |
SUSPEND ON   +-----------------------------+             +-------------------------------------+
                      |                    |               |   |             |
                      |         +----------+               +----------+      |
                      |         |          |               |   |      |      |
                      |         |          |               |   |      |      |
EVENT        +------------------+          +-------------------------------------------------+
                      |\        |\         |\             /|   |      |\     |\
                      | \       | \        | \           / |   | vad  | \    | \
                      |  WAKEUP |  VAD     |  VAD    VAD   |   | off  | VAD  |  WAKEUP
                      +  START  +  START   +  STOP   START +   + time + STOP +  END

///////////////////////////////////////////////////////////////////////////////////////////
*/
typedef enum {
    REC_EVENT_WAKEUP_START,
    REC_EVENT_WAKEUP_END,
    REC_EVENT_VAD_START,
    REC_EVENT_VAD_STOP,
} rec_event_type_t;

typedef void (*rec_callback)(rec_event_type_t type);
typedef esp_err_t (*rec_open)(void **handle);
typedef esp_err_t (*rec_fetch)(void *handle, char *data, int data_size);
typedef esp_err_t (*rec_close)(void *handle);

typedef enum {
    REC_VOICE_SUSPEND_OFF,
    REC_VOICE_SUSPEND_ON,
} rec_voice_suspend_t;

/**
 * @brief recorder configuration parameters
 */
typedef struct {
    int           one_frame_time_ms;          // Time of one frame data.
    int           sensitivity;                // For vad sensitivity in ms. Not supported now.
    int           vad_off_delay_ms;           // Vad off delay to stop if no voice is detected.
    int           wakeup_time_ms;             // Time of wakeup.
    rec_open      open;                       // Recorder open callback function
    rec_fetch     fetch;                      // Recorder fetch data callback function
    rec_close     close;                      // Recorder close callback function
    rec_callback  evt_cb;                     // Recorder event callback function
} rec_config_t;

/**
 * @brief Create recorder engine according to parameters.
 *
 * @note Sample rate is 16k, 1 channel, 16bits, by default.
 *       Upon completion of this function rec_open callback will be triggered.
 *
 * @param cfg   See rec_config_t structure for additional details
 *
 * @return
 *     - 0: Success
 *     - -1: Error
 */
esp_err_t rec_engine_create(rec_config_t *cfg);

/**
 * @brief Read voice data after REC_EVENT_VAD_START.
 *
 * @param buffer        data pointer
 * @param buffer_size   Size of buffer, must be equal to REC_ONE_BLOCK_SIZE.
 * @param waiting_time  Timeout for reading data. Default time of REC_ONE_BLOCK_SIZE is 100ms, larger than 100ms is recommended.
 *
 * @return
 *      - 0: no voice block, or last voice block.
 *      - others: voice block index.
 */
int rec_engine_data_read(uint8_t *buffer, int buffer_size, int waiting_time);

/**
 * @brief Suspend or enable voice detection by vad.
 *
 * @param flag  REC_VOICE_SUSPEND_ON: Voice detection is suspended
 *              REC_VOICE_SUSPEND_OFF: Voice detection is not suspended
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t rec_engine_detect_suspend(rec_voice_suspend_t flag);

/**
 * @brief Start recording by force.
 *
 * @param none.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t rec_engine_trigger_start(void);

/**
 * @brief Stop recording by force.
 *
 * @param none.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t rec_engine_trigger_stop(void);

/**
 * @brief Destroy the recorder engine.
 *
 * @note Upon completion of this function rec_close callback will be triggered.
 *
 * @param None.
 *
 * @return
 *      - 0: Success
 *      - -1: Error
 */
esp_err_t rec_engine_destroy(void);

#endif
