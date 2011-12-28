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

/* The strings returned by drct_get_filename() and drct_get_title() are pooled
 * and must be freed with str_unref(). */

AUD_VFUNC0 (drct_play)
AUD_VFUNC0 (drct_pause)
AUD_VFUNC0 (drct_stop)
AUD_FUNC0 (bool_t, drct_get_playing)
AUD_FUNC0 (bool_t, drct_get_ready)
AUD_FUNC0 (bool_t, drct_get_paused)
AUD_FUNC0 (char *, drct_get_filename)
AUD_FUNC0 (char *, drct_get_title)
AUD_VFUNC3 (drct_get_info, int *, bitrate, int *, samplerate, int *, channels)
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

/* The indexes passed to drct_pl_add_list(), drct_pl_open_list(), and
 * drct_pl_open_temp_list() contain pooled strings to which the caller gives up
 * one reference.  The indexes themselves are freed by these functions. */

AUD_VFUNC0 (drct_pl_next)
AUD_VFUNC0 (drct_pl_prev)

AUD_VFUNC2 (drct_pl_add, const char *, filename, int, at)
AUD_VFUNC2 (drct_pl_add_list, Index *, filenames, int, at)
AUD_VFUNC1 (drct_pl_open, const char *, filename)
AUD_VFUNC1 (drct_pl_open_list, Index *, filenames)
AUD_VFUNC1 (drct_pl_open_temp, const char *, filename)
AUD_VFUNC1 (drct_pl_open_temp_list, Index *, filenames)

AUD_VFUNC1 (drct_pl_delete_selected, int, playlist)
