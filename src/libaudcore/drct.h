/*
 * drct.h
 * Copyright 2010-2012 John Lindgren
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

#ifndef LIBAUDCORE_DRCT_H
#define LIBAUDCORE_DRCT_H

#include <libaudcore/audio.h>
#include <libaudcore/index.h>
#include <libaudcore/tuple.h>

class PluginHandle;

/* CAUTION: These functions are not thread safe. */

/* --- PLAYBACK CONTROL --- */

void aud_drct_play ();
void aud_drct_play_pause ();
void aud_drct_pause ();
void aud_drct_stop ();
bool aud_drct_get_playing ();
bool aud_drct_get_ready ();
bool aud_drct_get_paused ();

// returns entry number of playing song (zero-based)
int aud_drct_get_position ();

// returns filename of playing song
String aud_drct_get_filename ();

// returns formatted title of playing song
// connect to the "title change" hook to be notified of changes
String aud_drct_get_title ();

// returns metadata of playing song
// connect to the "tuple change" hook to be notified of changes
Tuple aud_drct_get_tuple ();

// returns some statistics of playing song
// connect to the "info change" hook to be notified of changes
void aud_drct_get_info (int & bitrate, int & samplerate, int & channels);

int aud_drct_get_time ();
int aud_drct_get_length ();
void aud_drct_seek (int time);

/* "A-B repeat": when playback reaches point B, it returns to point A (where A
 * and B are in milliseconds).  The value -1 is interpreted as the beginning of
 * the song (for A) or the end of the song (for B).  A-B repeat is disabled
 * entirely by setting both A and B to -1. */
void aud_drct_set_ab_repeat (int a, int b);
void aud_drct_get_ab_repeat (int & a, int & b);

/* --- RECORDING CONTROL --- */

/* Returns the output plugin that will be used for recording, or null if none is
 * available.  Connect to the "enable record" hook to monitor changes. */
PluginHandle * aud_drct_get_record_plugin ();

/* Returns true if output recording is enabled, otherwise false.  Connect to the
 * "enable record" hook to monitor changes. */
bool aud_drct_get_record_enabled ();

/* Enables or disables output recording.  If playback is active, recording
 * begins immediately.  Returns true on success, otherwise false. */
bool aud_drct_enable_record (bool enable);

/* --- VOLUME CONTROL --- */

StereoVolume aud_drct_get_volume ();
void aud_drct_set_volume (StereoVolume volume);
int aud_drct_get_volume_main ();
void aud_drct_set_volume_main (int volume);
int aud_drct_get_volume_balance ();
void aud_drct_set_volume_balance (int balance);

/* --- PLAYLIST CONTROL --- */

void aud_drct_pl_next ();
void aud_drct_pl_prev ();

void aud_drct_pl_add (const char * filename, int at);
void aud_drct_pl_add_list (Index<PlaylistAddItem> && items, int at);
void aud_drct_pl_open (const char * filename);
void aud_drct_pl_open_list (Index<PlaylistAddItem> && items);
void aud_drct_pl_open_temp (const char * filename);
void aud_drct_pl_open_temp_list (Index<PlaylistAddItem> && items);

#endif
