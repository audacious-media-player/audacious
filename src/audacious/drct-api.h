/*
 * drct-api.h
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

/* Do not include this file directly; use drct.h instead. */

/* CAUTION: These functions are not thread safe. */

/* --- PROGRAM CONTROL --- */

AUD_FUNC0 (void, drct_quit)

/* --- PLAYBACK CONTROL --- */

AUD_FUNC0 (void, drct_play)
AUD_FUNC0 (void, drct_pause)
AUD_FUNC0 (void, drct_stop)
AUD_FUNC0 (gboolean, drct_get_playing)
AUD_FUNC0 (gboolean, drct_get_paused)
AUD_FUNC0 (gchar *, drct_get_title)
AUD_FUNC3 (void, drct_get_info, gint *, bitrate, gint *, samplerate, gint *,
 channels)
AUD_FUNC0 (gint, drct_get_time)
AUD_FUNC0 (gint, drct_get_length)
AUD_FUNC1 (void, drct_seek, gint, time)

/* --- VOLUME CONTROL --- */

AUD_FUNC2 (void, drct_get_volume, gint *, left, gint *, right)
AUD_FUNC2 (void, drct_set_volume, gint, left, gint, right)
AUD_FUNC1 (void, drct_get_volume_main, gint *, volume)
AUD_FUNC1 (void, drct_set_volume_main, gint, volume)
AUD_FUNC1 (void, drct_get_volume_balance, gint *, balance)
AUD_FUNC1 (void, drct_set_volume_balance, gint, balance)

/* --- PLAYLIST CONTROL --- */

AUD_FUNC0 (gint, drct_pl_get_length)
AUD_FUNC0 (void, drct_pl_next)
AUD_FUNC0 (void, drct_pl_prev)
AUD_FUNC0 (gint, drct_pl_get_pos)
AUD_FUNC1 (void, drct_pl_set_pos, gint, pos)

AUD_FUNC0 (gboolean, drct_pl_repeat_is_enabled)
AUD_FUNC0 (void, drct_pl_repeat_toggle)
AUD_FUNC0 (gboolean, drct_pl_shuffle_is_enabled)
AUD_FUNC0 (void, drct_pl_shuffle_toggle)

AUD_FUNC1 (gchar *, drct_pl_get_file, gint, entry)
AUD_FUNC1 (gchar *, drct_pl_get_title, gint, entry)
AUD_FUNC1 (gint, drct_pl_get_time, gint, entry)

AUD_FUNC2 (void, drct_pl_add, const gchar *, filename, gint, at)
AUD_FUNC2 (void, drct_pl_add_list, GList *, list, gint, at)
AUD_FUNC1 (void, drct_pl_open, const gchar *, filename)
AUD_FUNC1 (void, drct_pl_open_list, GList *, list)
AUD_FUNC1 (void, drct_pl_open_temp, const gchar *, filename)
AUD_FUNC1 (void, drct_pl_open_temp_list, GList *, list)

AUD_FUNC1 (void, drct_pl_delete, gint, entry)
AUD_FUNC0 (void, drct_pl_clear)

/* --- PLAYLIST QUEUE CONTROL --- */

AUD_FUNC0 (gint, drct_pq_get_length)
AUD_FUNC1 (gint, drct_pq_get_entry, gint, queue_position)
AUD_FUNC1 (gboolean, drct_pq_is_queued, gint, entry)
AUD_FUNC1 (gint, drct_pq_get_queue_position, gint, entry)
AUD_FUNC1 (void, drct_pq_add, gint, entry)
AUD_FUNC1 (void, drct_pq_remove, gint, entry)
AUD_FUNC0 (void, drct_pq_clear)
