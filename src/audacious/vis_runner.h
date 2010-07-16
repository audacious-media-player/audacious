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

void vis_runner_start_stop (gboolean playing, gboolean paused);
void vis_runner_pass_audio (gint time, gfloat * data, gint samples, gint
 channels, gint rate);
void vis_runner_time_offset (gint offset);
void vis_runner_flush (void);

#endif
