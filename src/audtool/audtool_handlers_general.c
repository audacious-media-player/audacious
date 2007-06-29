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

void playqueue_add(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playqueue-add.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-add <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (!(audacious_remote_playqueue_is_queued(dbus_proxy, i - 1)))
		audacious_remote_playqueue_add(dbus_proxy, i - 1);
}

void playqueue_remove(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playqueue-remove.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-remove <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	if (audacious_remote_playqueue_is_queued(dbus_proxy, i - 1))
		audacious_remote_playqueue_remove(dbus_proxy, i - 1);
}

void playqueue_is_queued(gint argc, gchar **argv)
{
	gint i;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playqueue-is-queued.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-is-queued <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	exit(!(audacious_remote_playqueue_is_queued(dbus_proxy, i - 1)));
}

void playqueue_get_position(gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playqueue-get-position.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-get-position <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playlist_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	pos = audacious_remote_get_playqueue_position(dbus_proxy, i - 1) + 1;

	if (pos < 1)
		return;

	g_print("%d\n", pos);
}

void playqueue_get_qposition(gint argc, gchar **argv)
{
	gint i, pos;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for playqueue-get-qposition.\n", argv[0]);
		g_print("%s: syntax: %s playqueue-get-qposition <position>\n", argv[0], argv[0]);
		return;
	}

	i = atoi(argv[1]);

	if (i < 1 || i > audacious_remote_get_playqueue_length(dbus_proxy))
	{
		g_print("%s: invalid playlist position %d\n", argv[0], i);
		return;
	}

	pos = audacious_remote_get_playqueue_queue_position(dbus_proxy, i - 1) + 1;

	if (pos < 1)
		return;

	g_print("%d\n", pos);
}

void playqueue_display(gint argc, gchar **argv)
{
	gint i, ii, position, frames, length, total;
	gchar *songname;
	gchar *fmt = NULL, *p;
	gint column;
	
	i = audacious_remote_get_playqueue_length(dbus_proxy);

	g_print("%d queued tracks.\n", i);

	total = 0;

	for (ii = 0; ii < i; ii++)
	{
		position = audacious_remote_get_playqueue_queue_position(dbus_proxy, ii);
		songname = audacious_remote_get_playlist_title(dbus_proxy, position);
		frames = audacious_remote_get_playlist_time(dbus_proxy, position);
		length = frames / 1000;
		total += length;

		/* adjust width for multi byte characters */
		column = 60;
		if(songname) {
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

		fmt = g_strdup_printf("%%4d | %%4d | %%-%ds | %%d:%%.2d\n", column);
		g_print(fmt, ii + 1, position + 1, songname, length / 60, length % 60);
		g_free(fmt);
	}

	g_print("Total length: %d:%.2d\n", total / 60, total % 60);
}

void playqueue_length(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_playqueue_length(dbus_proxy);

	g_print("%d\n", i);
}

void playqueue_clear(gint argc, gchar **argv)
{
	audacious_remote_playqueue_clear(dbus_proxy);
}

void get_volume(gint argc, gchar **argv)
{
	gint i;

	i = audacious_remote_get_main_volume(dbus_proxy);

	g_print("%d\n", i);
}

void set_volume(gint argc, gchar **argv)
{
	gint i, current_volume;

	if (argc < 2)
	{
		g_print("%s: invalid parameters for set-volume.\n", argv[0]);
		g_print("%s: syntax: %s set-volume <level>\n", argv[0], argv[0]);
		return;
	}

	current_volume = audacious_remote_get_main_volume(dbus_proxy);
	switch (argv[1][0]) 
	{
		case '+':
		case '-':
			i = current_volume + atoi(argv[1]);
			break;
		default:
			i = atoi(argv[1]);
			break;
	}

	audacious_remote_set_main_volume(dbus_proxy, i);
}

void mainwin_show(gint argc, gchar **argv)
{
	if (argc > 1)
	{
		if (!strncmp(argv[1],"on",2)) {
			audacious_remote_main_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[1],"off",3)) {
			audacious_remote_main_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	g_print("%s: invalid parameter for mainwin-show.\n",argv[0]);
	g_print("%s: syntax: %s mainwin-show <on/off>\n",argv[0],argv[0]);
}

void playlist_show(gint argc, gchar **argv)
{
	if (argc > 1)
	{
		if (!strncmp(argv[1],"on",2)) {
			audacious_remote_pl_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[1],"off",3)) {
			audacious_remote_pl_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	g_print("%s: invalid parameter for playlist-show.\n",argv[0]);
	g_print("%s: syntax: %s playlist-show <on/off>\n",argv[0],argv[0]);
}

void equalizer_show(gint argc, gchar **argv)
{
	if (argc > 1)
	{
		if (!strncmp(argv[1],"on",2)) {
			audacious_remote_eq_win_toggle(dbus_proxy, TRUE);
			return;
		}
		else if (!strncmp(argv[1],"off",3)) {
			audacious_remote_eq_win_toggle(dbus_proxy, FALSE);
			return;
		}
	}
	g_print("%s: invalid parameter for equalizer-show.\n",argv[0]);
	g_print("%s: syntax: %s equalizer-show <on/off>\n",argv[0],argv[0]);
}

void show_preferences_window(gint argc, gchar **argv)
{
	audacious_remote_show_prefs_box(dbus_proxy);
}

void show_jtf_window(gint argc, gchar **argv)
{
	audacious_remote_show_jtf_box(dbus_proxy);
}

void shutdown_audacious_server(gint argc, gchar **argv)
{
	audacious_remote_quit(dbus_proxy);
}

void get_handlers_list(gint argc, gchar **argv)
{
	gint i;

	for (i = 0; handlers[i].name != NULL; i++)
	{
		if (!g_strcasecmp("<sep>", handlers[i].name))
			g_print("%s%s:\n", i == 0 ? "" : "\n", handlers[i].desc);
		else
			g_print("   %-34s - %s\n", handlers[i].name, handlers[i].desc);
	}

	g_print("\nHandlers may be prefixed with `--' (GNU-style long-options) or not, your choice.\n");
	g_print("Report bugs to http://bugs-meta.atheme.org/\n");
}
