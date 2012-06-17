/*
 * drct-api.h
 * Copyright 2010-2011 John Lindgren
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

/* Do not include this file directly; use drct.h instead. */

/* CAUTION: These functions are not thread safe. */

/* --- PROGRAM CONTROL --- */

AUD_VFUNC0 (drct_quit)

/* --- PLAYBACK CONTROL --- */

/* The strings returned by drct_get_filename() and drct_get_title() are pooled
 * and must be freed with str_unref(). */

AUD_VFUNC0 (drct_play)
AUD_VFUNC1 (drct_play_playlist, int, playlist)
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
