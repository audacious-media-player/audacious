/*
 * handlers_playlist.c
 * Copyright 2005-2013 George Averill, William Pitcock, Yoshiki Yazawa,
 *                     Matti Hämäläinen, and John Lindgren
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
#include <string.h>

#include "audtool.h"
#include "wrappers.h"

void playlist_reverse (int argc, char * * argv)
{
    obj_audacious_call_reverse_sync (dbus_proxy, NULL, NULL);
}

void playlist_advance (int argc, char * * argv)
{
    obj_audacious_call_advance_sync (dbus_proxy, NULL, NULL);
}

void playlist_auto_advance_status (int argc, char * * argv)
{
    gboolean advance = FALSE;
    obj_audacious_call_auto_advance_sync (dbus_proxy, & advance, NULL, NULL);
    audtool_report (advance ? "on" : "off");
}

void playlist_auto_advance_toggle (int argc, char * * argv)
{
    obj_audacious_call_toggle_auto_advance_sync (dbus_proxy, NULL, NULL);
}

void playlist_stop_after_status (int argc, char * * argv)
{
    gboolean stop_after = FALSE;
    obj_audacious_call_stop_after_sync (dbus_proxy, & stop_after, NULL, NULL);
    audtool_report (stop_after ? "on" : "off");
}

void playlist_stop_after_toggle (int argc, char * * argv)
{
    obj_audacious_call_toggle_stop_after_sync (dbus_proxy, NULL, NULL);
}

int check_args_playlist_pos (int argc, char * * argv)
{
    int pos;

    if (argc < 2 || (pos = atoi (argv[1])) < 1)
    {
        audtool_whine_args (argv[0], "<position>");
        exit (1);
    }

    return pos;
}

static char * construct_uri (char * string)
{
    char * filename = g_strdup (string);
    char * tmp, * path;
    char * uri = NULL;

    // case 1: filename is raw full path or uri
    if (filename[0] == '/' || strstr (filename, "://"))
    {
        uri = g_filename_to_uri (filename, NULL, NULL);

        if (! uri)
            uri = g_strdup (filename);

        g_free (filename);
    }
    // case 2: filename is not raw full path nor uri.
    // make full path with pwd. (using g_build_filename)
    else
    {
        path = g_get_current_dir();
        tmp = g_build_filename (path, filename, NULL);
        g_free (path);
        g_free (filename);
        uri = g_filename_to_uri (tmp, NULL, NULL);
        g_free (tmp);
    }

    return uri;
}

void playlist_add_url_string (int argc, char * * argv)
{
    char * uri;

    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<url>");
        exit (1);
    }

    uri = construct_uri (argv[1]);

    if (! uri)
        exit (1);

    obj_audacious_call_add_sync (dbus_proxy, uri, NULL, NULL);
    g_free (uri);
}

void playlist_delete (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    obj_audacious_call_delete_sync (dbus_proxy, pos - 1, NULL, NULL);
}

void playlist_length (int argc, char * * argv)
{
    audtool_report ("%d", get_playlist_length ());
}

void playlist_song (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    char * title = get_entry_title (pos - 1);
    audtool_report ("%s", title);
    g_free (title);
}

void number_of_playlists (int argc, char * * argv)
{
    int playlists = 0;
    obj_audacious_call_number_of_playlists_sync (dbus_proxy, & playlists, NULL, NULL);
    audtool_report ("%d", playlists);
}

void current_playlist (int argc, char * * argv)
{
    int playlist = -1;
    obj_audacious_call_get_active_playlist_sync (dbus_proxy, & playlist, NULL, NULL);
    audtool_report ("%d", playlist + 1);
}

void set_current_playlist (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<number>");
        exit (1);
    }

    obj_audacious_call_set_active_playlist_sync (dbus_proxy, atoi (argv[1]) - 1, NULL, NULL);
}

void playlist_title (int argc, char * * argv)
{
    char * title = NULL;
    obj_audacious_call_get_active_playlist_name_sync (dbus_proxy, & title, NULL, NULL);

    if (! title)
        exit (1);

    audtool_report ("%s", title);
    g_free (title);
}

void set_playlist_title (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<title>");
        exit (1);
    }

    obj_audacious_call_set_active_playlist_name_sync (dbus_proxy, argv[1], NULL, NULL);
}

void new_playlist (int argc, char * * argv)
{
    obj_audacious_call_new_playlist_sync (dbus_proxy, NULL, NULL);
}

void delete_current_playlist (int argc, char * * argv)
{
    obj_audacious_call_delete_active_playlist_sync (dbus_proxy, NULL, NULL);
}

void play_current_playlist (int argc, char * * argv)
{
    obj_audacious_call_play_active_playlist_sync (dbus_proxy, NULL, NULL);
}

void playlist_song_length (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    int length = get_entry_length (pos - 1) / 1000;
    audtool_report ("%d:%.2d", length / 60, length % 60);
}

void playlist_song_length_seconds (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    int length = get_entry_length (pos - 1) / 1000;
    audtool_report ("%d", length);
}

void playlist_song_length_frames (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    int length = get_entry_length (pos - 1);
    audtool_report ("%d", length);
}

void playlist_display (int argc, char * * argv)
{
    int entries = get_playlist_length ();

    audtool_report ("%d track%s.", entries, entries != 1 ? "s" : "");

    int total = 0;

    for (int entry = 0; entry < entries; entry ++)
    {
        char * title = get_entry_title (entry);
        int length = get_entry_length (entry) / 1000;

        total += length;

        /* adjust width for multi byte characters */
        int column = 60;

        for (const char * p = title; * p; p = g_utf8_next_char (p))
        {
            int stride = g_utf8_next_char (p) - p;

            if (g_unichar_iswide (g_utf8_get_char (p)) ||
             g_unichar_iswide_cjk (g_utf8_get_char (p)))
                column += (stride - 2);
            else
                column += (stride - 1);
        }

        char * fmt = g_strdup_printf ("%%4d | %%-%ds | %%d:%%.2d", column);
        audtool_report (fmt, entry + 1, title, length / 60, length % 60);

        g_free (fmt);
        g_free (title);
    }

    audtool_report ("Total length: %d:%.2d", total / 60, total % 60);
}

