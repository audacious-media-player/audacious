/*
 * dbus.c
 * Copyright 2007-2011 Ben Tucker, Yoshiki Yazawa, William Pitcock,
 *                     Matti Hämäläinen, Andrew Shadoura, John Lindgren, and
 *                     Tomasz Moń
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

#include <glib.h>
#include <string.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "dbus.h"
#include "dbus-service.h"
#include "dbus-server-bindings.h"

#include <math.h>

#include <libaudcore/hook.h>
#include <libaudgui/libaudgui.h>

#include "debug.h"
#include "drct.h"
#include "playlist.h"
#include "interface.h"
#include "misc.h"

static DBusGConnection *dbus_conn = NULL;

G_DEFINE_TYPE (RemoteObject, audacious_rc, G_TYPE_OBJECT)

#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

void audacious_rc_class_init(RemoteObjectClass * klass)
{
}

void audacious_rc_init(RemoteObject * object)
{
    GError *error = NULL;
    DBusGProxy *driver_proxy;
    unsigned int request_ret;

    AUDDBG ("Registering remote D-Bus interfaces.\n");

    dbus_g_object_type_install_info(audacious_rc_get_type(), &dbus_glib_audacious_rc_object_info);

    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn, AUDACIOUS_DBUS_PATH, G_OBJECT(object));

    // Register the service name, the constants here are defined in
    // dbus-glib-bindings.h
    driver_proxy = dbus_g_proxy_new_for_name(dbus_conn, DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

    if (!org_freedesktop_DBus_request_name(driver_proxy, AUDACIOUS_DBUS_SERVICE, 0, &request_ret, &error))
    {
        g_warning("Unable to register service: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(driver_proxy);
}

void init_dbus()
{
    GError *error = NULL;
    DBusConnection *local_conn;

    AUDDBG ("Trying to initialize D-Bus.\n");
    dbus_conn = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (dbus_conn == NULL)
    {
        g_warning("Unable to connect to dbus: %s", error->message);
        g_error_free(error);
        return;
    }

    g_object_new(audacious_rc_get_type(), NULL);

    local_conn = dbus_g_connection_get_connection(dbus_conn);
    dbus_connection_set_exit_on_disconnect(local_conn, FALSE);
}

void cleanup_dbus (void)
{
}

static GValue *tuple_value_to_gvalue(const Tuple * tuple, const char * key)
{
    GValue *val;
    TupleValueType type = tuple_get_value_type (tuple, -1, key);

    if (type == TUPLE_STRING)
    {
        val = g_new0(GValue, 1);
        g_value_init(val, G_TYPE_STRING);
        char * str = tuple_get_str (tuple, -1, key);
        g_value_set_string (val, str);
        str_unref (str);
        return val;
    }
    else if (type == TUPLE_INT)
    {
        val = g_new0(GValue, 1);
        g_value_init(val, G_TYPE_INT);
        int x = tuple_get_int (tuple, -1, key);
        g_value_set_int (val, x);
        return val;
    }
    return NULL;
}

static GValue * get_field (int playlist, int entry, const char * field)
{
    Tuple * tuple = playlist_entry_get_tuple (playlist, entry, FALSE);
    GValue * value = tuple ? tuple_value_to_gvalue (tuple, field) : NULL;

    if (tuple)
        tuple_unref (tuple);

    return value;
}

// Audacious General Information
bool_t audacious_rc_version(RemoteObject * obj, char ** version, GError ** error)
{
    *version = g_strdup(VERSION);
    return TRUE;
}

bool_t audacious_rc_quit(RemoteObject * obj, GError * *error)
{
    event_queue("quit", NULL);
    return TRUE;
}

bool_t audacious_rc_eject(RemoteObject * obj, GError ** error)
{
    if (headless_mode ())
        return FALSE;

    audgui_run_filebrowser (TRUE);
    return TRUE;
}

bool_t audacious_rc_main_win_visible (RemoteObject * obj,
 bool_t * visible, GError ** error)
{
    * visible = ! headless_mode () && interface_is_shown ();
    return TRUE;
}

bool_t audacious_rc_show_main_win (RemoteObject * obj, bool_t show,
 GError * * error)
{
    if (headless_mode ())
        return FALSE;

    interface_show (show);
    return TRUE;
}

bool_t audacious_rc_get_tuple_fields(RemoteObject * obj, char *** fields, GError ** error)
{
    * fields = g_new (char *, TUPLE_FIELDS);

    for (int i = 0; i < TUPLE_FIELDS; i ++)
        (* fields)[i] = g_strdup (tuple_field_get_name (i));

    (* fields)[TUPLE_FIELDS] = NULL;
    return TRUE;
}


// Playback Information/Manipulation

bool_t audacious_rc_play (RemoteObject * obj, GError * * error)
{
    drct_play ();
    return TRUE;
}

bool_t audacious_rc_pause (RemoteObject * obj, GError * * error)
{
    drct_pause ();
    return TRUE;
}

bool_t audacious_rc_stop (RemoteObject * obj, GError * * error)
{
    drct_stop ();
    return TRUE;
}

bool_t audacious_rc_playing (RemoteObject * obj, bool_t * is_playing, GError * * error)
{
    * is_playing = drct_get_playing ();
    return TRUE;
}

bool_t audacious_rc_paused (RemoteObject * obj, bool_t * is_paused, GError * * error)
{
    * is_paused = drct_get_paused ();
    return TRUE;
}

bool_t audacious_rc_stopped (RemoteObject * obj, bool_t * is_stopped, GError * * error)
{
    * is_stopped = ! drct_get_playing ();
    return TRUE;
}

bool_t audacious_rc_status (RemoteObject * obj, char * * status, GError * * error)
{
    if (drct_get_playing ())
        * status = strdup (drct_get_paused () ? "paused" : "playing");
    else
        * status = strdup ("stopped");

    return TRUE;
}

bool_t audacious_rc_info (RemoteObject * obj, int * rate, int * freq, int * nch,
 GError * * error)
{
    drct_get_info (rate, freq, nch);
    return TRUE;
}

bool_t audacious_rc_time (RemoteObject * obj, int * time, GError * * error)
{
    * time = drct_get_time ();
    return TRUE;
}

bool_t audacious_rc_seek (RemoteObject * obj, unsigned int pos, GError * * error)
{
    drct_seek (pos);
    return TRUE;
}

bool_t audacious_rc_volume(RemoteObject * obj, int * vl, int * vr, GError ** error)
{
    drct_get_volume (vl, vr);
    return TRUE;
}

bool_t audacious_rc_set_volume(RemoteObject * obj, int vl, int vr, GError ** error)
{
    drct_set_volume (vl, vr);
    return TRUE;
}

bool_t audacious_rc_balance(RemoteObject * obj, int * balance, GError ** error)
{
    drct_get_volume_balance (balance);
    return TRUE;
}

// Playlist Information/Manipulation

bool_t audacious_rc_position (RemoteObject * obj, int * pos, GError * * error)
{
    * pos = playlist_get_position (playlist_get_active ());
    return TRUE;
}

bool_t audacious_rc_advance(RemoteObject * obj, GError * *error)
{
    drct_pl_next ();
    return TRUE;
}

bool_t audacious_rc_reverse (RemoteObject * obj, GError * * error)
{
    drct_pl_prev ();
    return TRUE;
}

bool_t audacious_rc_length (RemoteObject * obj, int * length, GError * * error)
{
    * length = playlist_entry_count (playlist_get_active ());
    return TRUE;
}

bool_t audacious_rc_song_title (RemoteObject * obj, unsigned int pos, char * *
 title, GError * * error)
{
    char * title2 = playlist_entry_get_title (playlist_get_active (), pos, FALSE);
    if (! title2)
        return FALSE;

    * title = strdup (title2);
    str_unref (title2);
    return TRUE;
}

bool_t audacious_rc_song_filename (RemoteObject * obj, unsigned int pos,
 char * * filename, GError * * error)
{
    char * filename2 = playlist_entry_get_filename (playlist_get_active (), pos);
    if (! filename2)
        return FALSE;

    * filename = strdup (filename2);
    str_unref (filename2);
    return TRUE;
}

bool_t audacious_rc_song_length(RemoteObject * obj, unsigned int pos, int * length, GError * *error)
{
    audacious_rc_song_frames(obj, pos, length, error);
    *length /= 1000;
    return TRUE;
}

bool_t audacious_rc_song_frames (RemoteObject * obj, unsigned int pos, int *
 length, GError * * error)
{
    * length = playlist_entry_get_length (playlist_get_active (), pos, FALSE);
    return TRUE;
}

bool_t audacious_rc_song_tuple (RemoteObject * obj, unsigned int pos, char *
 field, GValue * value, GError * * error)
{
    GValue * value2 = get_field (playlist_get_active (), pos, field);
    if (! value2)
        return FALSE;

    memset (value, 0, sizeof (GValue));
    g_value_init (value, G_VALUE_TYPE (value2));
    g_value_copy (value2, value);
    g_value_unset (value2);
    g_free (value2);
    return TRUE;
}

bool_t audacious_rc_jump (RemoteObject * obj, unsigned int pos, GError * * error)
{
    playlist_set_position (playlist_get_active (), pos);
    return TRUE;
}

bool_t audacious_rc_add(RemoteObject * obj, char * file, GError * *error)
{
    return audacious_rc_playlist_ins_url_string(obj, file, -1, error);
}

bool_t audacious_rc_add_url(RemoteObject * obj, char * file, GError * *error)
{
    return audacious_rc_playlist_ins_url_string(obj, file, -1, error);
}

static Index * strings_to_index (char * * strings)
{
    Index * index = index_new ();

    while (* strings)
        index_append (index, str_get (* strings ++));

    return index;
}

bool_t audacious_rc_add_list (RemoteObject * obj, char * * filenames,
 GError * * error)
{
    drct_pl_add_list (strings_to_index (filenames), -1);
    return TRUE;
}

bool_t audacious_rc_open_list (RemoteObject * obj, char * * filenames,
 GError * * error)
{
    drct_pl_open_list (strings_to_index (filenames));
    return TRUE;
}

bool_t audacious_rc_open_list_to_temp (RemoteObject * obj, char * *
 filenames, GError * * error)
{
    drct_pl_open_temp_list (strings_to_index (filenames));
    return TRUE;
}

bool_t audacious_rc_delete (RemoteObject * obj, unsigned int pos, GError * * error)
{
    playlist_entry_delete (playlist_get_active (), pos, 1);
    return TRUE;
}

bool_t audacious_rc_clear (RemoteObject * obj, GError * * error)
{
    int playlist = playlist_get_active ();
    playlist_entry_delete (playlist, 0, playlist_entry_count (playlist));
    return TRUE;
}

bool_t audacious_rc_auto_advance(RemoteObject * obj, bool_t * is_advance, GError ** error)
{
    * is_advance = ! get_bool (NULL, "no_playlist_advance");
    return TRUE;
}

bool_t audacious_rc_toggle_auto_advance(RemoteObject * obj, GError ** error)
{
    set_bool (NULL, "no_playlist_advance", ! get_bool (NULL, "no_playlist_advance"));
    return TRUE;
}

bool_t audacious_rc_repeat(RemoteObject * obj, bool_t * is_repeating, GError ** error)
{
    *is_repeating = get_bool (NULL, "repeat");
    return TRUE;
}

bool_t audacious_rc_toggle_repeat (RemoteObject * obj, GError * * error)
{
    set_bool (NULL, "repeat", ! get_bool (NULL, "repeat"));
    return TRUE;
}

bool_t audacious_rc_shuffle(RemoteObject * obj, bool_t * is_shuffling, GError ** error)
{
    *is_shuffling = get_bool (NULL, "shuffle");
    return TRUE;
}

bool_t audacious_rc_toggle_shuffle (RemoteObject * obj, GError * * error)
{
    set_bool (NULL, "shuffle", ! get_bool (NULL, "shuffle"));
    return TRUE;
}

bool_t audacious_rc_stop_after (RemoteObject * obj, bool_t * is_stopping, GError * * error)
{
    * is_stopping = get_bool (NULL, "stop_after_current_song");
    return TRUE;
}

bool_t audacious_rc_toggle_stop_after (RemoteObject * obj, GError * * error)
{
    set_bool (NULL, "stop_after_current_song", ! get_bool (NULL, "stop_after_current_song"));
    return TRUE;
}

/* New on Oct 5 */
bool_t audacious_rc_show_prefs_box(RemoteObject * obj, bool_t show, GError ** error)
{
    event_queue("prefswin show", GINT_TO_POINTER(show));
    return TRUE;
}

