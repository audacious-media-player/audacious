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

#include "debug.h"
#include "drct.h"
#include "playlist.h"
#include "interface.h"
#include "misc.h"

static DBusGConnection *dbus_conn = NULL;
static unsigned int signals[LAST_SIG] = { 0 };
static unsigned int tracklist_signals[LAST_TRACKLIST_SIG] = { 0 };

MprisPlayer * mpris = NULL;
MprisTrackList * mpris_tracklist = NULL;

G_DEFINE_TYPE (RemoteObject, audacious_rc, G_TYPE_OBJECT)
G_DEFINE_TYPE (MprisRoot, mpris_root, G_TYPE_OBJECT)
G_DEFINE_TYPE (MprisPlayer, mpris_player, G_TYPE_OBJECT)
G_DEFINE_TYPE (MprisTrackList, mpris_tracklist, G_TYPE_OBJECT)

#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

static void mpris_playlist_update_hook(void * unused, MprisTrackList *obj);

void audacious_rc_class_init(RemoteObjectClass * klass)
{
}

void mpris_root_class_init(MprisRootClass * klass)
{
}

void mpris_player_class_init(MprisPlayerClass * klass)
{
    signals[CAPS_CHANGE_SIG] = g_signal_new("caps_change", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
    signals[TRACK_CHANGE_SIG] =
        g_signal_new("track_change",
                     G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, DBUS_TYPE_G_STRING_VALUE_HASHTABLE);

    GType status_type = dbus_g_type_get_struct ("GValueArray", G_TYPE_INT,
     G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INVALID);
    signals[STATUS_CHANGE_SIG] =
     g_signal_new ("status_change", G_OBJECT_CLASS_TYPE (klass),
     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL,
     g_cclosure_marshal_VOID__BOXED, G_TYPE_NONE, 1, status_type);
}

void mpris_tracklist_class_init(MprisTrackListClass * klass)
{
    tracklist_signals[TRACKLIST_CHANGE_SIG] = g_signal_new("track_list_change", G_OBJECT_CLASS_TYPE(klass),
	G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
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

    if (!org_freedesktop_DBus_request_name(driver_proxy, AUDACIOUS_DBUS_SERVICE_MPRIS, 0, &request_ret, &error))
    {
        g_warning("Unable to register service: %s", error->message);
        g_error_free(error);
    }

    g_object_unref(driver_proxy);
}

void mpris_root_init(MprisRoot * object)
{
    dbus_g_object_type_install_info(mpris_root_get_type(), &dbus_glib_mpris_root_object_info);

    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn, AUDACIOUS_DBUS_PATH_MPRIS_ROOT, G_OBJECT(object));
}

void mpris_player_init(MprisPlayer * object)
{
    dbus_g_object_type_install_info(mpris_player_get_type(), &dbus_glib_mpris_player_object_info);

    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn, AUDACIOUS_DBUS_PATH_MPRIS_PLAYER, G_OBJECT(object));

    // Add signals
    DBusGProxy *proxy = object->proxy;
    if (proxy != NULL)
    {
        dbus_g_proxy_add_signal (proxy, "StatusChange", dbus_g_type_get_struct
         ("GValueArray", G_TYPE_INT, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT,
         G_TYPE_INVALID), G_TYPE_INVALID);
        dbus_g_proxy_add_signal (proxy, "CapsChange", G_TYPE_INT, G_TYPE_INVALID);
        dbus_g_proxy_add_signal(proxy, "TrackChange", DBUS_TYPE_G_STRING_VALUE_HASHTABLE, G_TYPE_INVALID);
    }
    else
    {
        /* XXX / FIXME: Why does this happen? -- ccr */
        AUDDBG ("object->proxy == NULL; not adding some signals.\n");
    }
}

