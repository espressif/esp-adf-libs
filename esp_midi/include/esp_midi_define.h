/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once
#pragma pack(1)

#include <stdint.h>

#define ESP_MIDI_SPT_CH            (16)
#define ESP_MIDI_TRACK_HEADER_SIZE (8)

#define ESP_MIDI_FOURCC(_a, _b, _c, _d) \
    (uint32_t)(((uint32_t)(_d) << 24) | ((uint32_t)(_c) << 16) | ((uint32_t)(_b) << 8) | (uint32_t)(_a))

#define ESP_MIDI_READ_2_BYTES(buffer, value) {     \
    (value) = (((buffer)[0] << 8) | (buffer)[1]);  \
}
#define ESP_MIDI_READ_4_BYTES(buffer, value) {                                                 \
    (value) = (((buffer)[0] << 24) | ((buffer)[1] << 16) | ((buffer)[2] << 8) | (buffer)[3]);  \
}

#define MTHD_FCC ESP_MIDI_FOURCC('M', 'T', 'h', 'd')
#define MTRK_FCC ESP_MIDI_FOURCC('M', 'T', 'r', 'k')

/**
 * @brief  RIFF chunk structure
 */
typedef struct {
    uint32_t  chunk_id;    /*!< Unique identifier for the chunk */
    uint32_t  chunk_size;  /*!< Size of the data contained within the chunk */
} esp_midi_chunk_t;

/**
 * @brief  This structure defines the header of a MIDI file within the RIFF (Resource Interchange File Format) specification.
 *         It contains information about the overall structure and format of the MIDI data that follows
 *         Chunk Type : 4 bytes(MThd)
 *         Chunk Length : 4 bytes(usually 6)
 *         Format Type : 2 bytes(0, 1, or 2)
 *         Number of Tracks : 2 bytes
 *         Time Division : 2 bytes
 */
typedef struct {
    esp_midi_chunk_t  chunk;          /*!< MIDI chunk */
    uint16_t          format_type;    /*!< Format type
                                           0 - the file contains a single multi - channel track
                                           1 - the file contains one or more simultaneous tracks(or MIDI outputs) of a sequence
                                           2 - the file contains one or more sequentially independent single - track patterns */
    uint16_t          num_of_tracks;  /*!< Number of track */
    uint16_t          time_division;  /*!< Chunk defines a "division" which is delta ticks per quarter note
                                           Time(in ms.) = (Number of Ticks) * (Tempo(uS / qn) / Div(ticks / qn)) / 1000 */
} esp_midi_header_t;

/**
 * @brief  This enumeration lists various MIDI control change values
 */
