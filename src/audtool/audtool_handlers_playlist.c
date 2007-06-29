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

void playlist_add_url_string(gint argc, gchar **argv)
{
	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-addurl.\n", argv[0]);
		g_print("%s: syntax: %s playlist-addurl <url>\n", argv[0], argv[0]);
		return;
	}

	audacious_remote_playlist_add_url_string(dbus_proxy, argv[1]);
}

void playlist_delete(gint argc, gchar **argv)
{
	gint playpos;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-delete.\n", argv[0]);
		g_print("%s: syntax: %s playlist-delete <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[1]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	audacious_remote_playlist_delete(dbus_proxy, playpos - 1);
}

void playlist_length(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playlist_length(dbus_proxy);

	g_print("%d\n", i);
}

void playlist_song(gint argc, gchar **argv)
{
	gint playpos;
	gchar *song;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-song-title.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-title <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[1]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	song = audacious_remote_get_playlist_title(dbus_proxy, playpos - 1);

	g_print("%s\n", song);
}


void playlist_song_length(gint argc, gchar **argv)
{
	gint playpos, frames, length;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-song-length.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[1]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);
	length = frames / 1000;

	g_print("%d:%.2d\n", length / 60, length % 60);
}

void playlist_song_length_seconds(gint argc, gchar **argv)
{
	gint playpos, frames, length;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-song-length-seconds.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length-seconds <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[1]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);
	length = frames / 1000;

	g_print("%d\n", length);
}

void playlist_song_length_frames(gint argc, gchar **argv)
{
	gint playpos, frames;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-song-length-frames.\n", argv[0]);
		g_print("%s: syntax: %s playlist-song-length-frames <position>\n", argv[0], argv[0]);
		return;
	}

	playpos = atoi(argv[1]);

	if (playpos < 1 || playpos > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], playpos);
		return;
	}

	frames = audacious_remote_get_playlist_time(dbus_proxy, playpos - 1);

	g_print("%d\n", frames);
}

void playlist_display(gint argc, gchar **argv)
{
	gint i, ii, frames, length, total;
	gchar *songname;
	gchar *fmt = NULL, *p;
	gint column;

	i = audacious_remote_get_playlist_length(dbus_proxy);

	g_print("%d track%s.\n", i, i != 1 ? "s" : "");

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
#if ( (GLIB_MAJOR_VERSION == 2) && (GLIB_MINOR_VERSION >= 12) )
				   || g_unichar_iswide_cjk(g_utf8_get_char(p))
#endif
                                ){
					column += (stride - 2);
				}
				else {
					column += (stride - 1);
				}
				p = g_utf8_next_char(p);
			}

		}

		fmt = g_strdup_printf("%%4d | %%-%ds | %%d:%%.2d\n", column);
		g_print(fmt, ii + 1, songname, length / 60, length % 60);
		g_free(fmt);
	}

	g_print("Total length: %d:%.2d\n", total / 60, total % 60);
}

void playlist_position(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playlist_pos(dbus_proxy);

	g_print("%d\n", i + 1);
}

void playlist_song_filename(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-filename.\n", argv[0]);
		g_print("%s: syntax: %s playlist-filename <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	g_print("%s\n", audacious_remote_get_playlist_file(dbus_proxy, i - 1));
}

void playlist_jump(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playlist-jump.\n", argv[0]);
		g_print("%s: syntax: %s playlist-jump <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	audacious_remote_set_playlist_pos(dbus_proxy, i - 1);
}

void playlist_clear(gint argc, gchar **argv)
{
	audacious_remote_playlist_clear(dbus_proxy);
}

void playlist_repeat_status(gint argc, gchar **argv)
{
	if (audacious_remote_is_repeat(dbus_proxy))
	{
		g_print("on\n");
		return;
	}
	else
	{
		g_print("off\n");
		return;
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
		g_print("on\n");
		return;
	}
	else
	{
		g_print("off\n");
		return;
	}
}

void playlist_shuffle_toggle(gint argc, gchar **argv)
{
	audacious_remote_toggle_shuffle(dbus_proxy);
}

void playlist_tuple_field_data(gint argc, gchar **argv)
{
	gint i;
	gpointer data;

	if (argc < 3)
	{
		g_print("%s: invalid parameters for playlist-tuple-data.\n", argv[0]);
		g_print("%s: syntax: %s playlist-tuple-data <fieldname> <position>\n", argv[0], argv[0]);
		g_print("%s:   - fieldname example choices: performer, album_name,\n", argv[0]);
		g_print("%s:       track_name, track_number, year, date, genre, comment,\n", argv[0]);
		g_print("%s:       file_name, file_ext, file_path, length, formatter,\n", argv[0]);
		g_print("%s:       custom, mtime\n", argv[0]);
		return;
	}

	i = atoi(argv[2]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (!(data = audacious_get_tuple_field_data(dbus_proxy, argv[1], i - 1)))
	{
		return;
	}
	
	if (!strcasecmp(argv[1], "track_number") || !strcasecmp(argv[1], "year") || !strcasecmp(argv[1], "length") || !strcasecmp(argv[1], "mtime"))
	{
		if (*(gint *)data > 0)
		{
			g_print("%d\n", *(gint *)data);
		}
		return;
	}

	g_print("%s\n", (gchar *)data);
}