void mpris_tracklist_init(MprisTrackList * object)
{
    dbus_g_object_type_install_info(mpris_tracklist_get_type(), &dbus_glib_mpris_tracklist_object_info);

    // Register DBUS path
    dbus_g_connection_register_g_object(dbus_conn, AUDACIOUS_DBUS_PATH_MPRIS_TRACKLIST, G_OBJECT(object));

    // Add signals
    DBusGProxy *proxy = object->proxy;
    if (proxy != NULL)
    {
        dbus_g_proxy_add_signal(proxy, "TrackListChange", G_TYPE_INT, G_TYPE_INVALID);
    }
    else
    {
        /* XXX / FIXME: Why does this happen? -- ccr */
        AUDDBG ("object->proxy == NULL, not adding some signals.\n");
    }
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
    g_object_new(mpris_root_get_type(), NULL);
    mpris = g_object_new(mpris_player_get_type(), NULL);
    mpris_tracklist = g_object_new(mpris_tracklist_get_type(), NULL);

    local_conn = dbus_g_connection_get_connection(dbus_conn);
    dbus_connection_set_exit_on_disconnect(local_conn, FALSE);

    hook_associate ("playlist update",
     (HookFunction) mpris_playlist_update_hook, mpris_tracklist);
}

void cleanup_dbus (void)
{
    hook_dissociate ("playlist update", (HookFunction) mpris_playlist_update_hook);
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

/**
 * Retrieves value named tuple_key and inserts it inside hash table.
 *
 * @param[in,out] md GHashTable to insert into
 * @param[in] tuple Tuple to read data from
 * @param[in] tuple_key Tuple field key
 * @param[in] key key used for inserting into hash table.
 */
static void tuple_insert_to_hash_full(GHashTable * md, const Tuple * tuple,
                                      const char * tuple_key, const char *key)
{
    GValue *value = tuple_value_to_gvalue(tuple, tuple_key);
    if (value != NULL)
        g_hash_table_insert (md, (void *) key, value);
}

static void tuple_insert_to_hash(GHashTable * md, const Tuple * tuple,
                                 const char *key)
{
    tuple_insert_to_hash_full(md, tuple, key, key);
}

static void remove_metadata_value(void * value)
{
    g_value_unset((GValue *) value);
    g_free((GValue *) value);
}

static GHashTable *make_mpris_metadata(const char * filename, const Tuple * tuple)
{
    GHashTable *md = NULL;
    void * value;

    md = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, remove_metadata_value);

    value = g_malloc(sizeof(GValue));
    memset(value, 0, sizeof(GValue));
    g_value_init(value, G_TYPE_STRING);
    g_value_take_string(value, g_strdup(filename));
    g_hash_table_insert(md, "location", value);

    if (tuple != NULL)
    {
        tuple_insert_to_hash_full(md, tuple, "length", "mtime");
        tuple_insert_to_hash(md, tuple, "title");
        tuple_insert_to_hash(md, tuple, "artist");
        tuple_insert_to_hash(md, tuple, "album");
        tuple_insert_to_hash(md, tuple, "comment");
        tuple_insert_to_hash(md, tuple, "genre");
        tuple_insert_to_hash(md, tuple, "year");
        tuple_insert_to_hash(md, tuple, "codec");
        tuple_insert_to_hash(md, tuple, "quality");
        tuple_insert_to_hash_full(md, tuple, "track-number", "tracknumber");
        tuple_insert_to_hash_full(md, tuple, "bitrate", "audio-bitrate");
    }

    return md;
}

static GValue * get_field (int playlist, int entry, const char * field)
{
    Tuple * tuple = playlist_entry_get_tuple (playlist, entry, FALSE);
    GValue * value = tuple ? tuple_value_to_gvalue (tuple, field) : NULL;

    if (tuple)
        tuple_unref (tuple);

    return value;
}

static GHashTable * get_mpris_metadata (int playlist, int entry)
{
    char * filename = playlist_entry_get_filename (playlist, entry);
    Tuple * tuple = playlist_entry_get_tuple (playlist, entry, FALSE);

    GHashTable * metadata = NULL;
    if (filename && tuple)
        metadata = make_mpris_metadata (filename, tuple);

    str_unref (filename);
    if (tuple)
        tuple_unref (tuple);

    return metadata;
}