bool_t audacious_rc_show_about_box(RemoteObject * obj, bool_t show, GError ** error)
{
    event_queue("aboutwin show", GINT_TO_POINTER(show));
    return TRUE;
}

bool_t audacious_rc_show_jtf_box(RemoteObject * obj, bool_t show, GError ** error)
{
    if (headless_mode ())
        return FALSE;

    if (show)
        audgui_jump_to_track ();
    else
        audgui_jump_to_track_hide ();

    return TRUE;
}

bool_t audacious_rc_show_filebrowser(RemoteObject * obj, bool_t show, GError ** error)
{
    if (headless_mode ())
        return FALSE;

    if (show)
        audgui_run_filebrowser (FALSE);
    else
        audgui_hide_filebrowser ();

    return TRUE;
}

bool_t audacious_rc_play_pause (RemoteObject * obj, GError * * error)
{
    drct_play_pause ();
    return TRUE;
}

bool_t audacious_rc_get_info (RemoteObject * obj, int * rate, int * freq,
 int * nch, GError * * error)
{
    drct_get_info (rate, freq, nch);
    return TRUE;
}

bool_t audacious_rc_toggle_aot(RemoteObject * obj, bool_t ontop, GError ** error)
{
    hook_call("mainwin set always on top", &ontop);
    return TRUE;
}

