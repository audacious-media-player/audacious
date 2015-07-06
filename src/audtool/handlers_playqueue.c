/*
 * handlers_playqueue.c
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

#include "audtool.h"
#include "wrappers.h"

void playqueue_add (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    obj_audacious_call_playqueue_add_sync (dbus_proxy, pos - 1, NULL, NULL);
}

void playqueue_remove (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    obj_audacious_call_playqueue_remove_sync (dbus_proxy, pos - 1, NULL, NULL);
}

void playqueue_is_queued (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    find_in_queue (pos - 1); /* calls exit(1) if not found */

    exit (0);
}

void playqueue_get_queue_position (int argc, char * * argv)
{
    int pos = check_args_playlist_pos (argc, argv);
    audtool_report ("%d", find_in_queue (pos - 1) + 1);
}

void playqueue_get_list_position (int argc, char * * argv)
{
    int qpos = check_args_playlist_pos (argc, argv);
    audtool_report ("%d", get_queue_entry (qpos - 1) + 1);
}

void playqueue_display (int argc, char * * argv)
{
    int qlength = get_queue_length ();

    audtool_report ("%d queued tracks.", qlength);

    int total = 0;

    for (int qpos = 0; qpos < qlength; qpos ++)
    {
        int pos = get_queue_entry (qpos);
        char * title = get_entry_title (pos);
        int length = get_entry_length (pos) / 1000;

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

        char * fmt = g_strdup_printf ("%%4d | %%4d | %%-%ds | %%d:%%.2d", column);
        audtool_report (fmt, qpos + 1, pos + 1, title, length / 60, length % 60);
        g_free (fmt);
    }

    audtool_report ("Total length: %d:%.2d", total / 60, total % 60);
}

void playqueue_length (int argc, char * * argv)
{
    audtool_report ("%d", get_queue_length ());
}

void playqueue_clear (int argc, char * * argv)
{
    obj_audacious_call_playqueue_clear_sync (dbus_proxy, NULL, NULL);
}
