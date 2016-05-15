/*
 * wrappers.c
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

#include <stdlib.h>

#include "audtool.h"
#include "wrappers.h"

void generic_on_off (int argc, char * * argv, OnOffFunc func)
{
    gboolean show;

    if (argc == 1)
        show = TRUE;
    else if (argc == 2 && ! g_ascii_strcasecmp (argv[1], "on"))
        show = TRUE;
    else if (argc == 2 && ! g_ascii_strcasecmp (argv[1], "off"))
        show = FALSE;
    else
    {
        audtool_whine_args (argv[0], "<on/off>");
        exit (1);
    }

    func (dbus_proxy, show, NULL, NULL);
}

int get_playlist_length (void)
{
    int length = -1;
    obj_audacious_call_length_sync (dbus_proxy, & length, NULL, NULL);

    if (length < 0)
        exit (1);

    return length;
}

int get_queue_length (void)
{
    int length = -1;
    obj_audacious_call_get_playqueue_length_sync (dbus_proxy, & length, NULL, NULL);

    if (length < 0)
        exit (1);

    return length;
}

int get_queue_entry (int qpos)
{
    unsigned entry = -1;
    obj_audacious_call_queue_get_list_pos_sync (dbus_proxy, qpos, & entry, NULL, NULL);

    if (entry == (unsigned) -1)
        exit (1);

    return entry;
}

int find_in_queue (int entry)
{
    unsigned qpos = -1;
    obj_audacious_call_queue_get_queue_pos_sync (dbus_proxy, entry, & qpos, NULL, NULL);

    if (qpos == (unsigned) -1)
        exit (1);

    return qpos;
}

int get_current_entry (void)
{
    unsigned entry = -1;
    obj_audacious_call_position_sync (dbus_proxy, & entry, NULL, NULL);

    if (entry == (unsigned) -1)
        exit (1);

    return entry;
}

char * get_entry_filename (int entry)
{
    char * uri = NULL;
    obj_audacious_call_song_filename_sync (dbus_proxy, entry, & uri, NULL, NULL);

    if (! uri)
        exit (1);

    char * filename = g_filename_from_uri (uri, NULL, NULL);

    if (filename)
    {
        g_free (uri);
        return filename;
    }

    return uri;
}

char * get_entry_title (int entry)
{
    char * title = NULL;
    obj_audacious_call_song_title_sync (dbus_proxy, entry, & title, NULL, NULL);

    if (! title)
        exit (1);

    return title;
}

int get_entry_length (int entry)
{
    int length = -1;
    obj_audacious_call_song_frames_sync (dbus_proxy, entry, & length, NULL, NULL);

    if (length < 0)
        exit (1);

    return length;
}

char * get_entry_field (int entry, const char * field)
{
    GVariant * var = NULL;
    obj_audacious_call_song_tuple_sync (dbus_proxy, entry, field, & var, NULL, NULL);

    if (! var || ! g_variant_is_of_type (var, G_VARIANT_TYPE_VARIANT))
        exit (1);

    GVariant * var2 = g_variant_get_variant (var);

    if (! var2)
        exit (1);

    char * str;

    if (g_variant_is_of_type (var2, G_VARIANT_TYPE_STRING))
        str = g_strdup (g_variant_get_string (var2, NULL));
    else if (g_variant_is_of_type (var2, G_VARIANT_TYPE_INT32))
        str = g_strdup_printf ("%d", (int) g_variant_get_int32 (var2));
    else
        exit (1);

    g_variant_unref (var);
    g_variant_unref (var2);

    return str;
}

int get_current_time (void)
{
    unsigned time = -1;
    obj_audacious_call_time_sync (dbus_proxy, & time, NULL, NULL);

    if (time == (unsigned) -1)
        exit (1);

    return time;
}

void get_current_info (int * bitrate_p, int * samplerate_p, int * channels_p)
{
    int bitrate = -1, samplerate = -1, channels = -1;
    obj_audacious_call_get_info_sync (dbus_proxy, & bitrate, & samplerate, & channels, NULL, NULL);

    if (bitrate < 0 || samplerate < 0 || channels < 0)
        exit (1);

    if (bitrate_p)
        * bitrate_p = bitrate;
    if (samplerate_p)
        * samplerate_p = samplerate;
    if (channels_p)
        * channels_p = channels;
}
