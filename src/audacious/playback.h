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

/* for use from playback.c and playlist-new.c ONLY */
/* anywhere else, use drct_* and/or playlist_* functions */
void playback_play (int seek_time, bool_t pause);
void playback_stop (void);

#endif /* AUDACIOUS_PLAYBACK_H */
