/*
 * input.h
 * Copyright 2013 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#ifndef LIBAUDCORE_INPUT_H
#define LIBAUDCORE_INPUT_H

#include <libaudcore/tuple.h>

/* These functions are to be used only from the play() function of an input plugin. */

struct ReplayGainInfo {
    float track_gain; /* dB */
    float track_peak; /* 0-1 */
    float album_gain; /* dB */
    float album_peak; /* 0-1 */
};

/* Prepares the output system for playback in the specified format.  Returns
 * true on success, false if the selected format is not supported.  Also
 * triggers the "playback ready" hook.  Hence, if you call aud_input_set_gain,
 * aud_input_set_tuple, or aud_input_set_bitrate, consider doing so before
 * calling aud_input_open_audio. */
bool aud_input_open_audio (int format, int rate, int channels);

/* Informs the output system of replay gain values for the current song so
 * that volume levels can be adjusted accordingly, if the user so desires.
 * This may be called at any time during playback should the values change. */
void aud_input_set_gain (const ReplayGainInfo * info);

/* Passes audio data to the output system for playback.  The data must be in
 * the format passed to open_audio, and the length (in bytes) must be an
 * integral number of frames.  This function blocks until all the data has
 * been written (though it may not yet be heard by the user). */
void aud_input_write_audio (const void * data, int length);

/* Returns the current tuple for the stream. */
Tuple aud_input_get_tuple (void);

/* Updates the tuple for the stream. */
void aud_input_set_tuple (Tuple && tuple);

/* Updates the displayed bitrate, in bits per second. */
void aud_input_set_bitrate (int bitrate);

/* Checks whether playback is to be stopped.  The play() function should poll
 * check_stop() periodically and return as soon as check_stop() returns true. */
bool aud_input_check_stop (void);

/* Checks whether a seek has been requested.  If so, discards any buffered audio
 * and returns the position to seek to, in milliseconds.  Otherwise, returns -1. */
int aud_input_check_seek (void);

#endif