bool_t audacious_rc_playqueue_add (RemoteObject * obj, int pos, GError * * error)
{
    playlist_queue_insert (playlist_get_active (), -1, pos);
    return TRUE;
}

bool_t audacious_rc_playqueue_remove (RemoteObject * obj, int pos, GError * * error)
{
    int playlist = playlist_get_active ();
    int at = playlist_queue_find_entry (playlist, pos);

    if (at >= 0)
        playlist_queue_delete (playlist, at, 1);

    return TRUE;
}

bool_t audacious_rc_playqueue_clear (RemoteObject * obj, GError * * error)
{
    int playlist = playlist_get_active ();
    playlist_queue_delete (playlist, 0, playlist_queue_count (playlist));
    return TRUE;
}

bool_t audacious_rc_get_playqueue_length (RemoteObject * obj, int * length,
 GError * * error)
{
    * length = playlist_queue_count (playlist_get_active ());
    return TRUE;
}

bool_t audacious_rc_queue_get_list_pos (RemoteObject * obj, int qpos, int * pos,
 GError * * error)
{
    * pos = playlist_queue_get_entry (playlist_get_active (), qpos);
    return TRUE;
}

bool_t audacious_rc_queue_get_queue_pos (RemoteObject * obj, int pos, int *
 qpos, GError * * error)
{
    * qpos = playlist_queue_find_entry (playlist_get_active (), pos);
    return TRUE;
}

