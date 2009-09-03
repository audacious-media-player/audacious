/*
 * Audtool2
 * Copyright (c) 2007 Audacious development team
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <mowgli.h>
#include <locale.h>
#include "libaudclient/audctrl.h"
#include "audtool.h"

void playlist_reverse(gint argc, gchar **argv)
{
	audacious_remote_playlist_prev(dbus_proxy);
}

void playlist_advance(gint argc, gchar **argv)
{
	audacious_remote_playlist_next(dbus_proxy);
}


gint check_args_playlist_pos(gint argc, gchar **argv)
{
	gint playpos;

	if (argc < 2)
	{
		audtool_whine_args(argv[0], "<position>");
		exit(1);
	}

	playpos = atoi(argv[1]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d ('%s')\n", playpos, argv[1]);
		exit(2);
	}

	return playpos;
}


static gchar * construct_uri(gchar *string)
{
    gchar *filename = g_strdup(string);
    gchar *tmp, *path;
    gchar *uri = NULL;

    // case 1: filename is raw full path or uri
    if (filename[0] == '/' || strstr(filename, "://")) {
        uri = g_filename_to_uri(filename, NULL, NULL);
        if(!uri) {
            uri = g_strdup(filename);
        }
        g_free(filename);
    }
    // case 2: filename is not raw full path nor uri.
    // make full path with pwd. (using g_build_filename)
    else {
        path = g_get_current_dir();
        tmp = g_build_filename(path, filename, NULL);
        g_free(path); g_free(filename);
        uri = g_filename_to_uri(tmp, NULL, NULL);
        g_free(tmp);
    }

    return uri;
}

void playlist_add_url_string(gint argc, gchar **argv)
{
    gchar *uri;

	if (argc < 2)
	{
		audtool_whine_args(argv[0], "<url>");
		exit(1);
	}

    uri = construct_uri(argv[1]);
    if (uri) {
        audacious_remote_playlist_add_url_string(dbus_proxy, uri);
    }
    g_free(uri);
}

void playlist_delete(gint argc, gchar **argv)
{
    gint playpos = check_args_playlist_pos(argc, argv);
	audacious_remote_playlist_delete(dbus_proxy, playpos - 1);
}

void playlist_length(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playlist_length(dbus_proxy);

	audtool_report("%d", i);
}

void playlist_song(gint argc, gchar **argv)
{
    gint playpos = check_args_playlist_pos(argc, argv);
	gchar *song;

	song = audacious_remote_get_playlist_title(dbus_proxy, playpos - 1);
	audtool_report("%s", song);
}


void playlist_song_length(gint argc, gchar **argv)
{
    gint playpos = check_args_playlist_pos(argc, argv);
	gint frames, length;

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);
	length = frames / 1000;

	audtool_report("%d:%.2d", length / 60, length % 60);
}

void playlist_song_length_seconds(gint argc, gchar **argv)
{
    gint playpos = check_args_playlist_pos(argc, argv);
	gint frames, length;

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);
	length = frames / 1000;

	audtool_report("%d", length);
}

void playlist_song_length_frames(gint argc, gchar **argv)
{
    gint playpos = check_args_playlist_pos(argc, argv);
	gint frames;

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);

	audtool_report("%d", frames);
}

void playlist_display(gint argc, gchar **argv)
{
	gint i, ii, frames, length, total;
	gchar *songname;
	gchar *fmt = NULL, *p;
	gint column;

	i = audacious_remote_get_playlist_length(dbus_proxy);

	audtool_report("%d track%s.", i, i != 1 ? "s" : "");

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		songname = audacious_remote_get_playlist_title(dbus_proxy, ii);
		frames = audacious_remote_get_playlist_time(dbus_proxy, ii);
		length = frames / 1000;
		total += length;

		/* adjust width for multi byte characters */
		column = 60;
		if(songname){
			p = songname;
			while(*p){
				gint stride;
				stride = g_utf8_next_char(p) - p;
				if(g_unichar_iswide(g_utf8_get_char(p))
				   || g_unichar_iswide_cjk(g_utf8_get_char(p))
                                ){
					column += (stride - 2);
				}
				else {
					column += (stride - 1);
				}
				p = g_utf8_next_char(p);
			}

		}

		fmt = g_strdup_printf("%%4d | %%-%ds | %%d:%%.2d", column);
		audtool_report(fmt, ii + 1, songname, length / 60, length % 60);
		g_free(fmt);
	}

	audtool_report("Total length: %d:%.2d", total / 60, total % 60);
}

void playlist_position(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playlist_pos(dbus_proxy);

	audtool_report("%d", i + 1);
}

void playlist_song_filename (gint argc, gchar * * argv)
{
    gint playpos = check_args_playlist_pos (argc, argv);
    gchar * uri, * filename;

    uri = audacious_remote_get_playlist_file (dbus_proxy, playpos - 1);
    filename = (uri != NULL) ? g_filename_from_uri (uri, NULL, NULL) : NULL;

    audtool_report ("%s", (filename != NULL) ? filename : (uri != NULL) ? uri :
     _("Position not found."));

    g_free (uri);
    g_free (filename);
}

void playlist_jump(gint argc, gchar **argv)
{
    gint playpos = check_args_playlist_pos(argc, argv);

	audacious_remote_set_playlist_pos(dbus_proxy, playpos - 1);
}

void playlist_clear(gint argc, gchar **argv)
{
	audacious_remote_stop(dbus_proxy);
	audacious_remote_playlist_clear(dbus_proxy);
}

void playlist_repeat_status(gint argc, gchar **argv)
{
	if (audacious_remote_is_repeat(dbus_proxy))
	{
		audtool_report("on");
	}
	else
	{
		audtool_report("off");
	}
}

void playlist_repeat_toggle(gint argc, gchar **argv)
{
	audacious_remote_toggle_repeat(dbus_proxy);
}

void playlist_shuffle_status(gint argc, gchar **argv)
{
	if (audacious_remote_is_shuffle(dbus_proxy))
	{
		audtool_report("on");
	}
	else
	{
		audtool_report("off");
	}
}

void playlist_shuffle_toggle(gint argc, gchar **argv)
{
	audacious_remote_toggle_shuffle(dbus_proxy);
}

void playlist_tuple_field_data(gint argc, gchar **argv)
{
    gint i;
	gchar *data;

	if (argc < 3)
	{
		audtool_whine_args(argv[0], "<fieldname> <position>");
		audtool_whine_tuple_fields();
		exit(1);
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		audtool_whine("invalid playlist position %d\n", i);
		exit(1);
	}

	if (!(data = audacious_get_tuple_field_data(dbus_proxy, argv[1], i - 1)))
	{
		return;
	}

	audtool_report("%s", data);

	g_free(data);
}

void playlist_ins_url_string(gint argc, gchar **argv)
{
    gint pos = -1;
    gchar *uri;

    if (argc < 3)
    {
        audtool_whine_args(argv[0], "<url> <position>");
        exit(1);
    }

    pos = atoi(argv[2]) - 1;
    if(pos >= 0) {
        uri = construct_uri(argv[1]);
        if (uri) {
            audacious_remote_playlist_ins_url_string(dbus_proxy, uri, pos);
        }
        g_free(uri);
    }
}

void playlist_enqueue_to_temp(gint argc, gchar **argv)
{
    gchar *uri;

    if (argc < 2)
    {
        audtool_whine_args(argv[0], "<url>");
        exit(1);
    }

    uri = construct_uri(argv[1]);
    if (uri) {
        audacious_remote_playlist_enqueue_to_temp(dbus_proxy, uri);
    }
    g_free(uri);
}