typedef enum midi_control_change {
    ESP_MIDI_CC_BANK_SELECT_MSB      = 0x00,  /*!<Bank select(MSB) */
    ESP_MIDI_CC_MODULATION_MSB       = 0x01,  /*!<Modulation (MSB) */
    ESP_MIDI_CC_BREATH_MSB           = 0x02,  /*!<Breath Controller (MSB) */
    ESP_MIDI_CC_FOOT_MSB             = 0x04,  /*!<Foot Controller (MSB)*/
    ESP_MIDI_CC_PORTAMENTO_TIME_MSB  = 0x05,  /*!<Portamento Time (MSB)*/
    ESP_MIDI_CC_DATA_ENTRY_MSB       = 0x06,  /*!<Data Entry (MSB) */
    ESP_MIDI_CC_VOLUME_MSB           = 0x07,  /*!<Volume (MSB) */
    ESP_MIDI_CC_BALANCE_MSB          = 0x08,  /*!<Balance (MSB) */
    ESP_MIDI_CC_PAN_MSB              = 0x0A,  /*!<Pan (MSB) */
    ESP_MIDI_CC_EXPRESSION_MSB       = 0x0B,  /*!<Expression (MSB) */
    ESP_MIDI_CC_EFFECTS1_MSB         = 0x0C,  /*!<Effects 1 (MSB)*/
    ESP_MIDI_CC_EFFECTS2_MSB         = 0x0D,  /*!<Effects 2 (MSB) */
    ESP_MIDI_CC_GPC1_MSB             = 0x10,  /*!<General Purpose Controller 1 (MSB) */
    ESP_MIDI_CC_GPC2_MSB             = 0x11,  /*!<General Purpose Controller 2 (MSB)*/
    ESP_MIDI_CC_GPC3_MSB             = 0x12,  /*!<General Purpose Controller 3 (MSB) */
    ESP_MIDI_CC_GPC4_MSB             = 0x13,  /*!<General Purpose Controller 4 (MSB) */
    ESP_MIDI_CC_BANK_SELECT_LSB      = 0x20,  /*!<Bank select(LSB) */
    ESP_MIDI_CC_MODULATION_WHEEL_LSB = 0x21,  /*!<Modulation (LSB) */
    ESP_MIDI_CC_BREATH_LSB           = 0x22,  /*!<Breath Controller (LSB)  */
    ESP_MIDI_CC_FOOT_LSB             = 0x24,  /*!<Foot Controller (LSB) */
    ESP_MIDI_CC_PORTAMENTO_TIME_LSB  = 0x25,  /*!<Portamento Time (LSB) */
    ESP_MIDI_CC_DATA_ENTRY_LSB       = 0x26,  /*!<Data Entry (LSB)  */
    ESP_MIDI_CC_VOLUME_LSB           = 0x27,  /*!<Volume (LSB) */
    ESP_MIDI_CC_BALANCE_LSB          = 0x28,  /*!<Balance (LSB) */
    ESP_MIDI_CC_PAN_LSB              = 0x2A,  /*!<Pan (LSB)  */
    ESP_MIDI_CC_EXPRESSION_LSB       = 0x2B,  /*!<Expression (LSB)*/
    ESP_MIDI_CC_EFFECTS1_LSB         = 0x2C,  /*!<Effects 1 (LSB) */
    ESP_MIDI_CC_EFFECTS2_LSB         = 0x2D,  /*!<Effects 2 (LSB) */
    ESP_MIDI_CC_GPC1_LSB             = 0x30,  /*!<General Purpose Controller 1 (LSB) */
    ESP_MIDI_CC_GPC2_LSB             = 0x31,  /*!<General Purpose Controller 2 (LSB) */
    ESP_MIDI_CC_GPC3_LSB             = 0x32,  /*!<General Purpose Controller 3 (LSB) */
    ESP_MIDI_CC_GPC4_LSB             = 0x33,  /*!<General Purpose Controller 4 (LSB) */
    ESP_MIDI_CC_SUSTAIN_SWITCH       = 0x40,  /*!<Sustain switch */
    ESP_MIDI_CC_PORTAMENTO_SWITCH    = 0x41,  /*!<Portamento switch */
    ESP_MIDI_CC_SOSTENUTO_SWITCH     = 0x42,  /*!<Sostenuto switch */
    ESP_MIDI_CC_SOFT_PEDAL_SWITCH    = 0x43,  /*!<Software pedal switch */
    ESP_MIDI_CC_LEGATO_SWITCH        = 0x44,  /*!<Legato switch */
    ESP_MIDI_CC_HOLD2_SWITCH         = 0x45,  /*!<Hold 2 switch */
    ESP_MIDI_CC_SOUND_CTRL1          = 0x46,  /*!<Sound control 1  */
    ESP_MIDI_CC_SOUND_CTRL2          = 0x47,  /*!<Sound control 2  */
    ESP_MIDI_CC_SOUND_CTRL3          = 0x48,  /*!<Sound control 3  */
    ESP_MIDI_CC_SOUND_CTRL4          = 0x49,  /*!<Sound control 4  */
    ESP_MIDI_CC_SOUND_CTRL5          = 0x4A,  /*!<Sound control 5 */
    ESP_MIDI_CC_SOUND_CTRL6          = 0x4B,  /*!<Sound control 6  */
    ESP_MIDI_CC_SOUND_CTRL7          = 0x4C,  /*!<Sound control 7  */
    ESP_MIDI_CC_SOUND_CTRL8          = 0x4D,  /*!<Sound control 8  */
    ESP_MIDI_CC_SOUND_CTRL9          = 0x4E,  /*!<Sound control 9 */
    ESP_MIDI_CC_SOUND_CTRL10         = 0x4F,  /*!<Sound control 10 */
    ESP_MIDI_CC_GPC5                 = 0x50,  /*!<General Purpose Controller 5 */
    ESP_MIDI_CC_GPC6                 = 0x51,  /*!<General Purpose Controller 6 */
    ESP_MIDI_CC_GPC7                 = 0x52,  /*!<General Purpose Controller 7*/
    ESP_MIDI_CC_GPC8                 = 0x53,  /*!<General Purpose Controller 8 */
    ESP_MIDI_CC_PORTAMENTO_CTRL      = 0x54,  /*!<Portamento control */
    ESP_MIDI_CC_EFFECTS_DEPTH1       = 0x5B,  /*!<Effects depth 1  */
    ESP_MIDI_CC_EFFECTS_DEPTH2       = 0x5C,  /*!<Effects depth 2  */
    ESP_MIDI_CC_EFFECTS_DEPTH3       = 0x5D,  /*!<Effects depth 3   */
    ESP_MIDI_CC_EFFECTS_DEPTH4       = 0x5E,  /*!<Effects depth 4   */
    ESP_MIDI_CC_EFFECTS_DEPTH5       = 0x5F,  /*!<Effects depth 5  */
    ESP_MIDI_CC_DATA_ENTRY_INCR      = 0x60,  /*!<Control the increment of changing data entry */
    ESP_MIDI_CC_DATA_ENTRY_DECR      = 0x61,  /*!<Reduce the data input value*/
    ESP_MIDI_CC_NRPN_LSB             = 0x62,  /*!<NRPN(LSB) */
    ESP_MIDI_CC_NRPN_MSB             = 0x63,  /*!<NRPN(MSB) */
    ESP_MIDI_CC_RPN_LSB              = 0x64,  /*!<RPN(LSB) */
    ESP_MIDI_CC_RPN_MSB              = 0x65,  /*!<RPN(MSB) */
    ESP_MIDI_CC_ALL_SOUND_OFF        = 0x78,  /*!<All sound off */
    ESP_MIDI_CC_ALL_CTRL_OFF         = 0x79,  /*!<All control off */
    ESP_MIDI_CC_LOCAL_CONTROL        = 0x7A,  /*!<Local control*/
    ESP_MIDI_CC_ALL_NOTES_OFF        = 0x7B,  /*!<All notes off*/
    ESP_MIDI_CC_OMNI_OFF             = 0x7C,  /*!<OMNI off */
    ESP_MIDI_CC_OMNI_ON              = 0x7D,  /*!<OMNI on */
    ESP_MIDI_CC_POLY_OFF             = 0x7E,  /*!<Poly off */
    ESP_MIDI_CC_POLY_ON              = 0x7F   /*!<Poly on */
} esp_midi_control_change_type_t;