bool_t audacious_rc_playqueue_is_queued (RemoteObject * obj, int pos, bool_t *
 is_queued, GError * * error)
{
    * is_queued = (playlist_queue_find_entry (playlist_get_active (), pos) >= 0);
    return TRUE;
}

bool_t audacious_rc_playlist_ins_url_string (RemoteObject * obj, char * url, int
 pos, GError * * error)
{
    playlist_entry_insert (playlist_get_active (), pos, url, NULL, FALSE);
    return TRUE;
}

bool_t audacious_rc_playlist_add(RemoteObject * obj, void *list, GError * *error)
{
    return audacious_rc_playlist_ins_url_string(obj, list, -1, error);
}

bool_t audacious_rc_playlist_enqueue_to_temp (RemoteObject * obj, char * url,
 GError * * error)
{
    drct_pl_open_temp (url);
    return TRUE;
}

/* New on Nov 7: Equalizer */
bool_t audacious_rc_get_eq(RemoteObject * obj, double * preamp, GArray ** bands, GError ** error)
{
    * preamp = get_double (NULL, "equalizer_preamp");
    * bands = g_array_new (FALSE, FALSE, sizeof (double));
    g_array_set_size (* bands, AUD_EQUALIZER_NBANDS);
    eq_get_bands ((double *) (* bands)->data);

    return TRUE;
}

bool_t audacious_rc_get_eq_preamp(RemoteObject * obj, double * preamp, GError ** error)
{
    * preamp = get_double (NULL, "equalizer_preamp");
    return TRUE;
}

bool_t audacious_rc_get_eq_band(RemoteObject * obj, int band, double * value, GError ** error)
{
    * value = eq_get_band (band);
    return TRUE;
}

bool_t audacious_rc_set_eq(RemoteObject * obj, double preamp, GArray * bands, GError ** error)
{
    set_double (NULL, "equalizer_preamp", preamp);
    eq_set_bands ((double *) bands->data);
    return TRUE;
}

bool_t audacious_rc_set_eq_preamp(RemoteObject * obj, double preamp, GError ** error)
{
    set_double (NULL, "equalizer_preamp", preamp);
    return TRUE;
}

bool_t audacious_rc_set_eq_band(RemoteObject * obj, int band, double value, GError ** error)
{
    eq_set_band (band, value);
    return TRUE;
}

bool_t audacious_rc_equalizer_activate(RemoteObject * obj, bool_t active, GError ** error)
{
    set_bool (NULL, "equalizer_active", active);
    return TRUE;
}

bool_t audacious_rc_get_active_playlist_name (RemoteObject * obj, char * *
 title, GError * * error)
{
    char * title2 = playlist_get_title (playlist_get_active ());
    * title = strdup (title2);
    str_unref (title2);
    return TRUE;
}

DBusGProxy *audacious_get_dbus_proxy(void)
{
    DBusGConnection *connection = NULL;
    GError *error = NULL;
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    g_clear_error(&error);
    return dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE, AUDACIOUS_DBUS_PATH, AUDACIOUS_DBUS_INTERFACE);
}
