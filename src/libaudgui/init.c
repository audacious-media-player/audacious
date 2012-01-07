/*
 * init.c
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

#include <audacious/misc.h>
#include <audacious/playlist.h>
#include <libaudcore/hook.h>

#include "init.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static const char * const audgui_defaults[] = {
 "close_dialog_add", "FALSE",
 "close_dialog_open", "TRUE",
 "close_jtf_dialog", "TRUE",
 "playlist_manager_close_on_activate", "FALSE",
 "remember_jtf_entry", "TRUE",
 NULL};

AudAPITable * _aud_api_table = NULL;

static void playlist_set_playing_cb (void * unused, void * unused2)
{
    audgui_pixbuf_uncache ();
}

static void playlist_position_cb (void * list, void * unused)
{
    if (GPOINTER_TO_INT (list) == aud_playlist_get_playing ())
        audgui_pixbuf_uncache ();
}

void audgui_init (AudAPITable * table)
{
    _aud_api_table = table;
    aud_config_set_defaults ("audgui", audgui_defaults);

    hook_associate ("playlist set playing", playlist_set_playing_cb, NULL);
    hook_associate ("playlist position", playlist_position_cb, NULL);
}

void audgui_cleanup (void)
{
    hook_dissociate ("playlist set playing", playlist_set_playing_cb);
    hook_dissociate ("playlist position", playlist_position_cb);

    audgui_hide_equalizer_window ();
    audgui_jump_to_track_hide ();
    audgui_pixbuf_uncache ();
    audgui_playlist_manager_cleanup ();
    audgui_queue_manager_cleanup ();

    _aud_api_table = NULL;
}