/**
 * @brief  Enumeration listing MIDI Registered Parameter Number (RPN) event numbers.
 *         This enumeration defines the standard MIDI RPN event numbers,
 *         which are used to access specific parameters in MIDI devices.
 *         Each number corresponds to a specific parameter, such as pitch bend range,
 *         fine tune, coarse tune, and so on. RPN messages consist of two parts:
 *         the MSB (Most Significant Byte) and the LSB (Least Significant Byte),
 *         which together form a unique RPN number.
 */
typedef enum midi_rpn_event {
    ESP_MIDI_RPN_PITCH_BEND_RANGE       = 0x00,  /*!< Pitch bend range */
    ESP_MIDI_RPN_CHANNEL_FINE_TUNE      = 0x01,  /*!< Channel fine tune */
    ESP_MIDI_RPN_CHANNEL_COARSE_TUNE    = 0x02,  /*!< Channel coarse tune */
    ESP_MIDI_RPN_TUNING_PROGRAM_CHANGE  = 0x03,  /*!< Tuning program change */
    ESP_MIDI_RPN_TUNING_BANK_SELECT     = 0x04,  /*!< Tuning bank select */
    ESP_MIDI_RPN_MODULATION_DEPTH_RANGE = 0x05,  /*!< Modulation depth range */
} esp_midi_rpn_type_t;