void playlist_position (int argc, char * * argv)
{
    audtool_report ("%d", get_current_entry () + 1);
}

void playlist_song_filename (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    char * filename = get_entry_filename (pos - 1);
    audtool_report ("%s", filename);
    g_free (filename);
}

void playlist_jump (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    obj_audacious_call_jump_sync (dbus_proxy, pos - 1, NULL, NULL);
}

void playlist_clear (int argc, char * * argv)
{
    obj_audacious_call_clear_sync (dbus_proxy, NULL, NULL);
}

void playlist_repeat_status (int argc, char * * argv)
{
    gboolean repeat = FALSE;
    obj_audacious_call_repeat_sync (dbus_proxy, & repeat, NULL, NULL);
    audtool_report (repeat ? "on" : "off");
}

void playlist_repeat_toggle (int argc, char * * argv)
{
    obj_audacious_call_toggle_repeat_sync (dbus_proxy, NULL, NULL);
}

void playlist_shuffle_status (int argc, char * * argv)
{
    gboolean shuffle = FALSE;
    obj_audacious_call_shuffle_sync (dbus_proxy, & shuffle, NULL, NULL);
    audtool_report (shuffle ? "on" : "off");
}

void playlist_shuffle_toggle (int argc, char * * argv)
{
    obj_audacious_call_toggle_shuffle_sync (dbus_proxy, NULL, NULL);
}

void playlist_tuple_field_data (int argc, char * * argv)
{
    int pos;

    if (argc < 3 || (pos = atoi (argv[2])) < 1)
    {
        audtool_whine_args (argv[0], "<fieldname> <position>");
        audtool_whine_tuple_fields ();
        exit (1);
    }

    char * str = get_entry_field (pos - 1, argv[1]);
    audtool_report ("%s", str);
    g_free (str);
}

void playlist_ins_url_string (int argc, char * * argv)
{
    int pos;

    if (argc < 3 || (pos = atoi (argv[2])) < 1)
    {
        audtool_whine_args (argv[0], "<url> <position>");
        exit (1);
    }

    char * uri = construct_uri (argv[1]);

    if (! uri)
        exit (1);

    obj_audacious_call_playlist_ins_url_string_sync (dbus_proxy, uri, pos - 1, NULL, NULL);

    g_free (uri);
}

void playlist_enqueue_to_temp (int argc, char * * argv)
{
    if (argc < 2)
    {
        audtool_whine_args (argv[0], "<url>");
        exit (1);
    }

    char * uri = construct_uri (argv[1]);

    if (! uri)
        exit (1);

    obj_audacious_call_playlist_enqueue_to_temp_sync (dbus_proxy, uri, NULL, NULL);

    g_free (uri);
}
