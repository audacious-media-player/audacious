/*
 *  XMMS Crossfade Plugin
 *  Copyright (C) 2000-2004  Peter Eisenlohr <peter@eisenlohr.org>
 *
 *  based on the original OSS Output Plugin
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

/*
 *  Volume adjustment
 */

#ifndef __VOLUME_H__
#define __VOLUME_H__

#include "crossfade.h"
#include "format.h"

#include <sys/time.h>

typedef struct
{
  gboolean active;
  gint     target_rms;
  gint     song_rms;
  gfloat   factor;

  struct timeval tv_last;
  gint clips;
}
volume_context_t;

void volume_init(volume_context_t *cc);
void volume_flow(volume_context_t *cc, gpointer *buffer, gint length);
void volume_free(volume_context_t *cc);

void volume_set_active    (volume_context_t *cc, gboolean active);
void volume_set_target_rms(volume_context_t *cc, gint target_rms);
void volume_set_song_rms  (volume_context_t *cc, gint song_rms);

gfloat volume_compute_factor(gint percent, gint dB_range); /*compute factor corresponding to
                                                             attenuations in [- dB_range, 0]
                                                           */

#endif  /* _VOLUME_H_ */