/**
 * @brief  Meta message type.A meta event is a special type of MIDI event that carries metadata information instead of musical note data.
 *         vents contain information about the MIDI file itself, such as track tags, copyright information, and time signatures.
 */
typedef enum {
    ESP_MIDI_META_TYPE_SEQ             = 0x01,  /*<!Sequence Number */
    ESP_MIDI_META_TYPE_TEXT            = 0x01,  /*<!Text Event */
    ESP_MIDI_META_TYPE_COPYRIGHT       = 0x02,  /*<!Copyright Notice */
    ESP_MIDI_META_TYPE_TRACK_NAME      = 0x03,  /*<!Sequence/Track Name */
    ESP_MIDI_META_TYPE_INST_NAME       = 0x04,  /*<!Instrument Name */
    ESP_MIDI_META_TYPE_LYRIC           = 0x05,  /*<!Lyric */
    ESP_MIDI_META_TYPE_MARKER          = 0x06,  /*<!Marker */
    ESP_MIDI_META_TYPE_CUE_POINT       = 0x07,  /*<!Cue Point */
    ESP_MIDI_META_TYPE_CHANNEL_PREFIX  = 0x20,  /*<!MIDI Channel Prefix */
    ESP_MIDI_META_TYPE_EOT             = 0x2f,  /*<!End of Track */
    ESP_MIDI_META_TYPE_SET_TEMPO       = 0x51,  /*<!Set Tempo (in microseconds per MIDI quarter-note)*/
    ESP_MIDI_META_TYPE_SMPTE_OFFSET    = 0x54,  /*<!SMPTE Offset */
    ESP_MIDI_META_TYPE_TIME_SIGNATURE  = 0x58,  /*<!Time Signature */
    ESP_MIDI_META_TYPE_KEY_SIGNATURE   = 0x59,  /*<!Key Signature */
    ESP_MIDI_META_TYPE_SEQUENCER_EVENT = 0x7f   /*<!Sequencer Specific Meta-Event */
} esp_midi_meta_msg_type_t;

/**
 * @brief  MIDI Meta Message structure.
 *         This structure represents a MIDI meta message, which is a special type of
 *         MIDI message that contains additional data not directly related to musical
 *         performance. Meta messages are used for various purposes, such as setting
 *         the MIDI sequence number, tempo, time signature, and so on.
 */
typedef struct {
    esp_midi_meta_msg_type_t  type;  /*!< Meta message type */
    uint32_t                  len;   /*!< Represents the size of the data */
    uint8_t                  *data;  /*!< A pointer to store data */
} esp_midi_meta_msg_t;

/**
 * @brief  MIDI Channel Message structure.
 *         This structure represents a MIDI channel message, which is a type of MIDI
 *         message that contains information about a musical note or control signal
 *         sent to a specific MIDI channel. Channel messages are used for various
 *         purposes, such as playing notes, changing velocities, and sending control
 *         signals to MIDI devices.
 *
 */
typedef struct {
    uint8_t  channel;   /*!< The index of the MIDI channel */
    uint8_t  note;      /*!< The index of the musical note */
    uint8_t  velocity;  /*!< The velocity (or intensity) of the note */
} esp_midi_ch_msg_t;