/* MPRIS API */
// MPRIS /
bool_t mpris_root_identity(MprisRoot * obj, char ** identity, GError ** error)
{
    *identity = g_strdup_printf("Audacious %s", VERSION);
    return TRUE;
}

bool_t mpris_root_quit(MprisPlayer * obj, GError ** error)
{
    event_queue("quit", NULL);
    return TRUE;
}

// MPRIS /Player

bool_t mpris_player_next (MprisPlayer * obj, GError * * error)
{
    drct_pl_next ();
    return TRUE;
}

bool_t mpris_player_prev (MprisPlayer * obj, GError * * error)
{
    drct_pl_prev ();
    return TRUE;
}

bool_t mpris_player_pause (MprisPlayer * obj, GError * * error)
{
    drct_pause ();
    return TRUE;
}

bool_t mpris_player_stop (MprisPlayer * obj, GError * * error)
{
    drct_stop ();
    return TRUE;
}

bool_t mpris_player_play (MprisPlayer * obj, GError * * error)
{
    drct_play ();
    return TRUE;
}

bool_t mpris_player_repeat(MprisPlayer * obj, bool_t rpt, GError ** error)
{
    set_bool (NULL, "repeat", rpt);
    return TRUE;
}

static void append_int_value(GValueArray * ar, int tmp)
{
    GValue value;
    memset(&value, 0, sizeof(value));
    g_value_init(&value, G_TYPE_INT);
    g_value_set_int(&value, tmp);
    g_value_array_append(ar, &value);
}

static int get_playback_status (void)
{
    if (! drct_get_playing ())
        return MPRIS_STATUS_STOP;

    return drct_get_paused () ? MPRIS_STATUS_PAUSE : MPRIS_STATUS_PLAY;
}

bool_t mpris_player_get_status(MprisPlayer * obj, GValueArray * *status, GError * *error)
{
    *status = g_value_array_new(4);

    append_int_value(*status, (int) get_playback_status());
    append_int_value (* status, get_bool (NULL, "shuffle"));
    append_int_value (* status, get_bool (NULL, "no_playlist_advance"));
    append_int_value (* status, get_bool (NULL, "repeat"));
    return TRUE;
}

bool_t mpris_player_get_metadata (MprisPlayer * obj, GHashTable * * metadata,
 GError * * error)
{
    int playlist = playlist_get_playing ();
    int entry = (playlist >= 0) ? playlist_get_position (playlist) : -1;

    * metadata = (entry >= 0) ? get_mpris_metadata (playlist, entry) : NULL;
    if (! * metadata)
        * metadata = g_hash_table_new (g_str_hash, g_str_equal);

    return TRUE;
}

bool_t mpris_player_get_caps(MprisPlayer * obj, int * capabilities, GError ** error)
{
    *capabilities = MPRIS_CAPS_CAN_GO_NEXT | MPRIS_CAPS_CAN_GO_PREV | MPRIS_CAPS_CAN_PAUSE | MPRIS_CAPS_CAN_PLAY | MPRIS_CAPS_CAN_SEEK | MPRIS_CAPS_CAN_PROVIDE_METADATA | MPRIS_CAPS_PROVIDES_TIMING;
    return TRUE;
}

bool_t mpris_player_volume_set(MprisPlayer * obj, int vol, GError ** error)
{
    drct_set_volume_main (vol);
    return TRUE;
}

bool_t mpris_player_volume_get(MprisPlayer * obj, int * vol, GError ** error)
{
    drct_get_volume_main (vol);
    return TRUE;
}

bool_t mpris_player_position_set (MprisPlayer * obj, int pos, GError * * error)
{
    drct_seek (pos);
    return TRUE;
}

bool_t mpris_player_position_get (MprisPlayer * obj, int * pos, GError * * error)
{
    * pos = drct_get_time ();
    return TRUE;
}

// MPRIS /Player signals
bool_t mpris_emit_caps_change(MprisPlayer * obj)
{
    g_signal_emit(obj, signals[CAPS_CHANGE_SIG], 0, 0);
    return TRUE;
}

