/*
 * drct-api.h
 * Copyright 2010-2011 John Lindgren
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

/* Do not include this file directly; use drct.h instead. */

/* CAUTION: These functions are not thread safe. */

/* --- PROGRAM CONTROL --- */

AUD_VFUNC0 (drct_quit)

/* --- PLAYBACK CONTROL --- */

AUD_VFUNC0 (drct_play)
AUD_VFUNC0 (drct_pause)
AUD_VFUNC0 (drct_stop)
AUD_FUNC0 (boolean, drct_get_playing)
AUD_FUNC0 (boolean, drct_get_paused)
AUD_FUNC0 (char *, drct_get_title)
AUD_VFUNC3 (drct_get_info, int *, bitrate, int *, samplerate, int *,
 channels)
AUD_FUNC0 (int, drct_get_time)
AUD_FUNC0 (int, drct_get_length)
AUD_VFUNC1 (drct_seek, int, time)

/* --- VOLUME CONTROL --- */

AUD_VFUNC2 (drct_get_volume, int *, left, int *, right)
AUD_VFUNC2 (drct_set_volume, int, left, int, right)
AUD_VFUNC1 (drct_get_volume_main, int *, volume)
AUD_VFUNC1 (drct_set_volume_main, int, volume)
AUD_VFUNC1 (drct_get_volume_balance, int *, balance)
AUD_VFUNC1 (drct_set_volume_balance, int, balance)

/* --- PLAYLIST CONTROL --- */

AUD_FUNC0 (int, drct_pl_get_length)
AUD_VFUNC0 (drct_pl_next)
AUD_VFUNC0 (drct_pl_prev)
AUD_FUNC0 (int, drct_pl_get_pos)
AUD_VFUNC1 (drct_pl_set_pos, int, pos)

AUD_FUNC1 (char *, drct_pl_get_file, int, entry)
AUD_FUNC1 (char *, drct_pl_get_title, int, entry)
AUD_FUNC1 (int, drct_pl_get_time, int, entry)

AUD_VFUNC2 (drct_pl_add, const char *, filename, int, at)
AUD_VFUNC2 (drct_pl_add_list, GList *, list, int, at)
AUD_VFUNC1 (drct_pl_open, const char *, filename)
AUD_VFUNC1 (drct_pl_open_list, GList *, list)
AUD_VFUNC1 (drct_pl_open_temp, const char *, filename)
AUD_VFUNC1 (drct_pl_open_temp_list, GList *, list)

AUD_VFUNC1 (drct_pl_delete, int, entry)
AUD_VFUNC0 (drct_pl_delete_selected)
AUD_VFUNC0 (drct_pl_clear)

/* --- PLAYLIST QUEUE CONTROL --- */

AUD_FUNC0 (int, drct_pq_get_length)
AUD_FUNC1 (int, drct_pq_get_entry, int, queue_position)
AUD_FUNC1 (boolean, drct_pq_is_queued, int, entry)
AUD_FUNC1 (int, drct_pq_get_queue_position, int, entry)
AUD_VFUNC1 (drct_pq_add, int, entry)
AUD_VFUNC1 (drct_pq_remove, int, entry)
AUD_VFUNC0 (drct_pq_clear)

/* New in 2.5-alpha2 */
AUD_FUNC0 (boolean, drct_get_ready)