/**
 * @brief  Program change message
 *         | Instrument Name          | MIDI Code |
 *         |--------------------------|-----------|
 *         | Acoustic Grand Piano     | 0         |
 *         | Bright Acoustic Piano    | 1         |
 *         | Electric Grand Piano     | 2         |
 *         | Honky-tonk Piano         | 3         |
 *         | Electric Piano 1         | 4         |
 *         | Electric Piano 2         | 5         |
 *         | Harpsichord              | 6         |
 *         | Clavinet                 | 7         |
 *         | Celesta                  | 8         |
 *         | Glockenspiel             | 9         |
 *         | Music Box                | 10        |
 *         | Vibraphone               | 11        |
 *         | Marimba                  | 12        |
 *         | Xylophone                | 13        |
 *         | Tubular Bells            | 14        |
 *         | Dulcimer                 | 15        |
 *         | Drawbar Organ            | 16        |
 *         | Percussive Organ         | 17        |
 *         | Rock Organ               | 18        |
 *         | Church Organ             | 19        |
 *         | Reed Organ               | 20        |
 *         | Accordion                | 21        |
 *         | Harmonica                | 22        |
 *         | Tango Accordion          | 23        |
 *         | Acoustic Guitar (nylon)  | 24        |
 *         | Acoustic Guitar (steel)  | 25        |
 *         | Electric Guitar (jazz)   | 26        |
 *         | Electric Guitar (clean)  | 27        |
 *         | Electric Guitar (muted)  | 28        |
 *         | Overdriven Guitar        | 29        |
 *         | Distortion Guitar        | 30        |
 *         | Guitar Harmonics         | 31        |
 *         | Acoustic Bass            | 32        |
 *         | Electric Bass (finger)   | 33        |
 *         | Electric Bass (pick)     | 34        |
 *         | Fretless Bass            | 35        |
 *         | Slap Bass 1              | 36        |
 *         | Slap Bass 2              | 37        |
 *         | Synth Bass 1             | 38        |
 *         | Synth Bass 2             | 39        |
 *         | Violin                   | 40        |
 *         | Viola                    | 41        |
 *         | Cello                    | 42        |
 *         | Contrabass               | 43        |
 *         | Tremolo Strings          | 44        |
 *         | Pizzicato Strings        | 45        |
 *         | Orchestral Harp          | 46        |
 *         | Timpani                  | 47        |
 *         | String Ensemble 1        | 48        |
 *         | String Ensemble 2        | 49        |
 *         | SynthStrings 1           | 50        |
 *         | SynthStrings 2           | 51        |
 *         | Choir Aahs               | 52        |
 *         | Voice Oohs               | 53        |
 *         | Synth Voice              | 54        |
 *         | Orchestra Hit            | 55        |
 *         | Trumpet                  | 56        |
 *         | Trombone                 | 57        |
 *         | Tuba                     | 58        |
 *         | Muted Trumpet            | 59        |
 *         | French Horn              | 60        |
 *         | Brass Section            | 61        |
 *         | SynthBrass 1             | 62        |
 *         | SynthBrass 2             | 63        |
 *         | Soprano Sax              | 64        |
 *         | Alto Sax                 | 65        |
 *         | Tenor Sax                | 66        |
 *         | Baritone Sax             | 67        |
 *         | Oboe                     | 68        |
 *         | English Horn             | 69        |
 *         | Bassoon                  | 70        |
 *         | Clarinet                 | 71        |
 *         | Piccolo                  | 72        |
 *         | Flute                    | 73        |
 *         | Recorder                 | 74        |
 *         | Pan Flute                | 75        |
 *         | Blown Bottle             | 76        |
 *         | Shakuhachi               | 77        |
 *         | Whistle                  | 78        |
 *         | Ocarina                  | 79        |
 *         | Lead 1 (square)          | 80        |
 *         | Lead 2 (sawtooth)        | 81        |
 *         | Lead 3 (calliope)        | 82        |
 *         | Lead 4 (chiff)           | 83        |
 *         | Lead 5 (charang)         | 84        |
 *         | Lead 6 (voice)           | 85        |
 *         | Lead 7 (fifths)          | 86        |
 *         | Lead 8 (bass + lead)     | 87        |
 *         | Pad 1 (new age)          | 88        |
 *         | Pad 2 (warm)             | 89        |
 *         | Pad 3 (polysynth)        | 90        |
 *         | Pad 4 (choir)            | 91        |
 *         | Pad 5 (bowed)            | 92        |
 *         | Pad 6 (metallic)         | 93        |
 *         | Pad 7 (halo)             | 94        |
 *         | Pad 8 (sweep)            | 95        |
 *         | FX 1 (rain)              | 96        |
 *         | FX 2 (soundtrack)        | 97        |
 *         | FX 3 (crystal)           | 98        |
 *         | FX 4 (atmosphere)        | 99        |
 *         | FX 5 (brightness)        | 100       |
 *         | FX 6 (goblins)           | 101       |
 *         | FX 7 (echoes)            | 102       |
 *         | FX 8 (sci-fi)            | 103       |
 *         | Sitar                    | 104       |
 *         | Banjo                    | 105       |
 *         | Shamisen                 | 106       |
 *         | Koto                     | 107       |
 *         | Kalimba                  | 108       |
 *         | Bagpipe                  | 109       |
 *         | Fiddle                   | 110       |
 *         | Shanai                   | 111       |
 *         | Tinkle Bell              | 112       |
 *         | Agogo                    | 113       |
 *         | Steel Drums              | 114       |
 *         | Woodblock                | 115       |
 *         | Taiko Drum               | 116       |
 *         | Melodic Tom              | 117       |
 *         | Synth Drum               | 118       |
 *         | Reverse Cymbal           | 119       |
 *         | Guitar Fret Noise        | 120       |
 *         | Breath Noise             | 121       |
 *         | Seashore                 | 122       |
 *         | Bird Tweet               | 123       |
 *         | Telephone Ring           | 124       |
 *         | Helicopter               | 125       |
 *         | Applause                 | 126       |
 *         | Gunshot                  | 127       |
 */
