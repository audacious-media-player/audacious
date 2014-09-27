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

/* CAUTION: These functions are not thread safe. */

/* --- PLAYBACK CONTROL --- */

void aud_drct_play (void);
void aud_drct_play_pause (void);
void aud_drct_play_playlist (int playlist);
void aud_drct_pause (void);
void aud_drct_stop (void);
bool aud_drct_get_playing (void);
bool aud_drct_get_ready (void);
bool aud_drct_get_paused (void);
String aud_drct_get_filename (void);
String aud_drct_get_title (void);
void aud_drct_get_info (int * bitrate, int * samplerate, int * channels);
int aud_drct_get_time (void);
int aud_drct_get_length (void);
void aud_drct_seek (int time);

/* "A-B repeat": when playback reaches point B, it returns to point A (where A
 * and B are in milliseconds).  The value -1 is interpreted as the beginning of
 * the song (for A) or the end of the song (for B).  A-B repeat is disabled
 * entirely by setting both A and B to -1. */
void aud_drct_set_ab_repeat (int a, int b);
void aud_drct_get_ab_repeat (int * a, int * b);

/* --- VOLUME CONTROL --- */

StereoVolume aud_drct_get_volume ();
void aud_drct_set_volume (StereoVolume volume);
int aud_drct_get_volume_main ();
void aud_drct_set_volume_main (int volume);
int aud_drct_get_volume_balance ();
void aud_drct_set_volume_balance (int balance);

/* --- PLAYLIST CONTROL --- */

/* The indexes passed to drct_pl_add_list(), drct_pl_open_list(), and
 * drct_pl_open_temp_list() contain pooled strings to which the caller gives up
 * one reference.  The indexes themselves are freed by these functions. */

void aud_drct_pl_next (void);
void aud_drct_pl_prev (void);

void aud_drct_pl_add (const char * filename, int at);
void aud_drct_pl_add_list (Index<PlaylistAddItem> && items, int at);
void aud_drct_pl_open (const char * filename);
void aud_drct_pl_open_list (Index<PlaylistAddItem> && items);
void aud_drct_pl_open_temp (const char * filename);
void aud_drct_pl_open_temp_list (Index<PlaylistAddItem> && items);

#endif