bool_t mpris_emit_track_change(MprisPlayer * obj)
{
    int playlist, entry;
    GHashTable *metadata;

    playlist = playlist_get_playing();
    entry = playlist_get_position(playlist);
    char * filename = playlist_entry_get_filename (playlist, entry);
    Tuple * tuple = playlist_entry_get_tuple (playlist, entry, FALSE);

    if (filename && tuple)
    {
        metadata = make_mpris_metadata (filename, tuple);
        g_signal_emit (obj, signals[TRACK_CHANGE_SIG], 0, metadata);
        g_hash_table_destroy (metadata);
    }

    str_unref (filename);
    if (tuple)
        tuple_unref (tuple);

    return (filename && tuple);
}

bool_t mpris_emit_status_change(MprisPlayer * obj, PlaybackStatus status)
{
    GValueArray *ar = g_value_array_new(4);

    if (status == MPRIS_STATUS_INVALID)
        status = get_playback_status ();

    append_int_value(ar, (int) status);
    append_int_value (ar, get_bool (NULL, "shuffle"));
    append_int_value (ar, get_bool (NULL, "no_playlist_advance"));
    append_int_value (ar, get_bool (NULL, "repeat"));

    g_signal_emit(obj, signals[STATUS_CHANGE_SIG], 0, ar);
    g_value_array_free(ar);
    return TRUE;
}

// MPRIS /TrackList
bool_t mpris_emit_tracklist_change(MprisTrackList * obj, int playlist)
{
    g_signal_emit(obj, tracklist_signals[TRACKLIST_CHANGE_SIG], 0, playlist_entry_count(playlist));
    return TRUE;
}

static void mpris_playlist_update_hook(void * unused, MprisTrackList * obj)
{
    int playlist = playlist_get_active();

    mpris_emit_tracklist_change(obj, playlist);
}

bool_t mpris_tracklist_get_metadata (MprisTrackList * obj, int pos,
 GHashTable * * metadata, GError * * error)
{
    * metadata = get_mpris_metadata (playlist_get_active (), pos);
    if (! * metadata)
        * metadata = g_hash_table_new (g_str_hash, g_str_equal);

    return TRUE;
}

bool_t mpris_tracklist_get_current_track (MprisTrackList * obj, int * pos,
 GError * * error)
{
    * pos = playlist_get_position (playlist_get_active ());
    return TRUE;
}

bool_t mpris_tracklist_get_length (MprisTrackList * obj, int * length, GError * * error)
{
    * length = playlist_entry_count (playlist_get_active ());
    return TRUE;
}

bool_t mpris_tracklist_add_track (MprisTrackList * obj, char * uri, bool_t play,
 GError * * error)
{
    playlist_entry_insert (playlist_get_active (), -1, uri, NULL, play);
    return TRUE;
}

bool_t mpris_tracklist_del_track (MprisTrackList * obj, int pos, GError * * error)
{
    playlist_entry_delete (playlist_get_active (), pos, 1);
    return TRUE;
}

bool_t mpris_tracklist_loop (MprisTrackList * obj, bool_t loop, GError * *
 error)
{
    set_bool (NULL, "repeat", loop);
    return TRUE;
}

bool_t mpris_tracklist_random (MprisTrackList * obj, bool_t random,
 GError * * error)
{
    set_bool (NULL, "shuffle", random);
    return TRUE;
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
    interface_show_filebrowser (TRUE);
    return TRUE;
}

bool_t audacious_rc_main_win_visible (RemoteObject * obj,
 bool_t * visible, GError ** error)
{
    * visible = interface_is_shown ();
    return TRUE;
}

bool_t audacious_rc_show_main_win (RemoteObject * obj, bool_t show,
 GError * * error)
{
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
    if (show)
        interface_show_jump_to_track ();

    return TRUE;
}

bool_t audacious_rc_show_filebrowser(RemoteObject * obj, bool_t show, GError ** error)
{
    if (show)
        interface_show_filebrowser (FALSE);

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