typedef struct
{
    uint8_t  channel;         /*!< Channel index */
    uint8_t  program_change;  /*!< program change */
} esp_midi_program_change_msg_t;

/**
 * @brief  Channel pressure message structure
 *         This structure represents a MIDI channel pressure message, which indicates
 *         the amount of pressure applied to the keyboard for a specific channel.
 */
typedef struct
{
    uint8_t  channel;           /*!< Channel index: Identifies which MIDI channel the message applies to. */
    uint8_t  channel_pressure;  /*!< Channel pressure: The pressure value, typically ranging from 0 to 127. */
} esp_midi_channel_pressure_msg_t;

/**
 * @brief  Pitch message structure
 *         This structure represents a MIDI pitch message, which indicates the pitch
 *         bend value for a specific channel.
 */
typedef struct
{
    uint8_t   channel;  /*!< Channel index: Identifies which MIDI channel the message applies to. */
    uint16_t  pitch;    /*!< Pitch value: The pitch bend value, typically a 14-bit number ranging from 0 to 16383. */
} esp_midi_pitch_msg_t;

/**
 * @brief  MIDI event message structure
 *         This structure represents a MIDI event message, which can be of various types
 *         such as note on/off, control change, program change, etc. It uses a union to
 *         store different types of MIDI messages.
 */
typedef struct {
    uint32_t  message_len;  /*<! The length of message */
    union {
        esp_midi_ch_msg_t                note_msg;         /*<! Channel information message */
        esp_midi_pitch_msg_t             pitch_msg;        /*<! Pitch message */
        esp_midi_channel_pressure_msg_t  ch_pressure_msg;  /*<! Channel pressure message*/
        esp_midi_program_change_msg_t    prog_ch_msg;      /*<! Program change message*/
        esp_midi_meta_msg_t              meta_msg;         /*<! Meta message */
        uint8_t                         *message;          /*<! Others message */
    };
} esp_midi_event_msg_t;

/**
 * @brief  MIDI event type enumeration
 *         This enumeration lists all possible MIDI event types, including channel
 *         messages, system common messages, system real-time messages, and meta events.
 */
