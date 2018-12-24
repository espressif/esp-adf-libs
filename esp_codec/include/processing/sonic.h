/*
*------------------------------------------------------------------
*Copyright 2010
* Bill Cox
* This file is part of the Sonic Library.
* This file is licensed under the Apache 2.0 license.
*------------------------------------------------------------------
*/

#ifndef __ESP_CHANGE_VOICE_H
#define __ESP_CHANGE_VOICE_H

/* For all of the following functions, numChannels is multiplied by numSamples
to determine the actual number of values read or returned. */

/**
 * @brief      Create a sonic stream
 *
 * @param      sampleRate   The sample rate of the stream
 * @param      numChannels  The number channel(s) of the stream (1 : mono,  2 : dual)
 *
 * @return     allocate the stream.  NULL only if we are out of memory and cannot  
 */
void* changeVoiceCreateStream(int sampleRate, int numChannels);

/**
 * @brief      Set the resample method
 *
 * @param      handle                               The handle of the sonic stream
 * @param      resample_linear_interpolate          1 for linear interpolation, faster but lower accuracy
 */
void changeVoiceSetResampleMode(void* handle, int resample_linear_interpolate);

/**
 * @brief      Set the speed of the stream
 *
 * @param      handle        The handle of the sonic stream
 * @param      speed         The scaling factor of speed of the stream
 */
void changeVoiceSetSpeed(void* handle, float speed);

/**
 * @brief      Set the pitch of the stream
 *
 * @param      handle        The handle of the sonic stream
 * @param      pitch         The scaling factor of pitch of the stream
 */
void changeVoiceSetPitch(void* handle, float pitch);

/**
 * @brief      Set the rate of the stream
 *
 * @param      handle        The handle of the sonic stream
 * @param      rate          The rate of the stream
 */
void changeVoiceSetRate(void* handle, float rate);

/**
 * @brief      Set the scaling factor of the stream
 *
 * @param      handle        The handle of the sonic stream
 * @param      volume        The scaling factor of volume of the stream
 */
void changeVoiceSetVolume(void* handle, float volume);

/**
 * @brief      Set chord pitch mode on or off.
 *
 * @param      handle          The handle of the sonic stream
 * @param      useChordPitch   Default is off.
 */
void changeVoiceSetChordPitch(void* handle, int useChordPitch);

/**
 * @brief      Set the "quality"
 *
 * @param      handle        The handle of the sonic stream
 * @param      quality       Default 0 is virtually as good as 1, but very much faster
 */
void changeVoiceSetQuality(void* handle, int quality);

/**
 * @brief      Force the sonic stream to generate output using whatever data it currently
 *             has.  No extra delay will be added to the output, but flushing in the middle
 *             of words could introduce distortion. 
 *
 * @param      handle         The handle of the sonic stream
 */
int changeVoiceFlushStream(void* handle);

/**
 * @brief      Use this to write 16-bit data to be speed up or down into the stream
 *
 * @param      handle         The handle of the sonic stream
 * @param      samples        The buffer of output stream
 * @param      numSamples     The length of output stream
 *
 * @return     0 if memory realloc failed, otherwise 1  
 */
int changeVoiceWriteToStream(void* handle, short* samples, int numSamples);

/**
 * @brief      Use this to read 16-bit data out of the stream.  Sometimes no data will
 *             be available, and zero is returned, which is not an error condition.
 *
 * @param      handle         The handle of the sonic stream
 * @param      samples        The buffer of input stream 
 * @param      maxSamples     The maximum of the length of "samples"
 *
 * @return     allocate the stream.  NULL will be returned ,only if we are out of memory and cannot  
 */
int changeVoiceReadFromStream(void* handle, short* samples, int maxSamples);

/**
 * @brief      Destroy the sonic stream
 *
 * @param      handle         The handle of the sonic stream
 */
void changeVoiceDestroyStream(void* handle);

#endif