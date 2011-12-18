/*
 * vis_runner.h
 * Copyright 2009-2010 John Lindgren
 *
 * This file is part of Audacious.
 *
 * Audacious is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, version 3 of the License.
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

#ifndef AUD_VIS_RUNNER_H
#define AUD_VIS_RUNNER_H

#include <libaudcore/core.h>

/* When the decoder thread wants to send data to the vis runner, it must block
 * the vis timeout before blocking output functions; otherwise, the vis timeout
 * will hang up waiting for those output functions to be unblocked while the
 * decoder thread hangs up waiting for the vis timeout to finish. */
void vis_runner_lock (void);
void vis_runner_unlock (void);
bool_t vis_runner_locked (void);

void vis_runner_start_stop (bool_t playing, bool_t paused);
void vis_runner_pass_audio (int time, float * data, int samples, int
 channels, int rate);
void vis_runner_time_offset (int offset);
void vis_runner_flush (void);

void vis_runner_enable (bool_t enable);

#endif
