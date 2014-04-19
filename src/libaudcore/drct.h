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

#include <libaudcore/core.h>
#include <libaudcore/index.h>

/* CAUTION: These functions are not thread safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* --- PLAYBACK CONTROL --- */

/* The strings returned by drct_get_filename() and drct_get_title() are pooled
 * and must be freed with str_unref(). */

void aud_drct_play (void);
void aud_drct_play_pause (void);
void aud_drct_play_playlist (int playlist);
void aud_drct_pause (void);
void aud_drct_stop (void);
bool_t aud_drct_get_playing (void);
bool_t aud_drct_get_ready (void);
bool_t aud_drct_get_paused (void);
char * aud_drct_get_filename (void);
char * aud_drct_get_title (void);
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

void aud_drct_get_volume (int * left, int * right);
void aud_drct_set_volume (int left, int right);
void aud_drct_get_volume_main (int * volume);
void aud_drct_set_volume_main (int volume);
void aud_drct_get_volume_balance (int * balance);
void aud_drct_set_volume_balance (int balance);

/* --- PLAYLIST CONTROL --- */

/* The indexes passed to drct_pl_add_list(), drct_pl_open_list(), and
 * drct_pl_open_temp_list() contain pooled strings to which the caller gives up
 * one reference.  The indexes themselves are freed by these functions. */

void aud_drct_pl_next (void);
void aud_drct_pl_prev (void);

void aud_drct_pl_add (const char * filename, int at);
void aud_drct_pl_add_list (Index * filenames, int at);
void aud_drct_pl_open (const char * filename);
void aud_drct_pl_open_list (Index * filenames);
void aud_drct_pl_open_temp (const char * filename);
void aud_drct_pl_open_temp_list (Index * filenames);

#ifdef __cplusplus
}
#endif

#endif
