/*
 * init.c
 * Copyright 2010-2013 John Lindgren
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

#include <assert.h>
#include <stdlib.h>

#include <libaudcore/audstrings.h>
#include <libaudcore/hook.h>
#include <libaudcore/playlist.h>
#include <libaudcore/plugins.h>
#include <libaudcore/runtime.h>

#include "internal.h"
#include "libaudgui.h"
#include "libaudgui-gtk.h"

static const char * const audgui_defaults[] = {
    "clear_song_fields", "TRUE",
    "close_dialog_add", "FALSE",
    "close_dialog_open", "TRUE",
    "close_jtf_dialog", "TRUE",
    "record", "FALSE",
    "remember_jtf_entry", "TRUE",
    nullptr
};

static const char * const window_names[AUDGUI_NUM_UNIQUE_WINDOWS] = {
    "about_win",
    "equalizer_win",
    "eq_preset_win",
    "filebrowser_win",
    nullptr, /* infopopup position is not saved */
    "info_win",
    "jump_to_time_win",
    "jump_to_track_win",
    "playlist_export_win",
    "playlist_import_win",
    "preset_browser_win",
    "queue_manager_win",
    "url_opener_win"
};

static int init_count = 0;

static GtkWidget * windows[AUDGUI_NUM_UNIQUE_WINDOWS];

static gboolean configure_cb (GtkWidget * window, GdkEventConfigure * event, const char * name)
{
    if (gtk_widget_get_visible (window))
    {
        int pos[4];
        gtk_window_get_position ((GtkWindow *) window, & pos[0], & pos[1]);
        gtk_window_get_size ((GtkWindow *) window, & pos[2], & pos[3]);

        pos[2] = audgui_to_portable_dpi (pos[2]);
        pos[3] = audgui_to_portable_dpi (pos[3]);

        aud_set_str ("audgui", name, int_array_to_str (pos, 4));
    }

    return false;
}

void audgui_show_unique_window (int id, GtkWidget * widget)
{
    g_return_if_fail (id >= 0 && id < AUDGUI_NUM_UNIQUE_WINDOWS);

    if (windows[id])
        gtk_widget_destroy (windows[id]);

    windows[id] = widget;
    g_signal_connect (widget, "destroy", (GCallback) gtk_widget_destroyed, & windows[id]);

    if (window_names[id])
    {
        String str = aud_get_str ("audgui", window_names[id]);
        int pos[4];

        if (str_to_int_array (str, pos, 4))
        {
            pos[2] = audgui_to_native_dpi (pos[2]);
            pos[3] = audgui_to_native_dpi (pos[3]);

            gtk_window_move ((GtkWindow *) widget, pos[0], pos[1]);
            gtk_window_set_default_size ((GtkWindow *) widget, pos[2], pos[3]);
        }

        g_signal_connect (widget, "configure-event", (GCallback) configure_cb,
         (void *) window_names[id]);
    }

    gtk_widget_show_all (widget);
}

bool audgui_reshow_unique_window (int id)
{
    g_return_val_if_fail (id >= 0 && id < AUDGUI_NUM_UNIQUE_WINDOWS, false);

    if (! windows[id])
        return false;

    gtk_window_present ((GtkWindow *) windows[id]);
    return true;
}

void audgui_hide_unique_window (int id)
{
    g_return_if_fail (id >= 0 && id < AUDGUI_NUM_UNIQUE_WINDOWS);

    if (windows[id])
        gtk_widget_destroy (windows[id]);
}

static void playlist_set_playing_cb (void *, void *)
{
    audgui_pixbuf_uncache ();
}

static void playlist_position_cb (void * list, void *)
{
    if (aud::from_ptr<int> (list) == aud_playlist_get_playing ())
        audgui_pixbuf_uncache ();
}

EXPORT void audgui_init ()
{
    assert (aud_get_mainloop_type () == MainloopType::GLib);

    if (init_count ++)
        return;

    gtk_init (nullptr, nullptr);

    aud_config_set_defaults ("audgui", audgui_defaults);

    record_init ();
    status_init ();

    hook_associate ("playlist set playing", playlist_set_playing_cb, nullptr);
    hook_associate ("playlist position", playlist_position_cb, nullptr);

#ifndef _WIN32
    gtk_window_set_default_icon_name ("audacious");
#endif
}

EXPORT void audgui_cleanup ()
{
    if (-- init_count)
        return;

    hook_dissociate ("playlist set playing", playlist_set_playing_cb);
    hook_dissociate ("playlist position", playlist_position_cb);

    record_cleanup ();
    status_cleanup ();

    for (int id = 0; id < AUDGUI_NUM_UNIQUE_WINDOWS; id ++)
        audgui_hide_unique_window (id);

    audgui_hide_prefs_window ();
    audgui_infopopup_hide ();

    plugin_menu_cleanup ();
    plugin_prefs_cleanup ();
}
