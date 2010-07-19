/*
 * effect.h
 * Copyright 2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 2 or version 3 of the License.
 *
 * Audacious is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * Audacious. If not, see <http://www.gnu.org/licenses/>.
 *
 * The Audacious team does not consider modular code linking to Audacious or
 * using our public API to be a derived work.
 */


#ifndef AUDACIOUS_EFFECT_H
#define AUDACIOUS_EFFECT_H

#include <glib.h>

void effect_start (gint * channels, gint * rate);
void effect_process (gfloat * * data, gint * samples);
void effect_flush (void);
void effect_finish (gfloat * * data, gint * samples);
gint effect_decoder_to_output_time (gint time);
gint effect_output_to_decoder_time (gint time);

#endif