typedef enum {
    /* Channel messages */
    ESP_MIDI_EVENT_TYPE_NOTE_OFF         = 0x80,  /*!< Note off: Sent when a key is released. */
    ESP_MIDI_EVENT_TYPE_NOTE_ON          = 0x90,  /*!< Note on: Sent when a key is pressed. */
    ESP_MIDI_EVENT_TYPE_KEY_PRESSURE     = 0xa0,  /*!< Key pressure: Sent when pressure is applied to a key after it is pressed. */
    ESP_MIDI_EVENT_TYPE_CONTROL_CHANGE   = 0xb0,  /*!< Control change: Sent when a controller value changes. */
    ESP_MIDI_EVENT_TYPE_PROGRAM_CHANGE   = 0xc0,  /*!< Program change: Sent when the patch number (instrument) changes. */
    ESP_MIDI_EVENT_TYPE_CHANNEL_PRESSURE = 0xd0,  /*!< Channel pressure: Sent when pressure is applied to any key, indicating the maximum pressure. */
    ESP_MIDI_EVENT_TYPE_PITCH_BEND       = 0xe0,  /*!< Pitch bend: Sent to indicate a change in the pitch wheel. */
    ESP_MIDI_EVENT_TYPE_SYSEX            = 0xf0,  /*!< System exclusive: Used for manufacturer-specific messages. */
    /* System common messages (not stored in MIDI files) */
    ESP_MIDI_EVENT_TYPE_TIME_CODE        = 0xf1,  /*!< Time code: Used for MIDI time code. */
    ESP_MIDI_EVENT_TYPE_SONG_POSITION    = 0xf2,  /*!< Song position pointer: Indicates the current position in the song. */
    ESP_MIDI_EVENT_TYPE_SONG_SELECT      = 0xf3,  /*!< Song select: Specifies which song or sequence to play. */
    ESP_MIDI_EVENT_TYPE_TUNE_REQUEST     = 0xf6,  /*!< Tune request: Requests all analog synthesizers to tune their oscillators. */
    ESP_MIDI_EVENT_TYPE_EOX              = 0xf7,  /*!< End of exclusive: Used to terminate a system exclusive message. */
    /* System real-time messages (not stored in MIDI files) */
    ESP_MIDI_EVENT_TYPE_SYNC             = 0xf8,  /*!< Timing clock: Sent 24 times per quarter note for synchronization. */
    ESP_MIDI_EVENT_TYPE_TICK             = 0xf9,  /*!< Tick: Used for timing purposes. */
    ESP_MIDI_EVENT_TYPE_START            = 0xfa,  /*!< Start: Starts the current sequence playing. */
    ESP_MIDI_EVENT_TYPE_CONTINUE         = 0xfb,  /*!< Continue: Continues playing the sequence from where it was stopped. */
    ESP_MIDI_EVENT_TYPE_STOP             = 0xfc,  /*!< Stop: Stops the current sequence. */
    ESP_MIDI_EVENT_TYPE_ACTIVE_SENSING   = 0xfe,  /*!< Active sensing: Used to ensure the connection is still active. */
    ESP_MIDI_EVENT_TYPE_SYSTEM_RESET     = 0xff,  /*!< Reset: Resets all receivers in the system. */
    /* Meta event (for MIDI files only) */
    ESP_MIDI_EVENT_TYPE_META_EVENT       = 0xff,  /*!< Meta event: Used for meta-information in MIDI files. */
} esp_midi_event_type_t;

/**
 * @brief  MIDI event structure for parsing
 *         This structure represents a parsed MIDI event, including the event type,
 *         the associated message, and the delta time (in ticks) since the previous event.
 */
typedef struct midi_event {
    struct midi_event     *next;         /*!< Next node: Pointer to the next event in the linked list. */
    esp_midi_event_type_t  type;         /*!< Event type: Indicates the type of the MIDI event. */
    esp_midi_event_msg_t   msg;          /*!< Event message: Contains the data associated with the MIDI event. */
    uint32_t               delta_ticks;  /*!< Delta ticks: The time difference (in ticks) between this event and the previous one. */
} esp_midi_parse_event_t;
