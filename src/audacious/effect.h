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

#include <libaudcore/core.h>

#include "types.h"

void effect_start (int * channels, int * rate);
void effect_process (float * * data, int * samples);
void effect_flush (void);
void effect_finish (float * * data, int * samples);
int effect_decoder_to_output_time (int time);
int effect_output_to_decoder_time (int time);

bool_t effect_plugin_start (PluginHandle * plugin);
void effect_plugin_stop (PluginHandle * plugin);

#endif
