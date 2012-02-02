/*
 * playback.h
 * Copyright 2006-2011 William Pitcock and John Lindgren
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

#ifndef AUDACIOUS_PLAYBACK_H
#define AUDACIOUS_PLAYBACK_H

#include <libaudcore/core.h>

void playback_play (int seek_time, bool_t pause);
int playback_get_time(void);
void playback_pause(void);
void playback_stop(void);
bool_t playback_get_playing(void);
bool_t playback_get_ready (void);
bool_t playback_get_paused(void);
void playback_seek(int time);

char * playback_get_filename (void); /* pooled */
char * playback_get_title (void); /* pooled */
int playback_get_length (void);
void playback_get_info (int * bitrate, int * samplerate, int * channels);

void playback_get_volume (int * l, int * r);
void playback_set_volume (int l, int r);

#endif /* AUDACIOUS_PLAYBACK_H */
