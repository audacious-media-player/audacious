/*
 * dbus-server.c
 * Copyright 2013 John Lindgren
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

#include <stdio.h>

#include <libaudcore/drct.h>
#include <libaudcore/equalizer.h>
#include <libaudcore/interface.h>
#include <libaudcore/playlist.h>
#include <libaudcore/runtime.h>

#include "aud-dbus.h"
#include "main.h"

typedef ObjAudacious Obj;
typedef GDBusMethodInvocation Invoc;

#define FINISH(name) \
 obj_audacious_complete_##name (obj, invoc)

#define FINISH2(name, ...) \
 obj_audacious_complete_##name (obj, invoc, __VA_ARGS__)

static Index * strv_to_index (const char * const * strv)
{
    Index * index = index_new ();
    while (* strv)
        index_insert (index, -1, str_get (* strv ++));

    return index;
}

static bool_t do_add (Obj * obj, Invoc * invoc, const char * file)
{
    aud_playlist_entry_insert (aud_playlist_get_active (), -1, file, NULL, FALSE);
    FINISH (add);
    return TRUE;
}

static bool_t do_add_list (Obj * obj, Invoc * invoc, const char * const * filenames)
{
    aud_playlist_entry_insert_batch (aud_playlist_get_active (), -1,
     strv_to_index (filenames), NULL, FALSE);
    FINISH (add_list);
    return TRUE;
}

static bool_t do_add_url (Obj * obj, Invoc * invoc, const char * url)
{
    aud_playlist_entry_insert (aud_playlist_get_active (), -1, url, NULL, FALSE);
    FINISH (add_url);
    return TRUE;
}

static bool_t do_advance (Obj * obj, Invoc * invoc)
{
    aud_drct_pl_next ();
    FINISH (advance);
    return TRUE;
}

static bool_t do_auto_advance (Obj * obj, Invoc * invoc)
{
    FINISH2 (auto_advance, ! aud_get_bool (NULL, "no_playlist_advance"));
    return TRUE;
}

static bool_t do_balance (Obj * obj, Invoc * invoc)
{
    int balance;
    aud_drct_get_volume_balance (& balance);
    FINISH2 (balance, balance);
    return TRUE;
}

static bool_t do_clear (Obj * obj, Invoc * invoc)
{
    int playlist = aud_playlist_get_active ();
    aud_playlist_entry_delete (playlist, 0, aud_playlist_entry_count (playlist));
    FINISH (clear);
    return TRUE;
}

static bool_t do_delete (Obj * obj, Invoc * invoc, unsigned pos)
{
    aud_playlist_entry_delete (aud_playlist_get_active (), pos, 1);
    FINISH (delete);
    return TRUE;
}

static bool_t do_delete_active_playlist (Obj * obj, Invoc * invoc)
{
    aud_playlist_delete (aud_playlist_get_active ());
    FINISH (delete_active_playlist);
    return TRUE;
}

static bool_t do_eject (Obj * obj, Invoc * invoc)
{
    if (! aud_get_headless_mode ())
        aud_ui_show_filebrowser (TRUE);

    FINISH (eject);
    return TRUE;
}

static bool_t do_equalizer_activate (Obj * obj, Invoc * invoc, bool_t active)
{
    aud_set_bool (NULL, "equalizer_active", active);
    FINISH (equalizer_activate);
    return TRUE;
}

static bool_t do_get_active_playlist (Obj * obj, Invoc * invoc)
{
    FINISH2 (get_active_playlist, aud_playlist_get_active ());
    return TRUE;
}

static bool_t do_get_active_playlist_name (Obj * obj, Invoc * invoc)
{
    char * title = aud_playlist_get_title (aud_playlist_get_active ());
    FINISH2 (get_active_playlist_name, title ? title : "");
    str_unref (title);
    return TRUE;
}

static bool_t do_get_eq (Obj * obj, Invoc * invoc)
{
    double preamp = aud_get_double (NULL, "equalizer_preamp");
    double bands[AUD_EQ_NBANDS];
    aud_eq_get_bands (bands);

    GVariant * var = g_variant_new_fixed_array (G_VARIANT_TYPE_DOUBLE, bands,
     AUD_EQ_NBANDS, sizeof (double));
    FINISH2 (get_eq, preamp, var);
    return TRUE;
}

static bool_t do_get_eq_band (Obj * obj, Invoc * invoc, int band)
{
    FINISH2 (get_eq_band, aud_eq_get_band (band));
    return TRUE;
}

static bool_t do_get_eq_preamp (Obj * obj, Invoc * invoc)
{
    FINISH2 (get_eq_preamp, aud_get_double (NULL, "equalizer_preamp"));
    return TRUE;
}

static bool_t do_get_info (Obj * obj, Invoc * invoc)
{
    int bitrate, samplerate, channels;
    aud_drct_get_info (& bitrate, & samplerate, & channels);
    FINISH2 (get_info, bitrate, samplerate, channels);
    return TRUE;
}

static bool_t do_get_playqueue_length (Obj * obj, Invoc * invoc)
{
    FINISH2 (get_playqueue_length, aud_playlist_queue_count (aud_playlist_get_active ()));
    return TRUE;
}

static bool_t do_get_tuple_fields (Obj * obj, Invoc * invoc)
{
    const char * fields[TUPLE_FIELDS + 1];

    for (int i = 0; i < TUPLE_FIELDS; i ++)
        fields[i] = tuple_field_get_name (i);

    fields[TUPLE_FIELDS] = NULL;

    FINISH2 (get_tuple_fields, fields);
    return TRUE;
}

static bool_t do_info (Obj * obj, Invoc * invoc)
{
    int bitrate, samplerate, channels;
    aud_drct_get_info (& bitrate, & samplerate, & channels);
    FINISH2 (info, bitrate, samplerate, channels);
    return TRUE;
}

static bool_t do_jump (Obj * obj, Invoc * invoc, unsigned pos)
{
    aud_playlist_set_position (aud_playlist_get_active (), pos);
    FINISH (jump);
    return TRUE;
}

static bool_t do_length (Obj * obj, Invoc * invoc)
{
    FINISH2 (length, aud_playlist_entry_count (aud_playlist_get_active ()));
    return TRUE;
}

static bool_t do_main_win_visible (Obj * obj, Invoc * invoc)
{
    FINISH2 (main_win_visible, ! aud_get_headless_mode () && aud_ui_is_shown ());
    return TRUE;
}

static bool_t do_new_playlist (Obj * obj, Invoc * invoc)
{
    aud_playlist_insert (-1);
    aud_playlist_set_active (aud_playlist_count () - 1);
    FINISH (new_playlist);
    return TRUE;
}

static bool_t do_number_of_playlists (Obj * obj, Invoc * invoc)
{
    FINISH2 (number_of_playlists, aud_playlist_count ());
    return TRUE;
}

static bool_t do_open_list (Obj * obj, Invoc * invoc, const char * const * filenames)
{
    aud_drct_pl_open_list (strv_to_index (filenames));
    FINISH (open_list);
    return TRUE;
}

static bool_t do_open_list_to_temp (Obj * obj, Invoc * invoc, const char * const * filenames)
{
    aud_drct_pl_open_temp_list (strv_to_index (filenames));
    FINISH (open_list_to_temp);
    return TRUE;
}

static bool_t do_pause (Obj * obj, Invoc * invoc)
{
    aud_drct_pause ();
    FINISH (pause);
    return TRUE;
}

static bool_t do_paused (Obj * obj, Invoc * invoc)
{
    FINISH2 (paused, aud_drct_get_paused ());
    return TRUE;
}

static bool_t do_play (Obj * obj, Invoc * invoc)
{
    aud_drct_play ();
    FINISH (play);
    return TRUE;
}

static bool_t do_play_active_playlist (Obj * obj, Invoc * invoc)
{
    aud_drct_play_playlist (aud_playlist_get_active ());
    FINISH (play_active_playlist);
    return TRUE;
}

static bool_t do_play_pause (Obj * obj, Invoc * invoc)
{
    aud_drct_play_pause ();
    FINISH (play_pause);
    return TRUE;
}

static bool_t do_playing (Obj * obj, Invoc * invoc)
{
    FINISH2 (playing, aud_drct_get_playing ());
    return TRUE;
}

static bool_t do_playlist_add (Obj * obj, Invoc * invoc, const char * list)
{
    aud_playlist_entry_insert (aud_playlist_get_active (), -1, list, NULL, FALSE);
    FINISH (playlist_add);
    return TRUE;
}

static bool_t do_playlist_enqueue_to_temp (Obj * obj, Invoc * invoc, const char * url)
{
    aud_drct_pl_open_temp (url);
    FINISH (playlist_enqueue_to_temp);
    return TRUE;
}

static bool_t do_playlist_ins_url_string (Obj * obj, Invoc * invoc, const char * url, int pos)
{
    aud_playlist_entry_insert (aud_playlist_get_active (), pos, url, NULL, FALSE);
    FINISH (playlist_ins_url_string);
    return TRUE;
}

static bool_t do_playqueue_add (Obj * obj, Invoc * invoc, int pos)
{
    aud_playlist_queue_insert (aud_playlist_get_active (), -1, pos);
    FINISH (playqueue_add);
    return TRUE;
}

static bool_t do_playqueue_clear (Obj * obj, Invoc * invoc)
{
    int playlist = aud_playlist_get_active ();
    aud_playlist_queue_delete (playlist, 0, aud_playlist_queue_count (playlist));
    FINISH (playqueue_clear);
    return TRUE;
}

static bool_t do_playqueue_is_queued (Obj * obj, Invoc * invoc, int pos)
{
    bool_t queued = (aud_playlist_queue_find_entry (aud_playlist_get_active (), pos) >= 0);
    FINISH2 (playqueue_is_queued, queued);
    return TRUE;
}

static bool_t do_playqueue_remove (Obj * obj, Invoc * invoc, int pos)
{
    int playlist = aud_playlist_get_active ();
    int qpos = aud_playlist_queue_find_entry (playlist, pos);

    if (qpos >= 0)
        aud_playlist_queue_delete (playlist, qpos, 1);

    FINISH (playqueue_remove);
    return TRUE;
}

static bool_t do_position (Obj * obj, Invoc * invoc)
{
    FINISH2 (position, aud_playlist_get_position (aud_playlist_get_active ()));
    return TRUE;
}

static bool_t do_queue_get_list_pos (Obj * obj, Invoc * invoc, unsigned qpos)
{
    FINISH2 (queue_get_list_pos, aud_playlist_queue_get_entry (aud_playlist_get_active (), qpos));
    return TRUE;
}

static bool_t do_queue_get_queue_pos (Obj * obj, Invoc * invoc, unsigned pos)
{
    FINISH2 (queue_get_queue_pos, aud_playlist_queue_find_entry (aud_playlist_get_active (), pos));
    return TRUE;
}

static bool_t do_quit (Obj * obj, Invoc * invoc)
{
    aud_quit ();
    FINISH (quit);
    return TRUE;
}

static bool_t do_repeat (Obj * obj, Invoc * invoc)
{
    FINISH2 (repeat, aud_get_bool (NULL, "repeat"));
    return TRUE;
}

static bool_t do_reverse (Obj * obj, Invoc * invoc)
{
    aud_drct_pl_prev ();
    FINISH (reverse);
    return TRUE;
}

static bool_t do_seek (Obj * obj, Invoc * invoc, unsigned pos)
{
    aud_drct_seek (pos);
    FINISH (seek);
    return TRUE;
}

static bool_t do_set_active_playlist (Obj * obj, Invoc * invoc, int playlist)
{
    aud_playlist_set_active (playlist);
    FINISH (set_active_playlist);
    return TRUE;
}

static bool_t do_set_active_playlist_name (Obj * obj, Invoc * invoc, const char * title)
{
    aud_playlist_set_title (aud_playlist_get_active (), title);
    FINISH (set_active_playlist_name);
    return TRUE;
}

static bool_t do_set_eq (Obj * obj, Invoc * invoc, double preamp, GVariant * var)
{
    if (! g_variant_is_of_type (var, G_VARIANT_TYPE ("ad")))
        return FALSE;

    size_t nbands = 0;
    const double * bands = (double *) g_variant_get_fixed_array (var, & nbands, sizeof (double));

    if (nbands != AUD_EQ_NBANDS)
        return FALSE;

    aud_set_double (NULL, "equalizer_preamp", preamp);
    aud_eq_set_bands (bands);
    FINISH (set_eq);
    return TRUE;
}

static bool_t do_set_eq_band (Obj * obj, Invoc * invoc, int band, double value)
{
    aud_eq_set_band (band, value);
    FINISH (set_eq_band);
    return TRUE;
}

static bool_t do_set_eq_preamp (Obj * obj, Invoc * invoc, double preamp)
{
    aud_set_double (NULL, "equalizer_preamp", preamp);
    FINISH (set_eq_preamp);
    return TRUE;
}

static bool_t do_set_volume (Obj * obj, Invoc * invoc, int vl, int vr)
{
    aud_drct_set_volume (vl, vr);
    FINISH (set_volume);
    return TRUE;
}

static bool_t do_show_about_box (Obj * obj, Invoc * invoc, bool_t show)
{
    if (! aud_get_headless_mode ())
    {
        if (show)
            aud_ui_show_about_window ();
        else
            aud_ui_hide_about_window ();
    }

    FINISH (show_about_box);
    return TRUE;
}

static bool_t do_show_filebrowser (Obj * obj, Invoc * invoc, bool_t show)
{
    if (! aud_get_headless_mode ())
    {
        if (show)
            aud_ui_show_filebrowser (FALSE);
        else
            aud_ui_hide_filebrowser ();
    }

    FINISH (show_filebrowser);
    return TRUE;
}

static bool_t do_show_jtf_box (Obj * obj, Invoc * invoc, bool_t show)
{
    if (! aud_get_headless_mode ())
    {
        if (show)
            aud_ui_show_jump_to_song ();
        else
            aud_ui_hide_jump_to_song ();
    }

    FINISH (show_jtf_box);
    return TRUE;
}

static bool_t do_show_main_win (Obj * obj, Invoc * invoc, bool_t show)
{
    if (! aud_get_headless_mode ())
        aud_ui_show (show);

    FINISH (show_main_win);
    return TRUE;
}

static bool_t do_show_prefs_box (Obj * obj, Invoc * invoc, bool_t show)
{
    if (! aud_get_headless_mode ())
    {
        if (show)
            aud_ui_show_prefs_window ();
        else
            aud_ui_hide_prefs_window ();
    }

    FINISH (show_prefs_box);
    return TRUE;
}

static bool_t do_shuffle (Obj * obj, Invoc * invoc)
{
    FINISH2 (shuffle, aud_get_bool (NULL, "shuffle"));
    return TRUE;
}

static bool_t do_song_filename (Obj * obj, Invoc * invoc, unsigned pos)
{
    char * filename = aud_playlist_entry_get_filename (aud_playlist_get_active (), pos);
    FINISH2 (song_filename, filename ? filename : "");
    str_unref (filename);
    return TRUE;
}

static bool_t do_song_frames (Obj * obj, Invoc * invoc, unsigned pos)
{
    FINISH2 (song_frames, aud_playlist_entry_get_length (aud_playlist_get_active (), pos, FALSE));
    return TRUE;
}

static bool_t do_song_length (Obj * obj, Invoc * invoc, unsigned pos)
{
    int length = aud_playlist_entry_get_length (aud_playlist_get_active (), pos, FALSE);
    FINISH2 (song_length, length >= 0 ? length / 1000 : -1);
    return TRUE;
}

static bool_t do_song_title (Obj * obj, Invoc * invoc, unsigned pos)
{
    char * title = aud_playlist_entry_get_title (aud_playlist_get_active (), pos, FALSE);
    FINISH2 (song_title, title ? title : "");
    str_unref (title);
    return TRUE;
}

static bool_t do_song_tuple (Obj * obj, Invoc * invoc, unsigned pos, const char * key)
{
    int field = tuple_field_by_name (key);
    Tuple * tuple = NULL;
    GVariant * var = NULL;

    if (field >= 0)
        tuple = aud_playlist_entry_get_tuple (aud_playlist_get_active (), pos, FALSE);

    if (tuple)
    {
        char * str;

        switch (tuple_get_value_type (tuple, field))
        {
        case TUPLE_STRING:
            str = tuple_get_str (tuple, field);
            var = g_variant_new_string (str);
            str_unref (str);
            break;

        case TUPLE_INT:
            var = g_variant_new_int32 (tuple_get_int (tuple, field));
            break;

        default:
            break;
        }

        tuple_unref (tuple);
    }

    if (! var)
        var = g_variant_new_string ("");

    FINISH2 (song_tuple, g_variant_new_variant (var));
    return TRUE;
}

static bool_t do_status (Obj * obj, Invoc * invoc)
{
    const char * status = "stopped";
    if (aud_drct_get_playing ())
        status = aud_drct_get_paused () ? "paused" : "playing";

    FINISH2 (status, status);
    return TRUE;
}

static bool_t do_stop (Obj * obj, Invoc * invoc)
{
    aud_drct_stop ();
    FINISH (stop);
    return TRUE;
}

static bool_t do_stop_after (Obj * obj, Invoc * invoc)
{
    FINISH2 (stop_after, aud_get_bool (NULL, "stop_after_current_song"));
    return TRUE;
}

static bool_t do_stopped (Obj * obj, Invoc * invoc)
{
    FINISH2 (stopped, ! aud_drct_get_playing ());
    return TRUE;
}

static bool_t do_time (Obj * obj, Invoc * invoc)
{
    FINISH2 (time, aud_drct_get_time ());
    return TRUE;
}

static bool_t do_toggle_auto_advance (Obj * obj, Invoc * invoc)
{
    aud_set_bool (NULL, "no_playlist_advance", ! aud_get_bool (NULL, "no_playlist_advance"));
    FINISH (toggle_auto_advance);
    return TRUE;
}

static bool_t do_toggle_repeat (Obj * obj, Invoc * invoc)
{
    aud_set_bool (NULL, "repeat", ! aud_get_bool (NULL, "repeat"));
    FINISH (toggle_repeat);
    return TRUE;
}

static bool_t do_toggle_shuffle (Obj * obj, Invoc * invoc)
{
    aud_set_bool (NULL, "shuffle", ! aud_get_bool (NULL, "shuffle"));
    FINISH (toggle_shuffle);
    return TRUE;
}

static bool_t do_toggle_stop_after (Obj * obj, Invoc * invoc)
{
    aud_set_bool (NULL, "stop_after_current_song", ! aud_get_bool (NULL, "stop_after_current_song"));
    FINISH (toggle_stop_after);
    return TRUE;
}

static bool_t do_version (Obj * obj, Invoc * invoc)
{
    FINISH2 (version, VERSION);
    return TRUE;
}

static bool_t do_volume (Obj * obj, Invoc * invoc)
{
    int left, right;
    aud_drct_get_volume (& left, & right);
    FINISH2 (volume, left, right);
    return TRUE;
}

static const struct
{
    const char * signal;
    GCallback callback;
}
handlers[] =
{
    {"handle-add", (GCallback) do_add},
    {"handle-add-list", (GCallback) do_add_list},
    {"handle-add-url", (GCallback) do_add_url},
    {"handle-advance", (GCallback) do_advance},
    {"handle-auto-advance", (GCallback) do_auto_advance},
    {"handle-balance", (GCallback) do_balance},
    {"handle-clear", (GCallback) do_clear},
    {"handle-delete", (GCallback) do_delete},
    {"handle-delete-active-playlist", (GCallback) do_delete_active_playlist},
    {"handle-eject", (GCallback) do_eject},
    {"handle-equalizer-activate", (GCallback) do_equalizer_activate},
    {"handle-get-active-playlist", (GCallback) do_get_active_playlist},
    {"handle-get-active-playlist-name", (GCallback) do_get_active_playlist_name},
    {"handle-get-eq", (GCallback) do_get_eq},
    {"handle-get-eq-band", (GCallback) do_get_eq_band},
    {"handle-get-eq-preamp", (GCallback) do_get_eq_preamp},
    {"handle-get-info", (GCallback) do_get_info},
    {"handle-get-playqueue-length", (GCallback) do_get_playqueue_length},
    {"handle-get-tuple-fields", (GCallback) do_get_tuple_fields},
    {"handle-info", (GCallback) do_info},
    {"handle-jump", (GCallback) do_jump},
    {"handle-length", (GCallback) do_length},
    {"handle-main-win-visible", (GCallback) do_main_win_visible},
    {"handle-new-playlist", (GCallback) do_new_playlist},
    {"handle-number-of-playlists", (GCallback) do_number_of_playlists},
    {"handle-open-list", (GCallback) do_open_list},
    {"handle-open-list-to-temp", (GCallback) do_open_list_to_temp},
    {"handle-pause", (GCallback) do_pause},
    {"handle-paused", (GCallback) do_paused},
    {"handle-play", (GCallback) do_play},
    {"handle-play-active-playlist", (GCallback) do_play_active_playlist},
    {"handle-play-pause", (GCallback) do_play_pause},
    {"handle-playing", (GCallback) do_playing},
    {"handle-playlist-add", (GCallback) do_playlist_add},
    {"handle-playlist-enqueue-to-temp", (GCallback) do_playlist_enqueue_to_temp},
    {"handle-playlist-ins-url-string", (GCallback) do_playlist_ins_url_string},
    {"handle-playqueue-add", (GCallback) do_playqueue_add},
    {"handle-playqueue-clear", (GCallback) do_playqueue_clear},
    {"handle-playqueue-is-queued", (GCallback) do_playqueue_is_queued},
    {"handle-playqueue-remove", (GCallback) do_playqueue_remove},
    {"handle-position", (GCallback) do_position},
    {"handle-queue-get-list-pos", (GCallback) do_queue_get_list_pos},
    {"handle-queue-get-queue-pos", (GCallback) do_queue_get_queue_pos},
    {"handle-quit", (GCallback) do_quit},
    {"handle-repeat", (GCallback) do_repeat},
    {"handle-reverse", (GCallback) do_reverse},
    {"handle-seek", (GCallback) do_seek},
    {"handle-set-active-playlist", (GCallback) do_set_active_playlist},
    {"handle-set-active-playlist-name", (GCallback) do_set_active_playlist_name},
    {"handle-set-eq", (GCallback) do_set_eq},
    {"handle-set-eq-band", (GCallback) do_set_eq_band},
    {"handle-set-eq-preamp", (GCallback) do_set_eq_preamp},
    {"handle-set-volume", (GCallback) do_set_volume},
    {"handle-show-about-box", (GCallback) do_show_about_box},
    {"handle-show-filebrowser", (GCallback) do_show_filebrowser},
    {"handle-show-jtf-box", (GCallback) do_show_jtf_box},
    {"handle-show-main-win", (GCallback) do_show_main_win},
    {"handle-show-prefs-box", (GCallback) do_show_prefs_box},
    {"handle-shuffle", (GCallback) do_shuffle},
    {"handle-song-filename", (GCallback) do_song_filename},
    {"handle-song-frames", (GCallback) do_song_frames},
    {"handle-song-length", (GCallback) do_song_length},
    {"handle-song-title", (GCallback) do_song_title},
    {"handle-song-tuple", (GCallback) do_song_tuple},
    {"handle-status", (GCallback) do_status},
    {"handle-stop", (GCallback) do_stop},
    {"handle-stop-after", (GCallback) do_stop_after},
    {"handle-stopped", (GCallback) do_stopped},
    {"handle-time", (GCallback) do_time},
    {"handle-toggle-auto-advance", (GCallback) do_toggle_auto_advance},
    {"handle-toggle-repeat", (GCallback) do_toggle_repeat},
    {"handle-toggle-shuffle", (GCallback) do_toggle_shuffle},
    {"handle-toggle-stop-after", (GCallback) do_toggle_stop_after},
    {"handle-version", (GCallback) do_version},
    {"handle-volume", (GCallback) do_volume}
};

static GDBusInterfaceSkeleton * skeleton = NULL;

void dbus_server_init (void)
{
    GError * error = NULL;
    GDBusConnection * bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, & error);

    if (! bus)
        goto ERROR;

    g_bus_own_name_on_connection (bus, "org.atheme.audacious",
     (GBusNameOwnerFlags) 0, NULL, NULL, NULL, NULL);

    skeleton = (GDBusInterfaceSkeleton *) obj_audacious_skeleton_new ();

    for (unsigned i = 0; i < ARRAY_LEN (handlers); i ++)
        g_signal_connect (skeleton, handlers[i].signal, handlers[i].callback, NULL);

    if (! g_dbus_interface_skeleton_export (skeleton, bus, "/org/atheme/audacious", & error))
        goto ERROR;

    return;

ERROR:
    if (error)
    {
        fprintf (stderr, "D-Bus error: %s\n", error->message);
        g_error_free (error);
    }
}

void dbus_server_cleanup (void)
{
    if (skeleton)
    {
        g_object_unref (skeleton);
        skeleton = NULL;
    }
}
